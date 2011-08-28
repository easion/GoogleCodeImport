

#include <gi/gi.h>
#include <gi/property.h>
#include "config.h"

#include "gdkselection.h"
#include "gdkproperty.h"
#include "gdkprivate.h"

#include "gdkdisplay-gix.h"
#include "gdkprivate-gix.h"

#include <string.h>


typedef struct _OwnerInfo OwnerInfo;

struct _OwnerInfo
{
  GdkAtom    selection;
  GdkWindow *owner;
  gulong     serial;
};

static GSList *owner_list;

static GPtrArray *virtual_atom_array;
static GHashTable *virtual_atom_hash;

static const gchar xatoms_string[] = 
  /* These are all the standard predefined X atoms */
  "\0"  /* leave a space for 0, even though it is not a predefined atom */
  "PRIMARY\0"
  "SECONDARY\0"
  "ARC\0"
  "ATOM\0"
  "BITMAP\0"
  "CARDINAL\0"
  "COLORMAP\0"
  "CURSOR\0"
  "CUT_BUFFER0\0"
  "CUT_BUFFER1\0"
  "CUT_BUFFER2\0"
  "CUT_BUFFER3\0"
  "CUT_BUFFER4\0"
  "CUT_BUFFER5\0"
  "CUT_BUFFER6\0"
  "CUT_BUFFER7\0"
  "DRAWABLE\0"
  "FONT\0"
  "INTEGER\0"
  "PIXMAP\0"
  "POINT\0"
  "RECTANGLE\0"
  "RESOURCE_MANAGER\0"
  "RGB_COLOR_MAP\0"
  "RGB_BEST_MAP\0"
  "RGB_BLUE_MAP\0"
  "RGB_DEFAULT_MAP\0"
  "RGB_GRAY_MAP\0"
  "RGB_GREEN_MAP\0"
  "RGB_RED_MAP\0"
  "STRING\0"
  "VISUALID\0"
  "WINDOW\0"
  "WM_COMMAND\0"
  "WM_HINTS\0"
  "WM_CLIENT_MACHINE\0"
  "WM_ICON_NAME\0"
  "WM_ICON_SIZE\0"
  "WM_NAME\0"
  "WM_NORMAL_HINTS\0"
  "WM_SIZE_HINTS\0"
  "WM_ZOOM_HINTS\0"
  "MIN_SPACE\0"
  "NORM_SPACE\0"
  "MAX_SPACE\0"
  "END_SPACE\0"
  "SUPERSCRIPT_X\0"
  "SUPERSCRIPT_Y\0"
  "SUBSCRIPT_X\0"
  "SUBSCRIPT_Y\0"
  "UNDERLINE_POSITION\0"
  "UNDERLINE_THICKNESS\0"
  "STRIKEOUT_ASCENT\0"
  "STRIKEOUT_DESCENT\0"
  "ITALIC_ANGLE\0"
  "X_HEIGHT\0"
  "QUAD_WIDTH\0"
  "WEIGHT\0"
  "POINT_SIZE\0"
  "RESOLUTION\0"
  "COPYRIGHT\0"
  "NOTICE\0"
  "FONT_NAME\0"
  "FAMILY_NAME\0"
  "FULL_NAME\0"
  "CAP_HEIGHT\0"
  "WM_CLASS\0"
  "WM_TRANSIENT_FOR\0"
  /* Below here, these are our additions. Increment N_CUSTOM_PREDEFINED
   * if you add any.
   */
  "CLIPBOARD\0"			/* = 69 */
;

static const gint xatoms_offset[] = {
    0,   1,   9,  19,  23,  28,  35,  44,  53,  60,  72,  84,
   96, 108, 120, 132, 144, 156, 165, 170, 178, 185, 189, 201,
  218, 232, 245, 258, 274, 287, 301, 313, 320, 329, 336, 347,
  356, 374, 387, 400, 408, 424, 438, 452, 462, 473, 483, 493,
  507, 521, 533, 545, 564, 584, 601, 619, 632, 641, 652, 659,
  670, 681, 691, 698, 708, 720, 730, 741, 750, 767
};

#define N_CUSTOM_PREDEFINED 1

#define ATOM_TO_INDEX(atom) (GPOINTER_TO_UINT(atom))
#define INDEX_TO_ATOM(atom) ((GdkAtom)GUINT_TO_POINTER(atom))

static void
insert_atom_pair (GdkDisplay *display,
		  GdkAtom     virtual_atom,
		  gi_atom_id_t        xatom)
{
  GdkDisplayGix *display_x11 = GDK_DISPLAY_GIX (display);  
  
  if (!display_x11->atom_from_virtual)
    {
      display_x11->atom_from_virtual = g_hash_table_new (g_direct_hash, NULL);
      display_x11->atom_to_virtual = g_hash_table_new (g_direct_hash, NULL);
    }
  
  g_hash_table_insert (display_x11->atom_from_virtual, 
		       GDK_ATOM_TO_POINTER (virtual_atom), 
		       GUINT_TO_POINTER (xatom));
  g_hash_table_insert (display_x11->atom_to_virtual,
		       GUINT_TO_POINTER (xatom), 
		       GDK_ATOM_TO_POINTER (virtual_atom));
}

static gi_atom_id_t
lookup_cached_xatom (GdkDisplay *display,
		     GdkAtom     atom)
{
  GdkDisplayGix *display_x11 = GDK_DISPLAY_GIX (display);

  if (ATOM_TO_INDEX (atom) < G_N_ELEMENTS (xatoms_offset) - N_CUSTOM_PREDEFINED)
    return ATOM_TO_INDEX (atom);
  
  if (display_x11->atom_from_virtual)
    return GPOINTER_TO_UINT (g_hash_table_lookup (display_x11->atom_from_virtual,
						  GDK_ATOM_TO_POINTER (atom)));

  return 0;
}


gi_atom_id_t
gdk_x11_atom_to_xatom_for_display (GdkDisplay *display,
				   GdkAtom     atom)
{
  gi_atom_id_t xatom = 0;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), 0);

  if (atom == GDK_NONE)
    return 0;

  if (gdk_display_is_closed (display))
    return 0;

  xatom = lookup_cached_xatom (display, atom);

  if (!xatom)
    {
      char *name;

      g_return_val_if_fail (ATOM_TO_INDEX (atom) < virtual_atom_array->len, 0);

      name = g_ptr_array_index (virtual_atom_array, ATOM_TO_INDEX (atom));

      xatom = gi_intern_atom ( name, FALSE); //XInternAtom
      insert_atom_pair (display, atom, xatom);
    }

  return xatom;
}

gi_atom_id_t
gdk_x11_get_xatom_by_name_for_display (GdkDisplay  *display,
				       const gchar *atom_name)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), 0);
  return gdk_x11_atom_to_xatom_for_display (display,
					    gdk_atom_intern (atom_name, FALSE));
}

GdkAtom
gdk_x11_xatom_to_atom_for_display (GdkDisplay *display,
				   gi_atom_id_t	       xatom)
{
  GdkDisplayGix *display_x11;
  GdkAtom virtual_atom = GDK_NONE;
  
  g_return_val_if_fail (GDK_IS_DISPLAY (display), GDK_NONE);

  if (xatom == 0)
    return GDK_NONE;

  if (gdk_display_is_closed (display))
    return GDK_NONE;

  display_x11 = GDK_DISPLAY_GIX (display);
  
  if (xatom < G_N_ELEMENTS (xatoms_offset) - N_CUSTOM_PREDEFINED)
    return INDEX_TO_ATOM (xatom);
  
  if (display_x11->atom_to_virtual)
    virtual_atom = GDK_POINTER_TO_ATOM (g_hash_table_lookup (display_x11->atom_to_virtual,
							     GUINT_TO_POINTER (xatom)));
  
  if (!virtual_atom)
    {
      /* If this atom doesn't exist, we'll die with an X error unless
       * we take precautions
       */
      char *name;
      //gdk_x11_display_error_trap_push (display);
      name = gi_get_atom_name ( xatom);
      /*if (gdk_x11_display_error_trap_pop (display))
	{
	  g_warning (G_STRLOC " invalid X atom: %ld", xatom);
	}
      else*/
	{
	  virtual_atom = gdk_atom_intern (name, FALSE);
	  free (name);
	  
	  insert_atom_pair (display, virtual_atom, xatom);
	}
    }

  return virtual_atom;
}




GdkWindow *
_gdk_gix_display_get_selection_owner (GdkDisplay *display,
					  GdkAtom     selection)
{
  gi_window_id_t xwindow;

  if (gdk_display_is_closed (display))
    return NULL;

  xwindow = gi_get_selection_owner (gdk_x11_atom_to_xatom_for_display (display,
                                                                   selection));
  if (xwindow == 0)
    return NULL;

  return gdk_gix_window_lookup_for_display (display, xwindow);

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

  if (gdk_display_is_closed (display))
    return FALSE;

  if (owner)
    {
      if (GDK_WINDOW_DESTROYED (owner) || !GDK_WINDOW_IS_GIX (owner))
        return FALSE;

      gdk_window_ensure_native (owner);
      //xdisplay = GDK_WINDOW_XDISPLAY (owner);
      xwindow = GDK_WINDOW_XID (owner);
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
  xevent.requestor = GDK_WINDOW_XID (requestor);
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

  if (GDK_WINDOW_DESTROYED (requestor) || !GDK_WINDOW_IS_GIX (requestor))
    goto err;

  t = NULL;

  /* We can't delete the selection here, because it might be the INCR
     protocol, in which case the client has to make sure they'll be
     notified of PropertyChange events _before_ the property is deleted.
     Otherwise there's no guarantee we'll win the race ... */
  if (gi_get_window_property (
                          GDK_WINDOW_XID (requestor),
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

  if (GDK_WINDOW_DESTROYED (requestor) || !GDK_WINDOW_IS_GIX (requestor))
    return;

  //gdk_window_ensure_native (requestor);

  gi_convert_selection (
                     gdk_x11_atom_to_xatom_for_display (display, selection),
                     gdk_x11_atom_to_xatom_for_display (display, target),
                     gdk_x11_get_xatom_by_name_for_display (display, "GDK_SELECTION"),
                     GDK_WINDOW_XID (requestor), time);
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
