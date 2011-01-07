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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#include "gdk.h"
#include "gdkprivate.h"
#define GDK_FONT_XFONT(font)          (((GdkFontPrivate*) font)->fid)

#define PRINTFILE(s) g_message("gdkdraw.c:%s()", s)
extern gi_screen_info_t gdk_screen_info;

void
gdk_draw_point (GdkDrawable *drawable,
                GdkGC       *gc,
                gint         x,
                gint         y)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (gc != NULL);
  gi_point_t tmp_points[1];


  if (GDK_DRAWABLE_DESTROYED(drawable)) return;
  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

  //GrPoint (GDK_DRAWABLE_XID(drawable), GDK_GC_XGC(gc), x, y);


  tmp_points[0].x =x;
  tmp_points[0].y = y;

  gi_draw_points (
		   GDK_GC_XGC (gc),
		   tmp_points,
		   1);

}

void
gdk_draw_line (GdkDrawable *drawable,
	       GdkGC       *gc,
	       gint         x1,
	       gint         y1,
	       gint         x2,
	       gint         y2)
{

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (gc != NULL);


  if( GDK_DRAWABLE_DESTROYED(drawable))
    return;
  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

  gi_draw_line(GDK_GC_XGC(gc),x1,y1,x2,y2);
}

void
gdk_draw_rectangle (GdkDrawable *drawable,
		    GdkGC       *gc,
		    gint         filled,
		    gint         x,
		    gint         y,
		    gint         width,
		    gint         height)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (gc != NULL);

  if (GDK_DRAWABLE_DESTROYED(drawable))
    return;

  if (width == -1)
    width = ((GdkWindowPrivate*)drawable)->width;
  if (height == -1)
    height = ((GdkWindowPrivate*)drawable)->height;

  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

  if (filled){
	gi_fill_rect( GDK_GC_XGC(gc), x, y, width, height);
  }
  else
	gi_draw_rect(GDK_GC_XGC(gc),x,y,width,height);	//For some reason, nano-X draw wider and higher
}

void
gdk_draw_arc (GdkDrawable *drawable,
	      GdkGC       *gc,
	      gint         filled,
	      gint         x,
	      gint         y,
	      gint         width,
	      gint         height,
	      gint         angle1,
	      gint         angle2)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (gc != NULL);

  PRINTFILE("gdk_draw_arc"); // only used by gtkctree widget
  if (GDK_DRAWABLE_DESTROYED(drawable))
    return;

  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

  if (width == -1)
    width = ((GdkWindowPrivate*)drawable)->width;
  if (height == -1)
    height = ((GdkWindowPrivate*)drawable)->height;

  if (filled)
  {
	  gi_fill_arc(GDK_DRAWABLE_XID(drawable),GDK_GC_XGC(gc), x, y, 
		width, height, angle1, angle2);
  }
  else{
	   gi_draw_arc(GDK_DRAWABLE_XID(drawable),GDK_GC_XGC(gc), x, y, 
		width, height, angle1, angle2);
  }

  //GrArcAngle(GDK_DRAWABLE_XID(drawable), GDK_GC_XGC(gc), x, y, 
	//	width, height, angle1, angle2, filled);
}

/*
 * auto-close the points if the last point not equal to first point
 */
void
gdk_draw_polygon (GdkDrawable *drawable,
		  GdkGC       *gc,
		  gint         filled,
		  GdkPoint    *points,
		  gint         npoints)
{
	GdkWindowPrivate *drawable_private;
	GdkGCPrivate *gc_private;
	gi_point_t * nanoPoints;
	gint nano_npoints = npoints;
	int i;


	g_return_if_fail (drawable != NULL);
	g_return_if_fail (gc != NULL);

	drawable_private = (GdkWindowPrivate*) drawable;
	if (drawable_private->destroyed)
		return;
  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));
	gc_private = (GdkGCPrivate*) gc;
	if (points[npoints-1].x != points[0].x 
		|| points[npoints-1].y != points[0].y)
		nano_npoints = npoints + 1;
	nanoPoints = g_new(gi_point_t, nano_npoints);
	// GdkPoint (gint16) to gi_point_t (int)
	for (i=0;i<npoints;i++)
	{
		nanoPoints[i].x = points[i].x;
		nanoPoints[i].y = points[i].y;
	}
	if (nano_npoints > npoints) {
		nanoPoints[npoints].x = points[0].x;
		nanoPoints[npoints].y = points[0].y;
	}
	if (filled)
	{
		gi_fill_polygon ( gc_private->xgc, nano_npoints, nanoPoints,GI_SHAPE_Complex, GI_POLY_CoordOrigin);
	}
	else
	{
		gi_draw_lines ( gc_private->xgc, nano_npoints, nanoPoints);
	}
    g_free(nanoPoints);
}

/* gdk_draw_string
 *
 * Modified by Li-Da Lho to draw 16 bits and Multibyte strings
 *
 * Interface changed: add "GdkFont *font" to specify font or fontset explicitely
 */
void
gdk_draw_string (GdkDrawable *drawable,
		 GdkFont     *font,
		 GdkGC       *gc,
		 gint         x,
		 gint         y,
		 const gchar *string)
{
	gdk_draw_text(drawable, font, gc, x, y, string, strlen(string));
}

/* gdk_draw_text
 *
 * Modified by Li-Da Lho to draw 16 bits and Multibyte strings
 *
 * Interface changed: add "GdkFont *font" to specify font or fontset explicitely
 */
void
gdk_draw_text (GdkDrawable *drawable,
	       GdkFont     *font,
	       GdkGC       *gc,
	       gint         x,
	       gint         y,
	       const gchar *text,
	       gint         text_length)
{
	GR_FONT_INFO fid;
	g_return_if_fail (drawable != NULL);
  	g_return_if_fail (font != NULL);
  	g_return_if_fail (gc != NULL);
  	g_return_if_fail (text!= NULL);

	gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

	//printf("fontid is %d at %d\n", ((GdkFontPrivate*)font)->fid, GDK_GC_XGC(gc));
	//GrGetFontInfo(((GdkFontPrivate*)font)->fid, &fid);
	//GrSetGCUseBackground(GDK_GC_XGC(gc), FALSE);

	int r,g,b;

	gi_pixel_to_color(&gdk_screen_info,GDK_GC_XGC(gc)->values.foreground,&r, &g,&b);

	gi_ufont_set_text_attr(GDK_GC_XGC(gc),GDK_FONT_XFONT(font),FALSE,GI_RGB(r,g,b),0); //dpp, fixme
  gi_ufont_draw(GDK_GC_XGC(gc),GDK_FONT_XFONT(font),text, x,y,text_length);

 	//GrSetGCFont(GDK_GC_XGC(gc), ((GdkFontPrivate*)font)->fid);
/* [00/08/30] @@@ NANOGTK: begin modification @@@ */
  	//GrText(GDK_DRAWABLE_XID(drawable), GDK_GC_XGC (gc), x, y, text, text_length);
/* [00/08/30] @@@ NANOGTK: end modification @@@ */
 
}

void
gdk_draw_text_wc (GdkDrawable	 *drawable,
		  GdkFont	 *font,
		  GdkGC		 *gc,
		  gint		  x,
		  gint		  y,
		  const GdkWChar *text,
		  gint		  text_length)
{
	g_return_if_fail (drawable != NULL);
        g_return_if_fail (font != NULL);
        g_return_if_fail (gc != NULL);
        g_return_if_fail (text!= NULL);

	//GrSetGCUseBackground(GDK_GC_XGC(gc), FALSE); //fixme
	GDK_GC_XGC(gc)->values.font =  GDK_FONT_XFONT(font);
	//GrSetGCFont(GDK_GC_XGC(gc), ((GdkFontPrivate*)font)->fid);

/* [00/08/30] @@@ NANOGTK: begin modification @@@ */
        //GrText (GDK_DRAWABLE_XID(drawable), GDK_GC_XGC (gc),
        //        x, y, text, text_length, GR_TFUC32|GR_TFBASELINE);

        {
                gchar* mstr;
                GdkWChar* str;
                gint i;

                str = g_new(GdkWChar, text_length + 1);
                for (i = 0; i < text_length; i++) {
                        str[i] = text[i];
                }
                str[i] = 0;
                mstr = gdk_wcstombs(str);
				gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));
                //GrText (GDK_DRAWABLE_XID(drawable), GDK_GC_XGC (gc),
                 //       x, y, mstr, strlen(mstr));

				 int r,g,b;

				gi_pixel_to_color(&gdk_screen_info,GDK_GC_XGC(gc)->values.foreground,&r, &g,&b);

				gi_ufont_set_text_attr(GDK_GC_XGC(gc),GDK_FONT_XFONT(font),FALSE,GI_RGB(r,g,b),0); //
				gi_ufont_draw(GDK_GC_XGC(gc),GDK_FONT_XFONT(font),mstr, x,y,strlen(mstr));


                g_free(str);
                g_free(mstr);
        }
/* [00/08/30] @@@ NANOGTK: end modification @@@ */
}

void
gdk_draw_pixmap (GdkDrawable *drawable,
		 GdkGC       *gc,
		 GdkPixmap   *src,
		 gint         xsrc,
		 gint         ysrc,
		 gint         xdest,
		 gint         ydest,
		 gint         width,
		 gint         height)
{
  GdkWindowPrivate *drawable_private;
  GdkWindowPrivate *src_private;
  GdkGCPrivate *gc_private;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (src != NULL);
  g_return_if_fail (gc != NULL);

  drawable_private = (GdkWindowPrivate*) drawable;
  src_private = (GdkWindowPrivate*) src;
  if (drawable_private->destroyed || src_private->destroyed)
    return;
  gc_private = (GdkGCPrivate*) gc;

  if (width == -1)
    width = src_private->width;
  if (height == -1)
    height = src_private->height;

  if( drawable_private->height - ydest < height)
          height = drawable_private->height - ydest;

  if( drawable_private->width - xdest < width)
          width = drawable_private->width - xdest;
//gi_copy_area(drawable_private->xwindow,gc_private->xgc,xdest, ydest, width, height, src_private->xwindow, xsrc, ysrc);
  gi_copy_area (
	     src_private->xwindow,
	     drawable_private->xwindow,
	     gc_private->xgc,
	     xsrc, ysrc,
	     width, height,
	     xdest, ydest);
}

void
gdk_draw_image (GdkDrawable *drawable,
		GdkGC       *gc,
		GdkImage    *image,
		gint         xsrc,
		gint         ysrc,
		gint         xdest,
		gint         ydest,
		gint         width,
		gint         height)
{
  GdkImagePrivate *image_private;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (image != NULL);
  g_return_if_fail (gc != NULL);

  image_private = (GdkImagePrivate*) image;

  if (width == -1)
    width = image->width;
  if (height == -1)
    height = image->height;

  	/*(*image_private->image_put) (drawable, gc, image, xsrc, ysrc,
                                xdest, ydest, width, height);*/
 	gdk_image_put_normal (drawable, gc, image,
                      xsrc, ysrc, xdest, ydest,
                      width, height);


}

void
gdk_draw_points (GdkDrawable *drawable,
		 GdkGC       *gc,
		 GdkPoint    *points,
		 gint         npoints)
{
  gi_point_t	*nanoPoints;
  guint		i;
  g_return_if_fail (drawable != NULL);
  g_return_if_fail ((points != NULL) && (npoints > 0));
  g_return_if_fail (gc != NULL);

   if (GDK_DRAWABLE_DESTROYED(drawable))
	return;
	gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

 nanoPoints = g_new(gi_point_t,npoints);
  // GdkPoint (gint16) to gi_point_t (int)
 for (i=0;i<npoints;i++)
 {
 	nanoPoints[i].x = points[i].x;
	 nanoPoints[i].y = points[i].y;
 }

  gi_draw_points(GDK_GC_XGC(gc), npoints, nanoPoints);
  g_free(nanoPoints);
}

void
gdk_draw_segments (GdkDrawable *drawable,
		   GdkGC       *gc,
		   GdkSegment  *segs,
		   gint         nsegs)
{
  int i;
  gi_point_t pts[2];

	PRINTFILE("gdk_draw_segments");	// not used by any exists widget
  if (nsegs <= 0)
    return;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (segs != NULL);
  g_return_if_fail (gc != NULL);


  if(GDK_DRAWABLE_DESTROYED(drawable)) return;
  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));
  for (i = 0; i < nsegs; i++) {

	pts[0].x = segs[i].x1;
      pts[0].y = segs[i].y1;
      pts[1].x = segs[i].x2;
      pts[1].y = segs[i].y2;

	  gi_draw_lines( GDK_GC_XGC (gc), pts,2);

  }
}

void
gdk_draw_lines (GdkDrawable *drawable,
              GdkGC       *gc,
              GdkPoint    *points,
              gint         npoints)
{
  gi_point_t	*nanoPoints;
  gint	i;
	PRINTFILE("gdk_draw_lines"); // only used by gtkctree widget
  if (npoints <= 0)
    return;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (points != NULL);
  g_return_if_fail (gc != NULL);

  if(GDK_DRAWABLE_DESTROYED(drawable)) return;
  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

 nanoPoints = g_new(gi_point_t,npoints);
 for (i=0;i<npoints;i++)
 {
	nanoPoints[i].x = points[i].x;
	nanoPoints[i].y = points[i].y;
 }
  gi_draw_lines( GDK_GC_XGC(gc), npoints, nanoPoints);
  g_free(nanoPoints);
}
