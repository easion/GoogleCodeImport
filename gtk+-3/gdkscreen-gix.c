


#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include "gdkscreenprivate.h"
#include "gdkvisualprivate.h"
#include "gdkdisplay.h"
#include "gdkdisplay-gix.h"
#include "gdkgix.h"
#include "gdkprivate-gix.h"

static GdkVisual *system_visual = NULL;
static gint available_depths[1];
static GdkVisualType available_types[1];


G_DEFINE_TYPE (GdkScreenGix, _gdk_screen_gix, GDK_TYPE_SCREEN)


static gint
get_mm_from_pixels ( int pixels)
{ 
  float dpi = 96.0 / 72.0;

  return (pixels / dpi) * 25.4;
}


static void
init_monitor_geometry (GdkGixMonitor *monitor,
		       int x, int y, int width, int height)
{
  monitor->geometry.x = x;
  monitor->geometry.y = y;
  monitor->geometry.width = width;
  monitor->geometry.height = height;

  monitor->width_mm = get_mm_from_pixels(width); //-1
  monitor->height_mm = get_mm_from_pixels(height); //-1
  monitor->output_name = NULL;
  monitor->manufacturer = NULL;
}

static void
free_monitors (GdkGixMonitor *monitors,
               gint           n_monitors)
{
  int i;

  for (i = 0; i < n_monitors; ++i)
    {
      g_free (monitors[i].output_name);
      g_free (monitors[i].manufacturer);
    }

  g_free (monitors);
}

static void
deinit_multihead (GdkScreen *screen)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (screen);

  free_monitors (screen_gix->monitors, screen_gix->n_monitors);

  screen_gix->n_monitors = 0;
  screen_gix->monitors = NULL;
}

static void
init_multihead (GdkScreen *screen)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (screen);

  /* No multihead support of any kind for this screen */
  screen_gix->n_monitors = 1;
  screen_gix->monitors = g_new0 (GdkGixMonitor, 1);
  screen_gix->primary_monitor = 0;

  init_monitor_geometry (screen_gix->monitors, 0, 0,
			 screen_gix->width, screen_gix->height);
}

static void
gdk_gix_screen_dispose (GObject *object)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (object);

  if (screen_gix->root_window)
    _gdk_window_destroy (screen_gix->root_window, TRUE);

  G_OBJECT_CLASS (_gdk_screen_gix_parent_class)->dispose (object);
}

static void
gdk_gix_screen_finalize (GObject *object)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (object);

  if (screen_gix->root_window)
    g_object_unref (screen_gix->root_window);

  /* Visual Part */
  g_object_unref (system_visual);
  
  deinit_multihead (GDK_SCREEN (object));

  G_OBJECT_CLASS (_gdk_screen_gix_parent_class)->finalize (object);
}

static GdkDisplay *
gdk_gix_screen_get_display (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return _gdk_display;

  //return GDK_SCREEN_GIX (screen)->scr_display;
}

static gint
gdk_gix_screen_get_width (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_SCREEN_GIX (screen)->width;
}

static gint
gdk_gix_screen_get_height (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_SCREEN_GIX (screen)->height;
}

static gint
gdk_gix_screen_get_width_mm (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_SCREEN_GIX (screen)->width_mm;
}

static gint
gdk_gix_screen_get_height_mm (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_SCREEN_GIX (screen)->height_mm;
}

static gint
gdk_gix_screen_get_number (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return 0;
}

static GdkWindow *
gdk_gix_screen_get_root_window (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  //return GDK_SCREEN_GIX (screen)->root_window;
  return _gdk_root;
}

static gint
gdk_gix_screen_get_n_monitors (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_SCREEN_GIX (screen)->n_monitors;
}

static gint
gdk_gix_screen_get_primary_monitor (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return GDK_SCREEN_GIX (screen)->primary_monitor;
}

static gint
gdk_gix_screen_get_monitor_width_mm	(GdkScreen *screen,
					 gint       monitor_num)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (screen);

  g_return_val_if_fail (GDK_IS_SCREEN (screen), -1);
  g_return_val_if_fail (monitor_num >= 0, -1);
  g_return_val_if_fail (monitor_num < screen_gix->n_monitors, -1);

  return screen_gix->monitors[monitor_num].width_mm;
}

static gint
gdk_gix_screen_get_monitor_height_mm (GdkScreen *screen,
					  gint       monitor_num)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (screen);

  g_return_val_if_fail (GDK_IS_SCREEN (screen), -1);
  g_return_val_if_fail (monitor_num >= 0, -1);
  g_return_val_if_fail (monitor_num < screen_gix->n_monitors, -1);

  return screen_gix->monitors[monitor_num].height_mm;
}

static gchar *
gdk_gix_screen_get_monitor_plug_name (GdkScreen *screen,
					  gint       monitor_num)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (screen);

  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  g_return_val_if_fail (monitor_num >= 0, NULL);
  g_return_val_if_fail (monitor_num < screen_gix->n_monitors, NULL);

  return g_strdup (screen_gix->monitors[monitor_num].output_name);
}

static void
gdk_gix_screen_get_monitor_geometry (GdkScreen    *screen,
					 gint          monitor_num,
					 GdkRectangle *dest)
{
  GdkScreenGix *screen_gix = GDK_SCREEN_GIX (screen);

  g_return_if_fail (GDK_IS_SCREEN (screen));
  g_return_if_fail (monitor_num >= 0);
  g_return_if_fail (monitor_num < screen_gix->n_monitors);

  if (dest)
    *dest = screen_gix->monitors[monitor_num].geometry;
}

static GdkVisual *
gdk_gix_screen_get_system_visual (GdkScreen * screen)
{
  g_assert(system_visual != NULL);
  return (GdkVisual *) system_visual;
}

static GdkVisual *
gdk_gix_screen_get_rgba_visual (GdkScreen *screen)
{
  g_assert(system_visual != NULL);
  return (GdkVisual *) system_visual;
}

static gboolean
gdk_gix_screen_is_composited (GdkScreen *screen)
{
  //return TRUE;
  return FALSE;
}

static gchar *
gdk_gix_screen_make_display_name (GdkScreen *screen)
{
  //return NULL;
  return g_strdup (gdk_display_get_name (_gdk_display));
}

static GdkWindow *
gdk_gix_screen_get_active_window (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return NULL;
}

static GList *
gdk_gix_screen_get_window_stack (GdkScreen *screen)
{
  return NULL;
}

static void
gdk_gix_screen_broadcast_client_message (GdkScreen *screen,
					     GdkEvent  *event)
{
}

static gboolean
gdk_gix_screen_get_setting (GdkScreen   *screen,
				const gchar *name,
				GValue      *value)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

  /*
   * XXX : if these values get changed through the Windoze UI the
   *       respective gdk_events are not generated yet.
   */
  if (strcmp ("gtk-theme-name", name) == 0) 
    {
      g_value_set_string (value, "ms-windows");
    }
  else if (strcmp ("gtk-double-click-time", name) == 0)
    {
      //gint i = GetDoubleClickTime ();
      //GDK_NOTE(MISC, g_print("gdk_screen_get_setting(\"%s\") : %d\n", name, i));
      //g_value_set_int (value, i);
      return FALSE;
    }
  else if (strcmp ("gtk-double-click-distance", name) == 0)
    {
      //gint i = MAX(GetSystemMetrics (SM_CXDOUBLECLK), GetSystemMetrics (SM_CYDOUBLECLK));
      //GDK_NOTE(MISC, g_print("gdk_screen_get_setting(\"%s\") : %d\n", name, i));
      //g_value_set_int (value, i);
      return FALSE;
    }
  else if (strcmp ("gtk-dnd-drag-threshold", name) == 0)
    {
      //gint i = MAX(GetSystemMetrics (SM_CXDRAG), GetSystemMetrics (SM_CYDRAG));
      //GDK_NOTE(MISC, g_print("gdk_screen_get_setting(\"%s\") : %d\n", name, i));
      //g_value_set_int (value, i);
      return FALSE;
    }
  else if (strcmp ("gtk-split-cursor", name) == 0)
    {
      GDK_NOTE(MISC, g_print("gdk_screen_get_setting(\"%s\") : FALSE\n", name));
      g_value_set_boolean (value, FALSE);
      return TRUE;
    }
  else if (strcmp ("gtk-alternative-button-order", name) == 0)
    {
      GDK_NOTE(MISC, g_print("gdk_screen_get_setting(\"%s\") : TRUE\n", name));
      g_value_set_boolean (value, TRUE);
      return TRUE;
    }
  else if (strcmp ("gtk-alternative-sort-arrows", name) == 0)
    {
      GDK_NOTE(MISC, g_print("gdk_screen_get_setting(\"%s\") : TRUE\n", name));
      g_value_set_boolean (value, TRUE);
      return TRUE;
    }
#if 0
  /*
   * With 'MS Sans Serif' as windows menu font (default on win98se) you'll get a 
   * bunch of :
   *   WARNING **: Couldn't load font "MS Sans Serif 8" falling back to "Sans 8"
   * at least with testfilechooser (regardless of the bitmap check below)
   * so just disabling this code seems to be the best we can do --hb
   */
  else if (strcmp ("gtk-font-name", name) == 0)
    {
      NONCLIENTMETRICS ncm;
      ncm.cbSize = sizeof(NONCLIENTMETRICS);
      if (SystemParametersInfo (SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, FALSE))
        {
          /* Pango finally uses GetDeviceCaps to scale, we use simple
	   * approximation here.
	   */
          int nHeight = (0 > ncm.lfMenuFont.lfHeight ? -3*ncm.lfMenuFont.lfHeight/4 : 10);
          if (OUT_STRING_PRECIS == ncm.lfMenuFont.lfOutPrecision)
            GDK_NOTE(MISC, g_print("gdk_screen_get_setting(%s) : ignoring bitmap font '%s'\n", 
                                   name, ncm.lfMenuFont.lfFaceName));
          else if (ncm.lfMenuFont.lfFaceName && strlen(ncm.lfMenuFont.lfFaceName) > 0 &&
                   /* Avoid issues like those described in bug #135098 */
                   g_utf8_validate (ncm.lfMenuFont.lfFaceName, -1, NULL))
            {
              char* s = g_strdup_printf ("%s %d", ncm.lfMenuFont.lfFaceName, nHeight);
              GDK_NOTE(MISC, g_print("gdk_screen_get_setting(%s) : %s\n", name, s));
              g_value_set_string (value, s);

              g_free(s);
              return TRUE;
            }
        }
    }
#endif

  return FALSE;
}

static void
gdk_visual_decompose_mask (gulong  mask,
                           gint   *shift,
                           gint   *prec)
{
  *shift = 0;
  *prec = 0;

  if (mask == 0)
    {
      g_warning ("Mask is 0 in visual. Server bug ?");
      return;
    }

  while (!(mask & 0x1))
    {
      (*shift)++;
      mask >>= 1;
    }

  while (mask & 0x1)
    {
      (*prec)++;
      mask >>= 1;
    }
}

static gint
gdk_gix_screen_visual_get_best_depth (GdkScreen *screen)
{
  return available_depths[0];
}

static GdkVisualType
gdk_gix_screen_visual_get_best_type (GdkScreen *screen)
{
  return available_types[0];
}

static GdkVisual*
gdk_gix_screen_visual_get_best (GdkScreen *screen)
{
  g_assert(system_visual != NULL);
  return system_visual;
}

static GdkVisual*
gdk_gix_screen_visual_get_best_with_depth (GdkScreen *screen,
					       gint       depth)
{
  g_assert(system_visual != NULL);
  return system_visual;
}

static GdkVisual*
gdk_gix_screen_visual_get_best_with_type (GdkScreen     *screen,
					      GdkVisualType  visual_type)
{
  g_assert(system_visual != NULL);
  return system_visual;
}

static GdkVisual*
gdk_gix_screen_visual_get_best_with_both (GdkScreen     *screen,
					      gint           depth,
					      GdkVisualType  visual_type)
{
  g_assert(system_visual != NULL);
  return system_visual;
}

static void
gdk_gix_screen_query_depths  (GdkScreen  *screen,
				  gint      **depths,
				  gint       *count)
{
  *count = 1;
  *depths = available_depths;
}

static void
gdk_gix_screen_query_visual_types (GdkScreen      *screen,
				       GdkVisualType **visual_types,
				       gint           *count)
{
  *count = 1;
  *visual_types = available_types;
}

static GList *
gdk_gix_screen_list_visuals (GdkScreen *screen)
{
  GList *list;
  GdkScreenGix *screen_gix;

  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);
  screen_gix = GDK_SCREEN_GIX (screen);

  list = g_list_append (NULL, system_visual);
  return list;
}


void _gdk_visual_init (void)
{
  long format = gi_screen_format();
  gint map_entries = 0;

  //system_visual = g_object_new (GDK_TYPE_GIX_VISUAL, NULL);
  system_visual = g_object_new (GDK_TYPE_VISUAL, NULL);
  g_assert(system_visual != NULL);
  system_visual->screen = gdk_screen_get_default();
  g_assert(system_visual->screen != NULL);

  switch (format)
  {
  case GI_RENDER_r5g6b5:
	  system_visual->red_mask   = 0x00007C00;
      system_visual->green_mask = 0x000003E0;
      system_visual->blue_mask  = 0x0000001F;
	  system_visual->type = GDK_VISUAL_TRUE_COLOR;
	  break;

  case GI_RENDER_r8g8b8:
  case GI_RENDER_x8r8g8b8:
	  system_visual->type = GDK_VISUAL_TRUE_COLOR;
      system_visual->red_mask   = 0x00FF0000;
      system_visual->green_mask = 0x0000FF00;
      system_visual->blue_mask  = 0x000000FF;
	  break;
  default:
	  break;
  
  }

	gdk_visual_decompose_mask (system_visual->red_mask,
			 &system_visual->red_shift,
			 &system_visual->red_prec);

	gdk_visual_decompose_mask (system_visual->green_mask,
			 &system_visual->green_shift,
			 &system_visual->green_prec);

	gdk_visual_decompose_mask (system_visual->blue_mask,
		 &system_visual->blue_shift,
		 &system_visual->blue_prec);
	map_entries = 1 << (MAX (system_visual->red_prec,
		   MAX (system_visual->green_prec,
			system_visual->blue_prec)));
	system_visual->colormap_size = map_entries;

	system_visual->depth = GI_RENDER_FORMAT_BPP(format);
	system_visual->byte_order = GDK_LSB_FIRST;
	system_visual->bits_per_rgb = 42; /* Not used? */

	available_depths[0] = system_visual->depth;
	available_types[0] = system_visual->type;
}


GdkScreen *
_gdk_windowing_window_init (GdkDisplay *display, GdkScreen *screen)
{
  //GdkScreen *screen;
  GdkScreenGix *screen_gix;
  GdkDisplayGix *display_gix;

  display_gix = GDK_DISPLAY_GIX (display);
  g_assert(display_gix != NULL);

  //screen = g_object_new (GDK_TYPE_SCREEN_GIX, NULL);

  screen_gix = GDK_SCREEN_GIX (screen);
  //screen_gix->scr_display = display;
  screen_gix->width = gi_screen_width();
  screen_gix->height = gi_screen_height();
  screen_gix->xroot_window = GI_DESKTOP_WINDOW_ID;

  screen_gix->root_window =
    _gdk_gix_screen_create_root_window (screen,
					    screen_gix->width,
					    screen_gix->height);

  init_multihead (screen);

  g_print("_gdk_gix_screen_create_root_window = %p %p\n", 
	  screen_gix->root_window,screen_gix->root_window->visual);

  return screen;
}

static void
_gdk_screen_gix_class_init (GdkScreenGixClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkScreenClass *screen_class = GDK_SCREEN_CLASS (klass);

  object_class->dispose = gdk_gix_screen_dispose;
  object_class->finalize = gdk_gix_screen_finalize;

  screen_class->get_display = gdk_gix_screen_get_display;
  screen_class->get_width = gdk_gix_screen_get_width;
  screen_class->get_height = gdk_gix_screen_get_height;
  screen_class->get_width_mm = gdk_gix_screen_get_width_mm;
  screen_class->get_height_mm = gdk_gix_screen_get_height_mm;
  screen_class->get_number = gdk_gix_screen_get_number;
  screen_class->get_root_window = gdk_gix_screen_get_root_window;
  screen_class->get_n_monitors = gdk_gix_screen_get_n_monitors;
  screen_class->get_primary_monitor = gdk_gix_screen_get_primary_monitor;
  screen_class->get_monitor_width_mm = gdk_gix_screen_get_monitor_width_mm;
  screen_class->get_monitor_height_mm = gdk_gix_screen_get_monitor_height_mm;
  screen_class->get_monitor_plug_name = gdk_gix_screen_get_monitor_plug_name;
  screen_class->get_monitor_geometry = gdk_gix_screen_get_monitor_geometry;
  screen_class->get_system_visual = gdk_gix_screen_get_system_visual;
  screen_class->get_rgba_visual = gdk_gix_screen_get_rgba_visual;
  screen_class->is_composited = gdk_gix_screen_is_composited;
  screen_class->make_display_name = gdk_gix_screen_make_display_name;
  screen_class->get_active_window = gdk_gix_screen_get_active_window;
  screen_class->get_window_stack = gdk_gix_screen_get_window_stack;
  screen_class->broadcast_client_message = gdk_gix_screen_broadcast_client_message;
  screen_class->get_setting = gdk_gix_screen_get_setting;
  screen_class->visual_get_best_depth = gdk_gix_screen_visual_get_best_depth;
  screen_class->visual_get_best_type = gdk_gix_screen_visual_get_best_type;
  screen_class->visual_get_best = gdk_gix_screen_visual_get_best;
  screen_class->visual_get_best_with_depth = gdk_gix_screen_visual_get_best_with_depth;
  screen_class->visual_get_best_with_type = gdk_gix_screen_visual_get_best_with_type;
  screen_class->visual_get_best_with_both = gdk_gix_screen_visual_get_best_with_both;
  screen_class->query_depths = gdk_gix_screen_query_depths;
  screen_class->query_visual_types = gdk_gix_screen_query_visual_types;
  screen_class->list_visuals = gdk_gix_screen_list_visuals;
}

static void
_gdk_screen_gix_init (GdkScreenGix *screen_gix)
{
}
