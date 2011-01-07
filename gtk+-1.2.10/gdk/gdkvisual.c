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
#include "gdkx.h"


#define PRINTFILE(s)	g_message("gdkvisual.c:%s()",s)
static void  gdk_visual_add            (GdkVisual *visual);
static void  gdk_visual_decompose_mask (gulong     mask,
					gint      *shift,
					gint      *prec);
static guint gdk_visual_hash           ();
static gint  gdk_visual_compare        ();


static GdkVisualPrivate *system_visual;
static GdkVisualPrivate *visuals;
extern gi_screen_info_t gdk_screen_info;
static gint nvisuals;

static gint available_depths[7];
static gint navailable_depths;

static GdkVisualType available_types[6];
static gint navailable_types;

#ifdef G_ENABLE_DEBUG

static const gchar* visual_names[] =
{
  "static gray",
  "grayscale",
  "static color",
  "pseudo color",
  "true color",
  "direct color",
};

#endif /* G_ENABLE_DEBUG */

static GHashTable *visual_hash = NULL;

void
gdk_visual_init (void)
{
  static const gint possible_depths[8] = { 32, 24, 16, 15, 12, 8, 4, 1 };
  static const GdkVisualType possible_types[6] =
    {
      GDK_VISUAL_DIRECT_COLOR, /*DirectColor */
      GDK_VISUAL_TRUE_COLOR,
      GDK_VISUAL_PSEUDO_COLOR,
      GDK_VISUAL_STATIC_COLOR, /* StaticColor*/
      GDK_VISUAL_GRAYSCALE,	/* R/W Gray Scale */
      GDK_VISUAL_STATIC_GRAY	/*StaticGray */
    };

  static const gint npossible_depths = sizeof(possible_depths)/sizeof(gint);
  static const gint npossible_types = sizeof(possible_types)/sizeof(GdkVisualType);

  GdkVisualPrivate temp_visual;
  int nxvisuals;
  int i, j;

  visuals = g_new (GdkVisualPrivate, 1);

  visuals[0].si=&gdk_screen_info; 	// Get the Default Visual i.e the screen info

  nvisuals = 0;
  nxvisuals = 1;
  for (i = 0; i < nxvisuals; i++)
    {
      if (GI_RENDER_FORMAT_BPP(gdk_screen_info.format) >= 1)
	{
	  switch (gdk_screen_info.format)
	    {
#if 0
	    case MWPF_PALETTE:
	      if(gdk_screen_info.usegraypalette)
                visuals[nvisuals].visual.type = GDK_VISUAL_STATIC_GRAY;
              else
                visuals[nvisuals].visual.type = GDK_VISUAL_STATIC_COLOR;
              break;


	    case MWPF_RGB:
	      visuals[nvisuals].visual.type = GDK_VISUAL_PSEUDO_COLOR;
	      break;
#endif
	    case GI_RENDER_x8r8g8b8:
	    case GI_RENDER_r8g8b8:
	      visuals[nvisuals].visual.type = GDK_VISUAL_TRUE_COLOR;
	      visuals[nvisuals].visual.red_mask = 0xFF0000;
	      visuals[nvisuals].visual.green_mask = 0x00FF00;
	      visuals[nvisuals].visual.blue_mask = 0x0000FF;
	      break;

	    case GI_RENDER_r5g6b5:
	      visuals[nvisuals].visual.type = GDK_VISUAL_TRUE_COLOR;
	      visuals[nvisuals].visual.red_mask = 0xF800;
	      visuals[nvisuals].visual.green_mask = 0x07E0;
	      visuals[nvisuals].visual.blue_mask = 0x001F;
	      break;


            case GI_RENDER_x4b4g4r4:
              visuals[nvisuals].visual.type = GDK_VISUAL_TRUE_COLOR;
              visuals[nvisuals].visual.red_mask = 0x0F00;
              visuals[nvisuals].visual.green_mask = 0x00F0;
              visuals[nvisuals].visual.blue_mask = 0x000F;
              break;

	    case GI_RENDER_r3g3b2:
	      visuals[nvisuals].visual.type = GDK_VISUAL_TRUE_COLOR;
	      visuals[nvisuals].visual.red_mask = 0x70;
	      visuals[nvisuals].visual.green_mask = 0x1C;
	      visuals[nvisuals].visual.blue_mask = 0x03;
	      break;

	    default :
	      visuals[nvisuals].visual.type = GDK_VISUAL_TRUE_COLOR;
	    }

	  visuals[nvisuals].visual.depth = GI_RENDER_FORMAT_BPP(gdk_screen_info.format);
	  visuals[nvisuals].visual.byte_order = GDK_LSB_FIRST;

/*
	  visuals[nvisuals].visual.colormap_size = visual_list[i].colormap_size;
	  visuals[nvisuals].visual.bits_per_rgb = visual_list[i].bits_per_rgb;
	  visuals[nvisuals].xvisual = visual_list[i].visual;
*/

	  if ((visuals[nvisuals].visual.type == GDK_VISUAL_TRUE_COLOR) ||
	      (visuals[nvisuals].visual.type == GDK_VISUAL_DIRECT_COLOR))
	    {
	      gdk_visual_decompose_mask (visuals[nvisuals].visual.red_mask,
					 &visuals[nvisuals].visual.red_shift,
					 &visuals[nvisuals].visual.red_prec);

	      gdk_visual_decompose_mask (visuals[nvisuals].visual.green_mask,
					 &visuals[nvisuals].visual.green_shift,
					 &visuals[nvisuals].visual.green_prec);

	      gdk_visual_decompose_mask (visuals[nvisuals].visual.blue_mask,
					 &visuals[nvisuals].visual.blue_shift,
					 &visuals[nvisuals].visual.blue_prec);
	    }
	  else
	    {
	      visuals[nvisuals].visual.red_mask = 0;
	      visuals[nvisuals].visual.red_shift = 0;
	      visuals[nvisuals].visual.red_prec = 0;

	      visuals[nvisuals].visual.green_mask = 0;
	      visuals[nvisuals].visual.green_shift = 0;
	      visuals[nvisuals].visual.green_prec = 0;

	      visuals[nvisuals].visual.blue_mask = 0;
	      visuals[nvisuals].visual.blue_shift = 0;
	      visuals[nvisuals].visual.blue_prec = 0;
	    }


	  nvisuals += 1;
	}
    }

  for (i = 0; i < nvisuals; i++)
    {
      for (j = i+1; j < nvisuals; j++)
	{
	  if (visuals[j].visual.depth >= visuals[i].visual.depth)
	    {
	      if ((visuals[j].visual.depth == 8) && (visuals[i].visual.depth == 8))
		{
		  if (visuals[j].visual.type == GDK_VISUAL_PSEUDO_COLOR)
		    {
		      temp_visual = visuals[j];
		      visuals[j] = visuals[i];
		      visuals[i] = temp_visual;
		    }
		  else if ((visuals[i].visual.type != GDK_VISUAL_PSEUDO_COLOR) &&
			   visuals[j].visual.type > visuals[i].visual.type)
		    {
		      temp_visual = visuals[j];
		      visuals[j] = visuals[i];
		      visuals[i] = temp_visual;
		    }
		}
	      else if ((visuals[j].visual.depth > visuals[i].visual.depth) ||
		       ((visuals[j].visual.depth == visuals[i].visual.depth) &&
			(visuals[j].visual.type > visuals[i].visual.type)))
		{
		  temp_visual = visuals[j];
		  visuals[j] = visuals[i];
		  visuals[i] = temp_visual;
		}
	    }
	}
    }

    system_visual = &visuals[0]; //  there's only one visual available 

#ifdef G_ENABLE_DEBUG 
  if (gdk_debug_flags & GDK_DEBUG_MISC)
    for (i = 0; i < nvisuals; i++)
      g_message ("visual: %s: %d",
		 visual_names[visuals[i].visual.type],
		 visuals[i].visual.depth);
#endif /* G_ENABLE_DEBUG */

  navailable_depths = 0;
  for (i = 0; i < npossible_depths; i++)
    {
      for (j = 0; j < nvisuals; j++)
	{
	  if (visuals[j].visual.depth == possible_depths[i])
	    {
	      available_depths[navailable_depths++] = visuals[j].visual.depth;
	      break;
	    }
	}
    }

  if (navailable_depths == 0)
    g_error ("unable to find a usable depth");

  navailable_types = 0;
  for (i = 0; i < npossible_types; i++)
    {
      for (j = 0; j < nvisuals; j++)
	{
	  if (visuals[j].visual.type == possible_types[i])
	    {
	      available_types[navailable_types++] = visuals[j].visual.type;
	      break;
	    }
	}
    }

  for (i = 0; i < nvisuals; i++)
    gdk_visual_add ((GdkVisual*) &visuals[i]);

  if (npossible_types == 0)
    g_error ("unable to find a usable visual type");
}

GdkVisual*
gdk_visual_ref (GdkVisual *visual)
{
	return visual;
}

void
gdk_visual_unref (GdkVisual *visual)
{
	return;
}

gint
gdk_visual_get_best_depth (void)
{
	return available_depths[0];
}

GdkVisualType
gdk_visual_get_best_type (void)
{
	return available_types[0];
}

GdkVisual*
gdk_visual_get_system (void)
{
  return ((GdkVisual*) system_visual);
}

GdkVisual*
gdk_visual_get_best (void)
{
	return ((GdkVisual*) &(visuals[0]));
}

GdkVisual*
gdk_visual_get_best_with_depth (gint depth)
{
  GdkVisual *return_val;
  int i;

  PRINTFILE("__IMPLEMENTED__ gdk_visual_get_best_with_depth");
#if 0
  return_val = NULL;
  for (i = 0; i < nvisuals; i++)
    if (depth == visuals[i].visual.depth)
      {
        return_val = (GdkVisual*) &(visuals[i]);
        break;
      }
#else
        return_val = (GdkVisual*) &(visuals[0]);
#endif
  return return_val;

}

GdkVisual*
gdk_visual_get_best_with_type (GdkVisualType visual_type)
{
  GdkVisual *return_val;
  int i;

	PRINTFILE("__IMPLEMENTED__  gdk_visual_get_best_with_type");
#if 0
  return_val = NULL;
  for (i = 0; i < nvisuals; i++)
    if (visual_type == visuals[i].visual.type)
      {
        return_val = (GdkVisual*) &(visuals[i]);
        break;
      }
#else

        return_val = (GdkVisual*) &(visuals[0]);
#endif
  return return_val;

}

GdkVisual*
gdk_visual_get_best_with_both (gint          depth,
			       GdkVisualType visual_type)
{
  GdkVisual *return_val;
  int i;
#if 0
  return_val = NULL;
  for (i = 0; i < nvisuals; i++)
    if ((depth == visuals[i].visual.depth) &&
        (visual_type == visuals[i].visual.type))
      {
        return_val = (GdkVisual*) &(visuals[i]);
        break;
      }
#else
        return_val = (GdkVisual*) &(visuals[0]);
#endif
  return return_val;

}

void
gdk_query_depths  (gint **depths,
		   gint  *count)
{
  *count = navailable_depths;
  *depths = available_depths;

}

void
gdk_query_visual_types (GdkVisualType **visual_types,
			gint           *count)
{
  *count = navailable_types;
  *visual_types = available_types;
}

GList*
gdk_list_visuals (void)
{
  GList *list;
  guint i;

  list = NULL;
  for (i = 0; i < nvisuals; ++i)
    list = g_list_append (list, (gpointer) &visuals[i]);

  return list;

}


GdkVisual*
gdk_visual_lookup ()
{
	PRINTFILE("gdk_visual_lookup"); // not used by any known widget
     	return &system_visual; //  there's only one visual available 
}

GdkVisual*
gdkx_visual_get ()
{
	PRINTFILE("gdkx_visual_get"); // not used by any known widget
     	return &system_visual; //  there's only one visual available 
}


static void
gdk_visual_add (GdkVisual *visual)
{
  GdkVisualPrivate *private;

  if (!visual_hash)
    visual_hash = g_hash_table_new ((GHashFunc) gdk_visual_hash,
				    (GCompareFunc) gdk_visual_compare);

  private = (GdkVisualPrivate*) visual;

  g_hash_table_insert (visual_hash, private->si, visual);
}

static void
gdk_visual_decompose_mask (gulong  mask,
			   gint   *shift,
			   gint   *prec)
{
  *shift = 0;
  *prec = 0;

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

static guint
gdk_visual_hash ()
{
	return 0;
  //return key->visualid; FIXME !!!
}

static gint
gdk_visual_compare(GdkVisual *a,
		   GdkVisual *b)
{
	/* Only one default visual is available, so always equal*/
	PRINTFILE("gdk_visual_compare"); //FIXME: r there any different visual
					
	return 1;
}
