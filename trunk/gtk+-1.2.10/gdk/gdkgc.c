/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <string.h>
#include "gdk.h"
#include "gdkprivate.h"

#define PRINTFILE(s) g_message("gdkgc.c:%s()", s)
GdkGC*
gdk_gc_new (GdkWindow *window)
{
 return gdk_gc_new_with_values (window, NULL, 0);
}

GdkGC*
gdk_gc_new_with_values (GdkWindow	*window,
			GdkGCValues	*values,
			GdkGCValuesMask	 values_mask)
{
#ifdef UNUSED_WINDOW
  GdkWindowPrivate *window_private;
#endif
  GdkGC *gc;
  GdkGCPrivate *private;
  gi_gc_ptr_t gigc;

#ifdef UNUSED_WINDOW
Is this window arg used somewhere ? Remove it.
  g_return_val_if_fail (window != NULL, NULL);

  window_private = (GdkWindowPrivate*) window;
  if (window_private->destroyed)
    return NULL;
#endif

  private = g_new (GdkGCPrivate, 1);
  gc = (GdkGC*) private;

  private->ref_count = 1;
  gigc = gi_create_gc(GI_DESKTOP_WINDOW_ID,NULL); 

  if (values_mask & GDK_GC_CLIP_X_ORIGIN)
    private->clip_x_origin = values->clip_x_origin;
  if (values_mask & GDK_GC_CLIP_Y_ORIGIN)
    private->clip_y_origin = values->clip_y_origin;
  if (values_mask & GDK_GC_TS_X_ORIGIN)
    private->ts_x_origin = values->ts_x_origin;
  if (values_mask & GDK_GC_TS_Y_ORIGIN)
    private->ts_y_origin = values->ts_y_origin;

  if (values_mask & GDK_GC_FOREGROUND)
    {
	gi_set_gc_foreground_pixel(gigc, values->foreground.pixel);
    }
  if (values_mask & GDK_GC_BACKGROUND)
    {
    	gi_set_gc_background_pixel(gigc, values->background.pixel);
    }
  if ((values_mask & GDK_GC_FONT) && (values->font->type == GDK_FONT_FONT))
    {
	  gigc->values.font =  ((GdkFontPrivate*) (values->font))->fid;
    }

    if (values_mask & GDK_GC_FUNCTION)
    {
		int mode;

      switch (values->function)
        {
	case GDK_INVERT:
                mode= GI_MODE_INVERT;
                break;
        case GDK_XOR :
                mode= GI_MODE_XOR;
                break;
       // case GDK_OR :
        //        gigc.mode= GI_MODE_OR;
        //        break;
       // case GDK_AND :
       //         gigc.mode= GI_MODE_AND;
        //        break;
        default :
                mode = GI_MODE_SET;
        }
        gi_set_gc_function(gigc, mode);
    }

	/*
	 Disable this for now, it conflicts with FUNCTION
	*/
  if (values_mask & GDK_GC_LINE_STYLE)
    {
	  int line_style;

      switch (values->line_style)
	{
	case GDK_LINE_SOLID:
	/*  gigc.mode= GI_MODE_SET; */
	  	line_style = GI_GC_LINE_SOLID;
		
	  	break;
	case GDK_LINE_ON_OFF_DASH:
	/*  gigc.mode= GI_MODE_XOR; */
		line_style = GI_GC_LINE_ON_OFF_DASH;
	  	break;
	case GDK_LINE_DOUBLE_DASH:
	/*  gigc.mode= GI_MODE_OR; */
		line_style = GI_GC_LINE_ON_OFF_DASH;
	  	break;
	}
	/* GrSetGCMode(gigc.gcid, gigc.mode); */
	//gr_line_attrib_mask |= 1;

	gi_set_linestyle(gigc,line_style);
    }
    if (values_mask & GDK_GC_LINE_WIDTH)
    {
        gi_set_linewidth(gigc, values->line_width);
	//gr_line_attrib_mask |= 2;
    }
    //if (gr_line_attrib_mask != 0) 
	//GrSetGCLineAttrib(gigc.gcid, gr_line_attrib_mask, 
	//		gigc.line_width, gigc.line_style);
    if (values_mask & GDK_GC_SUBWINDOW)
    {
        //gigc.subwindowmode = values->subwindow_mode;
	//GrSetGCSubWindowMode(gigc.gcid, gigc.subwindowmode);
    }

  private->xgc= gigc;
#ifdef UNUSED_WINDOW
 private->xwindow= window_private->xwindow;
#else
 private->xwindow= 0;
#endif
//printf("New GC %d private %d and %d, %d\n", nvalues.gcid, private->xwindow, window_private->width, window_private->height);
  return gc;
}

void
gdk_gc_destroy (GdkGC *gc)
{
  gdk_gc_unref (gc);
}

GdkGC *
gdk_gc_ref (GdkGC *gc)
{
  GdkGCPrivate *private = (GdkGCPrivate*) gc;

  g_return_val_if_fail (gc != NULL, NULL);
  private->ref_count += 1;

  return gc;
}

void
gdk_gc_unref (GdkGC *gc)
{
  GdkGCPrivate *private = (GdkGCPrivate*) gc;
 
  g_return_if_fail (gc != NULL);
  g_return_if_fail (private->ref_count > 0);
 
  if (private->ref_count > 1)
    private->ref_count -= 1;
  else
    {
	gi_destroy_gc(GDK_GC_XGC(gc));
      memset (gc, 0, sizeof (GdkGCPrivate)); // why is this needed ??
      g_free (gc);
    }

}

void
gdk_gc_get_values (GdkGC       *gc,
		   GdkGCValues *values)
{
  gi_gc_ptr_t gcp;
  GR_FONT_INFO *fi;
  GdkFontPrivate private;

  g_return_if_fail (gc != NULL);
  g_return_if_fail (values != NULL);
  gcp = GDK_GC_XGC(gc);
  if(gcp)
  {
	values->foreground.pixel = gcp->values.foreground;
	values->background.pixel = gcp->values.background;
	values->font = gdk_font_lookup(gcp->values.font);

	if(!values->font)
	{
		g_warning("Unable to load font from cache - get from Server!");
		//GrGetFontInfo(gcp->values.font, &fi); //fixme
		fi = malloc(sizeof(GR_FONT_INFO));
		private.xfont = fi;
		

		fi->height = gi_ufont_ascent_get(gcp->values.font) + gi_ufont_descent_get(gcp->values.font);
	    fi->baseline = gi_ufont_ascent_get(gcp->values.font);

        	private.fid = gcp->values.font;
        	private.ref_count = 1;
        	private.names = NULL;

        	values->font = (GdkFont*) &private;
        	values->font->type = GDK_FONT_FONT;
        	values->font->ascent =   gi_ufont_ascent_get(gcp->values.font);
        	values->font->descent =  gi_ufont_descent_get(gcp->values.font);
	}
	
	switch(gcp->values.function)
	{
        	case GDK_XOR :
                	values->function = GI_MODE_XOR;
                	break;
        	//case GDK_OR :
            //    	values->function = GI_MODE_OR;
            //    	break;
        	//case GDK_AND :
            //    	values->function = GI_MODE_AND;
            //    	break;
        	default :
                	values->function = GI_MODE_SET;
			break;
	}
	values->subwindow_mode = 0;//gcp->values.subwindowmode;
#ifdef LINE_ATTRIB_SUPPORT
	values->line_width = gcp->values.line_width;
	switch (gcp->values.line_style) {
	case GI_GC_LINE_SOLID:
			values->line_style = GDK_LINE_SOLID;
			break;
	case GI_GC_LINE_ON_OFF_DASH:
			values->line_style = GDK_LINE_ON_OFF_DASH;
			break;
	}
#endif
  }
  else
      memset (values, 0, sizeof (GdkGCValues));
}

void
gdk_gc_set_foreground (GdkGC	*gc,
		       GdkColor *color)
{
  g_return_if_fail (gc != NULL);
  g_return_if_fail (color != NULL);
  gi_set_gc_foreground_pixel(GDK_GC_XGC(gc),color->pixel);
}

void
gdk_gc_set_background (GdkGC	*gc,
		       GdkColor *color)
{
  g_return_if_fail (gc != NULL);
  g_return_if_fail (color != NULL);
  gi_set_gc_background_pixel(GDK_GC_XGC(gc),color->pixel);
}


void
gdk_gc_set_font (GdkGC	 *gc,
		 GdkFont *font)
{
  GdkFontPrivate *font_private;

  g_return_if_fail (gc != NULL);
  g_return_if_fail (font != NULL);

  if (font->type == GDK_FONT_FONT)
    {
      font_private = (GdkFontPrivate*) font;
	//GrSetGCFont(GDK_GC_XGC(gc), font_private->fid);

	GDK_GC_XGC(gc)->values.font =  font_private->fid;
    }

}

void
gdk_gc_set_function (GdkGC	 *gc,
		     GdkFunction  function)
{
	int mode;
	g_return_if_fail (gc != NULL);

	switch (function)
        {
        case GDK_INVERT :
                mode= GI_MODE_INVERT;
                break;
        case GDK_XOR :
                mode= GI_MODE_XOR;
                break;
        //case GDK_OR :
        //        mode= GI_MODE_OR;
        //        break;
        //case GDK_AND :
        //        mode= GI_MODE_AND;
        //        break;
        default :
                mode = GI_MODE_SET;
        }
        gi_set_gc_function(GDK_GC_XGC(gc), mode);

}

void
gdk_gc_set_fill (GdkGC	 *gc,
		 GdkFill  fill)
{
  GdkGCPrivate *private;

  g_return_if_fail (gc != NULL);

  private = (GdkGCPrivate*) gc;


  switch (fill)
    {
    case GDK_SOLID:
      gi_set_fillstyle ( private->xgc, GI_GC_FILL_SOLID);
      break;
    case GDK_TILED:
      gi_set_fillstyle ( private->xgc, GI_GC_FILL_TILED);
      break;
    case GDK_STIPPLED:
      gi_set_fillstyle ( private->xgc, GI_GC_FILL_STIPPLED);
      break;
    case GDK_OPAQUE_STIPPLED:
      gi_set_fillstyle ( private->xgc, GI_GC_FILL_OPAQUE_STIPPLED);
      break;
    }
}

void
gdk_gc_set_tile (GdkGC	   *gc,
		 GdkPixmap *tile)
{
  GdkGCValues values;

  g_return_if_fail (gc != NULL);

  values.tile = tile;
  /*FIXME: gix no GDK_GC_TILE */

}

void
gdk_gc_set_stipple (GdkGC     *gc,
		    GdkPixmap *stipple)
{
  GdkGCValues values;

  g_return_if_fail (gc != NULL);

  values.stipple= stipple; 
  /*FIXME: gix no GDK_GC_TILE */

}

void
gdk_gc_set_ts_origin (GdkGC *gc,
		      gint   x,
		      gint   y)
{
 GdkGCPrivate *private;
  g_return_if_fail (gc != NULL);

  private = (GdkGCPrivate*)gc;
  private->ts_x_origin = x;
  private->ts_y_origin = y;
}

void
gdk_gc_set_clip_origin (GdkGC *gc,
			gint   x,
			gint   y)
{
  GdkGCPrivate *private;

  g_return_if_fail (gc != NULL);

  private = (GdkGCPrivate*)gc;


   private->clip_x_origin = x;
   private->clip_y_origin = y;
}

void
gdk_gc_set_clip_mask (GdkGC	*gc,
		      GdkBitmap *mask)
{
	g_return_if_fail (gc != NULL);

	/* FIXME: Didn't take care about the clipmask, always set the clipmask to
	   zero. */
	if(!mask)
		XSetGCRegion(GDK_GC_XGC(gc), 0);
/*	else
	{
		gi_window_info_t maskinfo;
	 	gi_cliprect_t cliprect;

	        if(GDK_DRAWABLE_DESTROYED(mask)) return;

		gi_get_window_info(GDK_DRAWABLE_XID(mask), &maskinfo);
      		cliprect.x = 0;
	      	cliprect.y = 0;
      		cliprect.width = maskinfo.width;
	      	cliprect.height = maskinfo.height;
		gi_set_gc_clip_rectangles(GDK_GC_XGC(gc), 0, 0, 1, &cliprect);
	}*/

}


void
gdk_gc_set_clip_rectangle (GdkGC	*gc,
			   GdkRectangle *rectangle)
{
  if(rectangle)
  {
	gi_boxrec_t cliprect;
      	cliprect.x1 = rectangle->x ;
      	cliprect.y1 = rectangle->y;
      	cliprect.x2 = rectangle->x + rectangle->width;
      	cliprect.y2 = rectangle->y + rectangle->height;
	gi_set_gc_clip_rectangles(GDK_GC_XGC(gc), &cliprect,  1);
    }

  else
	XSetGCRegion(GDK_GC_XGC(gc), 0);
} 

void
gdk_gc_set_clip_region (GdkGC		 *gc,
			GdkRegion	 *region)
{
	g_return_if_fail (gc != NULL);
	if (region)
	{
		GdkRegionPrivate *region_private = (GdkRegionPrivate*) region;
		XSetGCRegion(GDK_GC_XGC(gc), region_private->xregion);
	}
	else
		XSetGCRegion(GDK_GC_XGC(gc), 0);
}

void
gdk_gc_set_subwindow (GdkGC	       *gc,
		      GdkSubwindowMode	mode)
{
  g_return_if_fail (gc != NULL);

  //GrSetGCSubWindowMode(GDK_GC_XGC(gc), mode); //dpp
}

void
gdk_gc_set_exposures (GdkGC     *gc,
		      gboolean   exposures)
{
	GdkGCValues values;

  	g_return_if_fail (gc != NULL);

  	values.graphics_exposures = exposures;
	PRINTFILE("__UNSUPPORTED__ : gdk_gc_set_exposures");
	/*FIXME : gix have no GDK_GC_EXPOSURES */
}

void
gdk_gc_set_line_attributes (GdkGC	*gc,
			    gint	 line_width,
			    GdkLineStyle line_style,
			    GdkCapStyle	 cap_style,
			    GdkJoinStyle join_style)
{
	int gr_line_width = line_width;
	int gr_line_style;

	g_return_if_fail (gc != NULL);
	switch (line_style) {
	case GDK_LINE_SOLID: 
			gr_line_style = GI_GC_LINE_SOLID;
			break;
	case GDK_LINE_ON_OFF_DASH: 
			gr_line_style = GI_GC_LINE_ON_OFF_DASH;
			break;
	case GDK_LINE_DOUBLE_DASH: 
			gr_line_style = GI_GC_LINE_DOUBLE_DASH;
	                PRINTFILE("__UNSUPPORTED__ : gdk_gc_set_line_attributes : GDK_LINE_DOUBLE_DASH");
			break;
	}
	gi_set_line_attributes(GDK_GC_XGC(gc),  gr_line_width, gr_line_style, cap_style,join_style );
	
	PRINTFILE("__UNSUPPORTED__ : gdk_gc_set_line_attributes");
}

void
gdk_gc_set_dashes (GdkGC      *gc,
		   gint	       dash_offset,
		   gint8       dash_list[],
		   gint        n)
{
	int n_dashes;
	char *gr_dash_list;
	gint	actual_offset = dash_offset % n;
	gint 	i;

	/* take care of offset now */
	if (actual_offset < 0)
		return;
	n_dashes = n - dash_offset;
	gr_dash_list = g_new(char, n_dashes);
	for (i = 0; i < n_dashes; i++)
		gr_dash_list[i] = dash_list[i];
	gi_set_dashes(GDK_GC_XGC(gc),0, n_dashes, gr_dash_list);
	if (gr_dash_list != NULL)
		g_free(gr_dash_list);
}

void
gdk_gc_copy (GdkGC *dst_gc, GdkGC *src_gc)
{
	gi_destroy_gc(GDK_GC_XGC(dst_gc));
	GDK_GC_XGC(dst_gc) = gi_dup_gc(GDK_GC_XGC(src_gc));
}
