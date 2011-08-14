//
// "$Id: Fl_get_key_mac.cxx 8624 2011-04-26 17:28:10Z manolo $"
//
// MacOS keyboard state routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// Return the current state of a key.  Keys are named by fltk symbols,
// which are actually X keysyms.  So this has to translate to macOS
// symbols.

#include <FL/Fl.H>
#include <FL/x.H>
#include <config.h>
#include <ctype.h>

extern char fl_key_vector[32]; // in Fl_x.cxx


// convert a MSWindows G_KEY_x to an Fltk (X) Keysym:
// See also the inverse converter in Fl_get_key_win32.cxx
// This table is in numeric order by VK:
static const struct {unsigned short vk, fltk;} vktab[] = {
  {G_KEY_BACKSPACE,	FL_BackSpace},
  {G_KEY_TAB,	FL_Tab},
  {G_KEY_CLEAR,	FL_KP+'5'},
  {G_KEY_RETURN,	FL_Enter},
  {G_KEY_ENTER,	FL_Enter},
  {G_KEY_LSHIFT,	FL_Shift_L},
  {G_KEY_RSHIFT,		FL_Shift_R},
  {G_KEY_LCTRL,	FL_Control_L},
  {G_KEY_RCTRL,	FL_Control_R},
  {G_KEY_LALT,	FL_Alt_L},
  {G_KEY_RALT,	FL_Alt_R},
  {G_KEY_PAUSE,	FL_Pause},
  {G_KEY_CAPSLOCK,	FL_Caps_Lock},
  {G_KEY_ESCAPE,	FL_Escape},
  {G_KEY_SPACE,	' '},
  {G_KEY_PAGEUP,	FL_Page_Up},
  {G_KEY_PAGEDOWN,	FL_Page_Down},
  {G_KEY_END,	FL_End},
  {G_KEY_HOME,	FL_Home},
  {G_KEY_LEFT,	FL_Left},
  {G_KEY_UP,	FL_Up},
  {G_KEY_RIGHT,	FL_Right},
  {G_KEY_DOWN,	FL_Down},
  {G_KEY_PRINT,	FL_Print},	// does not work on NT
  {G_KEY_INSERT,	FL_Insert},
  {G_KEY_DELETE,	FL_Delete},
  {G_KEY_LMETA,	FL_Meta_L},
  {G_KEY_RMETA,	FL_Meta_R},
  {G_KEY_MENU,	FL_Menu},
  //{G_KEY_SLEEP, FL_Sleep},
  {G_KEY_ASTERISK,	FL_KP+'*'},
  {G_KEY_PLUS,	FL_KP+'+'},
  {G_KEY_MINUS,	FL_KP+'-'},
  {G_KEY_PERIOD,	FL_KP+'.'},
  {G_KEY_SLASH,	FL_KP+'/'},
  {G_KEY_NUMLOCK,	FL_Num_Lock},
  {G_KEY_SCROLLOCK,	FL_Scroll_Lock},
/*
# if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0500)
  {G_KEY_BROWSER_BACK, FL_Back},
  {G_KEY_BROWSER_FORWARD, FL_Forward},
  {G_KEY_BROWSER_REFRESH, FL_Refresh},
  {G_KEY_BROWSER_STOP, FL_Stop},
  {G_KEY_BROWSER_SEARCH, FL_Search},
  {G_KEY_BROWSER_FAVORITES, FL_Favorites},
  {G_KEY_BROWSER_HOME, FL_Home_Page},
  {G_KEY_VOLUME_MUTE, FL_Volume_Mute},
  {G_KEY_VOLUME_DOWN, FL_Volume_Down},
  {G_KEY_VOLUME_UP, FL_Volume_Up},
  {G_KEY_MEDIA_NEXT_TRACK, FL_Media_Next},
  {G_KEY_MEDIA_PREV_TRACK, FL_Media_Prev},
  {G_KEY_MEDIA_STOP, FL_Media_Stop},
  {G_KEY_MEDIA_PLAY_PAUSE, FL_Media_Play},
  {G_KEY_LAUNCH_MAIL, FL_Mail},
#endif
  {0xba,	';'},
  {0xbb,	'='},
  {0xbc,	','},
  {0xbd,	'-'},
  {0xbe,	'.'},
  {0xbf,	'/'},
  {0xc0,	'`'},
  {0xdb,	'['},
  {0xdc,	'\\'},
  {0xdd,	']'},
  {0xde,	'\''}
*/
};

static unsigned short vklut[G_KEY_LAST]={0,};

int fltk2gix(int flkey) 
{
  if (!vklut[1]) { // init the table
    int i;
    for (i = 0; i < 256; i++) vklut[i] = tolower(i);
    for (i=G_KEY_F1; i<=G_KEY_F15; i++) 
		vklut[i] = i+(FL_F-(G_KEY_F1-1));
    for (i=G_KEY_KP0; i<=G_KEY_KP9; i++) 
		vklut[i] = i+(FL_KP+'0'-G_KEY_KP0);
    for (i = 0; i < sizeof(vktab)/sizeof(*vktab); i++) {
      vklut[vktab[i].vk] = vktab[i].fltk;
    }
    
  }

  int i;
  for (i=0; i<G_KEY_LAST; i++)
  {
	  if (vklut[i] == flkey)
	  {
		  return i;
	  }
  }
  return 0;
}

int gix2fltk(int vk) 
{
  //static unsigned short extendedlut[256];

  if (!vklut[1]) { // init the table
    int i;
    for (i = 0; i < 256; i++) vklut[i] = tolower(i);
    for (i=G_KEY_F1; i<=G_KEY_F15; i++) 
		vklut[i] = i+(FL_F-(G_KEY_F1-1));
    for (i=G_KEY_KP0; i<=G_KEY_KP9; i++) 
		vklut[i] = i+(FL_KP+'0'-G_KEY_KP0);
    for (i = 0; i < sizeof(vktab)/sizeof(*vktab); i++) {
      vklut[vktab[i].vk] = vktab[i].fltk;
      //extendedlut[vktab[i].vk] = vktab[i].extended;
    }
    //for (i = 0; i < 256; i++) 
	//	if (!extendedlut[i]) extendedlut[i] = vklut[i];
  }
  return vklut[vk];
}


/*
int
XKeysymToKeycode( KeySym ks)
{
	DFBInputDeviceKeymapEntry entry;
	IDirectFBInputDevice* kbd = 0;
	int code = 0, id = fltk2id(ks);
	fl_dfb->GetInputDevice(fl_dfb, keyboard_id, &kbd);
	for (int i = 0; i < 255; i++) {
		kbd->GetKeymapEntry(kbd, i, &entry);
		if (id != 0){
			if (entry.identifier==id) {
				code = entry.code;
				break;
			}
		} else {
			for (int j = 0; j <= DIKSI_LAST; j++) {
				if (entry.symbols[j] == (int)ks) {
					code = entry.code;
					break;
				}
			}
		}
	}
	release(kbd);
	return code; 
}

*/

int Fl::event_key(int k) {
  if (k > FL_Button && k <= FL_Button+8)
    return Fl::event_state(8<<(k-FL_Button));
  int i;

    i = fltk2gix( k);
  if (i==0) return 0;
  return fl_key_vector[i/8] & (1 << (i%8));
}


int
XQueryKeymap( char s[32])
{
    return 0;
} 


int Fl::get_key(int k) {
  fl_open_display();
  XQueryKeymap( fl_key_vector);
  return event_key(k);
}

