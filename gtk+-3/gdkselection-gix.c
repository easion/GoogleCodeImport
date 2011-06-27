

#include <gi/gi.h>
#include <gi/property.h>
#include "config.h"

#include "gdkselection.h"
#include "gdkproperty.h"
#include "gdkprivate.h"

#include "gdkdisplay-gix.h"
#include "gdkprivate-gix.h"

#include <string.h>


static GPtrArray *virtual_atom_array;
static GHashTable *virtual_atom_hash;

static const gchar xatoms_string[] = 
  /* These are all the standard predefined X atoms */
  "\0"  /* leave a space for None, even though it is not a predefined atom */
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
  fprintf(stderr, "set selection owner: atom %ld, owner %p\n",
	  selection, owner);

  return TRUE;
}

void
_gdk_gix_display_send_selection_notify (GdkDisplay *dispay,
					    GdkWindow        *requestor,
					    GdkAtom          selection,
					    GdkAtom          target,
					    GdkAtom          property,
					    guint32          time)
{
}

gint
_gdk_gix_display_get_selection_property (GdkDisplay  *display,
					     GdkWindow   *requestor,
					     guchar     **data,
					     GdkAtom     *ret_type,
					     gint        *ret_format)
{
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

  gdk_window_ensure_native (requestor);

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

gchar *
_gdk_gix_display_utf8_to_string_target (GdkDisplay  *display,
					    const gchar *str)
{
  return NULL;
}
