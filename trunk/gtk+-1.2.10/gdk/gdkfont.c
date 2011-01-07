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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdk.h"
#include "gdkx.h"
#include "gdkprivate.h"

static GHashTable *font_name_hash = NULL;
static GHashTable *fontset_name_hash = NULL;

#define PRINTFILE(s) g_message("gdkfont.c:%s()", s)
#define GDK_FONT_XFONT(font)          (((GdkFontPrivate*) font)->fid)

void
_gdk_font_destroy (GdkFont *font)
{
  gi_delete_ufont(GDK_FONT_XFONT(font));
}

static void
gdk_font_hash_insert (GdkFontType type, GdkFont *font, const gchar *font_name)
{
  GdkFontPrivate *private = (GdkFontPrivate *)font;
  GHashTable **hashp = (type == GDK_FONT_FONT) ?
    &font_name_hash : &fontset_name_hash;

  if (!*hashp)
    *hashp = g_hash_table_new (g_str_hash, g_str_equal);

  private->names = g_slist_prepend (private->names, g_strdup (font_name));
  g_hash_table_insert (*hashp, private->names->data, font);
}

static void
gdk_font_hash_remove (GdkFontType type, GdkFont *font)
{
  GdkFontPrivate *private = (GdkFontPrivate *)font;
  GSList *tmp_list;
  GHashTable *hash = (type == GDK_FONT_FONT) ?
    font_name_hash : fontset_name_hash;

  tmp_list = private->names;
  while (tmp_list)
    {
      g_hash_table_remove (hash, tmp_list->data);
      g_free (tmp_list->data);

      tmp_list = tmp_list->next;
    }

  g_slist_free (private->names);
  private->names = NULL;
}

static GdkFont *
gdk_font_hash_lookup (GdkFontType type, const gchar *font_name)
{
  GdkFont *result;
  GHashTable *hash = (type == GDK_FONT_FONT) ?
    font_name_hash : fontset_name_hash;

  if (!hash)
    return NULL;
  else
    {
      result = g_hash_table_lookup (hash, font_name);
      if (result)
	gdk_font_ref (result);
      
      return result;
    }
}

#define FONT_DELIMITERS	"-"
#define MAX_TOKEN	14
#define FONT_WEIGHT	3
#define FONT_SLANT	4
#define FONT_PIXEL	7
#define FONT_POINT	8
#define FONT_FAMILY	2
#define FONT_CHARSET	13
/* font is foundry-Family-weight-slant-set width--Add Style-Pixel Size-
 * point size -Resolution x-Resolution U - Spacing - averagewidth -
 * charset registruy- charset encoding
*/
/* Name :_getfontsize
 * Desription: This function will only return 2 size i.e 10 & 12
 *      font size greater than 12 is treat as size 12 .
 *      font size less than 10 is treat as size 10.
 */
static gint
_getfontsize(const gchar **XLFS)
{
	gint fontsize;
	fontsize = atoi(XLFS[FONT_PIXEL]);
	if(!fontsize)
	{
		fontsize = atoi(XLFS[FONT_POINT]);
		if(strlen(XLFS[FONT_POINT]) > 2)
			fontsize = fontsize/10;
	}
	return fontsize;
}

/* Name: _getFontFamily
 * Description: This funtion will only handle 2 font weight i.e bold or normal.
 *      & font family is Helvetica either size 10 BOLD, size 12 BOLR or normal.
 *      & size 12 regular with famil is San Serif.
 */
static char*
_getFontFamily(const gchar **XLFS,gint * fontsize)
{
	if(!g_strcasecmp(XLFS[FONT_WEIGHT], "bold"))
	{

		if(*fontsize >= 12)
			return "gbk";
		return "gbk";
	}
	else
	if(!g_strcasecmp(XLFS[FONT_SLANT], "i"))
	{

		return  "gbk";
	}

	else
	if(!g_strcasecmp(XLFS[FONT_WEIGHT], "normal"))
	{
		if(*fontsize <= 10)
			return  "gbk";
	}
	return "gbk";
}

/* This function will always create a new font*/
GdkFont*
gdk_font_load (const gchar *font_name)
{
	GdkFont *font;
  	GdkFontPrivate *private;
  	gi_ufont_t fontID;
  	GR_FONT_INFO *fi;
  	gchar *fontname="";
  	gchar **XLFS;
	gint fontsize=12;
	char namebuf[512];

	printf("do load font name %s\n", font_name);

  	g_return_val_if_fail (font_name != NULL, NULL);

  	font = gdk_font_hash_lookup (GDK_FONT_FONT, font_name);
  	if(font)
		return (font);
  	XLFS = g_strsplit(font_name, FONT_DELIMITERS, 0);
  	if(XLFS)
	{
		fontsize = _getfontsize(XLFS);
		fontname = _getFontFamily((const gchar**)XLFS, &fontsize);
	}
  	else
          	fontname = "gbk";

	sprintf(namebuf,"%s/%d",fontname, fontsize);

  	/* FIXME : Modify to support various font */
  	fontID = gi_create_ufont((char*)namebuf);
	fi = g_malloc0(sizeof(*fi));
	if (fi == NULL) {
		g_warning("Memory problem on gdk_font_load\n");
		return -1;
	}
	fi->height = gi_ufont_ascent_get(fontID) + gi_ufont_descent_get(fontID);
	fi->baseline = 0;
/*  font = gdk_font_lookup(fontID); // Lookup the font using XID. 
    if (font != NULL)
	{

      private = (GdkFontPrivate *)fip;
      if (fip != private->xfont)
	_gdk_font_destroy (font);
      gdk_font_ref (font);
    }
  else*/
    	{
		private = g_new (GdkFontPrivate, 1);
		private->xfont = fi;
		private->fid = fontID;
		private->ref_count = 1;
		private->names = NULL;

	
		font = (GdkFont*) private;
		font->type = GDK_FONT_FONT;
        	private->font_name = fontname;
		font->ascent =  fi->baseline;
		font->descent = fi->height - fi->baseline;
		gdk_xid_table_insert (&private->fid, font);
    	}
  	gdk_font_hash_insert(GDK_FONT_FONT, font, font_name);
  	g_strfreev(XLFS);
  	return font;
}

GdkFont*
gdk_fontset_load (const gchar *fontset_name)
{
	GdkFont *fontset;
	fontset = gdk_font_load(fontset_name);
	fontset->type = GDK_FONT_FONTSET;
	return fontset;
}

GdkFont*
gdk_font_ref (GdkFont *font)
{
  GdkFontPrivate *private;

  g_return_val_if_fail (font != NULL, NULL);

  private = (GdkFontPrivate*) font;
  private->ref_count += 1;
  return font;
}

void
gdk_font_unref (GdkFont *font)
{
  GdkFontPrivate *private;
  private = (GdkFontPrivate*) font;

  g_return_if_fail (font != NULL);
  g_return_if_fail (private->ref_count > 0);

  private->ref_count -= 1;
  if (private->ref_count == 0) {
    gdk_font_hash_remove (font->type, font);
    gdk_xid_table_remove (private->fid);
    g_free (private->xfont);
    _gdk_font_destroy (font);
  }

}

gint
gdk_font_id (const GdkFont *font)
{
  const GdkFontPrivate *private;

  g_return_val_if_fail (font != NULL, 0);

  private = (const GdkFontPrivate*) font;

  if (font->type == GDK_FONT_FONT)
    {
	return private->fid;
    }
  else
    {
      return -1;
    }
}

gboolean
gdk_font_equal (const GdkFont *fonta,
                const GdkFont *fontb)
{
  const GdkFontPrivate *privatea;
  const GdkFontPrivate *privateb;

  g_return_val_if_fail (fonta != NULL, FALSE);
  g_return_val_if_fail (fontb != NULL, FALSE);

  privatea = (const GdkFontPrivate*) fonta;
  privateb = (const GdkFontPrivate*) fontb;

  if (fonta->type == GDK_FONT_FONT && fontb->type == GDK_FONT_FONT)
    {
	return ((privatea->fid)==(privateb->fid));
    }
/*  else if (fonta->type == GDK_FONT_FONTSET && fontb->type == GDK_FONT_FONTSET)
    {
      gchar *namea=NULL, *nameb=NULL;

      //namea = XBaseFontNameListOfFontSet((XFontSet) privatea->xfont);
      //nameb = XBaseFontNameListOfFontSet((XFontSet) privateb->xfont);
      
      return (strcmp(namea, nameb) == 0);
    }*/
  else
    /* fontset != font */
    return FALSE;
}

gint
gdk_string_width (GdkFont     *font, /* String is NULL terminated */
		  const gchar *string)
{
  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (string != NULL, -1);

  return gdk_text_width (font, string, strlen(string));

}

gint
gdk_text_width (GdkFont      *font,
		const gchar  *text,	/* Text is not NULL termibated*/
		gint          text_length)
{
	gint 	width=0;
	gint height, base;
  	static 	guchar 	* cached_char_widths=NULL;
	static 	gint cached_fid=0;
	static	gint cached_width=0; 
	const GdkFontPrivate *private = (GdkFontPrivate *)font;
	const GR_FONT_INFO *fontinfo = private->xfont;

  	g_return_val_if_fail (font != NULL, -1);
  	g_return_val_if_fail (text!= NULL, -1);



	gi_ufont_param(GDK_FONT_XFONT(font),text, text_length, &width, &height);

  	return width;
}

gint
gdk_text_width_wc (GdkFont	  *font,
		   const GdkWChar *text,
		   gint		   text_length)
{
	gint width = 0;
#ifndef CACHE_WIDTH
	gint height, base;
#endif
	gint i=0;
	GdkWChar *str;

	g_return_val_if_fail (font != NULL, -1);
	g_return_val_if_fail (text != NULL, -1);

	str = g_new(GdkWChar, text_length+1);
	if(str)
	{
		for(i=0; i < text_length; i++)
		str[i] = text[i];
		str[i] = 0;
		{
			gchar* mstr;
			
			mstr = gdk_wcstombs(str);

			gi_ufont_param(GDK_FONT_XFONT(font),mstr, strlen(mstr), &width, &height);
			g_free(mstr);
		}
		g_free(str);
	}
	return width;
}

guchar *
gdk_width_array(const GdkFont *font)
{
#if CACHE_WIDTH
	const GR_FONT_INFO *fi = (*(GdkFontPrivate *)font).xfont;

	if (!fi->fixed)
		return fi->widths;
	else
#endif
		return NULL;
}

/* Problem: What if a character is a 16 bits character ?? */
gint
gdk_char_width (GdkFont *font,
		gchar    character)
{
	g_return_val_if_fail (font != NULL, -1);
  	return gdk_text_width (font, &character, 1);
}

gint
gdk_char_width_wc (GdkFont *font,
		   GdkWChar character)
{
	g_return_val_if_fail (font != NULL, -1);
  	return gdk_text_width_wc (font, &character, 1);

}

gint
gdk_string_measure (GdkFont     *font,
                    const gchar *string)
{
	g_return_val_if_fail (font != NULL, -1);
	g_return_val_if_fail (string != NULL, -1);

  	return gdk_text_measure (font, string, strlen(string));

}

void
gdk_text_extents (GdkFont     *font,
                  const gchar *text,
                  gint         text_length,
		  gint        *lbearing,
		  gint        *rbearing,
		  gint        *width,
		  gint        *ascent,
		  gint        *descent)
{
  gint mwidth, height, base;

  gi_ufont_param(GDK_FONT_XFONT(font),(gchar*)text, text_length, &mwidth, &height);
  if (width)
        *width = mwidth;
  if (lbearing)
        *lbearing = 0;
  if (rbearing)
        *rbearing = mwidth;
  if (ascent)
        *ascent = base;
  if (descent)
        *descent = height - base;


}

void
gdk_text_extents_wc (GdkFont        *font,
		     const GdkWChar *text,
		     gint            text_length,
		     gint           *lbearing,
		     gint           *rbearing,
		     gint           *width,
		     gint           *ascent,
		     gint           *descent)
{
  gint mwidth, height, base;

  gi_ufont_param(GDK_FONT_XFONT(font),(gchar*)text, text_length, &mwidth, &height);

  if (width)
        *width = mwidth;
  if (lbearing)
        *lbearing = 0;
  if (rbearing)
        *rbearing = mwidth;
  if (ascent)
        *ascent = base;
  if (descent)
        *descent = height - base;

}

void
gdk_string_extents (GdkFont     *font,
		    const gchar *string,
		    gint        *lbearing,
		    gint        *rbearing,
		    gint        *width,
		    gint        *ascent,
		    gint        *descent)
{
  g_return_if_fail (font != NULL);
  g_return_if_fail (string != NULL);
 gdk_text_extents (font, string, strlen (string),
                    lbearing, rbearing, width, ascent, descent);

}


gint
gdk_text_measure (GdkFont     *font,
                  const gchar *text,
                  gint         text_length)
{
 gint rbearing;

  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (text != NULL, -1);

  gdk_text_extents (font, text, text_length, NULL, &rbearing, NULL, NULL, NULL);
  return rbearing;

}

gint
gdk_char_measure (GdkFont *font,
                  gchar    character)
{
  g_return_val_if_fail (font != NULL, -1);
  return gdk_text_measure (font, &character, 1);
}

gint
gdk_string_height (GdkFont     *font,
		   const gchar *string)
{
	PRINTFILE("gdk_string_height");
  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (string != NULL, -1);

  return gdk_text_height (font, string, strlen (string));
}

gint
gdk_text_height (GdkFont     *font,
		 const gchar *text,
		 gint         text_length)
{
  gint width, height, base;
  g_return_val_if_fail (font != NULL, -1);
  g_return_val_if_fail (text!= NULL, -1);

  return font->ascent+font->descent;
}

gint
gdk_char_height (GdkFont *font,
		 gchar    character)
{
  g_return_val_if_fail (font != NULL, -1);
  return gdk_text_height (font, &character, 1);
}
