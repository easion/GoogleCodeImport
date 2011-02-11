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
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#undef GDK_DISABLE_DEPRECATED

#include "config.h"
#include "gdk.h"

#include <string.h>

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkgc.h"
#include "gdkfont.h"
#include "gdkpixmap.h"
#include "gdkregion-generic.h"

#include "gdkalias.h"

static void gdk_gix_gc_get_values (GdkGC           *gc,
                                        GdkGCValues     *values);
static void gdk_gix_gc_set_values (GdkGC           *gc,
                                        GdkGCValues     *values,
                                        GdkGCValuesMask  values_mask);
static void gdk_gix_gc_set_dashes (GdkGC           *gc,
                                        gint             dash_offset,
                                        gint8            dash_list[],
                                        gint             n);

static void gdk_gc_gix_class_init (GdkGCGixClass *klass);
static void gdk_gc_gix_finalize   (GObject            *object);


static gpointer parent_class = NULL;


GType
gdk_gc_gix_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (GdkGCGixClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_gc_gix_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GdkGCGix),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };

      object_type = g_type_register_static (GDK_TYPE_GC,
                                            "GdkGCGix",
                                            &object_info, 0);
    }

  return object_type;
}

static void
gdk_gc_gix_class_init (GdkGCGixClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkGCClass   *gc_class     = GDK_GC_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_gc_gix_finalize;

  gc_class->get_values = gdk_gix_gc_get_values;
  gc_class->set_values = gdk_gix_gc_set_values;
  gc_class->set_dashes = gdk_gix_gc_set_dashes;
}

static void
gdk_gc_gix_finalize (GObject *object)
{
  GdkGC         *gc      = GDK_GC (object);
  GdkGCGix *private = GDK_GC_GIX (gc);

  if (private->gix_gc)
  {
	  //printf("gi_destroy_gc: called %x\n",private->gix_gc->gcid);
	  gi_destroy_gc(private->gix_gc);
	  private->gix_gc = NULL;
  }

  if (private->clip_region.numRects)
    temp_region_deinit (&private->clip_region);
  if (private->values.clip_mask)
    g_object_unref (private->values.clip_mask);
  if (private->values.stipple)
    g_object_unref (private->values.stipple);
  if (private->values.tile)
    g_object_unref (private->values.tile);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

GdkGC*
_gdk_gix_gc_new (GdkDrawable     *drawable,
                      GdkGCValues     *values,
                      GdkGCValuesMask  values_mask)
{
  GdkGC         *gc;
  GdkGCGix *private;

  g_return_val_if_fail (GDK_IS_DRAWABLE_IMPL_GIX (drawable), NULL);

  gc = GDK_GC (g_object_new (gdk_gc_gix_get_type (), NULL));

  _gdk_gc_init (gc, drawable, values, values_mask);

  private = GDK_GC_GIX (gc);
  private->gix_gc = gi_create_gc( GDK_DRAWABLE_IMPL_GIX (drawable)->window_id,NULL);
  if (!private->gix_gc)
	  return NULL;
#if 0
  private->values.background.pixel = 0;
  private->values.background.red   =
  private->values.background.green =
  private->values.background.blue  = 0;

  private->values.foreground.pixel = 0;
  private->values.foreground.red   =
  private->values.foreground.green =
  private->values.foreground.blue  = 0;
#endif

  private->values.cap_style = GDK_CAP_BUTT;

  gdk_gix_gc_set_values (gc, values, values_mask);

  return gc;
}

static void
gdk_gix_gc_get_values (GdkGC       *gc,
                            GdkGCValues *values)
{
  *values = GDK_GC_GIX (gc)->values;
}

#if 0
void
_gdk_windowing_gc_get_foreground (GdkGC    *gc,
                                  GdkColor *color)
{
  GdkGCGix *private;
  private = GDK_GC_GIX (gc);
  *color =private->values.foreground;


}
#endif

#if 0
static void
gdk_gix_gc_set_values (GdkGC           *gc,
                            GdkGCValues     *values,
                            GdkGCValuesMask  values_mask)
{
  GdkGCGix *private = GDK_GC_GIX (gc);

  if (values_mask & GDK_GC_FOREGROUND)
    {
      private->values.foreground = values->foreground;
      private->values_mask |= GDK_GC_FOREGROUND;
    }

  if (values_mask & GDK_GC_BACKGROUND)
    {
      private->values.background = values->background;
      private->values_mask |= GDK_GC_BACKGROUND;
    }

  if (values_mask & GDK_GC_FONT)
    {
      GdkFont *oldf = private->values.font;

      private->values.font = gdk_font_ref (values->font);
      private->values_mask |= GDK_GC_FONT;

      if (oldf)
        gdk_font_unref (oldf);
    }

  if (values_mask & GDK_GC_FUNCTION)
    {
      private->values.function = values->function;
      private->values_mask |= GDK_GC_FUNCTION;
    }

  if (values_mask & GDK_GC_FILL)
    {
      private->values.fill = values->fill;
      private->values_mask |= GDK_GC_FILL;
    }

  if (values_mask & GDK_GC_TILE)
    {
      GdkPixmap *oldpm = private->values.tile;

      if (values->tile)
        g_assert (GDK_PIXMAP_OBJECT (values->tile)->depth > 1);

      private->values.tile = values->tile ? g_object_ref (values->tile) : NULL;
      private->values_mask |= GDK_GC_TILE;

      if (oldpm)
        g_object_unref (oldpm);
    }

  if (values_mask & GDK_GC_STIPPLE)
    {
      GdkPixmap *oldpm = private->values.stipple;

      if (values->stipple)
        g_assert (GDK_PIXMAP_OBJECT (values->stipple)->depth == 1);

      private->values.stipple = (values->stipple ?
                                 g_object_ref (values->stipple) : NULL);
      private->values_mask |= GDK_GC_STIPPLE;

      if (oldpm)
        g_object_unref (oldpm);
    }

  if (values_mask & GDK_GC_CLIP_MASK)
    {
      GdkPixmap *oldpm = private->values.clip_mask;

      private->values.clip_mask = (values->clip_mask ?
                                   g_object_ref (values->clip_mask) : NULL);
      private->values_mask |= GDK_GC_CLIP_MASK;

      if (oldpm)
        g_object_unref (oldpm);

      temp_region_reset (&private->clip_region);
    }

  if (values_mask & GDK_GC_SUBWINDOW)
    {
      private->values.subwindow_mode = values->subwindow_mode;
      private->values_mask |= GDK_GC_SUBWINDOW;
    }

  if (values_mask & GDK_GC_TS_X_ORIGIN)
    {
      private->values.ts_x_origin = values->ts_x_origin;
      private->values_mask |= GDK_GC_TS_X_ORIGIN;
    }

  if (values_mask & GDK_GC_TS_Y_ORIGIN)
    {
      private->values.ts_y_origin = values->ts_y_origin;
      private->values_mask |= GDK_GC_TS_Y_ORIGIN;
    }

  if (values_mask & GDK_GC_CLIP_X_ORIGIN)
    {
      private->values.clip_x_origin = GDK_GC (gc)->clip_x_origin = values->clip_x_origin;
      private->values_mask |= GDK_GC_CLIP_X_ORIGIN;
    }

  if (values_mask & GDK_GC_CLIP_Y_ORIGIN)
    {
      private->values.clip_y_origin = GDK_GC (gc)->clip_y_origin = values->clip_y_origin;
      private->values_mask |= GDK_GC_CLIP_Y_ORIGIN;
    }

  if (values_mask & GDK_GC_EXPOSURES)
    {
      private->values.graphics_exposures = values->graphics_exposures;
      private->values_mask |= GDK_GC_EXPOSURES;
    }

  if (values_mask & GDK_GC_LINE_WIDTH)
    {
      private->values.line_width = values->line_width;
      private->values_mask |= GDK_GC_LINE_WIDTH;
    }

  if (values_mask & GDK_GC_LINE_STYLE)
    {
      private->values.line_style = values->line_style;
      private->values_mask |= GDK_GC_LINE_STYLE;
    }

  if (values_mask & GDK_GC_CAP_STYLE)
    {
      private->values.cap_style = values->cap_style;
      private->values_mask |= GDK_GC_CAP_STYLE;
    }

  if (values_mask & GDK_GC_JOIN_STYLE)
    {
      private->values.join_style = values->join_style;
      private->values_mask |= GDK_GC_JOIN_STYLE;
    }
}
#endif


static void
gdk_gix_gc_set_values (GdkGC           *gc,
                            GdkGCValues     *values,
                            GdkGCValuesMask  values_mask)
{
  GdkGCGix *private;
  int r=0,g=0,b=0;
  GdkColor color = { 0, 0, 0, 0 };
  guchar alpha = 0xFF;
  int flags;


  private = GDK_GC_GIX (gc);
  g_assert(private);
  g_assert(private->gix_gc);
  
  if (values_mask & GDK_GC_FOREGROUND)
    {
	  
	  
      private->values.foreground = values->foreground;
      private->values_mask |= GDK_GC_FOREGROUND;

	  color = values->foreground;

	  //gi_color_t gcolor = GI_ARGB (255,r,g,b);

	  //gi_set_gc_foreground_pixel(private->gix_gc, values->foreground.pixel);
	  //gi_set_gc_foreground(private->gix_gc, gcolor);
	  //printf("gi_set_gc_foreground XXX = %x,%p,%x\n", gcolor, gc,values->foreground.pixel);
    }
	

  if (values_mask & GDK_GC_BACKGROUND)
    {
	  r = values->background.red   >> 8;
	  g = values->background.green   >> 8;
	  b = values->background.blue   >> 8;

      private->values.background = values->background;
      private->values_mask |= GDK_GC_BACKGROUND;
	   gi_set_gc_background_pixel(private->gix_gc, values->background.pixel);
	   gi_set_gc_background(private->gix_gc, GI_RGB(r,g,b));
    }
	else{
		gi_set_gc_background_pixel(private->gix_gc, 0);
	   gi_set_gc_background(private->gix_gc, 0);
		//printf("background not set\n");
	}

  if (values_mask & GDK_GC_FONT)
    {
      GdkFont *oldf = private->values.font;

      private->values.font = gdk_font_ref (values->font);
      private->values_mask |= GDK_GC_FONT;
         
      if (oldf)
        gdk_font_unref (oldf);

	  D_UNIMPLEMENTED;
    }

  if (values_mask & GDK_GC_FUNCTION)
    {
      private->values.function = values->function;
      private->values_mask |= GDK_GC_FUNCTION;

	   if (values->function == GDK_INVERT)
	{
		color.red = color.green = color.blue = 0xFFFF;
          alpha = 0x0;
          flags = GI_MODE_XOR;
	}
	else if (values->function == GDK_XOR){
		alpha = 0x0;
          flags = GI_MODE_XOR;
	}
	else if (values->function == GDK_COPY){
		flags = GI_MODE_SET;
	}
	else if (values->function == GDK_CLEAR){
		color.red = color.green = color.blue = 0x0;
		color.pixel = 0;
          flags = GI_MODE_SET;
	}
	else if (values->function == GDK_SET){
		color.red = color.green = color.blue = 0xffff;
		color.pixel = -1;
          flags = GI_MODE_SET;
	}
	else if (values->function == GDK_NOOP){
	}
	else{
		g_message ("unsupported GC function %d",
                     values->function);
          flags = GI_MODE_SET;
	  D_UNIMPLEMENTED;
	}

	  
    }

	r = color.red   >> 8;
	g = color.green   >> 8;
	b = color.blue   >> 8;

	gi_set_gc_foreground_pixel(private->gix_gc, color.pixel);
	gi_set_gc_foreground(private->gix_gc, GI_ARGB(alpha,r,g,b));
	gi_set_gc_function(private->gix_gc,flags);

  //gdk_gix_set_color (impl, &color, alpha);

  if (values_mask & GDK_GC_FILL)
    {
      private->values.fill = values->fill;
      private->values_mask |= GDK_GC_FILL;

	    switch (values->fill)
	{
	case GDK_SOLID:
		gi_set_fillstyle(private->gix_gc,GI_GC_FILL_SOLID);
	  //xvalues->fill_style = FillSolid;
	  break;
	case GDK_TILED:
		gi_set_fillstyle(private->gix_gc,GI_GC_FILL_TILED);
	  //xvalues->fill_style = FillTiled;
	  break;
	case GDK_STIPPLED:
		gi_set_fillstyle(private->gix_gc,GI_GC_FILL_STIPPLED);
	  //xvalues->fill_style = FillStippled;
	  break;
	case GDK_OPAQUE_STIPPLED:
		gi_set_fillstyle(private->gix_gc,GI_GC_FILL_OPAQUE_STIPPLED);
	 // xvalues->fill_style = FillOpaqueStippled;
	  break;
	}

	  D_UNIMPLEMENTED;
    }

  if (values_mask & GDK_GC_TILE)
    {
      GdkPixmap *oldpm = private->values.tile;
         
      if (values->tile)
        g_assert (GDK_PIXMAP_OBJECT (values->tile)->depth > 1);

      private->values.tile = values->tile ? g_object_ref (values->tile) : NULL;
      private->values_mask |= GDK_GC_TILE;

      if (oldpm)
        g_object_unref (oldpm);
    }

  if (values_mask & GDK_GC_STIPPLE)
    {
      GdkPixmap *oldpm = private->values.stipple;

      if (values->stipple)
        g_assert (GDK_PIXMAP_OBJECT (values->stipple)->depth == 1);

      private->values.stipple = (values->stipple ? 
                                 g_object_ref (values->stipple) : NULL);
      private->values_mask |= GDK_GC_STIPPLE;

      if (oldpm)
        g_object_unref (oldpm);
    }

  if (values_mask & GDK_GC_CLIP_MASK)
    {
      GdkPixmap *oldpm = private->values.clip_mask;

      private->values.clip_mask = (values->clip_mask ?
                                   g_object_ref (values->clip_mask) : NULL);
      private->values_mask |= GDK_GC_CLIP_MASK;

      if (oldpm)
        g_object_unref (oldpm);

	  D_UNIMPLEMENTED;

      /*if (private->clip_region)
        {
          gdk_region_destroy (private->clip_region);
          private->clip_region = NULL;
        }
		*/
    }

  if (values_mask & GDK_GC_SUBWINDOW)
    {
      private->values.subwindow_mode = values->subwindow_mode;
      private->values_mask |= GDK_GC_SUBWINDOW;
    }

  if (values_mask & GDK_GC_TS_X_ORIGIN)
    {
      private->values.ts_x_origin = values->ts_x_origin;
      private->values_mask |= GDK_GC_TS_X_ORIGIN;
    }

  if (values_mask & GDK_GC_TS_Y_ORIGIN)
    {
      private->values.ts_y_origin = values->ts_y_origin;
      private->values_mask |= GDK_GC_TS_Y_ORIGIN;
    }

  if (values_mask & GDK_GC_CLIP_X_ORIGIN)
    {
      private->values.clip_x_origin = GDK_GC (gc)->clip_x_origin = values->clip_x_origin;
      private->values_mask |= GDK_GC_CLIP_X_ORIGIN;
	  gi_gc_set_clip_origin(private->gix_gc,values->clip_x_origin,values->clip_y_origin);
    }

  if (values_mask & GDK_GC_CLIP_Y_ORIGIN)
    {
      private->values.clip_y_origin = GDK_GC (gc)->clip_y_origin = values->clip_y_origin;
      private->values_mask |= GDK_GC_CLIP_Y_ORIGIN;
	  gi_gc_set_clip_origin(private->gix_gc,values->clip_x_origin,values->clip_y_origin);
    }

  if (values_mask & GDK_GC_EXPOSURES)
    {
      private->values.graphics_exposures = values->graphics_exposures;
      private->values_mask |= GDK_GC_EXPOSURES;
    }

  if (values_mask & GDK_GC_LINE_WIDTH)
    {
      private->values.line_width = values->line_width;
      private->values_mask |= GDK_GC_LINE_WIDTH;
	  gi_set_linewidth(private->gix_gc,values->line_width);
    }

  if (values_mask & GDK_GC_LINE_STYLE)
    {
      private->values.line_style = values->line_style;
      private->values_mask |= GDK_GC_LINE_STYLE;
	   D_UNIMPLEMENTED;
	   switch (values->line_style)
	{
	case GDK_LINE_SOLID:
		gi_set_linestyle(private->gix_gc,GI_GC_LINE_SOLID);
	  //xvalues->line_style = LineSolid;
	  break;
	case GDK_LINE_ON_OFF_DASH:
		gi_set_linestyle(private->gix_gc,GI_GC_LINE_ON_OFF_DASH);
	  //xvalues->line_style = LineOnOffDash;
	  break;
	case GDK_LINE_DOUBLE_DASH:
		gi_set_linestyle(private->gix_gc,GI_GC_LINE_DOUBLE_DASH);
	  //xvalues->line_style = LineDoubleDash;
	  break;
	}

    }

  if (values_mask & GDK_GC_CAP_STYLE)
    {
      private->values.cap_style = values->cap_style;
      private->values_mask |= GDK_GC_CAP_STYLE;

	  switch (values->cap_style)
	{
	case GDK_CAP_NOT_LAST:
		gi_set_capstyle(private->gix_gc,GI_GC_CAP_NOT_LAST);
	  //xvalues->cap_style = CapNotLast;
	  break;
	case GDK_CAP_BUTT:
		gi_set_capstyle(private->gix_gc,GI_GC_CAP_BUTT);
	  //xvalues->cap_style = CapButt;
	  break;
	case GDK_CAP_ROUND:
		gi_set_capstyle(private->gix_gc,GI_GC_CAP_ROUND);
	  //xvalues->cap_style = CapRound;
	  break;
	case GDK_CAP_PROJECTING:
		gi_set_capstyle(private->gix_gc,GI_GC_CAP_PROJECTING);
	 // xvalues->cap_style = CapProjecting;
	  break;
	}

    }

  if (values_mask & GDK_GC_JOIN_STYLE)
    {
      private->values.join_style = values->join_style;
      private->values_mask |= GDK_GC_JOIN_STYLE;

	  switch (values->join_style)
	{
	case GDK_JOIN_MITER:
		gi_set_joinstyle(private->gix_gc,GI_GC_JOIN_MITER);
	 // xvalues->join_style = JoinMiter;
	  break;
	case GDK_JOIN_ROUND:
		gi_set_joinstyle(private->gix_gc,GI_GC_JOIN_ROUND);
	 // xvalues->join_style = JoinRound;
	  break;
	case GDK_JOIN_BEVEL:
		gi_set_joinstyle(private->gix_gc,GI_GC_JOIN_BEVEL);
	  //xvalues->join_style = JoinBevel;
	  break;
	}

    }
 
}


static void
gdk_gix_gc_set_dashes (GdkGC *gc,
                            gint   dash_offset,
                            gint8  dash_list[],
                            gint   n)
{
	gi_set_dashes ( GDK_GC_GET_XGC (gc),
	      dash_offset, n, dash_list);
  g_warning ("gdk_gix_gc_set_dashes not implemented");
}

static void
gc_unset_clip_mask (GdkGC *gc)
{
  GdkGCGix *data = GDK_GC_GIX (gc);

  if (data->values.clip_mask)
    {
      g_object_unref (data->values.clip_mask);
      data->values.clip_mask = NULL;
      data->values_mask &= ~ GDK_GC_CLIP_MASK;
    }
}

void
_gdk_windowing_gc_copy (GdkGC *dst_gc,
             GdkGC *src_gc)
{
  GdkGCGix *dst_private;

  g_return_if_fail (dst_gc != NULL);
  g_return_if_fail (src_gc != NULL);

  dst_private = GDK_GC_GIX (dst_gc);

  temp_region_reset(&dst_private->clip_region);

  if (dst_private->values_mask & GDK_GC_FONT)
    gdk_font_unref (dst_private->values.font);
  if (dst_private->values_mask & GDK_GC_TILE)
    g_object_unref (dst_private->values.tile);
  if (dst_private->values_mask & GDK_GC_STIPPLE)
    g_object_unref (dst_private->values.stipple);
  if (dst_private->values_mask & GDK_GC_CLIP_MASK)
    g_object_unref (dst_private->values.clip_mask);

  *dst_gc = *src_gc;
  if (dst_private->values_mask & GDK_GC_FONT)
    gdk_font_ref (dst_private->values.font);
  if (dst_private->values_mask & GDK_GC_TILE)
    g_object_ref (dst_private->values.tile);
  if (dst_private->values_mask & GDK_GC_STIPPLE)
    g_object_ref (dst_private->values.stipple);
  if (dst_private->values_mask & GDK_GC_CLIP_MASK)
    g_object_ref (dst_private->values.clip_mask);
}

/**
 * gdk_gc_get_screen:
 * @gc: a #GdkGC.
 *
 * Gets the #GdkScreen for which @gc was created
 *
 * Returns: the #GdkScreen for @gc.
 *
 * Since: 2.2
 */
GdkScreen *  
gdk_gc_get_screen (GdkGC *gc)
{
  g_return_val_if_fail (GDK_IS_GC_GIX (gc), NULL);
  
  return _gdk_screen;
}

void
_gdk_region_get_xrectangles (const GdkRegion *region,
                             gint             x_offset,
                             gint             y_offset,
                             gi_boxrec_t     **rects,
                             gint            *n_rects)
{
  gi_boxrec_t *rectangles = g_new (gi_boxrec_t, region->numRects);
  GdkRegionBox *boxes = region->rects;
  gint i;
  
  for (i = 0; i < region->numRects; i++)
    {
	  rectangles[i].x1 = CLAMP (boxes[i].x1 + x_offset, G_MINSHORT, G_MAXSHORT);
      rectangles[i].y1 = CLAMP (boxes[i].y1 + y_offset, G_MINSHORT, G_MAXSHORT);
      rectangles[i].x2 = CLAMP (boxes[i].x2 + x_offset, G_MINSHORT, G_MAXSHORT) ;
      rectangles[i].y2 = CLAMP (boxes[i].y2 + y_offset, G_MINSHORT, G_MAXSHORT) ;

      /*rectangles[i].x = CLAMP (boxes[i].x1 + x_offset, G_MINSHORT, G_MAXSHORT);
      rectangles[i].y = CLAMP (boxes[i].y1 + y_offset, G_MINSHORT, G_MAXSHORT);
      rectangles[i].width = CLAMP (boxes[i].x2 + x_offset, G_MINSHORT, G_MAXSHORT) - rectangles[i].x;
      rectangles[i].height = CLAMP (boxes[i].y2 + y_offset, G_MINSHORT, G_MAXSHORT) - rectangles[i].y;
	  */
    }

  *rects = rectangles;
  *n_rects = region->numRects;
}


#if 0

gi_gc_ptr_t
_gdk_x11_gc_flush (GdkGC *gc)
{
  GdkGCGix *private = GDK_GC_GIX (gc);
  gi_gc_ptr_t gix_gc = private->gix_gc;

  if (private->dirty_mask & GDK_GC_DIRTY_CLIP)
    {
      GdkRegion *clip_region = _gdk_gc_get_clip_region (gc);
      
      if (!clip_region){
	//XSetClipOrigin (xdisplay, gix_gc,gc->clip_x_origin, gc->clip_y_origin);
	  }
      else
	{
	  gi_boxrec_t *rectangles;
          gint n_rects;

          _gdk_region_get_xrectangles (clip_region,
                                       gc->clip_x_origin,
                                       gc->clip_y_origin,
                                       &rectangles,
                                       &n_rects);

		 // gi_set_gc_clip_rectangles( data->gix_gc,  boxptr, region->numRects);
	  
	  gi_set_gc_clip_rectangles ( gix_gc, 
                              rectangles,
                              n_rects);
          
	  g_free (rectangles);
	}
    }

  if (private->dirty_mask & GDK_GC_DIRTY_TS)
    {
      //XSetTSOrigin (xdisplay, gix_gc,  gc->ts_x_origin, gc->ts_y_origin);
    }

  private->dirty_mask = 0;
  return gix_gc;
}

void
_gdk_windowing_gc_set_clip_region (GdkGC           *gc,
				   const GdkRegion *region)
{
  GdkGCGix *x11_gc = GDK_GC_GIX (gc);

  /* Unset immediately, to make sure Xlib doesn't keep the
   * XID of an old clip mask cached
   */
  if ((x11_gc->have_clip_region && !region) || x11_gc->have_clip_mask)
    {
      //XSetClipMask (GDK_GC_XDISPLAY (gc), GDK_GC_XGC (gc), None);
	  gi_set_gc_clip_rectangles( x11_gc->gix_gc,  NULL, 0);
      x11_gc->have_clip_mask = FALSE;
    }

  x11_gc->have_clip_region = region != NULL;

  gc->clip_x_origin = 0;
  gc->clip_y_origin = 0;

  x11_gc->dirty_mask |= GDK_GC_DIRTY_CLIP;
}

#else
void
_gdk_windowing_gc_set_clip_region (GdkGC           *gc,
                                   const GdkRegion *region)
{
  GdkGCGix *data;

  g_return_if_fail (gc != NULL);

  data = GDK_GC_GIX (gc);

  if (region == &data->clip_region)
    return;  

  if (region){
	  gi_boxrec_t *boxptr;
	  gi_boxrec_t _box_static[8];
	  int i;
	 

	  if (region->numRects<8) {
		  boxptr = &_box_static;
	  }
	  else{
		boxptr = g_malloc(region->numRects * sizeof(gi_boxrec_t));
	  }

	  if (boxptr) {
		  for (i = 0; i < region->numRects; i++)  {
			  boxptr[i].x1 = region->rects[i].x1;
			  boxptr[i].y1 = region->rects[i].y1;
			  boxptr[i].x2 = region->rects[i].x2;
			  boxptr[i].y2 = region->rects[i].y2;
		  }

		gi_set_gc_clip_rectangles( data->gix_gc,  boxptr, region->numRects);
		if(boxptr != &_box_static)
		  g_free(boxptr);
	  }
	  

    temp_region_init_copy (&data->clip_region, region);
  }
  else{
	gi_set_gc_clip_rectangles( data->gix_gc,  NULL, 0);
    temp_region_reset (&data->clip_region);
  }

  gc->clip_x_origin = 0;
  gc->clip_y_origin = 0;
  data->values.clip_x_origin = 0;
  data->values.clip_y_origin = 0;

  gc_unset_clip_mask (gc);
}
#endif

#define __GDK_GC_X11_C__
#include "gdkaliasdef.c"
