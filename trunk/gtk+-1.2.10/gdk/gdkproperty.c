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

#include <string.h>
#include "gdk.h"
#include "gdkprivate.h"

#define PRINTFILE(s) g_message("gdkproperty.c:%s()",s)

static guint
gdk_atom_id_hash (GdkAtom *xid)
{
  return *xid;
}

static gint
gdk_atom_id_compare (GdkAtom *a,
                 GdkAtom *b)
{
  return (*a == *b);
}

GdkAtom
gdk_atom_intern (const gchar *atom_name,
		 gint         only_if_exists)
{
  GdkAtom retval;
  static GHashTable *atom_hash = NULL;
  
  if (!atom_hash)
    atom_hash = g_hash_table_new (g_str_hash, g_str_equal);


  retval = GPOINTER_TO_UINT (g_hash_table_lookup (atom_hash, atom_name));
  if (!retval)
    {
	retval=	gi_intern_atom(atom_name, only_if_exists);
      if (retval != None)
	{
	g_hash_table_insert (atom_hash, 
			     g_strdup (atom_name), 
			     GUINT_TO_POINTER (retval));
	}
    }

  //g_message("Atom %s have id %d", atom_name, retval);
  return retval;
}

gchar*
gdk_atom_name (GdkAtom atom)
{
  gchar *name=NULL;

  name = gi_get_atom_name(atom);
  return name;
}

gboolean
gdk_property_get (GdkWindow   *window,
		  GdkAtom      property,
		  GdkAtom      type,
		  gulong       offset,
		  gulong       length,
		  gint         pdelete,
		  GdkAtom     *actual_property_type,
		  gint        *actual_format_type,
		  gint        *actual_length,
		  guchar     **data)
{
  gi_window_id_t xwindow;
  gi_atom_id_t ret_prop_type;
  gint ret_format;
  gulong ret_nitems;
  gulong ret_bytes_after;
  gulong ret_length;
  guchar *ret_data;

	PRINTFILE("gdk_property_get");
  if (window)
    {
      if (GDK_DRAWABLE_DESTROYED(window))
        return FALSE;

      xwindow = GDK_DRAWABLE_XID(window);
    }
  else
      xwindow = gdk_root_window;

  ret_data = NULL;
  gi_get_window_property(xwindow, property,  0L, 0x7fffffff, FALSE,GA_WINDOW,&ret_prop_type, &ret_format,
	&ret_nitems,NULL, &ret_data);
  if( (ret_prop_type == None) && (ret_format == 0))
	return FALSE;

  if (actual_property_type)
    *actual_property_type = ret_prop_type;
  if (actual_format_type)
    *actual_format_type = ret_format;

 if ((type != G_ANY_PROP_TYPE) && (ret_prop_type != type))
    {
      gchar *rn, *pn;

      rn = gdk_atom_name(ret_prop_type);
      pn = gdk_atom_name(type);
      g_warning("Couldn't match property type %s to %s\n", rn, pn);
      g_free(rn); g_free(pn);
      return FALSE;
    }

 if (data)
    {
      switch (ret_format)
        {
        case 8:
          ret_length = ret_nitems;
          break;
        case 16:
          ret_length = sizeof(short) * ret_nitems;
          break;
        case 32:
          ret_length = sizeof(long) * ret_nitems;
          break;
        default:
          g_warning ("unknown property return format: %d", ret_format);
          return FALSE;
        }

      *data = g_new (guchar, ret_length);
      memcpy (*data, ret_data, ret_length);
      if (actual_length)
        *actual_length = ret_length;
    }


  return TRUE;
}

void
gdk_property_change (GdkWindow   *window,
		     GdkAtom      property,
		     GdkAtom      type,
		     gint         format,
		     GdkPropMode  mode,
		     guchar      *data,
		     gint         nelements)
{
	gi_window_id_t xwindow;
	PRINTFILE("gdk_property_change");

	if(window)
	{
		if(GDK_DRAWABLE_DESTROYED(window)) return;
		xwindow = GDK_DRAWABLE_XID(window);
	}
	else
		xwindow = gdk_root_window;

	gi_change_property(xwindow, property, type, format,G_PROP_MODE_Replace, data,
		nelements);
}

void
gdk_property_delete (GdkWindow *window,
		     GdkAtom    property)
{
	gi_window_id_t xwindow;

	PRINTFILE("gdk_property_delete");

	if(window)
	{
		if(GDK_DRAWABLE_DESTROYED(window))
			return;

		xwindow = GDK_DRAWABLE_XID(window);
	}
	else
		xwindow = gdk_root_window;	
	
	
	gi_delete_property(xwindow, property);
	
}
