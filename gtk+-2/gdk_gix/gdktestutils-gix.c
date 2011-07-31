/* Gtk+ testing utilities
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
 * GTK+ Gix backend
 * Copyright (C) 2011 www.hanxuantech.com.
 * Written by Easion <easion@hanxuantech.com> , it's based
 * on DirectFB port.
 */
#include "config.h"

#include <unistd.h>

#include "gdk.h"
#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include <gdk/gdktestutils.h>
#include <gdk/gdkkeysyms.h>
#include "gdkalias.h"

#if 0

static long
_gdk_keyval_to_gix (guint keyval)
{
  switch (keyval) {
    case 0 ... 127:
      return ( keyval );
    case GDK_F1 ... GDK_F12:
      return keyval - GDK_F1 + G_KEY_F1;
    case GDK_BackSpace:
      return G_KEY_BACKSPACE;
    case GDK_Tab:
      return G_KEY_TAB;
    case GDK_Return:
      return G_KEY_RETURN;
    case GDK_Escape:
      return G_KEY_ESCAPE;
    case GDK_Delete:
      return G_KEY_DELETE; 
    case GDK_Left:
      return G_KEY_LEFT;
    case GDK_Up:
      return G_KEY_UP;
    case GDK_Right:
      return G_KEY_RIGHT;
    case GDK_Down:
      return G_KEY_DOWN;
    case GDK_Insert:
      return G_KEY_INSERT;
    case GDK_Home:
      return G_KEY_HOME;
    case GDK_End:
      return G_KEY_END;
    case GDK_Page_Up:
      return G_KEY_PAGEUP;
    case GDK_Page_Down:
      return G_KEY_PAGEDOWN;
    case GDK_Print:
      return G_KEY_PRINT;
    case GDK_Pause:
      return G_KEY_PAUSE;
    case GDK_Clear:
      return G_KEY_CLEAR;
    //case GDK_Cancel:
    //  return G_KEY_CANCEL;
      /* TODO: handle them all */
    default:
      break;
  }

  return G_KEY_UNKNOWN;
}

static long
_gdk_modifiers_to_gix (GdkModifierType modifiers)
{
  long gix_modifiers = 0;
  
  if (modifiers & GDK_MOD1_MASK)
    gix_modifiers |= G_MODIFIERS_LALT;
  //if (modifiers & GDK_MOD2_MASK)
  //  gix_modifiers |= G_MODIFIERS_ALTGR;
  if (modifiers & GDK_CONTROL_MASK)
    gix_modifiers |= G_MODIFIERS_CTRL;
  if (modifiers & GDK_SHIFT_MASK)
    gix_modifiers |= G_MODIFIERS_SHIFT;

  return gix_modifiers;
}

#endif

void
gdk_test_render_sync (GdkWindow *window)
{
}

gboolean
gdk_test_simulate_key (GdkWindow      *window,
                       gint            x,
                       gint            y,
                       guint           keyval,
                       GdkModifierType modifiers,
                       GdkEventType    key_pressrelease)
{
  g_return_val_if_fail (key_pressrelease == GDK_KEY_PRESS || key_pressrelease == GDK_KEY_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);
  return FALSE;
}

gboolean
gdk_test_simulate_button (GdkWindow      *window,
                          gint            x,
                          gint            y,
                          guint           button, /*1..3*/
                          GdkModifierType modifiers,
                          GdkEventType    button_pressrelease)
{
  g_return_val_if_fail (button_pressrelease == GDK_BUTTON_PRESS || button_pressrelease == GDK_BUTTON_RELEASE, FALSE);
  g_return_val_if_fail (window != NULL, FALSE);

  return FALSE;
}



#define __GDK_TEST_UTILS_X11_C__
#include "gdkaliasdef.c"

