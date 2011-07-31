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


#include "config.h"
#include "gdk.h"


#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkinternals.h"

#include "gdkimage.h"
#include "gdkalias.h"


static GList    *image_list   = NULL;
static gpointer  parent_class = NULL;

static void gdk_gix_image_destroy (GdkImage      *image);
static void gdk_image_init             (GdkImage      *image);
static void gdk_image_class_init       (GdkImageClass *klass);
static void gdk_image_finalize         (GObject       *object);

GType
gdk_image_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
        {
          sizeof (GdkImageClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) gdk_image_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (GdkImage),
          0,              /* n_preallocs */
          (GInstanceInitFunc) gdk_image_init,
        };

      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "GdkImage",
                                            &object_info, 0);
    }

  return object_type;
}

static void
gdk_image_init (GdkImage *image)
{
  image->windowing_data = g_new0 (GdkImageGix, 1);
  image->mem = NULL;

  image_list = g_list_prepend (image_list, image);
}

static void
gdk_image_class_init (GdkImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_image_finalize;
}

static void
gdk_image_finalize (GObject *object)
{
  GdkImage *image;

  image = GDK_IMAGE (object);

  image_list = g_list_remove (image_list, image);

  if (image->depth == 1)
    g_free (image->mem);

  gdk_gix_image_destroy (image);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* this function is called from the atexit handler! */
void
_gdk_image_exit (void)
{
  GObject *image;

  while (image_list)
    {
      image = image_list->data;

      gdk_image_finalize (image);
    }
}

GdkImage *
gdk_image_new_bitmap (GdkVisual *visual,
                      gpointer   data,
                      gint       w,
                      gint       h)
{
  GdkImage         *image;
  GdkImageGix *private;

  image = g_object_new (gdk_image_get_type (), NULL);
  private = image->windowing_data;

  image->type   = GDK_IMAGE_SHARED;
  image->visual = visual;
  image->width  = w;
  image->height = h;
  image->depth  = 1;

  GDK_NOTE (MISC, g_print ("gdk_image_new_bitmap: %dx%d\n", w, h));

  g_message ("not fully implemented %s", __FUNCTION__);

  image->bpl = (w + 7) / 8;
  image->mem = g_malloc (image->bpl * h);
#if G_BYTE_ORDER == G_BIG_ENDIAN
  image->byte_order = GDK_MSB_FIRST;
#else
  image->byte_order = GDK_LSB_FIRST;
#endif
  image->bpp = 1;

  return image;
}

void
_gdk_windowing_image_init (void)
{
}

GdkImage*
_gdk_image_new_for_depth (GdkScreen    *screen,
                          GdkImageType  type,
                          GdkVisual    *visual,
                          gint          width,
                          gint          height,
                          gint          depth)
{
  GdkImage              *image;
  GdkImageGix      *private;
  gint                   pitch;
  unsigned  format;
  gi_image_t      *surface;

  if (type == GDK_IMAGE_FASTEST || type == GDK_IMAGE_NORMAL)
    type = GDK_IMAGE_SHARED;

  if (visual)
    depth = visual->depth;

  switch (depth)
    {
    case 8:
      format = GI_RENDER_r3g3b2;
      break;
    case 15:
      format = GI_RENDER_x1r5g5b5;
      break;
    case 16:
      format = GI_RENDER_r5g6b5;
      break;
    case 24:
      format = GI_RENDER_x8r8g8b8;
      break;
    case 32:
      format = GI_RENDER_a8r8g8b8;
      break;
    default:
      g_message ("unimplemented %s for depth %d", __FUNCTION__, depth);
      return NULL;
    }

  surface = gi_create_image_depth(width,height,format);

  if (!surface)
    {
      GixError( "gdk_image_new: Create image failed" );
      return NULL;
    }

  image = g_object_new (gdk_image_get_type (), NULL);
  private = image->windowing_data;

  private->surface_img = surface;

  image->mem = surface->rgba;
  pitch = surface->pitch;

  
  image->type           = type;
  image->visual         = visual;
  image->byte_order     = GDK_LSB_FIRST;
  image->width          = width;
  image->height         = height;
  image->depth          = depth;
  image->bpp            = GI_RENDER_FORMAT_BPP (format)/8;
  image->bpl            = pitch;
  image->bits_per_pixel = GI_RENDER_FORMAT_BPP (format);

  image_list = g_list_prepend (image_list, image);

  return image;
}


GdkImage*
_gdk_gix_copy_to_image (GdkDrawable *drawable,
                             GdkImage    *image,
                             gint         src_x,
                             gint         src_y,
                             gint         dest_x,
                             gint         dest_y,
                             gint         width,
                             gint         height)
{
  GdkDrawableImplGix *impl;
  GdkImageGix        *private;
  int                      pitch; 
  int err;
  gi_window_info_t info;
  
  g_return_val_if_fail (GDK_IS_DRAWABLE_IMPL_GIX (drawable), NULL);
  g_return_val_if_fail (image != NULL || (dest_x == 0 && dest_y == 0), NULL);
  
  impl = GDK_DRAWABLE_IMPL_GIX (drawable);

  if (impl->wrapper == _gdk_parent_root)
    {
	  GixError ("_gdk_gix_copy_to_image - root window" );
 //FIXME DPP     
    }

  if (! impl->window_id)
    return NULL;

  if (!image)
    image =  gdk_image_new (GDK_IMAGE_NORMAL,
                            gdk_drawable_get_visual (drawable), width, height);

  private = image->windowing_data;
  
  err = gi_get_window_info(impl->window_id,&info);
  if (err)
  {
	  g_print("_gdk_gix_copy_to_image: bad window %x\n",impl->window_id);
	  return NULL;
  }

  g_print("_gdk_gix_copy_to_image: %d,%d/%d,%d\n", dest_x, dest_y,src_x, src_y);
  pitch = gi_image_get_pitch(info.format, width);
  err = gi_get_window_buffer(impl->window_id, dest_x, dest_y, width, height,
	  pitch<<16 , info.format, image->mem);
  if (err)
  {
  }

  image->bpl = pitch;
  return image;
}

guint32
gdk_image_get_pixel (GdkImage *image,
                     gint      x,
                     gint      y)
{
  guint32 pixel = 0;

  g_return_val_if_fail (GDK_IS_IMAGE (image), 0);

  if (!(x >= 0 && x < image->width && y >= 0 && y < image->height))
    return 0;

  if (image->depth == 1)
    pixel = (((guchar *) image->mem)[y * image->bpl + (x >> 3)] & (1 << (7 - (x & 0x7)))) != 0;
  else
    {
      guchar *pixelp = (guchar *) image->mem + y * image->bpl + x * image->bpp;

      switch (image->bpp)
        {
        case 1:
          pixel = *pixelp;
          break;

        case 2:
          pixel = pixelp[0] | (pixelp[1] << 8);
          break;

        case 3:
          pixel = pixelp[0] | (pixelp[1] << 8) | (pixelp[2] << 16);
          break;

        case 4:
          pixel = pixelp[0] | (pixelp[1] << 8) | (pixelp[2] << 16);
          break;
        }
    }

  return pixel;
}

void
gdk_image_put_pixel (GdkImage *image,
                     gint       x,
                     gint       y,
                     guint32    pixel)
{
  g_return_if_fail (image != NULL);

  if  (!(x >= 0 && x < image->width && y >= 0 && y < image->height))
    return;

  if (image->depth == 1)
    if (pixel & 1)
      ((guchar *) image->mem)[y * image->bpl + (x >> 3)] |= (1 << (7 - (x & 0x7)));
    else
      ((guchar *) image->mem)[y * image->bpl + (x >> 3)] &= ~(1 << (7 - (x & 0x7)));
  else
    {
      guchar *pixelp = (guchar *) image->mem + y * image->bpl + x * image->bpp;

      switch (image->bpp)
        {
        case 4:
          pixelp[3] = 0xFF;
        case 3:
          pixelp[2] = ((pixel >> 16) & 0xFF);
        case 2:
          pixelp[1] = ((pixel >> 8) & 0xFF);
        case 1:
          pixelp[0] = (pixel & 0xFF);
        }
    }
}

static void
gdk_gix_image_destroy (GdkImage *image)
{
  GdkImageGix *private;

  g_return_if_fail (GDK_IS_IMAGE (image));

  private = image->windowing_data;

  if (!private)
    return;

  GDK_NOTE (MISC, g_print ("gdk_gix_image_destroy: %#lx\n",
                           (gulong) private->surface));

  if(private->surface_img)
    gi_destroy_image( private->surface_img );

  g_free (private);
  image->windowing_data = NULL;
}

gint
_gdk_windowing_get_bits_for_depth (GdkDisplay *display,
                                   gint        depth)
{
  switch (depth)
    {
    case 1:
    case 8:
      return 8;
    case 15:
    case 16:
      return 16;
    case 24:
    case 32:
      return 32;
    }

  return 0;
}

#define __GDK_IMAGE_X11_C__
#include "gdkaliasdef.c"
