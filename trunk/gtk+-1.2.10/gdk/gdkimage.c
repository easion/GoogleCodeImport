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

#include <config.h>

/* gcc -ansi -pedantic on GNU/Linux causes warnings and errors
 * unless this is defined:
 * warning: #warning "Files using this header must be compiled with _SVID_SOURCE or _XOPEN_SOURCE"
 */
#ifndef _XOPEN_SOURCE
#  define _XOPEN_SOURCE 1
#endif

#include <stdlib.h>
#include <sys/types.h>

#if defined (HAVE_IPC_H) && defined (HAVE_SHM_H) && defined (HAVE_XSHM_H)
#define USE_SHM
#endif


#include "gdk.h"
#include "gdkprivate.h"

extern gi_screen_info_t gdk_screen_info;
void gdk_image_put_normal (GdkDrawable *drawable,
				  GdkGC       *gc,
				  GdkImage    *image,
				  gint         xsrc,
				  gint         ysrc,
				  gint         xdest,
				  gint         ydest,
				  gint         width,
				  gint         height);
static void gdk_image_put_shared (GdkDrawable *drawable,
				  GdkGC       *gc,
				  GdkImage    *image,
				  gint         xsrc,
				  gint         ysrc,
				  gint         xdest,
				  gint         ydest,
				  gint         width,
				  gint         height);


static GList *image_list = NULL;

static gi_gc_ptr_t  gc_for_image = 0;

#define PRINTFILE(s) g_message("gdkimage.c:%s()",s)

void
gdk_image_exit (void)
{
  GdkImage *image;

  while (image_list)
    {
      image = image_list->data;
      gdk_image_destroy (image);
    }
}

GdkImage *
gdk_image_new_bitmap(GdkVisual *visual, gpointer data, gint w, gint h)
/*
 * Desc: create a new bitmap image
 */
{
        gi_screen_info_t *xvisual;
        GdkImage *image;
        GdkImagePrivate *private;
        private = g_new(GdkImagePrivate, 1);
        image = (GdkImage *) private;
        //private->xdisplay = gdk_display;
        private->image_put = gdk_image_put_normal;
        image->type = GDK_IMAGE_NORMAL;
        image->visual = visual;
        image->width = w;
        image->height = h;
        image->depth = 1;
        //xvisual = ((GdkVisualPrivate*) visual)->xvisual;
        private->ximage = gi_create_image_with_data( w ,h, data, GI_RENDER_a8);
        //private->ximage->data = data;
        //private->ximage->bitmap_bit_order = MSBFirst;
        //private->ximage->byte_order = MSBFirst;
        //image->byte_order = MSBFirst;
        image->mem =  private->ximage->rgba ;
        image->bpl = 1*w;//private->ximage->bytes_per_line;
        image->bpp = 1;
	return(image);
} /* gdk_image_new_bitmap() */

static int
gdk_image_check_xshm()
/* 
 * Desc: query the server for support for the MIT_SHM extension
 * Return:  0 = not available
 *          1 = shared XImage support available
 *          2 = shared Pixmap support available also
 */
{
#if 0 //USE_SHM
  int major, minor, ignore;
  Bool pixmaps;
  
  if (XQueryExtension(display, "MIT-SHM", &ignore, &ignore, &ignore)) 
    {
      if (XShmQueryVersion(display, &major, &minor, &pixmaps )==True) 
	{
	  return (pixmaps==True) ? 2 : 1;
	}
    }
#endif /* USE_SHM */
  return 0;
}

void
gdk_image_init (void)
{
  if (gdk_use_xshm)
    {
      if (!gdk_image_check_xshm ())
	{
	  gdk_use_xshm = False;
	}
    }
}

GdkImage*
gdk_image_new (GdkImageType  type,
	       GdkVisual    *visual,
	       gint          width,
	       gint          height)
{
  GdkImage *image;
  GdkImagePrivate *private;

  //PRINTFILE("__IMPLEMENTED__ gdk_image_new");
  private = g_new (GdkImagePrivate, 1);
  image = (GdkImage*) private;

  private->image_put = gdk_image_put_normal;
  private->ximage = gi_create_image_depth(width, height, gi_get_choose_format(visual->depth));
  private->bnImageInServer = FALSE;
  image->type = GDK_IMAGE_NORMAL;
  image->visual = visual;
  image->width = width;
  image->height = height;
  image->depth = visual->depth;

  image->byte_order = LSBFirst; 
  if(GI_RENDER_FORMAT_BPP(gdk_screen_info.format) < 8) image->bpl = width;
  else
  if(GI_RENDER_FORMAT_BPP(gdk_screen_info.format) == 12) image->bpl = width*2;//((width+1)*3)/2;
  else
  	image->bpl = (width *GI_RENDER_FORMAT_BPP(gdk_screen_info.format)) >> 3;
  image->mem = private->ximage->rgba;//g_malloc(image->bpl * height);  
  image->bpp = (GI_RENDER_FORMAT_BPP(gdk_screen_info.format) + 7) >> 3;
  return image;
}


GdkImage*
gdk_image_get (GdkWindow *window,
	       gint       x,
	       gint       y,
	       gint       width,
	       gint       height)
{
  GdkImage *image;
  GdkImagePrivate *private;
  GdkWindowPrivate *win_private;
  gi_image_t *ximage;

  g_return_val_if_fail (window != NULL, NULL);

  win_private = (GdkWindowPrivate *) window;
  if (win_private->destroyed)
    return NULL;
  char *buffer = NULL;

  buffer = malloc(width*height*GI_RENDER_FORMAT_BPP(gdk_screen_info.format)/8);

  ximage = gi_get_window_image (
		      win_private->xwindow,
		      x, y, width, height,gdk_screen_info.format,FALSE,
		      buffer);
  
  if (ximage == NULL)
    return NULL;
  
  private = g_new (GdkImagePrivate, 1);
  image = (GdkImage*) private;

  //private->xdisplay = gdk_display;
  private->image_put = gdk_image_put_normal;
  private->ximage = ximage;
  image->type = GDK_IMAGE_NORMAL;
  image->visual = gdk_window_get_visual (window);
  image->width = width;
  image->height = height;
  image->depth = GI_RENDER_FORMAT_BPP(private->ximage->format);

  image->mem = private->ximage->rgba;
  //image->bpl = private->ximage->bytes_per_line;
  //image->bpp = private->ximage->bits_per_pixel;
  //image->byte_order = private->ximage->byte_order;
  image->byte_order = LSBFirst;

  image->bpl = image->depth/8;
  image->bpp = image->depth;

  return image;
}

guint32
gdk_image_get_pixel (GdkImage *image,
		     gint x,
		     gint y)
{
  guint32 pixel;
  GdkImagePrivate *private;

  g_return_val_if_fail (image != NULL, 0);

  private = (GdkImagePrivate *) image;

  pixel = gi_image_getpixel (private->ximage, x, y);

  return pixel;
}

void
gdk_image_put_pixel (GdkImage *image,
		     gint x,
		     gint y,
		     guint32 pixel)
{
  GdkImagePrivate *private;

  g_return_if_fail (image != NULL);

  private = (GdkImagePrivate *) image;

  gi_image_putpixel (private->ximage, x, y, pixel);
}

void
gdk_image_destroy (GdkImage *image)
{
	GdkImagePrivate *image_private;

	//PRINTFILE("gdk_image_destroy");
  	g_return_if_fail (image != NULL);
	image_private = (GdkImagePrivate*)image;
	
	if(image_private->ximage)
		gi_destroy_window(image_private->ximage);
	if (image->mem)
		g_free(image->mem);
	g_free(image);
}

#if 0

#else
void
gdk_image_put_buffer (GdkDrawable *drawable,
                      GdkGC       *gc,
                      guchar      *buffer,
                      gint         xsrc,
                      gint         ysrc,
                      gint         xdest,
                      gint         ydest,
                      gint         width,
                      gint         height,
                      gint         src_width,
                      gint         src_height,
		      gint	   alpha)
{
        guchar * small_buffer, *ptr;
        gint h;
		gi_image_t img;
        g_return_if_fail (drawable != NULL);
        g_return_if_fail (buffer != NULL);
        g_return_if_fail (gc != NULL);

        if (GDK_DRAWABLE_DESTROYED(drawable)) return;

		gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));



	img.w = width;
	img.h = height;
	img.rgba = buffer;
	img.format = gdk_screen_info.format;
	img.pitch = gi_image_get_pitch((gi_format_code_t)img.format, img.w);

	gi_bitblt_bitmap( gc,xsrc,ysrc,width, height,&img,xdest, ydest);
}
#endif


void
gdk_image_put_normal (GdkDrawable *drawable,
		      GdkGC       *gc,
		      GdkImage    *image,
		      gint         xsrc,
		      gint         ysrc,
		      gint         xdest,
		      gint         ydest,
		      gint         width,
		      gint         height)
{
  GdkWindowPrivate *drawable_private;
  GdkImagePrivate *image_private;
  GdkGCPrivate *gc_private;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (image != NULL);
  g_return_if_fail (gc != NULL);

  drawable_private = (GdkWindowPrivate*) drawable;
  if (drawable_private->destroyed)
    return;
  image_private = (GdkImagePrivate*) image;
  gc_private = (GdkGCPrivate*) gc;


  g_return_if_fail (image->type == GDK_IMAGE_NORMAL);
  gi_gc_attch_window(GDK_GC_XGC(gc), GDK_DRAWABLE_XID(drawable));

  gi_bitblt_bitmap( GDK_GC_XGC(gc),xsrc,ysrc,width,height,image_private->ximage,xdest,ydest);
}

static void
gdk_image_put_shared (GdkDrawable *drawable,
		      GdkGC       *gc,
		      GdkImage    *image,
		      gint         xsrc,
		      gint         ysrc,
		      gint         xdest,
		      gint         ydest,
		      gint         width,
		      gint         height)
{
#if 0 //USE_SHM
  GdkWindowPrivate *drawable_private;
  GdkImagePrivate *image_private;
  GdkGCPrivate *gc_private;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (image != NULL);
  g_return_if_fail (gc != NULL);

  drawable_private = (GdkWindowPrivate*) drawable;
  if (drawable_private->destroyed)
    return;
  image_private = (GdkImagePrivate*) image;
  gc_private = (GdkGCPrivate*) gc;

  g_return_if_fail (image->type == GDK_IMAGE_SHARED);

  XShmPutImage (drawable_private->xdisplay, drawable_private->xwindow,
		gc_private->xgc, image_private->ximage,
		xsrc, ysrc, xdest, ydest, width, height, False);
#else /* USE_SHM */
  g_error ("trying to draw shared memory image when gdk was compiled without shared memory support");
#endif /* USE_SHM */
}
