/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#include <assert.h>

#include <string.h>

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "../../gdk-pixbuf/gdk-pixbuf-private.h"

#include "gdkinternals.h"


#include "gdkregion-generic.h"
#include "gdkalias.h"

#include <cairo-gix.h>



#define ATTCH_WINDOW(gc,win) do{\
	err = gi_gc_attch_window(gc,win);\
	if (win == GI_DESKTOP_WINDOW_ID)\
	 g_print("%s: line%d gc attch window on root window!\n",__FUNCTION__,__LINE__);\
	if (err){\
		g_print("%s: line %d gc attch window on(%d) FAILED!\n",__FUNCTION__,__LINE__,win);\
		return;\
	}\
	}\
	while (0)

void
gdk_gix_draw_rectangle (GdkDrawable *drawable,
			GdkGC       *gc,
			gboolean     filled,
			gint         x,
			gint         y,
			gint         width,
			gint         height);

/* From Gix's <gfx/generix/duffs_device.h> */
#define DUFF_1() \
               case 1:\
                    SET_PIXEL( D[0], S[0] );

#define DUFF_2() \
               case 3:\
                    SET_PIXEL( D[2], S[2] );\
               case 2:\
                    SET_PIXEL( D[1], S[1] );\
               DUFF_1()

#define DUFF_3() \
               case 7:\
                    SET_PIXEL( D[6], S[6] );\
               case 6:\
                    SET_PIXEL( D[5], S[5] );\
               case 5:\
                    SET_PIXEL( D[4], S[4] );\
               case 4:\
                    SET_PIXEL( D[3], S[3] );\
               DUFF_2()

#define DUFF_4() \
               case 15:\
                    SET_PIXEL( D[14], S[14] );\
               case 14:\
                    SET_PIXEL( D[13], S[13] );\
               case 13:\
                    SET_PIXEL( D[12], S[12] );\
               case 12:\
                    SET_PIXEL( D[11], S[11] );\
               case 11:\
                    SET_PIXEL( D[10], S[10] );\
               case 10:\
                    SET_PIXEL( D[9], S[9] );\
               case 9:\
                    SET_PIXEL( D[8], S[8] );\
               case 8:\
                    SET_PIXEL( D[7], S[7] );\
               DUFF_3()

#define SET_PIXEL_DUFFS_DEVICE_N( D, S, w, n ) \
do {\
     while (w) {\
          register int l = w & ((1 << n) - 1);\
          switch (l) {\
               default:\
                    l = (1 << n);\
                    SET_PIXEL( D[(1 << n)-1], S[(1 << n)-1] );\
               DUFF_##n()\
          }\
          D += l;\
          S += l;\
          w -= l;\
     }\
} while(0)


static GdkScreen * gdk_gix_get_screen (GdkDrawable    *drawable);
static void gdk_drawable_impl_gix_class_init (GdkDrawableImplGixClass *klass);
static void gdk_gix_draw_lines (GdkDrawable *drawable,
                                     GdkGC       *gc,
                                     GdkPoint    *points,
                                     gint         npoints);

static cairo_surface_t *gdk_gix_ref_cairo_surface (GdkDrawable *drawable);


static gpointer  parent_class               = NULL;
static const cairo_user_data_key_t gdk_gix_cairo_key;

static void (*real_draw_pixbuf) (GdkDrawable *drawable,
                                 GdkGC       *gc,
                                 GdkPixbuf   *pixbuf,
                                 gint         src_x,
                                 gint         src_y,
                                 gint         dest_x,
                                 gint         dest_y,
                                 gint         width,
                                 gint         height,
                                 GdkRgbDither dither,
                                 gint         x_dither,
                                 gint         y_dither);


/**********************************************************
 * Gix specific implementations of generic functions *
 **********************************************************/


static void
gdk_gix_set_colormap (GdkDrawable *drawable,
                           GdkColormap *colormap)
{
  GdkDrawableImplGix *impl;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  D_DEBUG_AT(  "%s( %p, %p ) <- old %p\n", __FUNCTION__, drawable, colormap, impl->colormap );

  if (impl->colormap == colormap)
    return;

  if (impl->colormap)
    g_object_unref (impl->colormap);

  impl->colormap = colormap;

  if (colormap)
    g_object_ref (colormap);
}

static GdkColormap*
gdk_gix_get_colormap (GdkDrawable *drawable)
{
  GdkColormap *retval;

  retval = GDK_DRAWABLE_IMPL_GIX (drawable)->colormap;

  if (!retval) {
    retval = gdk_colormap_get_system ();
	gdk_gix_set_colormap(drawable,retval);
  }

  return retval;
}

static gint
gdk_gix_get_depth (GdkDrawable *drawable)
{
  GdkDrawableImplGix *impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  return GI_RENDER_FORMAT_BPP (impl->format);
}

static void
gdk_gix_get_size (GdkDrawable *drawable,
                       gint        *width,
                       gint        *height)
{
  GdkDrawableImplGix *impl;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  if (width)
    *width = impl->width;

  if (height)
    *height = impl->height;
}

static GdkVisual*
gdk_gix_get_visual (GdkDrawable *drawable)
{
  return gdk_visual_get_system ();
}

/* Calculates the real clipping region for a drawable, taking into account
 * other windows and the gc clip region.
 */
#ifdef USE_GC_BODY_REGION
void
gdk_gix_clip_region (GdkDrawable  *drawable,
                          GdkGC        *gc,
                          GdkRectangle *draw_rect,
                          GdkRegion    *ret_clip)
{
  GdkDrawableImplGix *private;
  GdkRectangle             rect;

  g_return_if_fail (GDK_IS_DRAWABLE (drawable));
  g_return_if_fail (GDK_IS_DRAWABLE_IMPL_GIX (drawable));
  g_return_if_fail (ret_clip != NULL);

  D_DEBUG_AT(  "%s( %p, %p, %p )\n", __FUNCTION__, drawable, gc, draw_rect );

  private = GDK_DRAWABLE_IMPL_GIX (drawable);

  if (!draw_rect)
    {
      rect.x      = 0;
      rect.y      = 0;
      rect.width  = private->width;
      rect.height = private->height;

      draw_rect = &rect;
    }
  D_DEBUG_AT(  "  -> draw rectangle   == %4d,%4d - %4dx%4d =\n",
              draw_rect->x, draw_rect->y, draw_rect->width, draw_rect->height );

  temp_region_init_rectangle( ret_clip, draw_rect );

  if (private->buffered) {
       D_DEBUG_AT(  "  -> buffered region   > %4d,%4d - %4dx%4d <  (%ld boxes)\n",
                   GDKGIX_RECTANGLE_VALS_FROM_BOX( &private->paint_region.extents ),
                   private->paint_region.numRects );

    gdk_region_intersect (ret_clip, &private->paint_region);
  }

  if (gc)
    {
      GdkGCGix *gc_private = GDK_GC_GIX (gc);
      GdkRegion     *region     = &gc_private->clip_region;

      if (region->numRects)
        {
          D_DEBUG_AT(  "  -> clipping region   > %4d,%4d - %4dx%4d <  (%ld boxes)\n",
                      GDKGIX_RECTANGLE_VALS_FROM_BOX( &region->extents ), region->numRects );

          if (gc->clip_x_origin || gc->clip_y_origin)
            {
              gdk_region_offset (ret_clip, -gc->clip_x_origin, -gc->clip_y_origin);
              gdk_region_intersect (ret_clip, region);
              gdk_region_offset (ret_clip, gc->clip_x_origin, gc->clip_y_origin);
            }
          else
            {
              gdk_region_intersect (ret_clip, region);
            }
        }

      if (gc_private->values_mask & GDK_GC_SUBWINDOW &&
          gc_private->values.subwindow_mode == GDK_INCLUDE_INFERIORS)
        return;
    }

  if (private->buffered) {
       D_DEBUG_AT(  "  => returning clip   >> %4d,%4d - %4dx%4d << (%ld boxes)\n",
                   GDKGIX_RECTANGLE_VALS_FROM_BOX( &ret_clip->extents ), ret_clip->numRects );
    return;
  }

  if (GDK_IS_WINDOW (private->wrapper) &&
      GDK_WINDOW_IS_MAPPED (private->wrapper) &&
      !GDK_WINDOW_OBJECT (private->wrapper)->input_only)
    {
      GList     *cur;
      GdkRegion  temp;

      temp.numRects = 1;
      temp.rects = &temp.extents;
      temp.size = 1;

      for (cur = GDK_WINDOW_OBJECT (private->wrapper)->children;
           cur;
           cur = cur->next)
        {
          GdkWindowObject         *cur_private;
          GdkDrawableImplGix *cur_impl;

          cur_private = GDK_WINDOW_OBJECT (cur->data);

          if (!GDK_WINDOW_IS_MAPPED (cur_private) || cur_private->input_only)
            continue;

          cur_impl = GDK_DRAWABLE_IMPL_GIX (cur_private->impl);

          temp.extents.x1 = cur_private->x;
          temp.extents.y1 = cur_private->y;
          temp.extents.x2 = cur_private->x + cur_impl->width;
          temp.extents.y2 = cur_private->y + cur_impl->height;

          D_DEBUG_AT(  "  -> clipping child    [ %4d,%4d - %4dx%4d ]  (%ld boxes)\n",
                      GDKGIX_RECTANGLE_VALS_FROM_BOX( &temp.extents ), temp.numRects );

          gdk_region_subtract (ret_clip, &temp);
        }
    }

  D_DEBUG_AT(  "  => returning clip   >> %4d,%4d - %4dx%4d << (%ld boxes)\n",
              GDKGIX_RECTANGLE_VALS_FROM_BOX( &ret_clip->extents ), ret_clip->numRects );
}
#endif

void
gdk_gix_draw_rectangle (GdkDrawable *drawable,
			GdkGC       *gc,
			gboolean     filled,
			gint         x,
			gint         y,
			gint         width,
			gint         height)
{
  GdkDrawableImplGix *impl;
  GdkGCGix           *gc_private = NULL;  
  int err;
  //GdkColor               color = { 0, 0, 0, 0 };

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  gc_private = GDK_GC_GIX (gc);
  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);

  if (filled){
    err = gi_fill_rect ( GDK_GC_GET_XGC (gc), x, y, width, height);	
  }
  else{
    err = gi_draw_rect ( GDK_GC_GET_XGC (gc) , x, y, width, height);
  }

#if 1  
  if (err)
  {
  perror("gdk_gix_draw_rectangle: failed\n");
  g_print("_gdk_gix_draw_rectangle window_tl = %x,fill%d, (%d,%d,%d,%d) \n",
	  impl->window_id,filled, x, y, width, height);
  }
#endif  
}


static void
gdk_gix_draw_arc (GdkDrawable *drawable,
                       GdkGC       *gc,
                       gint         filled,
                       gint         x,
                       gint         y,
                       gint         width,
                       gint         height,
                       gint         angle1,
                       gint         angle2)
{
  GdkDrawableImplGix *impl;
  GdkGCGix           *gc_private = NULL;  
  int err;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  gc_private = GDK_GC_GIX (gc);
  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);

  D_UNIMPLEMENTED;

  if (filled)
    err = gi_fill_arc ( impl->window_id,
	      GDK_GC_GET_XGC (gc), x, y, width, height, angle1, angle2);
  else
    err = gi_draw_arc ( impl->window_id,
	      GDK_GC_GET_XGC (gc), x, y, width, height, angle1, angle2);

  if (err)
  {
	  perror("gdk_gix_draw_arc");
  }

}

static void
gdk_gix_draw_polygon (GdkDrawable    *drawable,
		     GdkGC          *gc,
		     gboolean        filled,
		     GdkPoint       *points,
		     gint            npoints)
{
  gi_point_t *tmp_points;
  gint tmp_npoints, i,err;
  GdkDrawableImplGix *impl;

  D_UNIMPLEMENTED;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);

  
  if (!filled &&
      (points[0].x != points[npoints-1].x || points[0].y != points[npoints-1].y))
    {
      tmp_npoints = npoints + 1;
      tmp_points = g_new (gi_point_t, tmp_npoints);
      tmp_points[npoints].x = points[0].x;
      tmp_points[npoints].y = points[0].y;
    }
  else
    {
      tmp_npoints = npoints;
      tmp_points = g_new (gi_point_t, tmp_npoints);
    }

  for (i=0; i<npoints; i++)
    {
      tmp_points[i].x = points[i].x;
      tmp_points[i].y = points[i].y;
    }
  
  if (filled)
    err = gi_fill_polygon ( 
		  GDK_GC_GET_XGC (gc), tmp_points, tmp_npoints, GI_SHAPE_Complex, GI_POLY_CoordOrigin);
  else
    err = gi_draw_lines ( 
		GDK_GC_GET_XGC (gc), tmp_points, tmp_npoints);

  g_free (tmp_points);
  if (err)
  {
	  perror("gdk_gix_draw_polygon");
  }
}

static void
gdk_gix_draw_text (GdkDrawable *drawable,
                        GdkFont     *font,
                        GdkGC       *gc,
                        gint         x,
                        gint         y,
                        const gchar *text,
                        gint         text_length)
{
  D_UNIMPLEMENTED;
}

static void
gdk_gix_draw_text_wc (GdkDrawable    *drawable,
                           GdkFont        *font,
                           GdkGC          *gc,
                           gint            x,
                           gint            y,
                           const GdkWChar *text,
                           gint            text_length)
{
  D_UNIMPLEMENTED;
}
static void
gdk_gix_draw_drawable (GdkDrawable *drawable,
                            GdkGC       *gc,
                            GdkDrawable *src,
                            gint         xsrc,
                            gint         ysrc,
                            gint         xdest,
                            gint         ydest,
                            gint         width,
                            gint         height)
{
  GdkDrawableImplGix *impl;
  GdkDrawableImplGix *src_impl;
  //GdkRegion               *clip;
  int src_depth = gdk_drawable_get_depth (src);
  int dest_depth = gdk_drawable_get_depth (drawable);

  /*GdkRectangle             dest_rect = { xdest,
                                         ydest, 
                                         xdest + width  - 1,
                                         ydest + height - 1 };
										 */

  //gi_cliprect_t rect = { xsrc, ysrc, width, height };
  gint err = 0;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  if (GDK_IS_PIXMAP (src))
    src_impl = GDK_DRAWABLE_IMPL_GIX (GDK_PIXMAP_OBJECT (src)->impl);
  else if (GDK_IS_WINDOW (src))
    src_impl = GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (src)->impl);
  else if (GDK_IS_DRAWABLE_IMPL_GIX (src))
    src_impl = GDK_DRAWABLE_IMPL_GIX (src);
  else
    return;		 

  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);
 

  if (src_depth == 1)
    {	  
      err = gi_copy_area (  src_impl->window_id ,
		 impl->window_id,
		 GDK_GC_GET_XGC (gc),
		 xsrc, ysrc,
		 width, height,
		 xdest, ydest);
    }
  else if (dest_depth != 0 && src_depth == dest_depth)
    {
      err = gi_copy_area (  src_impl->window_id ,
		 impl->window_id,
		 GDK_GC_GET_XGC (gc),
		 xsrc, ysrc,
		 width, height,
		 xdest, ydest);
    }
  else
    g_warning ("Attempt to draw a drawable with depth %d to a drawable with depth %d",
               src_depth, dest_depth);

  if (err)
  {
	  perror("gdk_gix_draw_drawable #1");
	    /*g_print("gdk_gix_draw_drawable src=%x, ###dst=%x,  (%d,%d,%d,%d,%d,%d)\n", 
	  src_impl ? src_impl->window_id : 0,
		 impl->window_id,
		 xsrc, ysrc,
		 width, height,
		 xdest, ydest);
		 */
  }


}


static void
gdk_gix_draw_points (GdkDrawable *drawable,
		     GdkGC       *gc,
		     GdkPoint    *points,
		     gint         npoints)
{
  GdkDrawableImplGix *impl;
  //GdkRegion               *clip;
  int err;

  if (npoints < 1)
    return;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);
 
  
  /* We special-case npoints == 1, because X will merge multiple
   * consecutive XDrawPoint requests into a PolyPoint request
   */
  if (npoints == 1)
    {
	  gi_point_t tmp_points[1];

	  tmp_points[0].x = points[0].x;
	  tmp_points[0].y = points[0].y;

      gi_draw_points (
		   GDK_GC_GET_XGC (gc),
		   tmp_points,
		   npoints);
    }
  else
    {
      gint i;
      gi_point_t *tmp_points = g_new (gi_point_t, npoints);

      for (i=0; i<npoints; i++)
	{
	  tmp_points[i].x = points[i].x;
	  tmp_points[i].y = points[i].y;
	}
      
      gi_draw_points (
		   GDK_GC_GET_XGC (gc),
		   tmp_points,
		   npoints);

      g_free (tmp_points);
    }
}


static void
gdk_gix_draw_segments (GdkDrawable    *drawable,
		      GdkGC          *gc,
		      GdkSegment     *segs,
		      gint            nsegs)
{
  gi_point_t pts[2];
  int i, err;
  GdkDrawableImplGix *impl;

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);

  for(i = 0; i < nsegs; i++)
    {
      pts[0].x = segs[i].x1;
      pts[0].y = segs[i].y1;
      pts[1].x = segs[i].x2;
      pts[1].y = segs[i].y2;

	  gi_draw_lines( GDK_GC_GET_XGC (gc), pts,2);
    }
}


static void
gdk_gix_draw_lines (GdkDrawable *drawable,
                         GdkGC       *gc,
                         GdkPoint    *points,
                         gint         npoints)
{
  GdkDrawableImplGix *impl;
 // gint                     i;
  gi_point_t beg, end;
  int err;

  g_print("gdk_gix_draw_lines = %d\n", npoints);

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  beg.x = points->x;
	beg.y = points->y;
	ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);

  
  ++points;
	while (--npoints > 0) 
    {
	  end.x = points->x;
		end.y = points->y;
		
		++points;
		gi_draw_line( GDK_GC_GET_XGC (gc), beg.x, beg.y, end.x, end.y);
		beg = end;
    }
      
 
}


static void
gdk_gix_draw_image (GdkDrawable *drawable,
                         GdkGC       *gc,
                         GdkImage    *image,
                         gint         xsrc,
                         gint         ysrc,
                         gint         xdest,
                         gint         ydest,
                         gint         width,
                         gint         height)
{
  GdkDrawableImplGix *impl;
  GdkImageGix        *image_private;
  //GdkRegion                clip;
  //GdkRectangle             dest_rect = { xdest, ydest, width, height };

  //gint pitch = 0;
  //gint i;
  GdkGCGix           *gc_private = NULL;
  int err = 0;

  gc_private = GDK_GC_GIX (gc);

  g_return_if_fail (GDK_IS_DRAWABLE (drawable));
  g_return_if_fail (image != NULL);

  D_DEBUG_AT(  "%s( %p, %p, %p, %4d,%4d -> %4d,%4d - %dx%d )\n", __FUNCTION__,
              drawable, gc, image, xsrc, ysrc, xdest, ydest, width, height );

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  image_private = image->windowing_data;

  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);
 
  if(image_private->surface_img)
    err = gi_bitblt_image( GDK_GC_GET_XGC (gc),xsrc, ysrc,
	  width, height,image_private->surface_img,xdest, ydest);

  if (err)
  {
	  perror("gdk_gix_draw_image");
  }


}

#define RGB2PIXEL565(r,g,b)	\
	((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

static void
composite_565(guchar      *src_buf,
                gint         src_rowstride,
                guchar      *dest_buf,
                gint         dest_rowstride,
                
                gint         width,
                gint         height)
{
  guchar *src = src_buf;
  guchar *dest = dest_buf;

  while (height--)
    {
      gint twidth = width;   
	  uint32_t *p = (uint32_t *)src;
	  uint16_t *q = (uint16_t *)dest;

	  while (twidth--)
		{
		  //guint t;
		  //t = *p;
		  *q = RGB2PIXEL565( GI_BLUE(*p),GI_GREEN(*p),GI_RED(*p));
		  q++;
		  p++;            
		}    
      src += src_rowstride;
      dest += dest_rowstride;
    }
}

static void
composite_0888 (guchar      *src_buf,
                gint         src_rowstride,
                guchar      *dest_buf,
                gint         dest_rowstride,
               
                gint         width,
                gint         height)
{
  guchar *src = src_buf;
  guchar *dest = dest_buf;

  while (height--)
    {
      gint twidth = width;   
	  uint32_t *p = (uint32_t *)src;
	  uint32_t *q = (uint32_t *)dest;

	  while (twidth--)
		{
		  *q = GI_ARGB(GI_ALPHA(*p), GI_BLUE(*p),GI_GREEN(*p),GI_RED(*p));
		  q++;
		  p++;            
		}    
      src += src_rowstride;
      dest += dest_rowstride;
    }
}


static inline void
convert_rgba_pixbuf_to_image (guint32 *src,
                              guint    src_pitch,
                              guint32 *dest,
                              guint    dest_pitch,
                              guint    width,
                              guint    height)
{
  guint i;

  while (height--)
    {
      for (i = 0; i < width; i++)
        {
          guint32 pixel = GUINT32_FROM_BE (src[i]);
          dest[i] = (pixel >> 8) | (pixel << 24);
        }

      src  += src_pitch;
      dest += dest_pitch;
    }
}

static inline void
convert_rgb_pixbuf_to_image (guchar  *src,
                             guint    src_pitch,
                             guint32 *dest,
                             guint    dest_pitch,
                             guint    width,
                             guint    height)
{
  guint   i;
  guchar *s;

  while (height--)
    {
      s = src;

      for (i = 0; i < width; i++, s += 3)
        dest[i] = 0xFF000000 | (s[0] << 16) | (s[1] << 8) | s[2];

      src  += src_pitch;
      dest += dest_pitch;
    }
}



static void
gdk_gix_draw_pixbuf (GdkDrawable  *drawable,
                          GdkGC        *gc,
                          GdkPixbuf    *pixbuf,
                          gint          src_x,
                          gint          src_y,
                          gint          dest_x,
                          gint          dest_y,
                          gint          width,
                          gint          height,
                          GdkRgbDither  dither,
                          gint          x_dither,
                          gint          y_dither)
{
  GdkDrawableImplGix *impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  gi_image_t *gimg = NULL;
  int err;
  unsigned gformat;

  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (pixbuf->colorspace == GDK_COLORSPACE_RGB);
  g_return_if_fail (pixbuf->n_channels == 3 || pixbuf->n_channels == 4);
  g_return_if_fail (pixbuf->bits_per_sample == 8);

  g_return_if_fail (drawable != NULL);

  if (width == -1) 
    width = pixbuf->width;
  if (height == -1)
    height = pixbuf->height;

  g_return_if_fail (width >= 0 && height >= 0);
  g_return_if_fail (src_x >= 0 && src_x + width <= pixbuf->width);
  g_return_if_fail (src_y >= 0 && src_y + height <= pixbuf->height);

  D_DEBUG_AT(  "%s( %p, %p, %p, %4d,%4d -> %4d,%4d - %dx%d )\n", __FUNCTION__,
              drawable, gc, pixbuf, src_x, src_y, dest_x, dest_y, width, height );

  /* Clip to the drawable; this is required for get_from_drawable() so
   * can't be done implicitly
   */
  
  if (dest_x < 0)
    {
      src_x -= dest_x;
      width += dest_x;
      dest_x = 0;
    }

  if (dest_y < 0)
    {
      src_y -= dest_y;
      height += dest_y;
      dest_y = 0;
    }

  if ((dest_x + width) > impl->width)
    width = impl->width - dest_x;

  if ((dest_y + height) > impl->height)
    height = impl->height - dest_y;

  if (width <= 0 || height <= 0)
    return;


  gi_bool_t has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
#if 0
  img.w = gdk_pixbuf_get_width (pixbuf);
  img.h = gdk_pixbuf_get_height (pixbuf);
  img.pitch = gdk_pixbuf_get_rowstride (pixbuf);
  img.rgba = gdk_pixbuf_get_pixels (pixbuf);
  if (has_alpha)
	  img.format = GI_RENDER_a8r8g8b8;
  else
	  img.format = GI_RENDER_x8r8g8b8;
#endif

	GdkVisual *visual = gdk_drawable_get_visual (drawable);

	if (has_alpha)
		gformat = GI_RENDER_a8r8g8b8;
	else{
		if (visual->byte_order == (G_BYTE_ORDER == G_BIG_ENDIAN ? GDK_MSB_FIRST : GDK_LSB_FIRST) &&
              visual->depth == 16 &&
              visual->red_mask   == 0xf800 &&
              visual->green_mask == 0x07e0 &&
              visual->blue_mask  == 0x001f){
			gformat = GI_RENDER_r5g6b5;
		}
		else
			gformat = GI_RENDER_x8r8g8b8;
	}

	gimg = gi_create_image_depth(width, height,gformat);
	if (!gimg)
		return;
 
	void *data;
    int   pitch;

	data = gimg->rgba;
	pitch = gimg->pitch;

	if (gformat == GI_RENDER_r5g6b5)
	{
		composite_565 (pixbuf->pixels + src_y * pixbuf->rowstride + src_x * 4,
                                  pixbuf->rowstride,
                                  data,
                                  pitch,                                  
                                  width, height);
	}
	else{
		// convert_rgba_pixbuf_to_image
			composite_0888 (pixbuf->pixels + src_y * pixbuf->rowstride + src_x * 4,
                                  pixbuf->rowstride,
                                  data,
                                  pitch,                                 
                                  width, height);
	}
  
  ATTCH_WINDOW(GDK_GC_GET_XGC (gc),impl->window_id);
  err = gi_bitblt_image( GDK_GC_GET_XGC (gc),src_x, src_y,width, height,
	  gimg,dest_x, dest_y);

  gi_destroy_image(gimg);
  return;

}

/*
 * Object stuff
 */
static inline const char *
drawable_impl_type_name( GObject *object )
{
     if (GDK_IS_PIXMAP (object))
          return "PIXMAP";

     if (GDK_IS_WINDOW (object))
          return "WINDOW";

     if (GDK_IS_DRAWABLE_IMPL_GIX (object))
          return "DRAWABLE";

     return "unknown";
}


static void
gdk_drawable_impl_gix_finalize (GObject *object)
{
  GdkDrawableImplGix *impl;
  impl = GDK_DRAWABLE_IMPL_GIX (object);

  D_DEBUG_AT(  "%s( %p ) <- %dx%d (%s at %4d,%4d)\n", __FUNCTION__,
              object, impl->width, impl->height,
              drawable_impl_type_name( object ),
              impl->abs_x, impl->abs_y );

  gdk_gix_set_colormap (GDK_DRAWABLE (object), NULL);
  if( impl->cairo_surface ) {
	cairo_surface_finish(impl->cairo_surface);
  }

  //WARING_MSG; 
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gdk_drawable_impl_gix_class_init (GdkDrawableImplGixClass *klass)
{
  GdkDrawableClass *drawable_class = GDK_DRAWABLE_CLASS (klass);
  GObjectClass     *object_class   = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_drawable_impl_gix_finalize;

  drawable_class->create_gc      = _gdk_gix_gc_new;
  drawable_class->draw_rectangle = gdk_gix_draw_rectangle;
  drawable_class->draw_arc       = gdk_gix_draw_arc;
  drawable_class->draw_polygon   = gdk_gix_draw_polygon;
  drawable_class->draw_text      = gdk_gix_draw_text;
  drawable_class->draw_text_wc   = gdk_gix_draw_text_wc;
  drawable_class->draw_drawable  = gdk_gix_draw_drawable;
  drawable_class->draw_points    = gdk_gix_draw_points;
  drawable_class->draw_segments  = gdk_gix_draw_segments;
  drawable_class->draw_lines     = gdk_gix_draw_lines;
  drawable_class->draw_image     = gdk_gix_draw_image;

  drawable_class->ref_cairo_surface = gdk_gix_ref_cairo_surface;
  drawable_class->set_colormap   = gdk_gix_set_colormap;
  drawable_class->get_colormap   = gdk_gix_get_colormap;

  drawable_class->get_depth      = gdk_gix_get_depth;
  drawable_class->get_visual     = gdk_gix_get_visual;
  drawable_class->get_size       = gdk_gix_get_size;

  drawable_class->_copy_to_image = _gdk_gix_copy_to_image;
  drawable_class->get_screen = gdk_gix_get_screen;

  real_draw_pixbuf = drawable_class->draw_pixbuf;
  drawable_class->draw_pixbuf = gdk_gix_draw_pixbuf; 
}

GType
gdk_drawable_impl_gix_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
        {
          sizeof (GdkDrawableImplGixClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) gdk_drawable_impl_gix_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (GdkDrawableImplGix),
          0,              /* n_preallocs */
          (GInstanceInitFunc) NULL,
        };

      object_type = g_type_register_static (GDK_TYPE_DRAWABLE,
                                            "GdkDrawableImplGix",
                                            &object_info, 0);
    }

  return object_type;
}

static GdkScreen * gdk_gix_get_screen (GdkDrawable    *drawable){
        return gdk_screen_get_default();
}

static void
gdk_gix_cairo_surface_destroy (void *data)
{
  GdkDrawableImplGix *impl = data;
  impl->cairo_surface = NULL;
}


static cairo_surface_t *
gdk_gix_ref_cairo_surface (GdkDrawable *drawable)
{
  GdkDrawableImplGix *impl;
  
  g_return_val_if_fail (GDK_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GDK_IS_DRAWABLE_IMPL_GIX (drawable), NULL);

  impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  
  if (!impl->cairo_surface) {

    if (impl->window_id) {
      impl->cairo_surface = cairo_gix_surface_create (impl->window_id);
      if (impl->cairo_surface) {
        cairo_surface_set_user_data (impl->cairo_surface, 
                                     &gdk_gix_cairo_key, drawable, 
                                     gdk_gix_cairo_surface_destroy);
      }

    }
  } else {
    cairo_surface_reference (impl->cairo_surface);
  }
  
  g_assert (impl->cairo_surface != NULL);
  return impl->cairo_surface;
}


#define __GDK_DRAWABLE_X11_C__
#include "gdkaliasdef.c"
