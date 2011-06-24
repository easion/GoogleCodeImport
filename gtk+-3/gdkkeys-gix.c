


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
  //GdkModifierType modmap[8];
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
  if (n_keys)
    *n_keys = 1;
  if (keys)
    {
      *keys = g_new0 (GdkKeymapKey, 1);
      (*keys)->keycode = keyval;
    }

  return TRUE;
}

static gboolean
gdk_gix_keymap_get_entries_for_keycode (GdkKeymap     *keymap,
					    guint          hardware_keycode,
					    GdkKeymapKey **keys,
					    guint        **keyvals,
					    gint          *n_entries)
{
  if (n_entries)
    *n_entries = 1;
  if (keys)
    {
      *keys = g_new0 (GdkKeymapKey, 1);
      (*keys)->keycode = hardware_keycode;
    }
  if (keyvals)
    {
      *keyvals = g_new0 (guint, 1);
      (*keyvals)[0] = hardware_keycode;
    }
  return TRUE;
}

static guint
gdk_gix_keymap_lookup_key (GdkKeymap          *keymap,
			       const GdkKeymapKey *key)
{
  return key->keycode;
}

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
  if (keyval)
    *keyval = hardware_keycode;
  if (effective_group)
    *effective_group = 0;
  if (level)
    *level = 0;
  return TRUE;
}


static void
gdk_gix_keymap_add_virtual_modifiers (GdkKeymap       *keymap,
					  GdkModifierType *state)
{
}

static gboolean
gdk_gix_keymap_map_virtual_modifiers (GdkKeymap       *keymap,
					  GdkModifierType *state)
{

  return TRUE;
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

  keymap = g_object_new (GDK_TYPE_GIX_KEYMAP, NULL);
  GDK_KEYMAP (keymap)->display = display;

  //update_modmap (keymap);
  update_keymaps (keymap);

  return GDK_KEYMAP (keymap);
}

