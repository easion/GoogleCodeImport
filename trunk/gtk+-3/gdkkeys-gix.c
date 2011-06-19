


/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "gdk.h"
#include "gdkgix.h"

#include "gdkprivate-gix.h"
#include "gdkinternals.h"
#include "gdkdisplay-gix.h"
#include "gdkkeysprivate.h"

//#include <X11/extensions/XKBcommon.h>

typedef struct _GdkGixKeymap          GdkGixKeymap;
typedef struct _GdkGixKeymapClass     GdkGixKeymapClass;

struct _GdkGixKeymap
{
  GdkKeymap parent_instance;
  GdkModifierType modmap[8];
  //struct xkb_desc *xkb;
};

struct _GdkGixKeymapClass
{
  GdkKeymapClass parent_class;
};

#define GDK_TYPE_GIX_KEYMAP          (_gdk_gix_keymap_get_type ())
#define GDK_GIX_KEYMAP(object)       (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GIX_KEYMAP, GdkGixKeymap))
#define GDK_IS_GIX_KEYMAP(object)    (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GIX_KEYMAP))

G_DEFINE_TYPE (GdkGixKeymap, _gdk_gix_keymap, GDK_TYPE_KEYMAP)

static void
gdk_gix_keymap_finalize (GObject *object)
{
  G_OBJECT_CLASS (_gdk_gix_keymap_parent_class)->finalize (object);
}

static PangoDirection
gdk_gix_keymap_get_direction (GdkKeymap *keymap)
{
    return PANGO_DIRECTION_NEUTRAL;
}

static gboolean
gdk_gix_keymap_have_bidi_layouts (GdkKeymap *keymap)
{
    return FALSE;
}

static gboolean
gdk_gix_keymap_get_caps_lock_state (GdkKeymap *keymap)
{
  return FALSE;
}

static gboolean
gdk_gix_keymap_get_num_lock_state (GdkKeymap *keymap)
{
  return FALSE;
}

static gboolean
gdk_gix_keymap_get_entries_for_keyval (GdkKeymap     *keymap,
					   guint          keyval,
					   GdkKeymapKey **keys,
					   gint          *n_keys)
{
  GArray *retval;
  uint32_t keycode = 0;


  //xkb = GDK_GIX_KEYMAP (keymap)->xkb;
  //keycode = xkb->min_key_code;

  //retval = g_array_new (FALSE, FALSE, sizeof (GdkKeymapKey));


  return  0;
}

static gboolean
gdk_gix_keymap_get_entries_for_keycode (GdkKeymap     *keymap,
					    guint          hardware_keycode,
					    GdkKeymapKey **keys,
					    guint        **keyvals,
					    gint          *n_entries)
{
  GArray *key_array;
  GArray *keyval_array;
  //struct xkb_desc *xkb;
  gint max_shift_levels;
  gint group = 0;
  gint level = 0;
  gint total_syms;
  gint i;
  uint32_t *entry;

  g_return_val_if_fail (keymap == NULL || GDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (n_entries != NULL, FALSE);

  //xkb = GDK_GIX_KEYMAP (keymap)->xkb;

  //if (hardware_keycode < xkb->min_key_code ||
  //    hardware_keycode > xkb->max_key_code)
    {
      if (keys)
	*keys = NULL;
      if (keyvals)
	*keyvals = NULL;

      *n_entries = 0;
      return FALSE;
    }

  if (keys)
    key_array = g_array_new (FALSE, FALSE, sizeof (GdkKeymapKey));
  else
    key_array = NULL;

  if (keyvals)
    keyval_array = g_array_new (FALSE, FALSE, sizeof (guint));
  else
    keyval_array = NULL;

  /* See sec 15.3.4 in XKB docs */
  //max_shift_levels = XkbKeyGroupsWidth (xkb, hardware_keycode);
  //total_syms = XkbKeyNumSyms (xkb, hardware_keycode);

  /* entry is an array with all syms for group 0, all
   * syms for group 1, etc. and for each group the
   * shift level syms are in order
   */
  //entry = XkbKeySymsPtr (xkb, hardware_keycode);

  for (i = 0; i < total_syms; i++)
    {
      /* check out our cool loop invariant */
      g_assert (i == (group * max_shift_levels + level));

      if (key_array)
	{
	  GdkKeymapKey key;

	  key.keycode = hardware_keycode;
	  key.group = group;
	  key.level = level;

	  g_array_append_val (key_array, key);
	}

      if (keyval_array)
	g_array_append_val (keyval_array, entry[i]);

      ++level;

      if (level == max_shift_levels)
	{
	  level = 0;
	  ++group;
	}
    }

  *n_entries = 0;

  if (keys)
    {
      *n_entries = key_array->len;
      *keys = (GdkKeymapKey*) g_array_free (key_array, FALSE);
    }

  if (keyvals)
    {
      *n_entries = keyval_array->len;
      *keyvals = (guint*) g_array_free (keyval_array, FALSE);
    }

  return *n_entries > 0;
}

static guint
gdk_gix_keymap_lookup_key (GdkKeymap          *keymap,
			       const GdkKeymapKey *key)
{
  //struct xkb_desc *xkb;

  //xkb = GDK_GIX_KEYMAP (keymap)->xkb;

  return 0;//XkbKeySymEntry (xkb, key->keycode, key->level, key->group);
}

/* This is copied straight from XFree86 Xlib, to:
 *  - add the group and level return.
 *  - change the interpretation of mods_rtrn as described
 *    in the docs for gdk_keymap_translate_keyboard_state()
 * It's unchanged for ease of diff against the Xlib sources; don't
 * reformat it.
 */

static gboolean
gdk_gix_keymap_translate_keyboard_state (GdkKeymap       *keymap,
					     guint            hardware_keycode,
					     GdkModifierType  state,
					     gint             group,
					     guint           *keyval,
					     gint            *effective_group,
					     gint            *level,
					     GdkModifierType *consumed_modifiers)
{
  
  return 0;
}


static void
update_modmap (GdkGixKeymap *gix_keymap)
{
  
}

static void
gdk_gix_keymap_add_virtual_modifiers (GdkKeymap       *keymap,
					  GdkModifierType *state)
{
  GdkGixKeymap *gix_keymap;
  int i;

  gix_keymap = GDK_GIX_KEYMAP (keymap);

  for (i = 3; i < 8; i++)
    {
      if ((1 << i) & *state)
	{
	  if (gix_keymap->modmap[i] & GDK_MOD1_MASK)
	    *state |= GDK_MOD1_MASK;
	  if (gix_keymap->modmap[i] & GDK_SUPER_MASK)
	    *state |= GDK_SUPER_MASK;
	  if (gix_keymap->modmap[i] & GDK_HYPER_MASK)
	    *state |= GDK_HYPER_MASK;
	  if (gix_keymap->modmap[i] & GDK_META_MASK)
	    *state |= GDK_META_MASK;
	}
    }
}

static gboolean
gdk_gix_keymap_map_virtual_modifiers (GdkKeymap       *keymap,
					  GdkModifierType *state)
{
  const guint vmods[] = {
    GDK_SUPER_MASK, GDK_HYPER_MASK, GDK_META_MASK
  };
  int i, j;
  GdkGixKeymap *gix_keymap;
  gboolean retval;

  gix_keymap = GDK_GIX_KEYMAP (keymap);

  for (j = 0; j < 3; j++)
    {
      if (*state & vmods[j])
	{
	  for (i = 3; i < 8; i++)
	    {
	      if (gix_keymap->modmap[i] & vmods[j])
		{
		  if (*state & (1 << i))
		    retval = FALSE;
		  else
		    *state |= 1 << i;
		}
	    }
	}
    }

  return retval;
}

static void
_gdk_gix_keymap_class_init (GdkGixKeymapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkKeymapClass *keymap_class = GDK_KEYMAP_CLASS (klass);

  object_class->finalize = gdk_gix_keymap_finalize;

  keymap_class->get_direction = gdk_gix_keymap_get_direction;
  keymap_class->have_bidi_layouts = gdk_gix_keymap_have_bidi_layouts;
  keymap_class->get_caps_lock_state = gdk_gix_keymap_get_caps_lock_state;
  keymap_class->get_num_lock_state = gdk_gix_keymap_get_num_lock_state;
  keymap_class->get_entries_for_keyval = gdk_gix_keymap_get_entries_for_keyval;
  keymap_class->get_entries_for_keycode = gdk_gix_keymap_get_entries_for_keycode;
  keymap_class->lookup_key = gdk_gix_keymap_lookup_key;
  keymap_class->translate_keyboard_state = gdk_gix_keymap_translate_keyboard_state;
  keymap_class->add_virtual_modifiers = gdk_gix_keymap_add_virtual_modifiers;
  keymap_class->map_virtual_modifiers = gdk_gix_keymap_map_virtual_modifiers;
}

static void
_gdk_gix_keymap_init (GdkGixKeymap *keymap)
{
}

static void
update_keymaps (GdkGixKeymap *keymap)
{
 
}

GdkKeymap *
_gdk_gix_keymap_new (GdkDisplay *display)
{
  GdkGixKeymap *keymap;
  //struct xkb_rule_names names;

  keymap = g_object_new (_gdk_gix_keymap_get_type(), NULL);
  GDK_KEYMAP (keymap)->display = display;

  //names.rules = "evdev";
  //names.model = "pc105";
  //names.layout = "us";
  //names.variant = "";
  //names.options = "";
  //keymap->xkb = xkb_compile_keymap_from_rules(&names);

  update_modmap (keymap);
  update_keymaps (keymap);

  return GDK_KEYMAP (keymap);
}

