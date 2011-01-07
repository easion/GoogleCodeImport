 /* This library is free software; you can redistribute it and/or
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

/* Color Context module
 * Copyright 1994,1995 John L. Cwikla
 * Copyright (C) 1997 by Ripley Software Development
 * Copyright (C) 1997 by Federico Mena (port to Gtk/Gdk)
 */

/* Copyright 1994,1995 John L. Cwikla
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of John L. Cwikla or
 * Wolfram Research, Inc not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.  John L. Cwikla and Wolfram Research, Inc make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * John L. Cwikla and Wolfram Research, Inc disclaim all warranties with
 * regard to this software, including all implied warranties of
 * merchantability and fitness, in no event shall John L. Cwikla or
 * Wolfram Research, Inc be liable for any special, indirect or
 * consequential damages or any damages whatsoever resulting from loss of
 * use, data or profits, whether in an action of contract, negligence or
 * other tortious action, arising out of or in connection with the use or
 * performance of this software.
 *
 * Author:
 *  John L. Cwikla
 *  X Programmer
 *  Wolfram Research Inc.
 *
 *  cwikla@wri.com
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include <stdlib.h>
#include <string.h>
#include "gdk.h"
#include "gdkprivate.h"
#include "gdkx.h"




#define MAX_IMAGE_COLORS 256

#define PRINTFILE(s) g_message("gdkcc.c:%s",s)

extern gi_screen_info_t gdk_screen_info;

static guint
hash_color (gconstpointer key)
{
  const GdkColor *color = key;

  return (color->red * 33023 + color->green * 30013 + color->blue * 27011);

}

static gint
compare_colors (gconstpointer a,
		gconstpointer b)
{
  const GdkColor *aa = a;
  const GdkColor *bb = b;

  return ((aa->red == bb->red) && (aa->green == bb->green) && (aa->blue == bb->blue));

}

static void
free_hash_entry (gpointer key,
		 gpointer value,
		 gpointer user_data)
{
	g_free(key);
}

static int
pixel_sort (const void *a, const void *b)
{
 return ((GdkColor *) a)->pixel - ((GdkColor *) b)->pixel;
}

/* XXX: This function does an XQueryColors() the hard way, because there is
 * no corresponding function in Gdk.
 */


static void
my_x_query_colors (GdkColormap *colormap, /* this always be system colormap*/
		   GdkColor    *colors,
		   gint         ncolors)
{
  guint    i;
  int r,g,b;

  for (i = 0; i < ncolors; i++)
    {
	  gi_pixel_to_color(&gdk_screen_info,colors[i].pixel,&r,&g,&b);
	//GrFindColor(colors[i].pixel, &pal);
      colors[i].red   = r;
      colors[i].green = g;
      colors[i].blue  = b;
     // colors[i].pixel = pal; 
    }
}

static void
query_colors (GdkColorContext *cc)
{
  guint i;
  guint ncolors = cc->num_colors;

	PRINTFILE("__IMPLEMENTED__: query_colors (GdkColorContext *cc)");
  cc->cmap = g_new(GdkColor, ncolors);

  for (i = 0; i < ncolors  ; i++)
    cc->cmap[i].pixel = cc->clut ? cc->clut[i] :  i;

  my_x_query_colors (cc->colormap, cc->cmap, ncolors);

  qsort (cc->cmap, ncolors, sizeof (GdkColor), pixel_sort); //FIXME :ignore sorting

}

static void
init_bw (GdkColorContext *cc)
{
  GdkColor color;

  g_warning ("init_bw: failed to allocate colors, falling back to black and white");

  cc->mode = GDK_CC_MODE_BW;

  color.red = color.green = color.blue = 0;

  if (!gdk_color_alloc (cc->colormap, &color))
    cc->black_pixel = 0;
  else
    cc->black_pixel = color.pixel;

  color.red = color.green = color.blue = 0xffff;

  if (!gdk_color_alloc (cc->colormap, &color))
    cc->white_pixel = cc->black_pixel ? 0 : 1;
  else
    cc->white_pixel = color.pixel;

  cc->num_colors = 2;

}

static void
init_gray (GdkColorContext *cc)
{
  GdkColorContextPrivate *ccp = (GdkColorContextPrivate *) cc;
  GdkColor *clrs, *cstart;

	PRINTFILE("__IMPLEMENTED__ init_gray (GdkColorContext *cc)");
  cc->num_colors = GDK_VISUAL_XVISUAL (cc->visual)->ncolors;
  cc->clut = NULL;
#if 0
  cc->clut = g_new (gulong, cc->num_colors);
  cstart = g_new (GdkColor, cc->num_colors);

 retrygray:

  dinc = 65535.0 / (cc->num_colors - 1);

  clrs = cstart;

  for (i = 0; i < cc->num_colors; i++)
    {
      clrs->red = clrs->green = clrs->blue = dinc * i;

      if (!gdk_color_alloc (cc->colormap, clrs))
        {
          gdk_colors_free (cc->colormap, cc->clut, i, 0);

          cc->num_colors /= 2;

          if (cc->num_colors > 1)
            goto retrygray;
          else
            {
              g_free (cc->clut);
              cc->clut = NULL;
              init_bw (cc);
              g_free (cstart);
              return;
            }
        }

      cc->clut[i] = clrs++->pixel;
    }

  g_free (cstart);
#endif


  /* XXX: is this the right thing to do? */
  cc->white_pixel = gdk_color_find(255,255,255);//GrFindColor(WHITE, &(cc->white_pixel));	//FindNearestColor
  cc->black_pixel = gdk_color_find(0,0,0);//GrFindColor(BLACK, &(cc->black_pixel));	//FindNearestColor

  cc->mode = GDK_CC_MODE_MY_GRAY;
}

static void
init_color (GdkColorContext *cc)
{
  GdkColorContextPrivate *ccp = (GdkColorContextPrivate *) cc;
  gint cubeval;

  cubeval = 1;
/*  while ((cubeval * cubeval * cubeval) < GDK_VISUAL_XVISUAL (cc->visual)->ncolors)
    cubeval++;
  cubeval--;
*/

  cc->white_pixel = gdk_color_find(255,255,255);//GrFindColor(WHITE, &(cc->white_pixel));	//FindNearestColor
  cc->black_pixel = gdk_color_find(0,0,0);//GrFindColor(BLACK, &(cc->black_pixel));	//FindNearestColor
  cc->num_colors = GDK_VISUAL_XVISUAL (cc->visual)->ncolors;

  /* a CLUT for storing allocated pixel indices */

  cc->max_colors = cc->num_colors;
  cc->clut = g_new (gulong, cc->max_colors);

  cc->mode = GDK_CC_MODE_STD_CMAP;
}

static void
init_true_color (GdkColorContext *cc)
{
 GdkColorContextPrivate *ccp = (GdkColorContextPrivate *) cc;
  gulong rmask, gmask, bmask;

  cc->mode = GDK_CC_MODE_TRUE;

  /* Red */

  rmask = cc->masks.red = cc->visual->red_mask;

  cc->shifts.red = 0;
  cc->bits.red = 0;

  while (!(rmask & 1))
    {
      rmask >>= 1;
      cc->shifts.red++;
    }

  while (rmask & 1)
    {
      rmask >>= 1;
      cc->bits.red++;
    }

  /* Green */

  gmask = cc->masks.green = cc->visual->green_mask;

  cc->shifts.green = 0;
  cc->bits.green = 0;

  while (!(gmask & 1))
    {
      gmask >>= 1;
      cc->shifts.green++;
    }

  while (gmask & 1)
    {
      gmask >>= 1;
      cc->bits.green++;
    }
  /* Blue */

  bmask = cc->masks.blue = cc->visual->blue_mask;

  cc->shifts.blue = 0;
  cc->bits.blue = 0;

  while (!(bmask & 1))
    {
      bmask >>= 1;
      cc->shifts.blue++;
    }

  while (bmask & 1)
    {
      bmask >>= 1;
      cc->bits.blue++;
    }

  cc->num_colors = (cc->visual->red_mask | cc->visual->green_mask | cc->visual->blue_mask) + 1;

  cc->white_pixel = gi_color_to_pixel(&gdk_screen_info,255, 255, 255); //COLORVALTOPIXELVAL(WHITE);  dpp
  cc->black_pixel = gi_color_to_pixel(&gdk_screen_info,0, 0, 0);//COLORVALTOPIXELVAL(BLACK); 
  //GrFindColor(WHITE, &(cc->white_pixel));
  //GrFindColor(BLACK, &(cc->black_pixel));

}
#if 0
static void
init_direct_color (GdkColorContext *cc)
{
	/* Visual not supported */
	PRINTFILE("init_direct_color (GdkColorContext *cc)");
}
#endif

static void
init_palette (GdkColorContext *cc)
{
	PRINTFILE("__IMPLEMENTED__ init_palette (GdkColorContext *cc)");
  /* restore correct mode for this cc */

  switch (cc->visual->type)
    {
    case GDK_VISUAL_STATIC_GRAY:
    case GDK_VISUAL_GRAYSCALE:
      if (GDK_VISUAL_XVISUAL (cc->visual)->ncolors== 2)
        cc->mode = GDK_CC_MODE_BW;
      else
        cc->mode = GDK_CC_MODE_MY_GRAY;
      break;

    case GDK_VISUAL_TRUE_COLOR:
    case GDK_VISUAL_DIRECT_COLOR:
      cc->mode = GDK_CC_MODE_TRUE;
      break;

    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_PSEUDO_COLOR:
      cc->mode = GDK_CC_MODE_STD_CMAP;
      break;

    default:
      cc->mode = GDK_CC_MODE_UNDEFINED;
      break;
    }

  /* previous palette */

  if (cc->num_palette)
    g_free (cc->palette);

  if (cc->fast_dither)
    g_free (cc->fast_dither);

  /* clear hash table if present */

  if (cc->color_hash)
    {
      g_hash_table_foreach (cc->color_hash,
                            free_hash_entry,
                            NULL);
      g_hash_table_destroy (cc->color_hash);
      cc->color_hash = NULL;
    }

  cc->palette = NULL;
  cc->num_palette = 0;
  cc->fast_dither = NULL;
}

GdkColorContext *
gdk_color_context_new (GdkVisual   *visual,
		       GdkColormap *colormap)
{
  GdkColorContextPrivate *ccp;
  GdkColorContext *cc;
  guint retry_count;
  GdkColormap *default_colormap;

  g_assert (visual != NULL);
  g_assert (colormap != NULL);

  ccp = g_new (GdkColorContextPrivate, 1);
  cc = (GdkColorContext *) ccp;
  cc->visual = visual;
  cc->colormap = colormap;
  cc->clut = NULL;
  cc->cmap = NULL;
  cc->mode = GDK_CC_MODE_UNDEFINED;
  cc->need_to_free_colormap = FALSE;

  cc->color_hash = NULL;
  cc->palette = NULL;
  cc->num_palette = 0;
  cc->fast_dither = NULL;

  default_colormap = gdk_colormap_get_system ();
  retry_count = 0;

    while(retry_count < 2)
    {
      /* Only create a private colormap if the visual found isn't equal
       * to the default visual and we don't have a private colormap,
       * -or- if we are instructed to create a private colormap (which
       * never is the case for XmHTML).
       */
      switch (visual->type)
        {
        case GDK_VISUAL_STATIC_GRAY:
        case GDK_VISUAL_GRAYSCALE:
          GDK_NOTE (COLOR_CONTEXT,
                    g_message ("gdk_color_context_new: visual class is %s\n",
                               (visual->type == GDK_VISUAL_STATIC_GRAY) ?
                               "GDK_VISUAL_STATIC_GRAY" :
                               "GDK_VISUAL_GRAYSCALE"));
	if (GDK_VISUAL_XVISUAL (cc->visual)->ncolors== 2)
            init_bw (cc);
          else
            init_gray (cc);
	
          break;

        case GDK_VISUAL_TRUE_COLOR: /* shifts */
          GDK_NOTE (COLOR_CONTEXT,
                    g_message ("gdk_color_context_new: visual class is GDK_VISUAL_TRUE_COLOR\n"));

          init_true_color (cc);
          break;

        case GDK_VISUAL_STATIC_COLOR:
        case GDK_VISUAL_PSEUDO_COLOR:
          GDK_NOTE (COLOR_CONTEXT,
                    g_message ("gdk_color_context_new: visual class is %s\n",
                               (visual->type == GDK_VISUAL_STATIC_COLOR) ?
                               "GDK_VISUAL_STATIC_COLOR" :
                               "GDK_VISUAL_PSEUDO_COLOR"));

          init_color (cc);
          break;

        case GDK_VISUAL_DIRECT_COLOR: /* Unsupported */
        default:
          g_assert_not_reached ();
        }
	/* FIXME: no private color context is allowed */
        break;

    }
  /* no. of colors allocated yet */

  cc->num_allocated = 0;

  GDK_NOTE (COLOR_CONTEXT,
            g_message ("gdk_color_context_new: screen depth is %i, no. of colors is %i\n",
                       cc->visual->depth, cc->num_colors));

  return (GdkColorContext *) cc;
}

GdkColorContext *
gdk_color_context_new_mono (GdkVisual   *visual,
			    GdkColormap *colormap)
{
  GdkColorContextPrivate *ccp;
  GdkColorContext *cc;

  g_assert (visual != NULL);
  g_assert (colormap != NULL);

	PRINTFILE("__IMPLEMENTED__ gdk_color_context_new_mono()");
  cc = g_new (GdkColorContext, 1);
  ccp = (GdkColorContextPrivate *) cc;
  cc->visual = visual;
  cc->colormap = colormap;
  cc->clut = NULL;
  cc->cmap = NULL;
  cc->mode = GDK_CC_MODE_UNDEFINED;
  cc->need_to_free_colormap = FALSE;

  init_bw (cc);

  return (GdkColorContext *) cc;

}

/* This doesn't currently free black/white, hmm... */

void
gdk_color_context_free (GdkColorContext *cc)
{
  g_assert (cc != NULL);

  if ((cc->visual->type == GDK_VISUAL_STATIC_COLOR)
      || (cc->visual->type == GDK_VISUAL_PSEUDO_COLOR))
    {
      gdk_colors_free (cc->colormap, cc->clut, cc->num_allocated, 0);
      g_free (cc->clut);
    }
  else if (cc->clut != NULL)
    {
      gdk_colors_free (cc->colormap, cc->clut, cc->num_colors, 0);
      g_free (cc->clut);
    }

  if (cc->cmap != NULL)
    g_free (cc->cmap);

  if (cc->need_to_free_colormap)
    gdk_colormap_unref (cc->colormap);

  /* free any palette that has been associated with this GdkColorContext */

  init_palette (cc);

  g_free (cc);
}

gulong
gdk_color_context_get_pixel (GdkColorContext *cc,
			     gushort          red,
			     gushort          green,
			     gushort          blue,
			     gint            *failed)
{
  GdkColorContextPrivate *ccp = (GdkColorContextPrivate *) cc;
  gi_pixel_value_t pixelval;

  g_assert (cc != NULL);
  g_assert (failed != NULL);

	//PRINTFILE("__IMPLEMENTED__ gdk_color_context_get_pixel()");
  *failed = FALSE;

  switch (cc->mode)
  {
    case GDK_CC_MODE_BW:
    {
      gdouble value;

      value = (red / 65535.0 * 0.30
               + green / 65535.0 * 0.59
               + blue / 65535.0 * 0.11);


      if (value > 0.5)
        return cc->white_pixel;

      return cc->black_pixel;
    }
    case GDK_CC_MODE_MY_GRAY:
    case GDK_CC_MODE_STD_CMAP:
    default:
	pixelval = gdk_color_find(red,green,blue);//GrFindColor(GI_RGB(red, green, blue), &pixelval);
	return pixelval;

    case GDK_CC_MODE_TRUE:
	pixelval = gi_color_to_pixel(&gdk_screen_info,red, green, blue);
	return pixelval;
  }
}

void
gdk_color_context_get_pixels (GdkColorContext *cc,
			      gushort         *reds,
			      gushort         *greens,
			      gushort         *blues,
			      gint             ncolors,
			      gulong          *colors,
			      gint            *nallocated)
{
  gint i, k, idx;
  gint cmapsize, ncols = 0, nopen = 0, counter = 0;
  gint bad_alloc = FALSE;
  gint failed[MAX_IMAGE_COLORS], allocated[MAX_IMAGE_COLORS];
  GdkColor defs[MAX_IMAGE_COLORS], cmap[MAX_IMAGE_COLORS];
#ifdef G_ENABLE_DEBUG
  gint exact_col = 0, subst_col = 0, close_col = 0, black_col = 0;
#endif
  g_assert (cc != NULL);
  g_assert (reds != NULL);
  g_assert (greens != NULL);
  g_assert (blues != NULL);
  g_assert (colors != NULL);
  g_assert (nallocated != NULL);

	//PRINTFILE("__IMPLEMETED__ gdk_color_context_get_pixels()");
  memset (defs, 0, MAX_IMAGE_COLORS * sizeof (GdkColor));
  memset (failed, 0, MAX_IMAGE_COLORS * sizeof (gint));
  memset (allocated, 0, MAX_IMAGE_COLORS * sizeof (gint));

  /* Will only have a value if used by the progressive image loader */

  ncols = *nallocated;

  *nallocated = 0;

  /* First allocate all pixels */

  for (i = 0; i < ncolors; i++)
    {
      /* colors[i] is only zero if the pixel at that location hasn't
       * been allocated yet.  This is a sanity check required for proper
       * color allocation by the progressive image loader
       */

      if (colors[i] == 0)
        {
          defs[i].red   = reds[i];
          defs[i].green = greens[i];
          defs[i].blue  = blues[i];

          colors[i] = gdk_color_context_get_pixel (cc, reds[i], greens[i], blues[i],
                                                   &bad_alloc);

          /* successfully allocated, store it */

          if (!bad_alloc)
            {
              defs[i].pixel = colors[i];
              allocated[ncols++] = colors[i];
            }
          else
            failed[nopen++] = i;
        }
    }

  *nallocated = ncols;
  /* all colors available, all done */

  if ((ncols == ncolors) || (nopen == 0))
    {
      GDK_NOTE (COLOR_CONTEXT,
                g_message ("gdk_color_context_get_pixels: got all %i colors; "
                           "(%i colors allocated so far)\n", ncolors, cc->num_allocated));

    }
}

void
gdk_color_context_get_pixels_incremental (GdkColorContext *cc,
					  gushort         *reds,
					  gushort         *greens,
					  gushort         *blues,
					  gint             ncolors,
					  gint            *used,
					  gulong          *colors,
					  gint            *nallocated)
{
	PRINTFILE("gdk_color_context_get_pixels_incremental");
}

gint
gdk_color_context_query_color (GdkColorContext *cc,
			       GdkColor        *color)
{
 return gdk_color_context_query_colors (cc, color, 1);
}





gint
gdk_color_context_query_colors (GdkColorContext *cc,
				GdkColor        *colors,
				gint             num_colors)
{
  gint i;
  GdkColor *tc;
  
  g_assert (cc != NULL);
  g_assert (colors != NULL);
  
  switch (cc->mode)
    {
    case GDK_CC_MODE_BW:
      for (i = 0, tc = colors; i < num_colors; i++, tc++)
	{
	  if (tc->pixel == cc->white_pixel)
	    tc->red = tc->green = tc->blue = 65535;
	  else
	    tc->red = tc->green = tc->blue = 0;
	}
      break;
      
    case GDK_CC_MODE_TRUE:
      if (cc->clut == NULL)
	for (i = 0, tc = colors; i < num_colors; i++, tc++)
	  {
	    tc->red   = ((tc->pixel & cc->masks.red) >> cc->shifts.red) << (16 - cc->bits.red);
	    tc->green = ((tc->pixel & cc->masks.green) >> cc->shifts.green) << (16 - cc->bits.green);
	    tc->blue  = ((tc->pixel & cc->masks.blue) >> cc->shifts.blue) << (16 - cc->bits.blue);
	  }
      else
	{
	  my_x_query_colors (cc->colormap, colors, num_colors);
	  return 1;
	}
      break;
      
    case GDK_CC_MODE_STD_CMAP:
    default:
      if (cc->cmap == NULL)
	{
	  my_x_query_colors (cc->colormap, colors, num_colors);
	  return 1;
	}
      else
	{
	  gint first, last, half;
	  gulong half_pixel;
	  
	  for (i = 0, tc = colors; i < num_colors; i++)
	    {
	      first = 0;
	      last = cc->num_colors - 1;
	      
	      while (first <= last)
		{
		  half = (first + last) / 2;
		  half_pixel = cc->cmap[half].pixel;
		  
		  if (tc->pixel == half_pixel)
		    {
		      tc->red   = cc->cmap[half].red;
		      tc->green = cc->cmap[half].green;
		      tc->blue  = cc->cmap[half].blue;
		      first = last + 1; /* false break */
		    }
		  else
		    {
		      if (tc->pixel > half_pixel)
			first = half + 1;
		      else
			last = half - 1;
		    }
		}
	    }
	  return 1;
	}
      break;
    }
  return 1;
}


gint
gdk_color_context_add_palette (GdkColorContext *cc,
			       GdkColor        *palette,
			       gint             num_palette)
{
	/* we don't allow to add palette */
	PRINTFILE("gdk_color_context_add_palette()");
	return 0;
}

void
gdk_color_context_init_dither (GdkColorContext *cc)
{
  gint rr, gg, bb, err, erg, erb;
  gint success = FALSE;

  g_assert (cc != NULL);

	PRINTFILE("gdk_color_context_init_dither()");
  /* now we can initialize the fast dither matrix */

  if (cc->fast_dither == NULL)
    cc->fast_dither = g_new (GdkColorContextDither, 1);

  /* Fill it.  We ignore unsuccessful allocations, they are just mapped
   * to black instead */

  for (rr = 0; rr < 32; rr++)
    for (gg = 0; gg < 32; gg++)
      for (bb = 0; bb < 32; bb++)
        {
          err = (rr << 3) | (rr >> 2);
          erg = (gg << 3) | (gg >> 2);
          erb = (bb << 3) | (bb >> 2);

          cc->fast_dither->fast_rgb[rr][gg][bb] =
            gdk_color_context_get_index_from_palette (cc, &err, &erg, &erb, &success);
          cc->fast_dither->fast_err[rr][gg][bb] = err;
          cc->fast_dither->fast_erg[rr][gg][bb] = erg;
          cc->fast_dither->fast_erb[rr][gg][bb] = erb;
        }
}

void
gdk_color_context_free_dither (GdkColorContext *cc)
{
  g_assert (cc != NULL);

  if (cc->fast_dither)
    g_free (cc->fast_dither);

  cc->fast_dither = NULL;

}

gulong
gdk_color_context_get_pixel_from_palette (GdkColorContext *cc,
					  gushort         *red,
					  gushort         *green,
					  gushort         *blue,
					  gint            *failed)
{
  gulong pixel = 0;
  gint dif, dr, dg, db, j = -1;
  gint mindif = 0x7fffffff;
  gint err = 0, erg = 0, erb = 0;
  gint i;

  g_assert (cc != NULL);
  g_assert (red != NULL);
  g_assert (green != NULL);
  g_assert (blue != NULL);
  g_assert (failed != NULL);

	PRINTFILE("gdk_color_context_get_pixel_from_palette()");
  *failed = FALSE;

  for (i = 0; i < cc->num_palette; i++)
    {
      dr = *red - cc->palette[i].red;
      dg = *green - cc->palette[i].green;
      db = *blue - cc->palette[i].blue;

      dif = dr * dr + dg * dg + db * db;
 
      if (dif < mindif)
        {
          mindif = dif;
          j = i;
          pixel = cc->palette[i].pixel;
          err = dr;
          erg = dg;
          erb = db;

          if (mindif == 0)
            break;
        }
    }

  /* we failed to map onto a color */

  if (j == -1)
    *failed = TRUE;
  else
    {
      *red   = ABS (err);
      *green = ABS (erg);
      *blue  = ABS (erb);
    }

  return pixel;
}

guchar
gdk_color_context_get_index_from_palette (GdkColorContext *cc,
					  gint            *red,
					  gint            *green,
					  gint            *blue,
					  gint            *failed)
{
  gint dif, dr, dg, db, j = -1;
  gint mindif = 0x7fffffff;
  gint err = 0, erg = 0, erb = 0;
  gint i;

  g_assert (cc != NULL);
  g_assert (red != NULL);
  g_assert (green != NULL);
  g_assert (blue != NULL);
  g_assert (failed != NULL);

	PRINTFILE("gdk_color_context_get_index_from_palette()");
  *failed = FALSE;

  for (i = 0; i < cc->num_palette; i++)
    {
      dr = *red - cc->palette[i].red;
      dg = *green - cc->palette[i].green;
      db = *blue - cc->palette[i].blue;

      dif = dr * dr + dg * dg + db * db;

      if (dif < mindif)
        {
          mindif = dif;
          j = i;
          err = dr;
          erg = dg;
          erb = db;

          if (mindif == 0)
            break;
        }
    }

  /* we failed to map onto a color */

  if (j == -1)
    {
      *failed = TRUE;
      j = 0;
    }
  else
    {
      /* return error fractions */

      *red   = err;
      *green = erg;
      *blue  = erb;
    }

  return j;
}
