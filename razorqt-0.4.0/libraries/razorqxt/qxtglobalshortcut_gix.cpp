/* BEGIN_COMMON_COPYRIGHT_HEADER
 ****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtCore module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************
 * END_COMMON_COPYRIGHT_HEADER */

#include "qxtglobalshortcut_p.h"
#include <gi/gi.h>
//static QxtGlobalShortcutPrivate::hotid;
bool QxtGlobalShortcutPrivate::eventFilter(void* message)
{
    gi_msg_t* event = static_cast<gi_msg_t*>(message);
    if (event->type == GI_MSG_KEY_DOWN && event->attribute_1)
    {
        //XKeyEvent* key = (XKeyEvent*) event;
        activateShortcut(event->params[3], 
            // Mod1Mask == Alt, Mod4Mask == Meta
            event->body.message[3]);
    }
    return false;
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    // ShiftMask, LockMask, ControlMask, Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, and Mod5Mask
    quint32 native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= G_MODIFIERS_SHIFT;
    if (modifiers & Qt::ControlModifier)
        native |= G_MODIFIERS_CTRL;
    if (modifiers & Qt::AltModifier)
        native |= G_MODIFIERS_ALT;
    if (modifiers & Qt::MetaModifier)
        native |= G_MODIFIERS_META;
    // TODO: resolve these?
    //if (modifiers & Qt::KeypadModifier)
    //if (modifiers & Qt::GroupSwitchModifier)
    return native;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key)
{
   switch (key)
    {
    case Qt::Key_Escape:
        return G_KEY_ESCAPE;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return G_KEY_TAB;
    case Qt::Key_Backspace:
        return G_KEY_BACKSPACE;
    case Qt::Key_Return:
        return G_KEY_RETURN;
    case Qt::Key_Enter:
        return G_KEY_ENTER;
    case Qt::Key_Insert:
        return G_KEY_INSERT;
    case Qt::Key_Delete:
        return G_KEY_DELETE;
    case Qt::Key_Pause:
        return G_KEY_PAUSE;
    case Qt::Key_Print:
        return G_KEY_PRINT;
//  case Qt::Key_Clear:
//      return G_KEY_CLEAR;
    case Qt::Key_Home:
        return G_KEY_HOME;
    case Qt::Key_End:
        return G_KEY_END;
    case Qt::Key_Left:
        return G_KEY_LEFT;
    case Qt::Key_Up:
        return G_KEY_UP;
    case Qt::Key_Right:
        return G_KEY_RIGHT;
    case Qt::Key_Down:
        return G_KEY_DOWN;
    case Qt::Key_PageUp:
        return G_KEY_PAGEUP;
    case Qt::Key_PageDown:
        return G_KEY_PAGEDOWN;
    case Qt::Key_F1:
        return G_KEY_F1;
    case Qt::Key_F2:
        return G_KEY_F2;
    case Qt::Key_F3:
        return G_KEY_F3;
    case Qt::Key_F4:
        return G_KEY_F4;
    case Qt::Key_F5:
        return G_KEY_F5;
    case Qt::Key_F6:
        return G_KEY_F6;
    case Qt::Key_F7:
        return G_KEY_F7;
    case Qt::Key_F8:
        return G_KEY_F8;
    case Qt::Key_F9:
        return G_KEY_F9;
    case Qt::Key_F10:
        return G_KEY_F10;
    case Qt::Key_F11:
        return G_KEY_F11;
    case Qt::Key_F12:
        return G_KEY_F12;
    case Qt::Key_Space:
        return G_KEY_SPACE;
/*    case Qt::Key_Asterisk:
        return G_KEY_MULTIPLY;
    case Qt::Key_Plus:
        return G_KEY_ADD;
    case Qt::Key_Comma:
        return G_KEY_SEPARATOR;
    case Qt::Key_Minus:
        return G_KEY_SUBTRACT;
    case Qt::Key_Slash:
        return G_KEY_DIVIDE;
    case Qt::Key_MediaNext:
        return G_KEY_MEDIA_NEXT_TRACK;
    case Qt::Key_MediaPrevious:
        return G_KEY_MEDIA_PREV_TRACK;
    case Qt::Key_MediaPlay:
        return G_KEY_MEDIA_PLAY_PAUSE;
    case Qt::Key_MediaStop:
        return G_KEY_MEDIA_STOP;
        // couldn't find those in G_KEY_*
        //case Qt::Key_MediaLast:
        //case Qt::Key_MediaRecord:
    case Qt::Key_VolumeDown:
        return G_KEY_VOLUME_DOWN;
    case Qt::Key_VolumeUp:
        return G_KEY_VOLUME_UP;
    case Qt::Key_VolumeMute:
        return G_KEY_VOLUME_MUTE;
*/
        // numbers
    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4:
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9:
        return key;

        // letters
    case Qt::Key_A:
    case Qt::Key_B:
    case Qt::Key_C:
    case Qt::Key_D:
    case Qt::Key_E:
    case Qt::Key_F:
    case Qt::Key_G:
    case Qt::Key_H:
    case Qt::Key_I:
    case Qt::Key_J:
    case Qt::Key_K:
    case Qt::Key_L:
    case Qt::Key_M:
    case Qt::Key_N:
    case Qt::Key_O:
    case Qt::Key_P:
    case Qt::Key_Q:
    case Qt::Key_R:
    case Qt::Key_S:
    case Qt::Key_T:
    case Qt::Key_U:
    case Qt::Key_V:
    case Qt::Key_W:
    case Qt::Key_X:
    case Qt::Key_Y:
    case Qt::Key_Z:
        return key;

    default:
        return 0;
    }

	return key;
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
	int hotid =0;
	//return false;
	hotid = gi_register_hotkey(GI_DESKTOP_WINDOW_ID, nativeMods, nativeKey);
	return true;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
	//return false;
	int hotid =0;
    
	//gi_unregister_hotkey(hotid);
	return true;
}

