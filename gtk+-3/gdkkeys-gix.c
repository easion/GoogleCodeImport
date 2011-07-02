


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

typedef struct _GdkGixKeymap          GdkGixKeymap;
typedef struct _GdkGixKeymapClass     GdkGixKeymapClass;

struct _GdkGixKeymap
{
  GdkKeymap parent_instance;
};

struct _GdkGixKeymapClass
{
  GdkKeymapClass parent_class;
};

#define GDK_TYPE_GIX_KEYMAP          (_gdk_gix_keymap_get_type ())
#define GDK_GIX_KEYMAP(object)       (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GIX_KEYMAP, GdkGixKeymap))
#define GDK_IS_GIX_KEYMAP(object)    (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GIX_KEYMAP))

G_DEFINE_TYPE (GdkGixKeymap, _gdk_gix_keymap, GDK_TYPE_KEYMAP)
static guint *keysym_tab = NULL;
guint _scancode_rshift = 0;

gboolean _gdk_keyboard_has_altgr = FALSE;
static GdkModifierType gdk_shift_modifiers = GDK_SHIFT_MASK;


static void
handle_special (guint  vk,
		guint *ksymp,
		gint   shift)

{
  switch (vk)
    {
    //case G_KEY_CANCEL:
    //  *ksymp = GDK_KEY_Cancel; break;
    case G_KEY_BACKSPACE:
      *ksymp = GDK_KEY_BackSpace; break;
    case G_KEY_TAB:
      if (shift & 0x1)
	*ksymp = GDK_KEY_ISO_Left_Tab;
      else
	*ksymp = GDK_KEY_Tab;
      break;
    case G_KEY_CLEAR:
      *ksymp = GDK_KEY_Clear; break;
    case G_KEY_RETURN:
      *ksymp = GDK_KEY_Return; break;
    //case G_KEY_RSHIFT:
    case G_KEY_LSHIFT:
      *ksymp = GDK_KEY_Shift_L; break;
    //case G_KEY_RCTRL:
    case G_KEY_LCTRL:
      *ksymp = GDK_KEY_Control_L; break;
    case G_KEY_LALT:
    //case G_KEY_LMENU:
      *ksymp = GDK_KEY_Alt_L; break;
    case G_KEY_PAUSE:
      *ksymp = GDK_KEY_Pause; break;
    case G_KEY_ESCAPE:
      *ksymp = GDK_KEY_Escape; break;
    //case G_KEY_PRIOR:
    //  *ksymp = GDK_KEY_Prior; break;
    //case G_KEY_NEXT:
     // *ksymp = GDK_KEY_Next; break;
    case G_KEY_END:
      *ksymp = GDK_KEY_End; break;
    case G_KEY_HOME:
      *ksymp = GDK_KEY_Home; break;
    case G_KEY_LEFT:
      *ksymp = GDK_KEY_Left; break;
    case G_KEY_UP:
      *ksymp = GDK_KEY_Up; break;
    case G_KEY_RIGHT:
      *ksymp = GDK_KEY_Right; break;
    case G_KEY_DOWN:
      *ksymp = GDK_KEY_Down; break;
    //case G_KEY_SELECT:
    //  *ksymp = GDK_KEY_Select; break;
    case G_KEY_PRINT:
      *ksymp = GDK_KEY_Print; break;
    //case G_KEY_EXECUTE:
    //  *ksymp = GDK_KEY_Execute; break;
    case G_KEY_INSERT:
      *ksymp = GDK_KEY_Insert; break;
    case G_KEY_DELETE:
      *ksymp = GDK_KEY_Delete; break;
    case G_KEY_HELP:
      *ksymp = GDK_KEY_Help; break;
    case G_KEY_LMETA:
      *ksymp = GDK_KEY_Meta_L; break;
    case G_KEY_RMETA:
      *ksymp = GDK_KEY_Meta_R; break;
    case G_KEY_MENU:
      *ksymp = GDK_KEY_Menu; break;
    case G_KEY_KP_MULTIPLY:
      *ksymp = GDK_KEY_KP_Multiply; break;
    case G_KEY_PLUS:
      *ksymp = GDK_KEY_KP_Add; break;
   // case G_KEY_SEPARATOR:
    //  *ksymp = GDK_KEY_KP_Separator; break;
    //case G_KEY_SUBTRACT:
    //  *ksymp = GDK_KEY_KP_Subtract; break;
    case G_KEY_KP_DIVIDE:
      *ksymp = GDK_KEY_KP_Divide; break;
    case G_KEY_0:
      *ksymp = GDK_KEY_KP_0; break;
    case G_KEY_1:
      *ksymp = GDK_KEY_KP_1; break;
    case G_KEY_2:
      *ksymp = GDK_KEY_KP_2; break;
    case G_KEY_3:
      *ksymp = GDK_KEY_KP_3; break;
    case G_KEY_4:
      *ksymp = GDK_KEY_KP_4; break;
    case G_KEY_5:
      *ksymp = GDK_KEY_KP_5; break;
    case G_KEY_6:
      *ksymp = GDK_KEY_KP_6; break;
    case G_KEY_7:
      *ksymp = GDK_KEY_KP_7; break;
    case G_KEY_8:
      *ksymp = GDK_KEY_KP_8; break;
    case G_KEY_9:
      *ksymp = GDK_KEY_KP_9; break;
    case G_KEY_F1:
      *ksymp = GDK_KEY_F1; break;
    case G_KEY_F2:
      *ksymp = GDK_KEY_F2; break;
    case G_KEY_F3:
      *ksymp = GDK_KEY_F3; break;
    case G_KEY_F4:
      *ksymp = GDK_KEY_F4; break;
    case G_KEY_F5:
      *ksymp = GDK_KEY_F5; break;
    case G_KEY_F6:
      *ksymp = GDK_KEY_F6; break;
    case G_KEY_F7:
      *ksymp = GDK_KEY_F7; break;
    case G_KEY_F8:
      *ksymp = GDK_KEY_F8; break;
    case G_KEY_F9:
      *ksymp = GDK_KEY_F9; break;
    case G_KEY_F10:
      *ksymp = GDK_KEY_F10; break;
    case G_KEY_F11:
      *ksymp = GDK_KEY_F11; break;
    case G_KEY_F12:
      *ksymp = GDK_KEY_F12; break;
    case G_KEY_F13:
      *ksymp = GDK_KEY_F13; break;
    case G_KEY_F14:
      *ksymp = GDK_KEY_F14; break;
    case G_KEY_F15:
      *ksymp = GDK_KEY_F15; break;
    /*case G_KEY_F16:
      *ksymp = GDK_KEY_F16; break;
    case G_KEY_F17:
      *ksymp = GDK_KEY_F17; break;
    case G_KEY_F18:
      *ksymp = GDK_KEY_F18; break;
    case G_KEY_F19:
      *ksymp = GDK_KEY_F19; break;
    case G_KEY_F20:
      *ksymp = GDK_KEY_F20; break;
    case G_KEY_F21:
      *ksymp = GDK_KEY_F21; break;
    case G_KEY_F22:
      *ksymp = GDK_KEY_F22; break;
    case G_KEY_F23:
      *ksymp = GDK_KEY_F23; break;
    case G_KEY_F24:
      *ksymp = GDK_KEY_F24; break;
	  */
    case G_KEY_NUMLOCK:
      *ksymp = GDK_KEY_Num_Lock; break;
    case G_KEY_SCROLLOCK:
      *ksymp = GDK_KEY_Scroll_Lock; break;
    case G_KEY_RSHIFT:
      *ksymp = GDK_KEY_Shift_R; break;
    case G_KEY_RCTRL:
      *ksymp = GDK_KEY_Control_R; break;
    case G_KEY_RALT:
      *ksymp = GDK_KEY_Alt_R; break;
    }
}

static void
set_shift_vks (guchar *key_state,
	       gint    shift)
{
  switch (shift)
    {
    case 0:
      key_state[G_KEY_RSHIFT] = 0;
      key_state[G_KEY_RCTRL] = key_state[G_KEY_MENU] = 0;
      break;
    case 1:
      key_state[G_KEY_RSHIFT] = 0x80;
      key_state[G_KEY_RCTRL] = key_state[G_KEY_MENU] = 0;
      break;
    case 2:
      key_state[G_KEY_RSHIFT] = 0;
      key_state[G_KEY_RCTRL] = key_state[G_KEY_MENU] = 0x80;
      break;
    case 3:
      key_state[G_KEY_RSHIFT] = 0x80;
      key_state[G_KEY_RCTRL] = key_state[G_KEY_MENU] = 0x80;
      break;
    }
}

static void
update_keymap (void)
{
  guchar key_state[G_KEY_LAST];
  guint vk;
  gboolean capslock_tested = FALSE;

  if (keysym_tab != NULL)// && current_serial == _gdk_keymap_serial)
    return;

  if (keysym_tab == NULL)
    keysym_tab = g_new (guint, 4*G_KEY_LAST);
  memset (key_state, 0, sizeof (key_state));

  _gdk_keyboard_has_altgr = FALSE;
  gdk_shift_modifiers = GDK_SHIFT_MASK;

  for (vk = 0; vk < G_KEY_LAST; vk++)
    {
	  gint shift;
      //if ((scancode = MapVirtualKey (vk, 0)) == 0 &&
	  //vk != G_KEY_DIVIDE)
	keysym_tab[vk*4+0] =
	  keysym_tab[vk*4+1] =
	  keysym_tab[vk*4+2] =
	  keysym_tab[vk*4+3] = GDK_KEY_VoidSymbol;
     // else
	{

	  if (vk == G_KEY_RSHIFT)
	    _scancode_rshift = vk;

	  key_state[vk] = 0x80;
	  for (shift = 0; shift < 4; shift++)
	    {
	      guint *ksymp = keysym_tab + vk*4 + shift;
	      
	      set_shift_vks (key_state, shift);
	      *ksymp = 0;
	      handle_special (vk, ksymp, shift);

	      if (*ksymp == 0)
		{
			  *ksymp = gdk_unicode_to_keyval (vk);
		  /*wchar_t wcs[10];
		  gint k;

		  wcs[0] = wcs[1] = 0;
		  k = ToUnicodeEx (vk, scancode, key_state,
				   wcs, G_N_ELEMENTS (wcs),
				   0, _gdk_input_locale);

		  if (k == 1)
		    *ksymp = gdk_unicode_to_keyval (wcs[0]);
		  else if (k == -1)
		    {
		      guint keysym = gdk_unicode_to_keyval (wcs[0]);		     
		      reset_after_dead (key_state);
		      // Use dead keysyms instead of "undead" ones 
		      handle_dead (keysym, ksymp);
		    }
		  else if (k == 0)
		    {
		      reset_after_dead (key_state);
		    }
		  else
		    {
		    }
			*/
		}
	      if (*ksymp == 0)
		*ksymp = GDK_KEY_VoidSymbol;
	    }
	  key_state[vk] = 0;

	  /* Check if keyboard has an AltGr key by checking if
	   * the mapping with Control+Alt is different.
	   */
	  /*if (!_gdk_keyboard_has_altgr)
	    if ((keysym_tab[vk*4 + 2] != GDK_KEY_VoidSymbol &&
		 keysym_tab[vk*4] != keysym_tab[vk*4 + 2]) ||
		(keysym_tab[vk*4 + 3] != GDK_KEY_VoidSymbol &&
		 keysym_tab[vk*4 + 1] != keysym_tab[vk*4 + 3]))
	      _gdk_keyboard_has_altgr = TRUE;*/
	  
	  if (!capslock_tested)
	    {
	      if (g_unichar_isgraph (gdk_keyval_to_unicode (keysym_tab[vk*4 + 0])) &&
		  keysym_tab[vk*4 + 1] != keysym_tab[vk*4 + 0] &&
		  g_unichar_isgraph (gdk_keyval_to_unicode (keysym_tab[vk*4 + 1])) &&
		  keysym_tab[vk*4 + 1] != gdk_keyval_to_upper (keysym_tab[vk*4 + 0]))
		{
		  guchar chars[2];
		  
		  capslock_tested = TRUE;
		  
		  key_state[G_KEY_RSHIFT] = 0;
		  key_state[G_KEY_RCTRL] = key_state[G_KEY_MENU] = 0;
		  //key_state[G_KEY_CAPITAL] = 1;

		  /*if (ToAsciiEx (vk, scancode, key_state,
				 (LPWORD) chars, 0, _gdk_input_locale) == 1)
		    {
		      if (chars[0] >= GDK_KEY_space &&
			  chars[0] <= GDK_KEY_asciitilde &&
			  chars[0] == keysym_tab[vk*4 + 1])
			{
			  // CapsLock acts as ShiftLock 
			  gdk_shift_modifiers |= GDK_LOCK_MASK;
			}
		    }*/
		  //key_state[G_KEY_CAPITAL] = 0;
		}    
	    }
	}
    }
  GDK_NOTE (EVENTS, print_keysym_tab ());
} 


static void
gdk_gix_keymap_finalize (GObject *object)
{
  G_OBJECT_CLASS (_gdk_gix_keymap_parent_class)->finalize (object);
}

static PangoDirection
gdk_gix_keymap_get_direction (GdkKeymap *keymap)
{
	update_keymap ();
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

  g_return_val_if_fail (keymap == NULL || GDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (keys != NULL, FALSE);
  g_return_val_if_fail (n_keys != NULL, FALSE);
  g_return_val_if_fail (keyval != 0, FALSE);
  
  retval = g_array_new (FALSE, FALSE, sizeof (GdkKeymapKey));

  /* Accept only the default keymap */
  if (keymap == NULL || keymap == gdk_keymap_get_default ())
    {
      gint vk;
      
      update_keymap ();

      for (vk = 0; vk < G_KEY_LAST; vk++)
	{
	  gint i;

	  for (i = 0; i < 4; i++)
	    {
	      if (keysym_tab[vk*4+i] == keyval)
		{
		  GdkKeymapKey key;
		  
		  key.keycode = vk;
		  
		  /* 2 levels (normal, shift), two groups (normal, AltGr) */
		  key.group = i / 2;
		  key.level = i % 2;
		  
		  g_array_append_val (retval, key);
		}
	    }
	}
    }

#ifdef G_ENABLE_DEBUG
  if (_gdk_debug_flags & GDK_DEBUG_EVENTS)
    {
      gint i;

      g_print ("gdk_keymap_get_entries_for_keyval: %#.04x (%s):",
	       keyval, gdk_keyval_name (keyval));
      for (i = 0; i < retval->len; i++)
	{
	  GdkKeymapKey *entry = (GdkKeymapKey *) retval->data + i;
	  g_print ("  %#.02x %d %d", entry->keycode, entry->group, entry->level);
	}
      g_print ("\n");
    }
#endif

  if (retval->len > 0)
    {
      *keys = (GdkKeymapKey*) retval->data;
      *n_keys = retval->len;
    }
  else
    {
      *keys = NULL;
      *n_keys = 0;
    }
      
  g_array_free (retval, retval->len > 0 ? FALSE : TRUE);

  return *n_keys > 0;
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

  g_return_val_if_fail (keymap == NULL || GDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (n_entries != NULL, FALSE);

  if (hardware_keycode <= 0 ||
      hardware_keycode >= G_KEY_LAST)
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
  
  /* Accept only the default keymap */
  if (keymap == NULL || keymap == gdk_keymap_get_default ())
    {
      gint i;

      update_keymap ();

      for (i = 0; i < 4; i++)
	{
	  if (key_array)
            {
              GdkKeymapKey key;
	      
              key.keycode = hardware_keycode;
              
              key.group = i / 2;
              key.level = i % 2;
              
              g_array_append_val (key_array, key);
            }

          if (keyval_array)
            g_array_append_val (keyval_array, keysym_tab[hardware_keycode*4+i]);
	}
    }

  if ((key_array && key_array->len > 0) ||
      (keyval_array && keyval_array->len > 0))
    {
      if (keys)
        *keys = (GdkKeymapKey*) key_array->data;

      if (keyvals)
        *keyvals = (guint*) keyval_array->data;

      if (key_array)
        *n_entries = key_array->len;
      else
        *n_entries = keyval_array->len;
    }
  else
    {
      if (keys)
        *keys = NULL;

      if (keyvals)
        *keyvals = NULL;
      
      *n_entries = 0;
    }

  if (key_array)
    g_array_free (key_array, key_array->len > 0 ? FALSE : TRUE);
  if (keyval_array)
    g_array_free (keyval_array, keyval_array->len > 0 ? FALSE : TRUE);

  return *n_entries > 0;
}

static guint
gdk_gix_keymap_lookup_key (GdkKeymap          *keymap,
			       const GdkKeymapKey *key)
{
  guint sym;

  g_return_val_if_fail (keymap == NULL || GDK_IS_KEYMAP (keymap), 0);
  g_return_val_if_fail (key != NULL, 0);
  g_return_val_if_fail (key->group < 4, 0);
  
  /* Accept only the default keymap */
  if (keymap != NULL && keymap != gdk_keymap_get_default ())
    return 0;

  update_keymap ();
  
  if (key->keycode >= G_KEY_LAST ||
      key->group < 0 || key->group >= 2 ||
      key->level < 0 || key->level >= 2)
    return 0;
  
  sym = keysym_tab[key->keycode*4 + key->group*2 + key->level];
  
  if (sym == GDK_KEY_VoidSymbol)
    return 0;
  else
    return sym;
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
  guint tmp_keyval;
  guint *keyvals;
  gint shift_level;
  gboolean ignore_shift = FALSE;
  gboolean ignore_group = FALSE;
      
  g_return_val_if_fail (keymap == NULL || GDK_IS_KEYMAP (keymap), FALSE);
  g_return_val_if_fail (group < 4, FALSE);
  
#if 0
  GDK_NOTE (EVENTS, g_print ("gdk_keymap_translate_keyboard_state: keycode=%#x state=%#x group=%d\n",
			     hardware_keycode, state, group));
#endif
  if (keyval)
    *keyval = 0;
  if (effective_group)
    *effective_group = 0;
  if (level)
    *level = 0;
  if (consumed_modifiers)
    *consumed_modifiers = 0;

  /* Accept only the default keymap */
  if (keymap != NULL && keymap != gdk_keymap_get_default ())
    return FALSE;

  if (hardware_keycode >= G_KEY_LAST)
    return FALSE;

  if (group < 0 || group >= 2)
    return FALSE;

  update_keymap ();

  keyvals = keysym_tab + hardware_keycode*4;

  if ((state & GDK_LOCK_MASK) &&
      (state & GDK_SHIFT_MASK) &&
      ((gdk_shift_modifiers & GDK_LOCK_MASK) ||
       (keyvals[group*2 + 1] == gdk_keyval_to_upper (keyvals[group*2 + 0]))))
    /* Shift always disables ShiftLock. Shift disables CapsLock for
     * keys with lowercase/uppercase letter pairs.
     */
    shift_level = 0;
  else if (state & gdk_shift_modifiers)
    shift_level = 1;
  else
    shift_level = 0;

  /* Drop group and shift if there are no keysymbols on
   * the key for those.
   */
  if (shift_level == 1 &&
      keyvals[group*2 + 1] == GDK_KEY_VoidSymbol &&
      keyvals[group*2] != GDK_KEY_VoidSymbol)
    {
      shift_level = 0;
      ignore_shift = TRUE;
    }

  if (group == 1 &&
      keyvals[2 + shift_level] == GDK_KEY_VoidSymbol &&
      keyvals[0 + shift_level] != GDK_KEY_VoidSymbol)
    {
      group = 0;
      ignore_group = TRUE;
    }

  if (keyvals[group *2 + shift_level] == GDK_KEY_VoidSymbol &&
      keyvals[0 + 0] != GDK_KEY_VoidSymbol)
    {
      shift_level = 0;
      group = 0;
      ignore_group = TRUE;
      ignore_shift = TRUE;
    }

  /* See whether the group and shift level actually mattered
   * to know what to put in consumed_modifiers
   */
  if (keyvals[group*2 + 1] == GDK_KEY_VoidSymbol ||
      keyvals[group*2 + 0] == keyvals[group*2 + 1])
    ignore_shift = TRUE;

  if (keyvals[2 + shift_level] == GDK_KEY_VoidSymbol ||
      keyvals[0 + shift_level] == keyvals[2 + shift_level])
    ignore_group = TRUE;

  tmp_keyval = keyvals[group*2 + shift_level];

  /* If a true CapsLock is toggled, and Shift is not down,
   * and the shifted keysym is the uppercase of the unshifted,
   * use it.
   */
  if (!(gdk_shift_modifiers & GDK_LOCK_MASK) &&
      !(state & GDK_SHIFT_MASK) &&
      (state & GDK_LOCK_MASK))
    {
      guint upper = gdk_keyval_to_upper (tmp_keyval);
      if (upper == keyvals[group*2 + 1])
	tmp_keyval = upper;
    }

  if (keyval)
    *keyval = tmp_keyval;

  if (effective_group)
    *effective_group = group;

  if (level)
    *level = shift_level;

  if (consumed_modifiers)
    {
      *consumed_modifiers =
	(ignore_group ? 0 : GDK_MOD2_MASK) |
	(ignore_shift ? 0 : (GDK_SHIFT_MASK|GDK_LOCK_MASK));
    }
				
#if 0
  GDK_NOTE (EVENTS, g_print ("... group=%d level=%d cmods=%#x keyval=%s\n",
			     group, shift_level, tmp_modifiers, gdk_keyval_name (tmp_keyval)));
#endif

  return tmp_keyval != GDK_KEY_VoidSymbol;

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

GdkKeymap *
_gdk_gix_keymap_new (GdkDisplay *display)
{
  GdkGixKeymap *keymap;

  keymap = g_object_new (GDK_TYPE_GIX_KEYMAP, NULL);
  GDK_KEYMAP (keymap)->display = display;

  update_keymap (); 

  return GDK_KEYMAP (keymap);
}

