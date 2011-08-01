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
 * Copyright (C) 2011 www.hanxuantech.com.
 * Written by Easion <easion@hanxuantech.com> , it's based
 * on DirectFB port.
 */
#include "config.h"

#include <string.h>

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkproperty.h"
#include "gdkselection.h"
#include "gdkprivate.h"
#include "gdkalias.h"


typedef struct _OwnerInfo OwnerInfo;

struct _OwnerInfo
{
  GdkAtom    selection;
  GdkWindow *owner;
  gulong     serial;
};

GSList *owner_list = NULL;

#if 1



#define N_CUSTOM_PREDEFINED 1


GdkWindow *
_gdk_gix_display_get_selection_owner (GdkDisplay *display,
					  GdkAtom     selection)
{
  gi_window_id_t xwindow;

  if (display->closed)
    return NULL;

  xwindow = gi_get_selection_owner (gdk_x11_atom_to_xatom_for_display (display,
                                                                   selection));
  if (xwindow == 0)
    return NULL;

  return gdk_gix_window_id_table_lookup ( xwindow);

  //return NULL;
}

gboolean
_gdk_gix_display_set_selection_owner (GdkDisplay *display,
					  GdkWindow  *owner,
					  GdkAtom     selection,
					  guint32     time,
					  gboolean    send_event)
{
  //Display *xdisplay;
  gi_window_id_t xwindow;
  gi_atom_id_t xselection;
  GSList *tmp_list;
  OwnerInfo *info;

  if (display->closed)
    return FALSE;

  if (owner)
    {
      if (GDK_WINDOW_DESTROYED (owner) || !GDK_IS_WINDOW_IMPL_GIX (owner))
        return FALSE;

      //gdk_window_ensure_native (owner);
      //xdisplay = GDK_WINDOW_XDISPLAY (owner);
      xwindow = GDK_WINDOW_GIX_ID (owner);
    }
  else
    {
      //xdisplay = GDK_DISPLAY_XDISPLAY (display);
      xwindow = 0;
    }

  xselection = gdk_x11_atom_to_xatom_for_display (display, selection);

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection)
        {
          owner_list = g_slist_remove (owner_list, info);
          g_free (info);
          break;
        }
      tmp_list = tmp_list->next;
    }

  if (owner)
    {
      info = g_new (OwnerInfo, 1);
      info->owner = owner;
      info->serial = 0;//NextRequest (GDK_WINDOW_XDISPLAY (owner));
      info->selection = selection;

      owner_list = g_slist_prepend (owner_list, info);
    }

  gi_set_selection_owner ( xselection, xwindow, time);

  return (gi_get_selection_owner ( xselection) == xwindow);
}

void
_gdk_gix_display_send_selection_notify (GdkDisplay *dispay,
					    GdkWindow        *requestor,
					    GdkAtom          selection,
					    GdkAtom          target,
					    GdkAtom          property,
					    guint32          time)
{
/*
  XSelectionEvent xevent;

  xevent.type = SelectionNotify;
  xevent.serial = 0;
  xevent.send_event = True;
  xevent.requestor = GDK_WINDOW_GIX_ID (requestor);
  xevent.selection = gdk_x11_atom_to_xatom_for_display (display, selection);
  xevent.target = gdk_x11_atom_to_xatom_for_display (display, target);
  if (property == GDK_NONE)
    xevent.property = None;
  else
    xevent.property = gdk_x11_atom_to_xatom_for_display (display, property);
  xevent.time = time;

  _gdk_x11_display_send_xevent (display, xevent.requestor, False, NoEventMask, (XEvent*) & xevent);

*/
}

gint
_gdk_gix_display_get_selection_property (GdkDisplay  *display,
					     GdkWindow   *requestor,
					     guchar     **data,
					     GdkAtom     *ret_type,
					     gint        *ret_format)
{
  gulong nitems;
  gulong nbytes;
  gulong length = 0;
  gi_atom_id_t prop_type;
  gint prop_format;
  guchar *t = NULL;

  if (GDK_WINDOW_DESTROYED (requestor) || !GDK_IS_WINDOW_IMPL_GIX (requestor))
    goto err;

  t = NULL;

  /* We can't delete the selection here, because it might be the INCR
     protocol, in which case the client has to make sure they'll be
     notified of PropertyChange events _before_ the property is deleted.
     Otherwise there's no guarantee we'll win the race ... */
  if (gi_get_window_property (
                          GDK_WINDOW_GIX_ID (requestor),
                          gdk_x11_get_xatom_by_name_for_display (display, "GDK_SELECTION"),
                          0, 0x1FFFFFFF /* MAXINT32 / 4 */, FALSE,
                          G_ANY_PROP_TYPE, &prop_type, &prop_format,
                          &nitems, &nbytes, &t) != 0)
    goto err;

  if (prop_type != 0)
    {
      if (ret_type)
        *ret_type = gdk_x11_xatom_to_atom_for_display (display, prop_type);
      if (ret_format)
        *ret_format = prop_format;

      if (prop_type == GA_ATOM ||
          prop_type == gdk_x11_get_xatom_by_name_for_display (display, "ATOM_PAIR"))
        {
          gi_atom_id_t* atoms = (gi_atom_id_t*) t;
          GdkAtom* atoms_dest;
          gint num_atom, i;

          if (prop_format != 32)
            goto err;

          num_atom = nitems;
          length = sizeof (GdkAtom) * num_atom + 1;

          if (data)
            {
              *data = g_malloc (length);
              (*data)[length - 1] = '\0';
              atoms_dest = (GdkAtom *)(*data);

              for (i=0; i < num_atom; i++)
                atoms_dest[i] = gdk_x11_xatom_to_atom_for_display (display, atoms[i]);
            }
        }
      else
        {
          switch (prop_format)
            {
            case 8:
              length = nitems;
              break;
            case 16:
              length = sizeof(short) * nitems;
              break;
            case 32:
              length = sizeof(long) * nitems;
              break;
            default:
              g_assert_not_reached ();
              break;
            }

          /* Add on an extra byte to handle null termination.  X guarantees
             that t will be 1 longer than nitems and null terminated */
          length += 1;

          if (data)
            *data = g_memdup (t, length);
        }

      if (t)
        free (t);

      return length - 1;
    }

 err:
  if (ret_type)
    *ret_type = GDK_NONE;
  if (ret_format)
    *ret_format = 0;
  if (data)
    *data = NULL;

  return 0;
}

void
_gdk_gix_display_convert_selection (GdkDisplay *display,
					GdkWindow  *requestor,
					GdkAtom     selection,
					GdkAtom     target,
					guint32     time)
{
  g_return_if_fail (selection != GDK_NONE);

  if (GDK_WINDOW_DESTROYED (requestor) || !GDK_IS_WINDOW_IMPL_GIX (requestor))
    return;

  //gdk_window_ensure_native (requestor);

  gi_convert_selection (
                     gdk_x11_atom_to_xatom_for_display (display, selection),
                     gdk_x11_atom_to_xatom_for_display (display, target),
                     gdk_x11_get_xatom_by_name_for_display (display, "GDK_SELECTION"),
                     GDK_WINDOW_GIX_ID (requestor), time);
}

gint
_gdk_gix_display_text_property_to_utf8_list (GdkDisplay    *display,
						 GdkAtom        encoding,
						 gint           format,
						 const guchar  *text,
						 gint           length,
						 gchar       ***list)
{
  return 0;
}

static gchar *
sanitize_utf8 (const gchar *src,
               gboolean return_latin1)
{
  gint len = strlen (src);
  GString *result = g_string_sized_new (len);
  const gchar *p = src;

  while (*p)
    {
      if (*p == '\r')
        {
          p++;
          if (*p == '\n')
            p++;

          g_string_append_c (result, '\n');
        }
      else
        {
          gunichar ch = g_utf8_get_char (p);

          if (!((ch < 0x20 && ch != '\t' && ch != '\n') || (ch >= 0x7f && ch < 0xa0)))
            {
              if (return_latin1)
                {
                  if (ch <= 0xff)
                    g_string_append_c (result, ch);
                  else
                    g_string_append_printf (result,
                                            ch < 0x10000 ? "\\u%04x" : "\\U%08x",
                                            ch);
                }
              else
                {
                  char buf[7];
                  gint buflen;

                  buflen = g_unichar_to_utf8 (ch, buf);
                  g_string_append_len (result, buf, buflen);
                }
            }

          p = g_utf8_next_char (p);
        }
    }

  return g_string_free (result, FALSE);
}

gchar *
_gdk_gix_display_utf8_to_string_target (GdkDisplay  *display,
					    const gchar *str)
{
  //return NULL;
  return sanitize_utf8 (str, TRUE);
}

#endif

/* When a window is destroyed we check if it is the owner
 * of any selections. This is somewhat inefficient, but
 * owner_list is typically short, and it is a low memory,
 * low code solution
 */
void
_gdk_selection_window_destroyed (GdkWindow *window)
{
  GSList *tmp_list = owner_list;

  while (tmp_list)
    {
      OwnerInfo *info = tmp_list->data;

      tmp_list = tmp_list->next;

      if (info->owner == window)
	{
	  owner_list = g_slist_remove (owner_list, info);
	  g_free (info);
	}
    }
}

gint
gdk_selection_owner_set_for_display (GdkDisplay *display,
                                     GdkWindow  *owner,
                                     GdkAtom     selection,
                                     guint32     time,
                                     gint        send_event)
{
  GSList    *tmp_list;
  OwnerInfo *info;

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection)
	{
	  owner_list = g_slist_remove (owner_list, info);
	  g_free (info);
	  break;
	}
      tmp_list = tmp_list->next;
    }

  if (owner)
    {
      info = g_new (OwnerInfo, 1);
      info->owner = owner;
      info->selection = selection;

      owner_list = g_slist_prepend (owner_list, info);
    }

  return TRUE;
}

GdkWindow *
gdk_selection_owner_get_for_display (GdkDisplay *display,
                                     GdkAtom     selection)
{
  OwnerInfo *info;
  GSList    *tmp_list;

  tmp_list = owner_list;
  while (tmp_list)
    {
      info = tmp_list->data;
      if (info->selection == selection)
	{
	  return info->owner;
	}
      tmp_list = tmp_list->next;
    }
  return NULL;
}

void
gdk_selection_convert (GdkWindow *requestor,
		       GdkAtom    selection,
		       GdkAtom    target,
		       guint32    time)
{
  GdkEvent  *event;
  GdkWindow *owner;
  GdkWindow *event_window;

  owner = gdk_selection_owner_get (selection);

  if (owner)
    {
      event_window = gdk_gix_other_event_window (owner,
                                                      GDK_SELECTION_REQUEST);
      if (event_window)
	{
	  event = gdk_gix_event_make (event_window,
                                           GDK_SELECTION_REQUEST);
	  event->selection.requestor = GDK_WINDOW_GIX_ID (requestor);
	  event->selection.selection = selection;
	  event->selection.target    = target;
	  event->selection.property  = _gdk_selection_property;
	}
    }
  else
    {
      /* If no owner for the specified selection exists, the X server
       * generates a SelectionNotify event to the requestor with property None.
       */
      gdk_selection_send_notify (GDK_WINDOW_GIX_ID (requestor),
				 selection,
				 target,
				 GDK_NONE,
				 0);
    }
}

gint
gdk_selection_property_get (GdkWindow  *requestor,
			    guchar    **data,
			    GdkAtom    *ret_type,
			    gint       *ret_format)
{
  guchar *t = NULL;
  GdkAtom prop_type;
  gint prop_format;
  gint prop_len;

  g_return_val_if_fail (requestor != NULL, 0);
  g_return_val_if_fail (GDK_IS_WINDOW (requestor), 0);

  if (!gdk_property_get (requestor,
			 _gdk_selection_property,
			 0/*AnyPropertyType?*/,
			 0, 0,
			 FALSE,
			 &prop_type, &prop_format, &prop_len,
			 &t))
    {
      *data = NULL;
      return 0;
    }

  if (ret_type)
    *ret_type = prop_type;
  if (ret_format)
    *ret_format = prop_format;

  if (!gdk_property_get (requestor,
			 _gdk_selection_property,
			 0/*AnyPropertyType?*/,
			 0, prop_len + 1,
			 FALSE,
			 &prop_type, &prop_format, &prop_len,
			 &t))
    {
      *data = NULL;
      return 0;
    }

  *data = t;

  return prop_len;
}


void
gdk_selection_send_notify_for_display (GdkDisplay *display,
                                       guint32     requestor,
                                       GdkAtom     selection,
                                       GdkAtom     target,
                                       GdkAtom     property,
                                       guint32     time)
{
  GdkEvent  *event;
  GdkWindow *event_window;

  event_window = gdk_window_lookup ((GdkNativeWindow) requestor);

  if (!event_window)
    return;

  event_window = gdk_gix_other_event_window (event_window,
                                                  GDK_SELECTION_NOTIFY);

  if (event_window)
    {
      event = gdk_gix_event_make (event_window, GDK_SELECTION_NOTIFY);
      event->selection.selection = selection;
      event->selection.target = target;
      event->selection.property = property;
      event->selection.requestor = (GdkNativeWindow) requestor;
    }
}

gint
gdk_text_property_to_text_list_for_display (GdkDisplay      *display,
                                            GdkAtom          encoding,
                                            gint             format,
                                            const guchar    *text,
                                            gint             length,
                                            gchar         ***list)
{
  g_warning ("gdk_text_property_to_text_list() not implemented\n");
  return 0;
}

void
gdk_free_text_list (gchar **list)
{
  g_return_if_fail (list != NULL);
  g_warning ("gdk_free_text_list() not implemented\n");
}

gint
gdk_string_to_compound_text_for_display (GdkDisplay   *display,
                                         const gchar  *str,
                                         GdkAtom      *encoding,
                                         gint         *format,
                                         guchar      **ctext,
                                         gint         *length)
{
  g_warning ("gdk_string_to_compound_text() not implemented\n");
  return 0;
}

void
gdk_free_compound_text (guchar *ctext)
{
  g_warning ("gdk_free_compound_text() not implemented\n");
}

/**
 * gdk_utf8_to_string_target:
 * @str: a UTF-8 string
 *
 * Convert an UTF-8 string into the best possible representation
 * as a STRING. The representation of characters not in STRING
 * is not specified; it may be as pseudo-escape sequences
 * \x{ABCD}, or it may be in some other form of approximation.
 *
 * Return value: the newly allocated string, or %NULL if the
 *               conversion failed. (It should not fail for
 *               any properly formed UTF-8 string.)
 **/
gchar *
gdk_utf8_to_string_target (const gchar *str)
{
  g_warning ("gdk_utf8_to_string_target() not implemented\n");
  return 0;
}

/**
 * gdk_utf8_to_compound_text:
 * @str:      a UTF-8 string
 * @encoding: location to store resulting encoding
 * @format:   location to store format of the result
 * @ctext:    location to store the data of the result
 * @length:   location to store the length of the data
 *            stored in @ctext
 *
 * Convert from UTF-8 to compound text.
 *
 * Return value: %TRUE if the conversion succeeded, otherwise
 *               false.
 **/
gboolean
gdk_utf8_to_compound_text_for_display (GdkDisplay   *display,
                                       const gchar  *str,
                                       GdkAtom      *encoding,
                                       gint         *format,
                                       guchar      **ctext,
                                       gint         *length)
{
  g_warning ("gdk_utf8_to_compound_text() not implemented\n");
  return 0;
}

static gint
make_list (const gchar  *text,
	   gint          length,
	   gboolean      latin1,
	   gchar      ***list)
{
  GSList *strings = NULL;
  gint n_strings = 0;
  gint i;
  const gchar *p = text;
  const gchar *q;
  GSList *tmp_list;
  GError *error = NULL;

  while (p < text + length)
    {
      gchar *str;

      q = p;
      while (*q && q < text + length)
	q++;

      if (latin1)
	{
	  str = g_convert (p, q - p,
			   "UTF-8", "ISO-8859-1",
			   NULL, NULL, &error);

	  if (!str)
	    {
	      g_warning ("Error converting selection from STRING: %s",
			 error->message);
	      g_error_free (error);
	    }
	}
      else
	str = g_strndup (p, q - p);

      if (str)
	{
	  strings = g_slist_prepend (strings, str);
	  n_strings++;
	}

      p = q + 1;
    }

  if (list)
    *list = g_new (gchar *, n_strings + 1);

  (*list)[n_strings] = NULL;

  i = n_strings;
  tmp_list = strings;
  while (tmp_list)
    {
      if (list)
	(*list)[--i] = tmp_list->data;
      else
	g_free (tmp_list->data);

      tmp_list = tmp_list->next;
    }

  g_slist_free (strings);

  return n_strings;
}


/**
 * gdk_text_property_to_utf8_list:
 * @encoding: an atom representing the encoding of the text
 * @format:   the format of the property
 * @text:     the text to convert
 * @length:   the length of @text, in bytes
 * @list:     location to store the list of strings or %NULL. The
 *            list should be freed with g_strfreev().
 *
 * Convert a text property in the giving encoding to
 * a list of UTF-8 strings.
 *
 * Return value: the number of strings in the resulting
 *               list.
 **/
gint
gdk_text_property_to_utf8_list_for_display (GdkDisplay     *display,
                                            GdkAtom         encoding,
                                            gint            format,
                                            const guchar   *text,
                                            gint            length,
                                            gchar        ***list)
{
  g_return_val_if_fail (text != NULL, 0);
  g_return_val_if_fail (length >= 0, 0);

  if (encoding == GDK_TARGET_STRING)
    {
      return make_list ((gchar *)text, length, TRUE, list);
    }
  else if (encoding == gdk_atom_intern ("UTF8_STRING", FALSE))
    {
      return make_list ((gchar *)text, length, FALSE, list);
    }
  else
    {
      gchar **local_list;
      gint local_count;
      gint i;
      const gchar *charset = NULL;
      gboolean need_conversion = !g_get_charset (&charset);
      gint count = 0;
      GError *error = NULL;

      /* Probably COMPOUND text, we fall back to Xlib routines
       */
      local_count = gdk_text_property_to_text_list (encoding,
						    format,
						    text,
						    length,
						    &local_list);
      if (list)
	*list = g_new (gchar *, local_count + 1);

      for (i = 0; i < local_count; i++)
	{
	  /* list contains stuff in our default encoding
	   */
	  if (need_conversion)
	    {
	      gchar *utf = g_convert (local_list[i], -1,
				      "UTF-8", charset,
				      NULL, NULL, &error);
	      if (utf)
		{
		  if (list)
		    (*list)[count++] = utf;
		  else
		    g_free (utf);
		}
	      else
		{
		  g_warning ("Error converting to UTF-8 from '%s': %s",
			     charset, error->message);
		  g_error_free (error);
		  error = NULL;
		}
	    }
	  else
	    {
	      if (list)
		(*list)[count++] = g_strdup (local_list[i]);
	    }
	}

      gdk_free_text_list (local_list);
      (*list)[count] = NULL;

      return count;
    }
}

#define __GDK_SELECTION_X11_C__
#include "gdkaliasdef.c"
