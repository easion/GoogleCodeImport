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
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH 
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */
#include "config.h"

#include <unistd.h>

#include "gdk.h"
#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include <gdk/gdktestutils.h>
#include <gdk/gdkkeysyms.h>
#include "gdkalias.h"

#if 1

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
  long dfb_modifiers = 0;
  
  if (modifiers & GDK_MOD1_MASK)
    dfb_modifiers |= G_MODIFIERS_LALT;
  //if (modifiers & GDK_MOD2_MASK)
  //  dfb_modifiers |= G_MODIFIERS_ALTGR;
  if (modifiers & GDK_CONTROL_MASK)
    dfb_modifiers |= G_MODIFIERS_CTRL;
  if (modifiers & GDK_SHIFT_MASK)
    dfb_modifiers |= G_MODIFIERS_SHIFT;

  return dfb_modifiers;
}

/**
 * gdk_test_render_sync
 * @window: a mapped GdkWindow
 *
 * This function retrives a pixel from @window to force the windowing
 * system to carry out any pending rendering commands.
 * This function is intended to be used to syncronize with rendering
 * pipelines, to benchmark windowing system rendering operations.
 **/
void
gdk_test_render_sync (GdkWindow *window)
{
  static GdkImage *p1image = NULL;
  /* syncronize to X drawing queue, see:
   * http://mail.gnome.org/archives/gtk-devel-list/2006-October/msg00103.html
   */
  p1image = gdk_drawable_copy_to_image (window, p1image, 0, 0, 0, 0, 1, 1);
  //_gdk_display->gix->WaitIdle (_gdk_display->gix);    
}

/**
 * gdk_test_simulate_key
 * @window: Gdk window to simulate a key event for.
 * @x:      x coordinate within @window for the key event.
 * @y:      y coordinate within @window for the key event.
 * @keyval: A Gdk keyboard value.
 * @modifiers: Keyboard modifiers the event is setup with.
 * @key_pressrelease: either %GDK_KEY_PRESS or %GDK_KEY_RELEASE
 *
 * This function is intended to be used in Gtk+ test programs.
 * If (@x,@y) are > (-1,-1), it will warp the mouse pointer to
 * the given (@x,@y) corrdinates within @window and simulate a
 * key press or release event.
 * When the mouse pointer is warped to the target location, use
 * of this function outside of test programs that run in their
 * own virtual windowing system (e.g. Xvfb) is not recommended.
 * If (@x,@y) are passed as (-1,-1), the mouse pointer will not
 * be warped and @window origin will be used as mouse pointer
 * location for the event.
 * Also, gtk_test_simulate_key() is a fairly low level function,
 * for most testing purposes, gtk_test_widget_send_key() is the
 * right function to call which will generate a key press event
 * followed by its accompanying key release event.
 *
 * Returns: wether all actions neccessary for a key event simulation were carried out successfully.
 **/
gboolean
gdk_test_simulate_key (GdkWindow      *window,
                       gint            x,
                       gint            y,
                       guint           keyval,
                       GdkModifierType modifiers,
                       GdkEventType    key_pressrelease)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  gi_msg_t         evt; 

  g_return_val_if_fail (GDK_IS_WINDOW(window), FALSE);
  g_return_val_if_fail (key_pressrelease == GDK_KEY_PRESS || key_pressrelease == GDK_KEY_RELEASE, FALSE);
  
  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);

  D_UNIMPLEMENTED;
#if 0 //FIXME DPP

  if (x >= 0 && y >= 0) {
    int win_x, win_y;
    impl->window->GetPosition (impl->window, &win_x, &win_y);
    if (_gdk_display->layer->WarpCursor (_gdk_display->layer, win_x+x, win_y+y))
      return FALSE;
  }
 
  evt.clazz      = DFEC_WINDOW;
  evt.type       = (key_pressrelease == GDK_KEY_PRESS) ? DWET_KEYDOWN : DWET_KEYUP;
  evt.flags      = DWEF_NONE;
  evt.window_id  = impl->drawable.window_id;
  evt.x          = MAX(x, 0);
  evt.y          = MAX(y, 0);
  _gdk_display->layer->GetCursorPosition (_gdk_display->layer, &evt.cx, &evt.cy);
  evt.key_code   = -1;
  evt.key_symbol = _gdk_keyval_to_gix (keyval);
  evt.modifiers  = _gdk_modifiers_to_gix (modifiers);
  evt.locks      = (modifiers & GDK_LOCK_MASK) ? DILS_CAPS : 0;
  gettimeofday (&evt.timestamp, NULL);

  //gi_send_event (impl->drawable.window_id,FALSE,GI_MASK_CLIENT_MSG,&evt);
  gi_post_keyboard_event(0,key->key_sym,TRUE,FALSE);
#endif
  return TRUE;
}

/**
 * gdk_test_simulate_button
 * @window: Gdk window to simulate a button event for.
 * @x:      x coordinate within @window for the button event.
 * @y:      y coordinate within @window for the button event.
 * @button: Number of the pointer button for the event, usually 1, 2 or 3.
 * @modifiers: Keyboard modifiers the event is setup with.
 * @button_pressrelease: either %GDK_BUTTON_PRESS or %GDK_BUTTON_RELEASE
 *
 * This function is intended to be used in Gtk+ test programs.
 * It will warp the mouse pointer to the given (@x,@y) corrdinates
 * within @window and simulate a button press or release event.
 * Because the mouse pointer needs to be warped to the target
 * location, use of this function outside of test programs that
 * run in their own virtual windowing system (e.g. Xvfb) is not
 * recommended.
 * Also, gtk_test_simulate_button() is a fairly low level function,
 * for most testing purposes, gtk_test_widget_click() is the right
 * function to call which will generate a button press event followed
 * by its accompanying button release event.
 *
 * Returns: wether all actions neccessary for a button event simulation were carried out successfully.
 **/
gboolean
gdk_test_simulate_button (GdkWindow      *window,
                          gint            x,
                          gint            y,
                          guint           button, /*1..3*/
                          GdkModifierType modifiers,
                          GdkEventType    button_pressrelease)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  gi_msg_t         evt;  
  
  g_return_val_if_fail (GDK_IS_WINDOW(window), FALSE);
  g_return_val_if_fail (button_pressrelease == GDK_BUTTON_PRESS || button_pressrelease == GDK_BUTTON_RELEASE, FALSE);
  
  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);
  D_UNIMPLEMENTED;

#if 0 //FIXME DPP
  if (x >= 0 && y >= 0) {
    int win_x, win_y;
    impl->window->GetPosition (impl->window, &win_x, &win_y);
    if (_gdk_display->layer->WarpCursor (_gdk_display->layer, win_x+x, win_y+y))
      return FALSE;
  }

  evt.clazz      = DFEC_WINDOW;
  evt.type       = (button_pressrelease == GDK_BUTTON_PRESS) ? DWET_BUTTONDOWN : DWET_BUTTONUP;
  evt.flags      = DWEF_NONE;
  evt.window_id  = impl->drawable.window_id;
  evt.x          = MAX(x, 0);
  evt.y          = MAX(y, 0); 
  _gdk_display->layer->GetCursorPosition (_gdk_display->layer, &evt.cx, &evt.cy);
  evt.modifiers  = _gdk_modifiers_to_gix (modifiers);
  evt.locks      = (modifiers & GDK_LOCK_MASK) ? DILS_CAPS : 0;
  gettimeofday (&evt.timestamp, NULL);

  //gi_send_event (impl->drawable.window_id,FALSE,GI_MASK_CLIENT_MSG,&evt);
  gi_post_mouse_event();
#endif
  return TRUE;
}

#define __GDK_TEST_UTILS_X11_C__
#include "gdkaliasdef.c"
#endif
