/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-1999 Tor Lillqvist
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.
 */


/*
 * GTK+ Gix backend
 * Copyright (C) 2011 www.hanxuantech.com.
 * Written by Easion <easion@hanxuantech.com> , it's based
 * on DirectFB port.
 */

#include "config.h"
#include "gdk.h"

#include <stdlib.h>
#include <string.h>

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkinternals.h"

#include "gdkpixmap.h"
#include "gdkalias.h"

uint8_t *bitmap_create_from_data (const uint8_t *data, int width, int height,gi_bool_t x11_comp, int *pitch);

static void gdk_pixmap_impl_gix_init       (GdkPixmapImplGix      *pixmap);
static void gdk_pixmap_impl_gix_class_init (GdkPixmapImplGixClass *klass);
static void gdk_pixmap_impl_gix_finalize   (GObject                    *object);

static gpointer parent_class = NULL;

GType
gdk_pixmap_impl_gix_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
        {
          sizeof (GdkPixmapImplGixClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) gdk_pixmap_impl_gix_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (GdkPixmapImplGix),
          0,              /* n_preallocs */
          (GInstanceInitFunc) gdk_pixmap_impl_gix_init,
        };

      object_type = g_type_register_static (GDK_TYPE_DRAWABLE_IMPL_GIX,
                                            "GdkPixmapImplGix",
                                            &object_info, 0);
    }

  return object_type;
}

GType
_gdk_pixmap_impl_get_type (void)
{
  return gdk_pixmap_impl_gix_get_type ();
}

static void
gdk_pixmap_impl_gix_init (GdkPixmapImplGix *impl)
{
  GdkDrawableImplGix *draw_impl = GDK_DRAWABLE_IMPL_GIX (impl);
  draw_impl->width  = 1;
  draw_impl->height = 1;
}

static void
gdk_pixmap_impl_gix_class_init (GdkPixmapImplGixClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_pixmap_impl_gix_finalize;
}

static void
gdk_pixmap_impl_gix_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

GdkPixmap*
gdk_pixmap_new (GdkDrawable *drawable,
                gint       width,
                gint       height,
                gint       depth)
{
  GIXSurfacePixelFormat    format;
  GdkPixmap               *pixmap;
  GdkDrawableImplGix *draw_impl;

  g_return_val_if_fail (drawable == NULL || GDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (drawable != NULL || depth != -1, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  if (!drawable)
    drawable = _gdk_parent_root;

  if (GDK_IS_WINDOW (drawable) && GDK_WINDOW_DESTROYED (drawable))
    return NULL;

  GDK_NOTE (MISC, g_print ("gdk_pixmap_new: %dx%dx%d\n",
                           width, height, depth));

  if (depth == -1)
    {
      draw_impl =
        GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (drawable)->impl);

      g_return_val_if_fail (draw_impl != NULL, NULL);

	  format = gi_screen_format();
	  depth = GI_RENDER_FORMAT_BPP (format);      
    }
  else
    {
      switch (depth)
        {
        case  1:
          format = GI_RENDER_a8;
          break;
        case  8:
          format = GI_RENDER_r3g3b2;
          break;
        case 15:
          format = GI_RENDER_x1r5g5b5;
          break;
        case 16:
          format = GI_RENDER_r5g6b5;
          break;
        case 24:
          format = GI_RENDER_r8g8b8;
          break;
        case 32:
          format = GI_RENDER_x8r8g8b8;
          break;
        default:
          g_message ("unimplemented %s for depth %d", __FUNCTION__, depth);
          return NULL;
        }
    }

 
  pixmap = g_object_new (gdk_pixmap_get_type (), NULL);
  draw_impl = GDK_DRAWABLE_IMPL_GIX (GDK_PIXMAP_OBJECT (pixmap)->impl);
  draw_impl->window_id = gi_create_pixmap_window(
	  GDK_DRAWABLE_IMPL_GIX (drawable)->window_id,width,height,format);

  //surface->Clear (surface, 0x0, 0x0, 0x0, 0x0);
  //surface->GetSize (surface, &draw_impl->width, &draw_impl->height);
  //surface->GetPixelFormat (surface, &draw_impl->format);

  draw_impl->width = width;
  draw_impl->height = height;
  draw_impl->format = format;
  draw_impl->abs_x = draw_impl->abs_y = 0;
  GDK_PIXMAP_OBJECT (pixmap)->depth = depth;
  return pixmap;
}


GdkPixmap *
gdk_bitmap_create_from_data (GdkDrawable   *drawable,
                             const gchar *data,
                             gint         width,
                             gint         height)
{
  GdkPixmap *pixmap;
  gi_gc_ptr_t gc;

  g_return_val_if_fail (drawable == NULL || GDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  GDK_NOTE (MISC, g_print ("gdk_bitmap_create_from_data: %dx%d\n",
                           width, height));

  pixmap = gdk_pixmap_new (drawable, width, height, 1);

#define GET_PIXEL(data,pixel) \
  ((data[(pixel / 8)] & (0x1 << ((pixel) % 8))) >> ((pixel) % 8))

  if (pixmap)
    {
      guchar *dst;
      gint    pitch;

	  gc = gi_create_gc(GDK_DRAWABLE_IMPL_GIX (GDK_PIXMAP_OBJECT (pixmap)->impl)->window_id,NULL);

#if 1
	  dst = bitmap_create_from_data((const uint8_t *)data, width,height,TRUE, &pitch);
	  if(dst){
		gi_image_t img;
		img.rgba = (gi_color_t*)dst;
		img.format = GI_RENDER_a8;
		img.w = width;
		img.h = height;
		img.pitch = pitch;
		gi_draw_image(gc,&img,0,0);
		free(dst);
	  }
#else
	  gint i, j;
      surface = gi_create_image_depth(width,height,GI_RENDER_a8);
	  dst = surface->rgba;
	  pitch = surface->pitch;

	  for (i = 0; i < height; i++)
		{
	    for (j = 0; j < width; j++){
		  dst[j] = GET_PIXEL (data, j) * 255;
		}
		data += (width + 7) / 8;
		dst += pitch;
		}

		gi_draw_image(gc,surface,0,0);
		gi_destroy_image(surface);
#endif
		gi_destroy_gc(gc);

    }

#undef GET_PIXEL

  return pixmap;
}

GdkPixmap*
gdk_pixmap_create_from_data (GdkDrawable   *drawable,
                             const gchar *data,
                             gint         width,
                             gint         height,
                             gint         depth,
                             const GdkColor    *fg,
                             const GdkColor    *bg)
{
  GdkPixmap *pixmap;
  gi_gc_ptr_t gc;

  g_return_val_if_fail (drawable == NULL || GDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (drawable != NULL || depth > 0, NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  GDK_NOTE (MISC, g_print ("gdk_pixmap_create_from_data: %dx%dx%d\n",
                           width, height, depth));

  pixmap = gdk_pixmap_new (drawable, width, height, depth);

  if (pixmap)
    {
	  gi_window_id_t wid;
      gi_image_t* surface;
      gchar            *dst;
      gint              pitch;
      gint              src_pitch;
	  unsigned format ;	  

      depth = gdk_drawable_get_depth (pixmap);
      src_pitch = width * ((depth + 7) / 8);
	  format = gi_get_choose_format(depth); //FIMXE

      wid = GDK_DRAWABLE_IMPL_GIX (GDK_PIXMAP_OBJECT (pixmap)->impl)->window_id;

	   surface=gi_create_image_depth(width,height,format);
	   dst = (gchar*)surface->rgba;
	   pitch = surface->pitch;

        {
          gint i;

          for (i = 0; i < height; i++)
            {
              memcpy (dst, data, src_pitch);
              dst += pitch;
              data += src_pitch;
            }

		gc = gi_create_gc(wid,NULL);
			gi_draw_image(gc,surface,0,0);
		gi_destroy_gc(gc);
		gi_destroy_image(surface);

        }
    }

  return pixmap;
}

GdkPixmap*
gdk_pixmap_foreign_new (GdkNativeWindow anid)
{
  g_warning(" gdk_pixmap_foreign_new unsuporrted \n");
  return NULL;
}

GdkPixmap*
gdk_pixmap_foreign_new_for_display (GdkDisplay *display, GdkNativeWindow anid)
{
  return gdk_pixmap_foreign_new(anid);
}

GdkPixmap*
gdk_pixmap_foreign_new_for_screen (GdkScreen       *screen,
                                   GdkNativeWindow  anid,
                                   gint             width,
                                   gint             height,
                                   gint             depth)
{
  /*Use the root drawable for now since only one screen */
  return gdk_pixmap_new(NULL,width,height,depth);
}


GdkPixmap*
gdk_pixmap_lookup (GdkNativeWindow anid)
{
  g_warning(" gdk_pixmap_lookup unsuporrted \n");
  return NULL;
}

GdkPixmap* gdk_pixmap_lookup_for_display (GdkDisplay *display,GdkNativeWindow anid)
{
  return gdk_pixmap_lookup (anid);
}

#define __GDK_PIXMAP_X11_C__
#include "gdkaliasdef.c"
