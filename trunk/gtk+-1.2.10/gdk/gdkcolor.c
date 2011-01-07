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

#include <time.h>
#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

#include "gdk.h"
#include "gdkprivate.h"
#include "gdkx.h"

static gint  gdk_colormap_match_color (GdkColormap *cmap,
				       GdkColor    *color,
				       const gchar *available);
static void  gdk_colormap_add         (GdkColormap *cmap);
static void  gdk_colormap_remove      (GdkColormap *cmap);
static guint gdk_colormap_hash        (gi_color_t    *cmap);
static gint  gdk_colormap_cmp         (gi_color_t    *a,
				       gi_color_t    *b);
static void gdk_colormap_real_destroy (GdkColormap *colormap);

static GHashTable *colormap_hash = NULL;
static gint displaycell = 0;
extern gi_screen_info_t gdk_screen_info;

#define PRINTFILE(s) g_message("gdkcolor.c:%s()",s) 

GdkColormap*
gdk_colormap_new (GdkVisual *visual,
		  gboolean   private_cmap)
{
 /* FIXME: used system colormap */
	return  gdk_colormap_get_system();
}

static void
gdk_colormap_real_destroy (GdkColormap *colormap)
{
  GdkColormapPrivate *private = (GdkColormapPrivate*) colormap;

  g_return_if_fail (colormap != NULL);
  g_return_if_fail (private->ref_count == 0);

  gdk_colormap_remove (colormap);

  if (private->hash)
    g_hash_table_destroy (private->hash);

  g_free (private->info);
  g_free (colormap->colors);
  g_free (colormap);

}

GdkColormap*
gdk_colormap_ref (GdkColormap *cmap)
{
  GdkColormapPrivate *private = (GdkColormapPrivate *)cmap;

  g_return_val_if_fail (cmap != NULL, NULL);

  private->ref_count += 1;
  return cmap;
}

void
gdk_colormap_unref (GdkColormap *cmap)
{
  GdkColormapPrivate *private = (GdkColormapPrivate *)cmap;

  g_return_if_fail (cmap != NULL);
  g_return_if_fail (private->ref_count > 0);

  private->ref_count -= 1;
  if (private->ref_count == 0)
    gdk_colormap_real_destroy (cmap);
}

GdkVisual *
gdk_colormap_get_visual (GdkColormap *colormap)
{
  GdkColormapPrivate *private;

  g_return_val_if_fail (colormap != NULL, NULL);

  private = (GdkColormapPrivate *)colormap;

  return private->visual;
}
     
#define MIN_SYNC_TIME 2

void
gdk_colormap_sync (GdkColormap *colormap,
		   gboolean     force) // needed function
{
  time_t current_time;
  GdkColormapPrivate *private = (GdkColormapPrivate *)colormap;
  
  g_return_if_fail (colormap != NULL);

  current_time = time (NULL);
  if (!force && ((current_time - private->last_sync_time) < MIN_SYNC_TIME))
    return;

	return ; // no sync function
#if 0
  private->last_sync_time = current_time;

  nlookup = 0;
  xpalette = g_new (XColor, colormap->size);
  
  for (i = 0; i < colormap->size; i++)
    {
      if (private->info[i].ref_count == 0)
	{
	  xpalette[nlookup].pixel = i;
	  xpalette[nlookup].red = 0;
	  xpalette[nlookup].green = 0;
	  xpalette[nlookup].blue = 0;
	  nlookup++;
	}
    }
  
  XQueryColors (gdk_display, private->xcolormap, xpalette, nlookup);
  
  for (i = 0; i < nlookup; i++)
    {
      gulong pixel = xpalette[i].pixel;
      colormap->colors[pixel].pixel = pixel;
      colormap->colors[pixel].red = xpalette[i].red;
      colormap->colors[pixel].green = xpalette[i].green;
      colormap->colors[pixel].blue = xpalette[i].blue;
    }
  
  g_free (xpalette);
#endif
}
		   

GdkColormap*
gdk_colormap_get_system (void)
{
  static GdkColormap *colormap = NULL;
  GdkColormapPrivate *private;
  int i;

  if (!colormap)
    {
      private = g_new (GdkColormapPrivate, 1);
      colormap = (GdkColormap*) private;

     // private->xdisplay = gdk_display;
     // private->xcolormap = DefaultColormap (gdk_display, gdk_screen);
      private->visual = gdk_visual_get_system ();
      private->private_val = FALSE;
      private->ref_count = 1;
      colormap->colors = NULL;
      colormap->size = private->visual->colormap_size;
      colormap->size = 32;
#if 1
	  colormap->colors = g_new (GdkColor, colormap->size);
	for (i=0; i<colormap->size;i++)
	{
		colormap->colors[i].red=i*7;
		colormap->colors[i].green=7*i;
		colormap->colors[i].blue=7*i;
		colormap->colors[i].pixel=i;
	}
#endif

      private->hash = NULL;
      private->last_sync_time = 0;
      private->info = NULL;



      if ((private->visual->type == GDK_VISUAL_GRAYSCALE) ||
	  (private->visual->type == GDK_VISUAL_PSEUDO_COLOR))
	{
	  private->info = g_new0 (GdkColorInfo, colormap->size);
	  colormap->colors = g_new (GdkColor, colormap->size);
	  
	  private->hash = g_hash_table_new ((GHashFunc) gdk_color_hash,
					    (GCompareFunc) gdk_color_equal);

	  gdk_colormap_sync (colormap, TRUE);
	}

      gdk_colormap_add (colormap);
    }

  return colormap;
}



gint
gdk_colormap_get_system_size (void)
{
	g_message("gdk_colormap_get_system_size");
	return displaycell;
}

void
gdk_colormap_change (GdkColormap *colormap,
		     gint         ncolors)
{
	g_message("__Unsupport__ gdk_colormap_change()\n");
/* Since no private colormap in PDA, not needed function
 * Change the value of the first ncolors in a private 
 *colormap to match the values in the colors array in the colormap
 */
}

void
gdk_colors_store (GdkColormap   *colormap,
		  GdkColor      *colors,
		  gint           ncolors)
{
	g_message("__Unsupport__ gdk_colors_store()");
/* Change the value of the first ncolors colors in a 
 * private colormap
 */
}

gboolean
gdk_colors_alloc (GdkColormap   *colormap,
		  gboolean       contiguous,
		  gulong        *planes,
		  gint           nplanes,
		  gulong        *pixels,
		  gint           npixels)
{
	g_message("__Unsupport__ gdk_colors_alloc()");
	return FALSE;
}

/*
 *--------------------------------------------------------------
 * gdk_color_copy
 *
 *   Copy a color structure into new storage.
 *
 * Arguments:
 *   "color" is the color struct to copy.
 *
 * Results:
 *   A new color structure.  Free it with gdk_color_free.
 *
 *--------------------------------------------------------------
 */

static GMemChunk *color_chunk;

GdkColor*
gdk_color_copy (const GdkColor *color)
{
  GdkColor *new_color;

  g_return_val_if_fail (color != NULL, NULL);

  if (color_chunk == NULL)
    color_chunk = g_mem_chunk_new ("colors",
                                   sizeof (GdkColor),
                                   4096,
                                   G_ALLOC_AND_FREE);

  new_color = g_chunk_new (GdkColor, color_chunk);
  *new_color = *color;
  return new_color;

}

/*
 *--------------------------------------------------------------
 * gdk_color_free
 *
 *   Free a color structure obtained from gdk_color_copy.  Do not use
 *   with other color structures.
 *
 * Arguments:
 *   "color" is the color struct to free.
 *
 *-------------------------------------------------------------- */

void
gdk_color_free (GdkColor *color)
{
  g_assert (color_chunk != NULL);
  g_return_if_fail (color != NULL);

  g_mem_chunk_free (color_chunk, color);
}


int gdk_color_find(guchar r, guchar g, guchar b)
{
        int             R, G, B;
        long            diff = 0x7fffffffL;
        long            sq;
        int             best = 0;
#if 1 //dpp
        //MWPALENTRY *    rgb;
	GdkColormap 	*palette;
	int i;

	palette = gdk_colormap_get_system();
        for(i = 0; diff && i < palette->size; ++i) {
                R = palette->colors[i].red - r;
                G = palette->colors[i].green - g;
                B = palette->colors[i].blue - b;
                /* speedy linear distance method*/
                sq = abs(R) + abs(G) + abs(B);
                if(sq < diff) {
                        best = i;
                        if((diff = sq) == 0)
                                return best;
                }
        }
#endif
        return best;

}

gboolean
gdk_color_white (GdkColormap *colormap,
		 GdkColor    *color)
{
  gint return_val;

  g_return_val_if_fail (colormap != NULL, FALSE);

  if (color)
    {
	
      color->pixel = gi_color_to_pixel(&gdk_screen_info,255,255,255);//GrFindColor(WHITE,&(color->pixel)); // this should be replace by FindNearestcolor
      color->red = 65535;
      color->green = 65535;
      color->blue = 65535;

      return_val = gdk_color_alloc (colormap, color);
    }
  else
    return_val = FALSE;

  return return_val;
}

gboolean
gdk_color_black (GdkColormap *colormap,
		 GdkColor    *color)// needed function
{
  gint return_val;

  g_return_val_if_fail (colormap != NULL, FALSE);

  if (color)
    {	
	
      color->pixel = gi_color_to_pixel(&gdk_screen_info,0, 0, 0);//GrFindColor(BLACK,&(color->pixel));
      color->red = 0;
      color->green = 0;
      color->blue = 0;

      return_val = gdk_color_alloc (colormap, color);
    }
  else
    return_val = FALSE;

  return return_val;
}

/* BEGIN NanoGTK changes
 *
 * gdk_color_parse rewritten by andy@emsoftltd.com
 * Instead of reading in RGB.txt on each call, load the color data into memory.
 */

typedef struct _ColorEntry
{
	gchar *name;
	guchar red;
	guchar green;
	guchar blue;
}
ColorEntry;

GSList *named_colors = NULL; /* Linked list of cached ColorEntries. */

/* Copy ColorEntry to GdkColor.
 */
static void
color_entry_to_gdk_color(ColorEntry *entry, GdkColor *color)
{
   	color->red = (entry->red > 0) ? (entry->red << 8) | 0xff : 0;
   	color->green = (entry->green > 0) ? (entry->green << 8) | 0xff : 0;
   	color->blue = (entry->blue > 0) ? (entry->blue << 8) | 0xff : 0;
}




/* Make a copy of the lookup string, making it upper case, removing spaces and
 * changing "GREY" to "GRAY".
 */
static gchar *
copy_string_for_lookup(const gchar *name)
{
	const gchar *source;
	gchar *target;
	gint length = 0;
	gchar *copy = NULL;
	gchar *pGREY;

	/* Find the length of buffer required if we ignore spaces:
	 */
	for (source = name; *source != '\0'; source++)
	{
		if (*source != ' ')
			length++;
	}

	copy = g_new(gchar, length + 1);

	if (!copy)
		return NULL;

	/* Copy the string, skipping spaces and making it upper case.
	 */
	for (source = name, target = copy; *source != '\0'; source++)
	{
		if (*source != ' ')
			*target++ = toupper(*source);
	}

	*target = '\0';

	/* Convert "GREY" substring to "GRAY":
	 */
	pGREY = strstr(copy, "GREY");

	if (pGREY)
		pGREY[2] = 'A';

	return copy;
}



gboolean
gdk_color_parse (const gchar *spec,
		 GdkColor *color)
{
  //Colormap xcolormap;
  gi_color_t xcolor;
  gboolean return_val;

  g_return_val_if_fail (spec != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  //xcolormap = DefaultColormap (gdk_display, gdk_screen);

  if (gi_parse_color (  spec, &xcolor))
    {
      return_val = TRUE;
      color->red = GI_RED(xcolor);
      color->green = GI_GREEN(xcolor);
      color->blue = GI_BLUE(xcolor);
    }
  else
    return_val = FALSE;

  return return_val;
}

/* END NanoGTK changes
 */


/* This is almost identical to gdk_colormap_free_colors.
 * Keep them in sync!
 */
void
gdk_colors_free (GdkColormap *colormap,
		 gulong      *in_pixels,
		 gint         in_npixels,
		 gulong       planes)
{
  GdkColormapPrivate *private;
  gulong *pixels;
  gint npixels = 0;
  gint i;

  g_return_if_fail (colormap != NULL);
  g_return_if_fail (in_pixels != NULL);

  private = (GdkColormapPrivate*) colormap;

  if ((private->visual->type != GDK_VISUAL_PSEUDO_COLOR) &&
      (private->visual->type != GDK_VISUAL_GRAYSCALE))
    return;

  pixels = g_new (gulong, in_npixels);

  for (i=0; i<in_npixels; i++)
    {
      gulong pixel = in_pixels[i];

      if (private->info[pixel].ref_count)
        {
          private->info[pixel].ref_count--;

          if (private->info[pixel].ref_count == 0)
            {
              pixels[npixels++] = pixel;
              if (!(private->info[pixel].flags & GDK_COLOR_WRITEABLE))
                g_hash_table_remove (private->hash, &colormap->colors[pixel]);
              private->info[pixel].flags = 0;
            }
        }
    }

  g_free (pixels);

}

/* This is almost identical to gdk_colors_free.
 * Keep them in sync!
 */
void
gdk_colormap_free_colors (GdkColormap *colormap,
			  GdkColor    *colors,
			  gint         ncolors)
{
  GdkColormapPrivate *private;
  gulong *pixels;
  gint npixels = 0;
  gint i;

  g_return_if_fail (colormap != NULL);
  g_return_if_fail (colors != NULL);

  private = (GdkColormapPrivate*) colormap;

  if ((private->visual->type != GDK_VISUAL_PSEUDO_COLOR) &&
      (private->visual->type != GDK_VISUAL_GRAYSCALE))
    return;

  pixels = g_new (gulong, ncolors);

  for (i=0; i<ncolors; i++)
    {
      gulong pixel = colors[i].pixel;

      if (private->info[pixel].ref_count)
        {
          private->info[pixel].ref_count--;

          if (private->info[pixel].ref_count == 0)
            {
              pixels[npixels++] = pixel;
              if (!(private->info[pixel].flags & GDK_COLOR_WRITEABLE))
                g_hash_table_remove (private->hash, &colormap->colors[pixel]);
              private->info[pixel].flags = 0;
            }
        }
    }

  g_free (pixels);
}

/********************
 * Color allocation *
 ********************/

/* Try to allocate a single color using XAllocColor. If it succeeds,
 * cache the result in our colormap, and store in ret.
 */
static gboolean 
gdk_colormap_alloc1 (GdkColormap *colormap,
		     GdkColor    *color,
		     GdkColor    *ret)
{
/*
  GdkColormapPrivate *private;
  XColor xcolor;

  private = (GdkColormapPrivate*) colormap;

  xcolor.red = color->red;
  xcolor.green = color->green;
  xcolor.blue = color->blue;
  xcolor.pixel = color->pixel;
  xcolor.flags = DoRed | DoGreen | DoBlue;

  if (XAllocColor (private->xdisplay, private->xcolormap, &xcolor))
    {
      ret->pixel = xcolor.pixel;
      ret->red = xcolor.red;
      ret->green = xcolor.green;
      ret->blue = xcolor.blue;
      
      if (ret->pixel < colormap->size)
	{
	  if (private->info[ret->pixel].ref_count) 
	    {
	      XFreeColors (private->xdisplay, private->xcolormap,
			   &ret->pixel, 1, 0);
	    }
	  else
	    {
	      colormap->colors[ret->pixel] = *color;
	      colormap->colors[ret->pixel].pixel = ret->pixel;
	      private->info[ret->pixel].ref_count = 1;

	      g_hash_table_insert (private->hash,
				   &colormap->colors[ret->pixel],
				   &colormap->colors[ret->pixel]);
	    }
	}
      return TRUE;
    }
  else
    {
*/
      return FALSE;
   // }
}

static gint
gdk_colormap_alloc_colors_writeable (GdkColormap *colormap,
				     GdkColor    *colors,
				     gint         ncolors,
				     gboolean     writeable,
				     gboolean     best_match,
				     gboolean    *success)
{
	g_message("gdk_colormap_alloc_colors_writeable()");
	return 0;
}

static gint
gdk_colormap_alloc_colors_private (GdkColormap *colormap,
				   GdkColor    *colors,
				   gint         ncolors,
				   gboolean     writeable,
				   gboolean     best_match,
				   gboolean    *success)
{
#if 0
  GdkColormapPrivate *private;
  gint i, index;
  GR_PALETTE *store = g_new (GR_PALETTE, 1);
  gint nstore = 0;
  gint nremaining = 0;
 
  private = (GdkColormapPrivate*) colormap;
  index = -1;

  /* First, store the colors we have room for */

	printf("Enter gdk_colormap_alloc_colors_private\n");
  index = 0;
  for (i=0; i<ncolors; i++)
    {
      if (!success[i])
        {
          while ((index < colormap->size) && (private->info[index].ref_count != 0))
            index++;

          if (index < colormap->size)
            {
              store->palette[nstore].r = colors[i].red;
              store->palette[nstore].b = colors[i].blue;
              store->palette[nstore].g = colors[i].green;
              nstore++;

              success[i] = TRUE;

              colors[i].pixel = index;
              private->info[index].ref_count++;
            }
          else
            nremaining++;
        }
    }
  store->count = nstore;
  GrSetSystemPalette(0, store);
  g_free (store);

  if (nremaining > 0 && best_match)
    {
      /* Get best matches for remaining colors */

      gchar *available = g_new (gchar, colormap->size);
      for (i = 0; i < colormap->size; i++)
        available[i] = TRUE;

      for (i=0; i<ncolors; i++)
        {
          if (!success[i])
            {
              index = gdk_colormap_match_color (colormap,
                                                &colors[i],
                                                available);
              if (index != -1)
                {
                  colors[i] = colormap->colors[index];
                  private->info[index].ref_count++;

                  success[i] = TRUE;
                  nremaining--;
                }
            }
        }
      g_free (available);
    }

  return (ncolors - nremaining);
#endif
  g_message("__Unsupport__ gdk_colormap_alloc_colors_private()\n");
  return 0;

}

static gint
gdk_colormap_alloc_colors_shared (GdkColormap *colormap,
				  GdkColor    *colors,
				  gint         ncolors,
				  gboolean     writeable,
				  gboolean     best_match,
				  gboolean    *success)
{
  GdkColormapPrivate *private;
  gint i, index;
  gint nremaining = 0;
  gint nfailed = 0;

  private = (GdkColormapPrivate*) colormap;
  index = -1;

  for (i=0; i<ncolors; i++)
    {
      if (!success[i])
	{
	  if (gdk_colormap_alloc1 (colormap, &colors[i], &colors[i]))
	    success[i] = TRUE;
	  else
	    nremaining++;
	}
    }


  if (nremaining > 0 && best_match)
    {
      gchar *available = g_new (gchar, colormap->size);
      for (i = 0; i < colormap->size; i++)
	available[i] = ((private->info[i].ref_count == 0) ||
			!(private->info[i].flags && GDK_COLOR_WRITEABLE));
      gdk_colormap_sync (colormap, FALSE);
      
      while (nremaining > 0)
	{
	  for (i=0; i<ncolors; i++)
	    {
	      if (!success[i])
		{
		  index = gdk_colormap_match_color (colormap, &colors[i], available);
		  if (index != -1)
		    {
		      if (private->info[index].ref_count)
			{
			  private->info[index].ref_count++;
			  colors[i] = colormap->colors[index];
			  success[i] = TRUE;
			  nremaining--;
			}
		      else
			{
			  if (gdk_colormap_alloc1 (colormap, 
						   &colormap->colors[index],
						   &colors[i]))
			    {
			      success[i] = TRUE;
			      nremaining--;
			      break;
			    }
			  else
			    {
			      available[index] = FALSE;
			    }
			}
		    }
		  else
		    {
		      nfailed++;
		      nremaining--;
		      success[i] = 2; /* flag as permanent failure */
		    }
		}
	    }
	}
      g_free (available);
    }

  /* Change back the values we flagged as permanent failures */
  if (nfailed > 0)
    {
      for (i=0; i<ncolors; i++)
	if (success[i] == 2)
	  success[i] = FALSE;
      nremaining = nfailed;
    }
  
  return (ncolors - nremaining);
}

static gint
gdk_colormap_alloc_colors_pseudocolor (GdkColormap *colormap,
				       GdkColor    *colors,
				       gint         ncolors,
				       gboolean     writeable,
				       gboolean     best_match,
				       gboolean    *success)
{
  GdkColormapPrivate *private;
  GdkColor *lookup_color;
  gint i;
  gint nremaining = 0;

  private = (GdkColormapPrivate*) colormap;

  /* Check for an exact match among previously allocated colors */

  for (i=0; i<ncolors; i++)
    {
      if (!success[i])
	{
	  lookup_color = g_hash_table_lookup (private->hash, &colors[i]);
	  if (lookup_color)
	    {
	      private->info[lookup_color->pixel].ref_count++;
	      colors[i].pixel = lookup_color->pixel;
	      success[i] = TRUE;
	    }
	  else
	    nremaining++;
	}
    }

  /* If that failed, we try to allocate a new color, or approxmiate
   * with what we can get if best_match is TRUE.
   */
  if (nremaining > 0)
    {
      if (private->private_val)
	return gdk_colormap_alloc_colors_private (colormap, colors, ncolors, writeable, best_match, success);
      else
	return gdk_colormap_alloc_colors_shared (colormap, colors, ncolors, writeable, best_match, success);
    }
  else
    return 0;
}

gint
gdk_colormap_alloc_colors (GdkColormap *colormap,
			   GdkColor    *colors,
			   gint         ncolors,
			   gboolean     writeable,
			   gboolean     best_match,
			   gboolean    *success)
{
  GdkColormapPrivate *private;
  GdkVisual *visual;
  gi_pixel_value_t pixel;
  gint i, shift;
  gint nremaining = 0;

  g_return_val_if_fail (colormap != NULL, FALSE);
  g_return_val_if_fail (colors != NULL, FALSE);

  private = (GdkColormapPrivate*) colormap;

  for (i=0; i<ncolors; i++)
    {
      success[i] = FALSE;
    }

  switch (private->visual->type)
    {
    case GDK_VISUAL_PSEUDO_COLOR:
    case GDK_VISUAL_GRAYSCALE:
      if (writeable)
	return gdk_colormap_alloc_colors_writeable (colormap, colors, ncolors,
						    writeable, best_match, success);
      else
	return gdk_colormap_alloc_colors_pseudocolor (colormap, colors, ncolors,
						    writeable, best_match, success);
      break;

    case GDK_VISUAL_DIRECT_COLOR:
    case GDK_VISUAL_TRUE_COLOR:
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_STATIC_GRAY:
      visual = private->visual;
      for (i=0; i<ncolors; i++)
        {
	 /*colors[i].pixel = (((colors[i].red >> (16 - visual->red_prec)) << visual->red_shift) +
                             ((colors[i].green >> (16 - visual->green_prec)) << visual->green_shift) +
                             ((colors[i].blue >> (16 - visual->blue_prec)) << visual->blue_shift));*/
	colors[i].pixel = gi_color_to_pixel(&gdk_screen_info,colors[i].red >> 8, colors[i].green >> 8,
			colors[i].blue >> 8);
	  success[i] = TRUE;
        }
	break;
#if 0
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_STATIC_GRAY:
	shift = 9- private->visual->depth;
	for(i=0; i < ncolors; i++)
	  {
		GrFindColor(GI_RGB(colors[i].red>>8 , colors[i].green>>8 ,
                        colors[i].blue>>8), &pixel);
/*		pixel = (colors[i].red + 
		((colors[i].blue + colors[i].green) >> 1)) >> shift;*/
		colors[i].pixel = pixel;
		success[i] = TRUE;
	  }
	break;
#endif
    }
  return nremaining;
}

gboolean
gdk_colormap_alloc_color (GdkColormap *colormap,
			  GdkColor    *color,
			  gboolean     writeable,
			  gboolean     best_match)
{
  gboolean success;

  gdk_colormap_alloc_colors (colormap, color, 1, FALSE, TRUE, &success);

  return success;

}

gboolean
gdk_color_alloc (GdkColormap *colormap,
		 GdkColor    *color)
{
  gboolean success;

  gdk_colormap_alloc_colors (colormap, color, 1, FALSE, TRUE, &success);

  return success;
}

gboolean
gdk_color_change (GdkColormap *colormap,
		  GdkColor    *color)
{
#if 0
  GdkColormapPrivate *private;
  GR_PALETTE xcolor;

  g_return_val_if_fail (colormap != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  xcolor.pixel = color->pixel;
  xcolor.red = color->red;
  xcolor.green = color->green;
  xcolor.blue = color->blue;

  private = (GdkColormapPrivate*) colormap;
  XStoreColor (private->xdisplay, private->xcolormap, &xcolor);

  GrSetSystemPalette(, xcolor); 
#endif
  PRINTFILE("gdk_color_change");
  return TRUE;

}

guint
gdk_color_hash (const GdkColor *colora,
		const GdkColor *colorb)
{
  return ((colora->red) +
	  (colora->green << 11) +
	  (colora->blue << 22) +
	  (colora->blue >> 6));
}

gboolean
gdk_color_equal (const GdkColor *colora,
		 const GdkColor *colorb)
{
  g_return_val_if_fail (colora != NULL, FALSE);
  g_return_val_if_fail (colorb != NULL, FALSE);

  return ((colora->red == colorb->red) &&
	  (colora->green == colorb->green) &&
	  (colora->blue == colorb->blue));
}

#if 0
/* XXX: Do not use this function until it is fixed. An X Colormap
 *      is useless unless we also have the visual.
 */
GdkColormap*
gdkx_colormap_get (gi_color_t xcolormap) // needed function
{
  GdkColormap *colormap;
  GdkColormapPrivate *private;

  colormap = gdk_colormap_lookup (xcolormap);
  if (colormap)
    return colormap;

/*
  if (xcolormap == DefaultColormap (gdk_display, gdk_screen))
    return gdk_colormap_get_system ();
*/
  private = g_new (GdkColormapPrivate, 1);
  colormap = (GdkColormap*) private;

  private->xdisplay = gdk_display;
  private->xcolormap = xcolormap;
  private->visual = NULL;
  private->private_val = TRUE;

  /* To do the following safely, we would have to have some way of finding
   * out what the size or visual of the given colormap is. It seems
   * X doesn't allow this
   */

#if 0
  for (i = 0; i < 256; i++)
    {
      xpalette[i].pixel = i;
      xpalette[i].red = 0;
      xpalette[i].green = 0;
      xpalette[i].blue = 0;
    }

  XQueryColors (gdk_display, private->xcolormap, xpalette, 256);

  for (i = 0; i < 256; i++)
    {
      colormap->colors[i].pixel = xpalette[i].pixel;
      colormap->colors[i].red = xpalette[i].red;
      colormap->colors[i].green = xpalette[i].green;
      colormap->colors[i].blue = xpalette[i].blue;
    }
#endif

  colormap->colors = NULL;
  colormap->size = 0;

  gdk_colormap_add (colormap);

  return colormap;
}
#endif

static gint
gdk_colormap_match_color (GdkColormap *cmap,
			  GdkColor    *color,
			  const gchar *available)
{
  GdkColor *colors;
  guint sum, max;
  gint rdiff, gdiff, bdiff;
  gint i, index;

  g_return_val_if_fail (cmap != NULL, 0);
  g_return_val_if_fail (color != NULL, 0);

  colors = cmap->colors;
  max = 3 * (65536);
  index = -1;

  for (i = 0; i < cmap->size; i++)
    {
      if ((!available) || (available && available[i]))
	{
	  rdiff = (color->red - colors[i].red);
	  gdiff = (color->green - colors[i].green);
	  bdiff = (color->blue - colors[i].blue);

	  sum = ABS (rdiff) + ABS (gdiff) + ABS (bdiff);

	  if (sum < max)
	    {
	      index = i;
	      max = sum;
	    }
	}
    }

  return index;
}


GdkColormap*
gdk_colormap_lookup (gi_color_t xcolormap)
{
 GdkColormap *cmap;

  if (!colormap_hash)
    return NULL;

  cmap = g_hash_table_lookup (colormap_hash, &xcolormap);
  return cmap;

}

static void
gdk_colormap_add (GdkColormap *cmap) // needed function
{
  GdkColormapPrivate *private;

  if (!colormap_hash)
    colormap_hash = g_hash_table_new ((GHashFunc) gdk_colormap_hash,
				      (GCompareFunc) gdk_colormap_cmp);

  private = (GdkColormapPrivate*) cmap;

  g_hash_table_insert (colormap_hash, &private->xcolormap, cmap);
}

static void
gdk_colormap_remove (GdkColormap *cmap)
{
  GdkColormapPrivate *private;

  if (!colormap_hash)
    colormap_hash = g_hash_table_new ((GHashFunc) gdk_colormap_hash,
                                      (GCompareFunc) gdk_colormap_cmp);

  private = (GdkColormapPrivate*) cmap;

  g_hash_table_remove (colormap_hash, &private->xcolormap);

}

static guint
gdk_colormap_hash (gi_color_t *cmap) //needed function
{
  return *cmap;
}

static gint
gdk_colormap_cmp (gi_color_t *a,
		  gi_color_t *b)
{
  return (*a == *b);
}
