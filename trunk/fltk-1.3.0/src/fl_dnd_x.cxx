//
// "$Id: fl_dnd_x.cxx 7992 2010-12-09 21:52:07Z manolo $"
//
// Drag & Drop code for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#include "flstring.h"


extern gi_atom_id_t fl_XdndAware;
extern gi_atom_id_t fl_XdndSelection;
extern gi_atom_id_t fl_XdndEnter;
extern gi_atom_id_t fl_XdndTypeList;
extern gi_atom_id_t fl_XdndPosition;
extern gi_atom_id_t fl_XdndLeave;
extern gi_atom_id_t fl_XdndDrop;
extern gi_atom_id_t fl_XdndStatus;
extern gi_atom_id_t fl_XdndActionCopy;
extern gi_atom_id_t fl_XdndFinished;
//extern gi_atom_id_t fl_XdndProxy;
extern gi_atom_id_t fl_XdndURIList;

extern char fl_i_own_selection[2];
extern char *fl_selection_buffer[2];

extern void fl_sendClientMessage(gi_window_id_t window, gi_atom_id_t message,
                                 unsigned long d0,
                                 unsigned long d1=0,
                                 unsigned long d2=0,
                                 unsigned long d3=0,
                                 unsigned long d4=0);

// return version # of Xdnd this window supports.  Also change the
// window to the proxy if it uses a proxy:
static int dnd_aware(gi_window_id_t& window) {
  gi_atom_id_t actual; int format; unsigned long count, remaining;
  unsigned char *data = 0;
  gi_get_window_property( window, fl_XdndAware,
		     0, 4, FALSE, GA_ATOM,
		     &actual, &format,
		     &count, &remaining, &data);
  if (actual == GA_ATOM && format==32 && count && data)
    return int(*(gi_atom_id_t*)data);
  return 0;
}

static int grabfunc(int event) {
  if (event == FL_RELEASE) Fl::pushed(0);
  return 0;
}

extern int (*fl_local_grab)(int); // in Fl.cxx

// send an event to an fltk window belonging to this program:
static int local_handle(int event, Fl_Window* window) {
  fl_local_grab = 0;
  Fl::e_x = Fl::e_x_root-window->x();
  Fl::e_y = Fl::e_y_root-window->y();
  int ret = Fl::handle(event,window);
  fl_local_grab = grabfunc;
  return ret;
}

int Fl::dnd() {
  Fl_Window *source_fl_win = Fl::first_window();
  Fl::first_window()->cursor(FL_CURSOR_MOVE);
  gi_window_id_t source_window = fl_xid(Fl::first_window());
  fl_local_grab = grabfunc;
  gi_window_id_t target_window = 0;
  Fl_Window* local_window = 0;
  int dndversion = 4; int dest_x, dest_y;
  gi_set_selection_owner( fl_XdndSelection, fl_message_window, fl_event_time);

  while (Fl::pushed()) {

    // figure out what window we are pointing at:
    gi_window_id_t new_window = 0; int new_version = 0;
    Fl_Window* new_local_window = 0;
	gi_window_id_t child = 0;
	int err;

    for (child = GI_DESKTOP_WINDOW_ID;;) {
      gi_window_id_t root= GI_DESKTOP_WINDOW_ID; 
	  unsigned int junk3;

	  child = GI_DESKTOP_WINDOW_ID;

	  //fixme dpp
      err = gi_query_pointer( root,  &child,
		    &e_x_root, &e_y_root, &dest_x, &dest_y, &junk3);
	  if (err < 0)
	  {
	  printf("line %d: dnd go loop fail (%d,%d)\n",__LINE__,  e_x_root,e_y_root);
	  }

	  if (junk3 & (GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M) == 0)
	  {
	  printf("line %d: dnd child %d go loop (%d,%d)\n",__LINE__, child, e_x_root,e_y_root);
	  }

      if (!child) {
	if (!new_window && (new_version = dnd_aware(root))) new_window = root;
	break;
      }

      new_window = child;
      if ((new_local_window = fl_find(child))) break;
      if ((new_version = dnd_aware(new_window))) break;


	  usleep(100000);
    }//end loop


    if (new_window != target_window) {

      if (local_window) {
	local_handle(FL_DND_LEAVE, local_window);
      } else if (dndversion) {
	fl_sendClientMessage(target_window, fl_XdndLeave, source_window);
      }

      dndversion = new_version;
      target_window = new_window;
      local_window = new_local_window;

      if (local_window) {
	local_handle(FL_DND_ENTER, local_window);
      } else if (dndversion) {

        // Send an X-DND message to the target window.  In order to
	// support dragging of files/URLs as well as arbitrary text,
	// we look at the selection buffer - if the buffer starts
	// with a common URI scheme, does not contain spaces, and
	// contains at least one CR LF, then we flag the data as
	// both a URI list (MIME media type "text/uri-list") and
	// plain text.  Otherwise, we just say it is plain text.
        if ((!strncmp(fl_selection_buffer[0], "file:///", 8) ||
	     !strncmp(fl_selection_buffer[0], "ftp://", 6) ||
	     !strncmp(fl_selection_buffer[0], "http://", 7) ||
	     !strncmp(fl_selection_buffer[0], "https://", 8) ||
	     !strncmp(fl_selection_buffer[0], "ipp://", 6) ||
	     !strncmp(fl_selection_buffer[0], "ldap:", 5) ||
	     !strncmp(fl_selection_buffer[0], "mailto:", 7) ||
	     !strncmp(fl_selection_buffer[0], "news:", 5) ||
	     !strncmp(fl_selection_buffer[0], "smb://", 6)) &&
	    !strchr(fl_selection_buffer[0], ' ') &&
	    strstr(fl_selection_buffer[0], "\r\n")) {
	  // Send file/URI list...
	  fl_sendClientMessage(target_window, fl_XdndEnter, source_window,
			       dndversion<<24, fl_XdndURIList, GA_STRING, 0);
        } else {
	  // Send plain text...
	  fl_sendClientMessage(target_window, fl_XdndEnter, source_window,
			       dndversion<<24, GA_UTF8_STRING, 0, 0);
	}

      } //dnd version

    } //new window not target window

    if (local_window) {
      local_handle(FL_DND_DRAG, local_window);
    } else if (dndversion) {
      fl_sendClientMessage(target_window, fl_XdndPosition, source_window,
			   0, (e_x_root<<16)|e_y_root, fl_event_time,
			   fl_XdndActionCopy);
    }

    Fl::wait();
  } //end of pushed

  if (local_window) {
    fl_i_own_selection[0] = 1;
    if (local_handle(FL_DND_RELEASE, local_window)) paste(*belowmouse(), 0);
  } else if (dndversion) {
    fl_sendClientMessage(target_window, fl_XdndDrop, source_window,
			 0, fl_event_time);
  } else if (target_window) {
    // fake a drop by clicking the middle mouse button:
    gi_msg_t msg;
    msg.type = GI_MSG_BUTTON_DOWN;
    msg.ev_window = target_window;
    //msg.root = GI_DESKTOP_WINDOW_ID;
    msg.effect_window = 0;
    msg.time = fl_event_time+1;
    msg.body.rect.x = dest_x;
    msg.body.rect.y = dest_y;
    msg.params[0] = Fl::e_x_root;
    msg.params[1] = Fl::e_y_root;
    //msg.state = 0x0;
    msg.params[2] = GI_BUTTON_M;
    gi_send_event( target_window, FALSE, 0L, (gi_msg_t*)&msg);
    msg.time++;
    //msg.state = 0x200; //fixme dpp
    msg.type = GI_MSG_BUTTON_UP;
	msg.params[3] = GI_BUTTON_M;
    gi_send_event( target_window, FALSE, 0L, (gi_msg_t*)&msg);
  }

  fl_local_grab = 0;
  source_fl_win->cursor(FL_CURSOR_DEFAULT);
  return 1;
}


//
// End of "$Id: fl_dnd_x.cxx 7992 2010-12-09 21:52:07Z manolo $".
//
