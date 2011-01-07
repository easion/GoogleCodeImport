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
#include "gdkx.h"

#define PRINTFILE(s) g_message("gdkselection.c:%s()",s)
gboolean
gdk_selection_owner_set (GdkWindow *owner,
			 GdkAtom    selection,
			 guint32    time,
			 gint       send_event)
{
   gi_window_id_t	xwindow;
   gchar* atom_name;
   PRINTFILE("gdk_selection_owner_set");
    if (owner)
    {
      GdkWindowPrivate *private;

      private = (GdkWindowPrivate*) owner;
      if (GDK_DRAWABLE_DESTROYED(owner))
        return FALSE;

      xwindow = GDK_DRAWABLE_XID(owner);
    }
  else
      xwindow = None;

  g_message("Set Selection Owner is %d for %d", xwindow, selection);
  gi_set_selection_owner(xwindow, selection, time);

  return (xwindow == gi_get_selection_owner(selection));
}

GdkWindow*
gdk_selection_owner_get (GdkAtom selection)
{
	gi_window_id_t xwindow;
	
	if(selection==None)
		return NULL;

	xwindow = gi_get_selection_owner(selection);

	g_message("Get Selection Owner is %d for %d", xwindow, selection);
	if(xwindow==None)
		return NULL;
	return gdk_window_lookup(xwindow);
}

void
gdk_selection_convert (GdkWindow *requestor,
		       GdkAtom    selection,
		       GdkAtom    target,
		       guint32    time)
{
  g_return_if_fail (requestor != NULL);

	PRINTFILE("gdk_selection_convert");
  if (GDK_DRAWABLE_DESTROYED(requestor))
    return;

  gi_convert_selection(selection, target, gdk_selection_property, 
		GDK_DRAWABLE_XID(requestor), time);
	gdk_flush();
}

gint
gdk_selection_property_get (GdkWindow  *requestor,
			    guchar    **data,
			    GdkAtom    *ret_type,
			    gint       *ret_format)
{
	gulong	nitems_return;
	gulong length;
	guchar *t=NULL;
	GdkAtom prop_type;
 	gint prop_format;
	unsigned long  extra;

	PRINTFILE("gdk_selection_property_get");
	if(GDK_DRAWABLE_DESTROYED(requestor))
		return 0;
	gi_get_window_property (GDK_DRAWABLE_XID(requestor), gdk_selection_property,  0L, 0x7fffffff, FALSE,GA_WINDOW,
			&prop_type, &prop_format, &nitems_return,&extra, &t);
 if (ret_type)
    *ret_type = prop_type;
  if (ret_format)
    *ret_format = prop_format;

	 length = nitems_return + 1;
  if (prop_type == None)
    {
      *data = NULL;
      return 0;
    }

  if (prop_type != None)
    {
      *data = g_new (guchar, length);
      memcpy (*data, t, length);
      return length-1;
    }
  else
    {
      *data = NULL;
      return 0;
    }

}


void
gdk_selection_send_notify (guint32  requestor,
			   GdkAtom  selection,
			   GdkAtom  target,
			   GdkAtom  property,
			   guint32  time)
{
	gi_msg_t s;

  s.type = GI_MSG_SELECTIONNOTIFY;
  s.params[ 0 ] = G_SEL_NOTIFY;
  s.params[ 2 ] = requestor;
  s.params[ 3 ] = selection;
  s.params[ 4 ]    = target;
  s.params[ 5 ]  = property;   //This means refusal
  s.params[ 1 ]      = time; 
  gi_send_event( requestor, TRUE, 0, &s);

	PRINTFILE("gdk_SELECTION_send_NOTIFY");
	//event.type = GR_EVENT_TYPE_SELECTION_NOTIFY;
	//event.owner = GR_NONE;
	//event.requestor = requestor;
	//event.selection = selection;
	//event.target = target;
	//event.property = property;
	//event.time = time;
	//GrInjectSelectionEvent(&event);
}

gint
gdk_text_property_to_text_list (GdkAtom encoding, gint format, 
			     guchar *text, gint length,
			     gchar ***list)
{
#ifdef SERVER 
	GR_TEXT_PROP property;
	gint count = 0;
	PRINTFILE("gdk_text_property_to_text_list");
	
	if(!list) 
		return 0;

	property.value = text;
 	property.encoding = encoding;
  	property.format = format;
 	property.nitems = length;

	GrTextPropToTextList(&property, list, &count);
	return count;
#else
	gchar **tmp;
	if(!list)
		return 0;

	*list = (gchar**)malloc(sizeof(char*));
	if(*list == NULL)
		return 0;

	tmp = *list;
	*tmp = text;
	return 1;	
#endif

}

void
gdk_free_text_list (gchar **list)
{
	PRINTFILE("gdk_free_text_list");
	g_return_if_fail(list != NULL);
	
//	GrFreeTextList(list);
}

gint
gdk_string_to_compound_text (const gchar *str,
			     GdkAtom *encoding, gint *format,
			     guchar **ctext, gint *length)
{
#ifdef SERVER
  gint res;
  GR_TEXT_PROP	property;

  PRINTFILE("gdk_string_to_compound_text");
  res = GrTextListToTextProp((char **)&str, 1, GR_COMPOUNDTEXT_STYLE,
                                   &property);

  if (!res )
    {
      property.encoding = None;
      property.format = None;
      property.value = NULL;
      property.nitems = 0;
    }

  if (encoding)
    *encoding = property.encoding;
  if (format)
    *format = property.format;
  if (ctext)
    *ctext = property.value;
  if (length)
    *length = property.nitems;

  return TRUE;
#else
  gulong len;

  len = strlen(str);

  if(encoding)
	*encoding = gi_intern_atom("COMPOUND_TEXT", FALSE);
  if(format)
	*format = 8;
 
  if(ctext) /* since it's might be multibyte */
  {
	int i;
	ctext[0] = (guchar*)malloc(len+1);
 	for(i=0; i < len; i++)
		ctext[0][i] = (guchar)str[i];
	ctext[0][i] = 0;
  }
  if(length)
	*length = len;

  return TRUE;
#endif
}

void gdk_free_compound_text (guchar *ctext)
{
	if(ctext)
		free(ctext);
}
