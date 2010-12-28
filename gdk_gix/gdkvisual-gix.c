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

#include "config.h"

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkscreen.h"
#include "gdkvisual.h"
#include "gdkalias.h"


struct _GdkVisualClass
{
  GObjectClass parent_class;
};


static void                gdk_visual_decompose_mask  (gulong   mask,
                                                       gint    *shift,
                                                       gint    *prec);
static GdkVisualGix * gdk_gix_visual_create (gi_format_code_t  pixelformat);

//FIXME DPP
static gi_format_code_t formats[] =
{
  GI_RENDER_a8r8g8b8,
  //DSPF_LUT8,

  GI_RENDER_x8r8g8b8,
  GI_RENDER_r8g8b8,
  GI_RENDER_r5g6b5,
  //GI_RENDER_x1r5g5b5,
  //GI_RENDER_r3g3b2
};

GdkVisual         * system_visual = NULL;
static GdkVisualGix * visuals[G_N_ELEMENTS (formats) + 1] = { NULL };
static gint                available_depths[G_N_ELEMENTS (formats) + 1] = {0};
static GdkVisualType       available_types[G_N_ELEMENTS (formats) + 1]  = {0};


static void
gdk_visual_finalize (GObject *object)
{
  g_error ("A GdkVisual object was finalized. This should not happen");
}

static void
gdk_visual_class_init (GObjectClass *class)
{
  class->finalize = gdk_visual_finalize;
}

GType
gdk_visual_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (GdkVisualClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gdk_visual_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GdkVisualGix),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "GdkVisual",
                                            &object_info, 0);
    }

  return object_type;
}

void
_gdk_visual_init ()
{
  gi_screen_info_t  dlc;
  //DFBSurfaceDescription  desc;
  //IGixSurface      *dest;
  gint                   i, c;


  gi_get_screen_info ( &dlc);
  g_assert( dlc.format != 0);

  //dest = gdk_display_dfb_create_surface(_gdk_display,dlc.pixelformat,8,8);
  //g_assert (dest != NULL);

  /* We could provide all visuals since Gix allows us to mix
     surface formats. Blitting with format conversion can however
     be incredibly slow, so we've choosen to register only those
     visuals that can be blitted to the display layer in hardware.

     If you want to use a special pixelformat that is not registered
     here, you can create it using the Gix-specific function
     gdk_gix_visual_by_format().
     Note:
     changed to do all formats but we should redo this code
     to ensure the base format ARGB LUT8 RGB etc then add ones supported
     by the hardware
   */
  for (i = 0; i < G_N_ELEMENTS (formats); i++)
    {
      /*IGixSurface    *src;
      DFBAccelerationMask  acc;

      desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
      desc.width       = 8;
      desc.height      = 8;
      desc.pixelformat = formats[i];
      if (_gdk_display->gix->CreateSurface (_gdk_display->gix,
	 &desc, &src) != 0) 
        continue;
		*/

      visuals[i] = gdk_gix_visual_create (formats[i]);

      //dest->GetAccelerationMask (dest, src, &acc);

      if ( formats[i] == dlc.format)
        {
			system_visual = GDK_VISUAL (visuals[i]);
        }

      //src->Release (src);
    }

  //dest->Release (dest);

  //fallback to ARGB must be supported
  if (!system_visual)
    {
       g_assert (visuals[GI_RENDER_a8r8g8b8] != NULL);
       system_visual = GDK_VISUAL(visuals[GI_RENDER_a8r8g8b8]);
    }

  g_assert (system_visual != NULL);
}

gint
gdk_visual_get_best_depth (void)
{
  return system_visual->depth;
}

GdkVisualType
gdk_visual_get_best_type (void)
{
  return system_visual->type;
}

GdkVisual*
gdk_screen_get_system_visual (GdkScreen *screen)
{
  g_assert( system_visual);
  return system_visual;
}

GdkVisual*
gdk_visual_get_best (void)
{
  return system_visual;
}

GdkVisual*
gdk_visual_get_best_with_depth (gint depth)
{
  gint i;

  for (i = 0; visuals[i]; i++)
    {
      if( visuals[i] ) {
        GdkVisual *visual = GDK_VISUAL (visuals[i]);

        if (depth == visual->depth)
            return visual;
      }
    }

  return NULL;
}

GdkVisual*
gdk_visual_get_best_with_type (GdkVisualType visual_type)
{
  gint i;

  for (i = 0; visuals[i]; i++)
    {
      if( visuals[i] ) {
        GdkVisual *visual = GDK_VISUAL (visuals[i]);

        if (visual_type == visual->type)
            return visual;
      }
    }

  return NULL;
}

GdkVisual*
gdk_visual_get_best_with_both (gint          depth,
                               GdkVisualType visual_type)
{
  gint i;

  for (i = 0; visuals[i]; i++)
    {
      if( visuals[i] ) {
        GdkVisual *visual = GDK_VISUAL (visuals[i]);

        if (depth == visual->depth && visual_type == visual->type)
            return visual;
      }
    }

  return system_visual;
}

void
gdk_query_depths (gint **depths,
                  gint  *count)
{
  gint i;

  for (i = 0; available_depths[i]; i++)
    ;

  *count = i;
  *depths = available_depths;
}

void
gdk_query_visual_types (GdkVisualType **visual_types,
                        gint           *count)
{
  gint i;

  for (i = 0; available_types[i]; i++)
    ;

  *count = i;
  *visual_types = available_types;
}

GList *
gdk_screen_list_visuals (GdkScreen *screen)
{
  GList *list = NULL;
  gint   i;

  for (i = 0; visuals[i]; i++)
   if( visuals[i] ) {
        GdkVisual * vis = GDK_VISUAL(visuals[i]);
        list = g_list_append (list,vis);
   }

  return list;
}

/**
 * gdk_gix_visual_by_format:
 * @pixel_format: the pixel_format of the requested visual
 *
 * This function is specific to the Gix backend. It allows
 * to specify a GdkVisual by @pixel_format.
 *
 * At startup, only those visuals that can be blitted
 * hardware-accelerated are registered.  By using
 * gdk_gix_visual_by_format() you can retrieve visuals that
 * don't match this criteria since this function will try to create
 * a new visual for the desired @pixel_format for you.
 *
 * Return value: a pointer to the GdkVisual or %NULL if the
 * pixel_format is unsupported.
 **/
GdkVisual *
gdk_gix_visual_by_format (gi_format_code_t pixel_format)
{
  gint i;

  /* first check if one the registered visuals matches */
  for (i = 0; visuals[i]; i++)
    if ( visuals[i] && visuals[i]->format == pixel_format)
      return GDK_VISUAL (visuals[i]);

#if 0 //FIXME DPP
  /* none matched, try to create a new one for this pixel_format */
  {
    DFBSurfaceDescription  desc;
    IGixSurface      *test;

    desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
    desc.width       = 8;
    desc.height      = 8;
    desc.pixelformat = pixel_format;

    if ( _gdk_display->gix->CreateSurface ( _gdk_display->gix, &desc, &test) != 0)
      return NULL;

    test->Release (test);
  }
#endif

  return GDK_VISUAL(gdk_gix_visual_create (pixel_format));
}

GdkScreen *
gdk_visual_get_screen (GdkVisual *visual)
{
  g_return_val_if_fail (GDK_IS_VISUAL (visual), NULL);

  return gdk_screen_get_default ();
}

static void
gdk_visual_decompose_mask (gulong  mask,
                           gint   *shift,
                           gint   *prec)
{
  *shift = 0;
  *prec  = 0;

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

static GdkVisualGix *
gdk_gix_visual_create (gi_format_code_t  pixelformat)
{
  GdkVisual *visual;
  gint       i;

  for (i = 0; i < G_N_ELEMENTS (formats); i++)
    if (formats[i] == pixelformat)
      break;

  if (i ==  G_N_ELEMENTS (formats))
    {
      g_warning ("unsupported pixelformat");
      return NULL;
    }

  visual = g_object_new (GDK_TYPE_VISUAL, NULL);

  switch (pixelformat)
    {
    //case DSPF_LUT8:
    //  visual->type         = GDK_VISUAL_PSEUDO_COLOR;
     // visual->bits_per_rgb = 8;
    //  break;

    case GI_RENDER_r3g3b2:
      visual->type         = GDK_VISUAL_STATIC_COLOR;
      visual->bits_per_rgb = 3;
      break;

    case GI_RENDER_x1r5g5b5:
      visual->type         = GDK_VISUAL_TRUE_COLOR;
      visual->red_mask     = 0x00007C00;
      visual->green_mask   = 0x000003E0;
      visual->blue_mask    = 0x0000001F;
      visual->bits_per_rgb = 5;
      break;

    case GI_RENDER_r5g6b5:
      visual->type         = GDK_VISUAL_TRUE_COLOR;
      visual->red_mask     = 0x0000F800;
      visual->green_mask   = 0x000007E0;
      visual->blue_mask    = 0x0000001F;
      visual->bits_per_rgb = 6;
      break;

    case GI_RENDER_r8g8b8:
    case GI_RENDER_x8r8g8b8:
    case GI_RENDER_a8r8g8b8:
      visual->type         = GDK_VISUAL_TRUE_COLOR;
      visual->red_mask     = 0x00FF0000;
      visual->green_mask   = 0x0000FF00;
      visual->blue_mask    = 0x000000FF;
      visual->bits_per_rgb = 8;
      break;

    default:
      g_assert_not_reached ();
    }

#if G_BYTE_ORDER == G_BIG_ENDIAN
  visual->byte_order = GDK_MSB_FIRST;
#else
  visual->byte_order = GDK_LSB_FIRST;
#endif

  visual->depth      = GI_RENDER_FORMAT_BPP (pixelformat);

  switch (visual->type)
    {
    case GDK_VISUAL_TRUE_COLOR:
      gdk_visual_decompose_mask (visual->red_mask,
                                 &visual->red_shift, &visual->red_prec);
      gdk_visual_decompose_mask (visual->green_mask,
                                 &visual->green_shift, &visual->green_prec);
      gdk_visual_decompose_mask (visual->blue_mask,
                                 &visual->blue_shift, &visual->blue_prec);

      /* the number of possible levels per color component */
      visual->colormap_size = 1 << MAX (visual->red_prec,
                                        MAX (visual->green_prec,
                                             visual->blue_prec));
      break;

    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_PSEUDO_COLOR:
      visual->colormap_size = 1 << visual->depth;

      visual->red_mask    = 0;
      visual->red_shift   = 0;
      visual->red_prec    = 0;

      visual->green_mask  = 0;
      visual->green_shift = 0;
      visual->green_prec  = 0;

      visual->blue_mask   = 0;
      visual->blue_shift  = 0;
      visual->blue_prec   = 0;

      break;

    default:
      g_assert_not_reached ();
    }

  ((GdkVisualGix *)visual)->format = pixelformat;

  for (i = 0; available_depths[i]; i++)
    if (available_depths[i] == visual->depth)
      break;
  if (!available_depths[i])
    available_depths[i] = visual->depth;

  for (i = 0; available_types[i]; i++)
    if (available_types[i] == visual->type)
      break;
  if (!available_types[i])
    available_types[i] = visual->type;

  return (GdkVisualGix *) visual;
}

#define __GDK_VISUAL_X11_C__
#include "gdkaliasdef.c"
