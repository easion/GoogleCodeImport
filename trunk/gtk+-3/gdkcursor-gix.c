


#include "config.h"

#define GDK_PIXBUF_ENABLE_BACKEND

#include <string.h>

#include "gdkprivate-gix.h"
#include "gdkcursorprivate.h"
#include "gdkdisplay-gix.h"
#include "gdkgix.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <sys/mman.h>


G_DEFINE_TYPE (GdkGixCursor, _gdk_gix_cursor, GDK_TYPE_CURSOR)


static void
gdk_gix_cursor_finalize (GObject *object)
{
  GdkGixCursor *cursor = GDK_GIX_CURSOR (object);

  if(cursor->name)
  g_free (cursor->name);

  G_OBJECT_CLASS (_gdk_gix_cursor_parent_class)->finalize (object);
}

static GdkPixbuf*
gdk_gix_cursor_get_image (GdkCursor *cursor)
{
  return NULL;
}

int
_gdk_gix_cursor_get_id (GdkCursor *cursor, int *x, int *y)
{
  GdkGixCursor *gix_cursor = GDK_GIX_CURSOR (cursor);

  *x = gix_cursor->x;
  *y = gix_cursor->y;

  return gix_cursor->id;
}

static void
_gdk_gix_cursor_class_init (GdkGixCursorClass *gix_cursor_class)
{
  GdkCursorClass *cursor_class = GDK_CURSOR_CLASS (gix_cursor_class);
  GObjectClass *object_class = G_OBJECT_CLASS (gix_cursor_class);

  object_class->finalize = gdk_gix_cursor_finalize;

  cursor_class->get_image = gdk_gix_cursor_get_image;
}

static void
_gdk_gix_cursor_init (GdkGixCursor *cursor)
{
}

static void
set_pixbuf (GdkGixCursor *cursor, GdkPixbuf *pixbuf, unsigned char *argb_pixels)
{
  int stride, i, n_channels;
  unsigned char *pixels, *end,  *s, *d;

  stride = gdk_pixbuf_get_rowstride(pixbuf);
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  n_channels = gdk_pixbuf_get_n_channels(pixbuf);

#define MULT(_d,c,a,t) \
	do { t = c * a + 0x7f; _d = ((t >> 8) + t) >> 8; } while (0)

  if (n_channels == 4)
    {
      for (i = 0; i < cursor->height; i++)
	{
	  s = pixels + i * stride;
	  end = s + cursor->width * 4;
	  d = argb_pixels + i * cursor->width * 4;
	  while (s < end)
	    {
	      unsigned int t;

	      MULT(d[0], s[2], s[3], t);
	      MULT(d[1], s[1], s[3], t);
	      MULT(d[2], s[0], s[3], t);
	      d[3] = s[3];
	      s += 4;
	      d += 4;
	    }
	}
    }
  else if (n_channels == 3)
    {
      for (i = 0; i < cursor->height; i++)
	{
	  s = pixels + i * stride;
	  end = s + cursor->width * 3;
	  d = argb_pixels + i * cursor->width * 4;
	  while (s < end)
	    {
	      d[0] = s[2];
	      d[1] = s[1];
	      d[2] = s[0];
	      d[3] = 0xff;
	      s += 3;
	      d += 4;
	    }
	}
    }
}

static int next_cursor_id = 0;

static GdkCursor *
gdk_gix_cursor_new_from_id (GdkDisplay *display, 
	int nscursor, GdkCursorType  cursor_type)
{
  GdkGixCursor *private;

  private = g_object_new (GDK_TYPE_GIX_CURSOR,
			 "cursor-type", GDK_CURSOR_IS_PIXMAP,
			 "display", display,
			 NULL);
  private->id = nscursor;

  return GDK_CURSOR (private);
}

GdkCursor *
_gdk_gix_display_get_cursor_for_pixbuf(GdkDisplay *gdk_display, GdkPixbuf *pixbuf, int x, int y)
{
  GdkGixCursor *cursor;
  int stride;
  int err;
  char *filename;
  GError *error = NULL;
  GdkDisplayGix *display;

  display = GDK_DISPLAY_GIX (gdk_display);

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), NULL);
  g_return_val_if_fail (0 <= x && x < gdk_pixbuf_get_width (pixbuf), NULL);
  g_return_val_if_fail (0 <= y && y < gdk_pixbuf_get_height (pixbuf), NULL);


  cursor = g_object_new (GDK_TYPE_GIX_CURSOR,
			 "cursor-type", GDK_CURSOR_IS_PIXMAP,
			 "display", display,
			 NULL);
  cursor->name = NULL;
  cursor->x = x;
  cursor->y = y;
  if (pixbuf)
    {
      cursor->width = gdk_pixbuf_get_width (pixbuf);
      cursor->height = gdk_pixbuf_get_height (pixbuf);
    }
  else
    {
      cursor->width = 1;
      cursor->height = 1;
    }

  stride = cursor->width * 4;
  cursor->size = stride * cursor->height;
  cursor->id = 0;
  cursor->name = NULL;

  unsigned char *map;
  gi_image_t *curosr_image;

  map = malloc(cursor->size);

  if (!map) {
	  return NULL;
  }

  if (pixbuf)
    set_pixbuf (cursor, pixbuf,map);
  else
    memset (map, 0, 4);

  LOG_UNUSED;

  curosr_image = gi_create_image_with_data(cursor->width,cursor->width,map,GI_RENDER_a8r8g8b8);
  if (!curosr_image)
  {
	  free(map);
	  return NULL;
  }

  next_cursor_id = (next_cursor_id+1)%(GI_CURSOR_MAX-GIX_CURSOR_BASE);
  cursor->id = GIX_CURSOR_BASE+next_cursor_id;
  err = gi_setup_cursor(cursor->id,  0,	0,  curosr_image);
  if (err != 0)
  {
	  cursor->id = 0;
	  goto fail;
  }

  gi_destroy_image(curosr_image);
  //gi_create_cursor_image(gi_window_id_t wid, int x, int y, gi_image_t* img);
  return GDK_CURSOR (cursor);

fail:
	gi_destroy_image(curosr_image);
	return NULL;
}

int
_gdk_cursor_type_to_gix ( GdkCursorType  cursor_type)
{
  int nscursor = -1;

  switch (cursor_type)
    {
    case GDK_XTERM:
      nscursor = GI_CURSOR_IBEAM;
      break;
    case GDK_SB_H_DOUBLE_ARROW:
      nscursor = GI_CURSOR_SIZEWE;
      break;
    case GDK_SB_V_DOUBLE_ARROW:
      nscursor = GI_CURSOR_SIZENS;
      break;
    case GDK_SB_UP_ARROW:
    case GDK_BASED_ARROW_UP:
    case GDK_BOTTOM_TEE:
    case GDK_TOP_SIDE:
      nscursor = GI_CURSOR_UPARROW;
      break;
    case GDK_SB_DOWN_ARROW:
    case GDK_BASED_ARROW_DOWN:
    case GDK_TOP_TEE:
    case GDK_BOTTOM_SIDE:
      nscursor = GI_CURSOR_SIZENESW;
      break;
    case GDK_SB_LEFT_ARROW:
    case GDK_RIGHT_TEE:
    case GDK_LEFT_SIDE:
      nscursor = GI_CURSOR_SIZENWSE;
      break;
    case GDK_SB_RIGHT_ARROW:
    case GDK_LEFT_TEE:
    case GDK_RIGHT_SIDE:
      nscursor = GI_CURSOR_SIZENS;
      break;
    case GDK_TCROSS:
    case GDK_CROSS:
    case GDK_CROSSHAIR:
    case GDK_DIAMOND_CROSS:
      nscursor = GI_CURSOR_CROSS;
      break;
    case GDK_HAND1:
    case GDK_HAND2:
      nscursor = GI_CURSOR_HAND;
      break;
    case GDK_CURSOR_IS_PIXMAP:
      return -1;
	case GDK_LEFT_PTR:
		return GI_CURSOR_ARROW;
    case GDK_BLANK_CURSOR:
      return GI_CURSOR_NONE;
    default:
      return -1;
    }

  return  (nscursor);
}



GdkCursor *
_gdk_gix_display_get_cursor_for_type (GdkDisplay    *display,
					  GdkCursorType  cursor_type)
{
  int nscursor;

  nscursor = _gdk_cursor_type_to_gix(cursor_type);
  if (nscursor == -1)
  {
	  return NULL;
  }

  g_return_val_if_fail (display == gdk_display_get_default (), NULL); 
  return gdk_gix_cursor_new_from_id (display, nscursor, cursor_type);
}

static struct {
  char *name;
  int id;
} default_cursors[] = {
  { "appstarting", GI_CURSOR_APPSTARTING },
  { "arrow", GI_CURSOR_ARROW },
  { "cross", GI_CURSOR_CROSS },

  { "hand",  GI_CURSOR_HAND },

  { "help",  GI_CURSOR_HELP },
  { "ibeam", GI_CURSOR_IBEAM },
  { "sizeall", GI_CURSOR_SIZEALL },
  { "sizenesw", GI_CURSOR_SIZENESW },
  { "sizens", GI_CURSOR_SIZENS },
  { "sizenwse", GI_CURSOR_SIZENWSE },
  { "sizewe", GI_CURSOR_SIZEWE },
  { "uparrow", GI_CURSOR_UPARROW },
  { "wait", GI_CURSOR_WAIT }
};

GdkCursor*
_gdk_gix_display_get_cursor_for_name (GdkDisplay  *display,
					  const gchar *name)
{
  GdkGixCursor *private;
  int i, id = 0;

  for (i = 0; i < G_N_ELEMENTS(default_cursors); i++)
    {
      if (0 == strcmp(default_cursors[i].name, name)){
        //hcursor = LoadCursor (NULL, default_cursors[i].id);
		id = default_cursors[i].id;
		break;
	  }
    }

	if (!id)
	{
		return NULL;
	}

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  private = g_object_new (GDK_TYPE_GIX_CURSOR,
                          "cursor-type", GDK_CURSOR_IS_PIXMAP,
                          "display", display,
                          NULL);
  private->name = g_strdup (name);
  private->id = id;

  return GDK_CURSOR (private);
}


void
_gdk_gix_display_get_default_cursor_size (GdkDisplay *display,
					      guint       *width,
					      guint       *height)
{
  /* FIXME: gix settings? */
  *width = 32;
  *height = 32;
}

void
_gdk_gix_display_get_maximal_cursor_size (GdkDisplay *display,
					      guint       *width,
					      guint       *height)
{
  *width = 64;
  *height = 64;
}

gboolean
_gdk_gix_display_supports_cursor_alpha (GdkDisplay *display)
{
  g_return_val_if_fail (display == _gdk_display, FALSE);
  return TRUE;
}

gboolean
_gdk_gix_display_supports_cursor_color (GdkDisplay *display)
{
  g_return_val_if_fail (display == _gdk_display, FALSE);
  return TRUE;
}
