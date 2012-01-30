//
// "$Id: Fl_x.cxx 8764 2011-05-30 16:47:48Z manolo $"
//
// X specific code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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

#ifdef WIN32
//#  include "Fl_win32.cxx"
#elif defined(__APPLE__)
//#  include "Fl_mac.cxx"
#elif !defined(FL_DOXYGEN)

#  define CONSOLIDATE_MOTION 1
/**** Define this if your keyboard lacks a backspace key... ****/
/* #define BACKSPACE_HACK 1 */

#  include <config.h>
#  include <FL/Fl.H>
#  include <FL/x.H>
#  include <FL/Fl_Window.H>
#  include <FL/fl_utf8.h>
#  include <FL/Fl_Tooltip.H>
#  include <FL/fl_draw.H>
#  include <FL/Fl_Paged_Device.H>
#  include <stdio.h>
#  include <stdlib.h>
#  include "flstring.h"
#  include <unistd.h>
#  include <sys/time.h>
#include <gi/gi.h>
#include <gi/property.h>
#include <gi/region.h>
#include <gi/user_font.h>

static Fl_Xlib_Graphics_Driver fl_xlib_driver;
static Fl_Display_Device fl_xlib_display(&fl_xlib_driver);
FL_EXPORT Fl_Graphics_Driver *fl_graphics_driver = (Fl_Graphics_Driver*)&fl_xlib_driver; // the current target device of graphics operations
Fl_Surface_Device* Fl_Surface_Device::_surface = (Fl_Surface_Device*)&fl_xlib_display; // the current target surface of graphics operations
Fl_Display_Device *Fl_Display_Device::_display = &fl_xlib_display;// the platform display

////////////////////////////////////////////////////////////////
// interface to poll/select call:

#  if USE_POLL

#    include <poll.h>
static pollfd *pollfds = 0;

#  else
#    if HAVE_SYS_SELECT_H
#      include <sys/select.h>
#    endif /* HAVE_SYS_SELECT_H */

// The following #define is only needed for HP-UX 9.x and earlier:
//#define select(a,b,c,d,e) select((a),(int *)(b),(int *)(c),(int *)(d),(e))

static fd_set fdsets[3];
static int maxfd;
#    define POLLIN 1
#    define POLLOUT 4
#    define POLLERR 8

#  endif /* USE_POLL */

static int nfds = 0;
static int fd_array_size = 0;
struct FD {
#  if !USE_POLL
  int fd;
  short events;
#  endif
  void (*cb)(int, void*);
  void* arg;
};

static FD *fd = 0;

void Fl::add_fd(int n, int events, void (*cb)(int, void*), void *v) {
  remove_fd(n,events);
  int i = nfds++;
  if (i >= fd_array_size) {
    FD *temp;
    fd_array_size = 2*fd_array_size+1;

    if (!fd) temp = (FD*)malloc(fd_array_size*sizeof(FD));
    else temp = (FD*)realloc(fd, fd_array_size*sizeof(FD));

    if (!temp) return;
    fd = temp;

#  if USE_POLL
    pollfd *tpoll;

    if (!pollfds) tpoll = (pollfd*)malloc(fd_array_size*sizeof(pollfd));
    else tpoll = (pollfd*)realloc(pollfds, fd_array_size*sizeof(pollfd));

    if (!tpoll) return;
    pollfds = tpoll;
#  endif
  }
  fd[i].cb = cb;
  fd[i].arg = v;
#  if USE_POLL
  pollfds[i].fd = n;
  pollfds[i].events = events;
#  else
  fd[i].fd = n;
  fd[i].events = events;
  if (events & POLLIN) FD_SET(n, &fdsets[0]);
  if (events & POLLOUT) FD_SET(n, &fdsets[1]);
  if (events & POLLERR) FD_SET(n, &fdsets[2]);
  if (n > maxfd) maxfd = n;
#  endif
}

void Fl::add_fd(int n, void (*cb)(int, void*), void* v) {
  Fl::add_fd(n, POLLIN, cb, v);
}

void Fl::remove_fd(int n, int events) {
  int i,j;
# if !USE_POLL
  maxfd = -1; // recalculate maxfd on the fly
# endif
  for (i=j=0; i<nfds; i++) {
#  if USE_POLL
    if (pollfds[i].fd == n) {
      int e = pollfds[i].events & ~events;
      if (!e) continue; // if no events left, delete this fd
      pollfds[j].events = e;
    }
#  else
    if (fd[i].fd == n) {
      int e = fd[i].events & ~events;
      if (!e) continue; // if no events left, delete this fd
      fd[i].events = e;
    }
    if (fd[i].fd > maxfd) maxfd = fd[i].fd;
#  endif
    // move it down in the array if necessary:
    if (j<i) {
      fd[j] = fd[i];
#  if USE_POLL
      pollfds[j] = pollfds[i];
#  endif
    }
    j++;
  }
  nfds = j;
#  if !USE_POLL
  if (events & POLLIN) FD_CLR(n, &fdsets[0]);
  if (events & POLLOUT) FD_CLR(n, &fdsets[1]);
  if (events & POLLERR) FD_CLR(n, &fdsets[2]);
#  endif
}

void Fl::remove_fd(int n) {
  remove_fd(n, -1);
}

#if CONSOLIDATE_MOTION
static Fl_Window* send_motion;
extern Fl_Window* fl_xmousewin;
#endif
static bool in_a_window; // true if in any of our windows, even destroyed ones
static void do_queued_events(int n) {
  int err;
  in_a_window = true;
  
  while (n>0)
	{
    gi_msg_t xevent;

    //err = gi_get_message(&xevent, MSG_NO_WAIT);
	err = gi_next_message(&xevent);
	if (err>0)
		fl_handle(xevent);
	else
		break;
	n--;
  }
  // we send FL_LEAVE only if the mouse did not enter some other window:
  if (!in_a_window) Fl::handle(FL_LEAVE, 0);
#if CONSOLIDATE_MOTION
  else if (send_motion == fl_xmousewin) {
    send_motion = 0;
    Fl::handle(FL_MOVE, fl_xmousewin);
  }
#endif
}

// these pointers are set by the Fl::lock() function:
static void nothing() {}
void (*fl_lock_function)() = nothing;
void (*fl_unlock_function)() = nothing;

// This is never called with time_to_wait < 0.0:
// It should return negative on error, 0 if nothing happens before
// timeout, and >0 if any callbacks were done.
int fl_wait(double time_to_wait) {
	int n = 0;

  // OpenGL and other broken libraries call XEventsQueued
  // unnecessarily and thus cause the file descriptor to not be ready,
  // so we must check for already-read events:
  if (fl_display>0 && (n=gi_get_event_count())>0) {do_queued_events(n); return 1;}

#  if !USE_POLL
  fd_set fdt[3];
  fdt[0] = fdsets[0];
  fdt[1] = fdsets[1];
  fdt[2] = fdsets[2];
#  endif

  fl_unlock_function();

  if (time_to_wait < 2147483.648) {
#  if USE_POLL
    n = ::poll(pollfds, nfds, int(time_to_wait*1000 + .5));
#  else
    timeval t;

    t.tv_sec = int(time_to_wait);
    t.tv_usec = int(1000000 * (time_to_wait-t.tv_sec));
    n = ::select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],&t);
#  endif
  } else {
#  if USE_POLL
    n = ::poll(pollfds, nfds, -1);
#  else
   n = ::select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],0);
#  endif
  }

  fl_lock_function();

  if (n > 0) {
    for (int i=0; i<nfds; i++) {
#  if USE_POLL
      if (pollfds[i].revents) fd[i].cb(pollfds[i].fd, fd[i].arg);
#  else
      int f = fd[i].fd;
      short revents = 0;
      if (FD_ISSET(f,&fdt[0])) revents |= POLLIN;
      if (FD_ISSET(f,&fdt[1])) revents |= POLLOUT;
      if (FD_ISSET(f,&fdt[2])) revents |= POLLERR;

      if (fd[i].events & revents) fd[i].cb(f, fd[i].arg);
#  endif
    }
  }

  Fl::flush();
  return n;
}

// fl_ready() is just like fl_wait(0.0) except no callbacks are done:
int fl_ready() {
  if (gi_get_event_count()) return 1;
  if (!nfds) return 0; // nothing to select or poll
#  if USE_POLL
  return ::poll(pollfds, nfds, 0);
#  else
  timeval t;
  t.tv_sec = 0;
  t.tv_usec = 0;
  fd_set fdt[3];
  fdt[0] = fdsets[0];
  fdt[1] = fdsets[1];
  fdt[2] = fdsets[2];
  return ::select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],&t);
#  endif
}

// replace \r\n by \n
static void convert_crlf(unsigned char *string, long& len) {
  unsigned char *a, *b;
  a = b = string;
  while (*a) {
    if (*a == '\r' && a[1] == '\n') { a++; len--; }
    else *b++ = *a++;
  }
  *b = 0;
}

////////////////////////////////////////////////////////////////

int fl_display=0;
gi_window_id_t fl_message_window = 0;
int fl_screen;
gi_screen_info_t *fl_visual;
//Colormap fl_colormap;

char fl_is_over_the_spot = 0;

static gi_atom_id_t TARGETS;
static gi_atom_id_t CLIPBOARD;
gi_atom_id_t fl_XdndAware;
gi_atom_id_t fl_XdndSelection;
gi_atom_id_t fl_XdndEnter;
gi_atom_id_t fl_XdndTypeList;
gi_atom_id_t fl_XdndPosition;
gi_atom_id_t fl_XdndLeave;
gi_atom_id_t fl_XdndDrop;
gi_atom_id_t fl_XdndStatus;
gi_atom_id_t fl_XdndActionCopy;
gi_atom_id_t fl_XdndFinished;
//gi_atom_id_t fl_XdndProxy;
gi_atom_id_t fl_XdndURIList;
gi_atom_id_t fl_Xatextplainutf;
gi_atom_id_t fl_Xatextplain;
static gi_atom_id_t fl_XaText;
gi_atom_id_t fl_XaCompoundText;
gi_atom_id_t fl_XaTextUriList;

/*
  X defines 32-bit-entities to have a format value of max. 32,
  although sizeof(atom) can be 8 (64 bits) on a 64-bit OS.
  See also fl_open_display() for sizeof(atom) < 4.
  Used for XChangeProperty (see STR #2419).
*/
static int atom_bits = 32;

static void fd_callback(int,void *) {
  int n = gi_get_event_count();
  do_queued_events(n);
}

extern char *fl_get_font_xfld(int fnum, int size);


static XRectangle    spot;


void fl_reset_spot(void)
{
  spot.x = -1;
  spot.y = -1;
}

void fl_set_spot(int font, int size, int X, int Y, int W, int H, Fl_Window *win)
{
#if 1
 	gi_composition_form_t attr;

	attr.x = X;
	attr.y = Y -  win->labelsize();
	gi_ime_associate_window(fl_xid(win), &attr); 
#endif
}

void fl_set_status(int x, int y, int w, int h)
{
  
}


void fl_open_display(void) {
  if (fl_display) return;

  setlocale(LC_CTYPE, "");

  FD_ZERO(&fdsets[0]);
  FD_ZERO(&fdsets[1]);
  FD_ZERO(&fdsets[2]);
  maxfd = 0;
  //memset(fdsets,0,sizeof(fdsets));
 
  int d = gi_init();
  if (d<0) Fl::fatal("Can't open display");
 
  fl_open_display(1);
}

void fl_open_display(int d) {
  fl_display = d;

  //WM_DELETE_WINDOW      = gi_intern_atom( "WM_DELETE_WINDOW",    0);
  //WM_PROTOCOLS          = gi_intern_atom( "WM_PROTOCOLS",        0);
  //fl_MOTIF_WM_HINTS     = gi_intern_atom( "_MOTIF_WM_HINTS",     0);
  TARGETS               = gi_intern_atom( "TARGETS",             0);
  CLIPBOARD             = gi_intern_atom( "CLIPBOARD",           0);
  fl_XdndAware          = gi_intern_atom( "XdndAware",           0);
  fl_XdndSelection      = gi_intern_atom( "XdndSelection",       0);
  fl_XdndEnter          = gi_intern_atom( "XdndEnter",           0);
  fl_XdndTypeList       = gi_intern_atom( "XdndTypeList",        0);
  fl_XdndPosition       = gi_intern_atom( "XdndPosition",        0);
  fl_XdndLeave          = gi_intern_atom( "XdndLeave",           0);
  fl_XdndDrop           = gi_intern_atom( "XdndDrop",            0);
  fl_XdndStatus         = gi_intern_atom( "XdndStatus",          0);
  fl_XdndActionCopy     = gi_intern_atom( "XdndActionCopy",      0);
  fl_XdndFinished       = gi_intern_atom( "XdndFinished",        0);
  //fl_XdndProxy        = gi_intern_atom( "XdndProxy",           0);
  fl_XdndEnter          = gi_intern_atom( "XdndEnter",           0);
  fl_XdndURIList        = gi_intern_atom( "text/uri-list",       0);
  fl_Xatextplainutf     = gi_intern_atom( "text/plain;charset=UTF-8",0);
  fl_Xatextplain        = gi_intern_atom( "text/plain",          0);
  fl_XaText             = gi_intern_atom( "TEXT",                0);
  fl_XaCompoundText     = gi_intern_atom( "COMPOUND_TEXT",       0);
  fl_XaTextUriList      = gi_intern_atom( "text/uri-list",       0);

  if (sizeof(gi_atom_id_t) < 4)
    atom_bits = sizeof(gi_atom_id_t) * 8;

  Fl::add_fd(gi_get_connection_fd(), POLLIN, fd_callback);

  fl_message_window =
    gi_create_window( GI_DESKTOP_WINDOW_ID, 0,0,1,1,0, GI_FLAGS_NON_FRAME);

// construct an gi_screen_info_t that matches the default Visual:
  //gi_screen_info_t templt; int num;
  fl_visual = (gi_screen_info_t *)malloc(sizeof(gi_screen_info_t));
  gi_get_screen_info(fl_visual);
  //fl_colormap = DefaultColormap(d, fl_screen);

#if !USE_COLORMAP
  Fl::visual(FL_RGB);
#endif
}

void fl_close_display() {
  Fl::remove_fd(gi_get_connection_fd());
  gi_exit();
}

static int fl_workarea_xywh[4] = { -1, -1, -1, -1 };

static void fl_init_workarea() {
  fl_open_display();

  gi_atom_id_t _NET_WORKAREA = gi_intern_atom( "_WM_MAXIMIZE_COORD", 0);
  gi_atom_id_t actual;
  unsigned long count, remaining;
  int format;
  unsigned *xywh;

  if (gi_get_window_property( GI_DESKTOP_WINDOW_ID,
                         _NET_WORKAREA, 0, 4 * sizeof(unsigned), FALSE,
                         GA_CARDINAL, &actual, &format, &count, &remaining,
                         (unsigned char **)&xywh) || !xywh || !xywh[2] ||
                         !xywh[3])
  {
    fl_workarea_xywh[0] = 0;
    fl_workarea_xywh[1] = 0;
    fl_workarea_xywh[2] = gi_screen_width();
    fl_workarea_xywh[3] = gi_screen_height();
  }
  else
  {
    fl_workarea_xywh[0] = (int)xywh[0];
    fl_workarea_xywh[1] = (int)xywh[1];
    fl_workarea_xywh[2] = (int)xywh[2];
    fl_workarea_xywh[3] = (int)xywh[3];
    free(xywh);
  }
}

int Fl::x() {
  if (fl_workarea_xywh[0] < 0) fl_init_workarea();
  return fl_workarea_xywh[0];
}

int Fl::y() {
  if (fl_workarea_xywh[0] < 0) fl_init_workarea();
  return fl_workarea_xywh[1];
}

int Fl::w() {
  if (fl_workarea_xywh[0] < 0) fl_init_workarea();
  return fl_workarea_xywh[2];
}

int Fl::h() {
  if (fl_workarea_xywh[0] < 0) fl_init_workarea();
  return fl_workarea_xywh[3];
}

void Fl::get_mouse(int &xx, int &yy) {
  fl_open_display();
  gi_window_id_t root = GI_DESKTOP_WINDOW_ID;
  gi_window_id_t c; 
  int mx,my,cx,cy; 
  unsigned int mask;
  int err;
  err = gi_query_pointer(root, &c,&mx,&my,&cx,&cy,&mask);
  if (err < 0)
  {
  }
  xx = mx;
  yy = my;
}

////////////////////////////////////////////////////////////////
// Code used for paste and DnD into the program:

Fl_Widget *fl_selection_requestor;
char *fl_selection_buffer[2];
int fl_selection_length[2];
int fl_selection_buffer_length[2];
char fl_i_own_selection[2] = {0,0};

// Call this when a "paste" operation happens:
void Fl::paste(Fl_Widget &receiver, int clipboard) {
  if (fl_i_own_selection[clipboard]) {
    // We already have it, do it quickly without window server.
    // Notice that the text is clobbered if set_selection is
    // called in response to FL_PASTE!
    Fl::e_text = fl_selection_buffer[clipboard];
    Fl::e_length = fl_selection_length[clipboard];
    if (!Fl::e_text) Fl::e_text = (char *)"";
    receiver.handle(FL_PASTE);
    return;
  }
  // otherwise get the window server to return it:
  fl_selection_requestor = &receiver;
  gi_atom_id_t property = clipboard ? CLIPBOARD : GA_PRIMARY;
  gi_convert_selection( property, TARGETS, property,
                    fl_xid(Fl::first_window()), fl_event_time);
}

gi_window_id_t fl_dnd_source_window;
gi_atom_id_t *fl_dnd_source_types; // null-terminated list of data types being supplied
gi_atom_id_t fl_dnd_type;
gi_atom_id_t fl_dnd_source_action;
gi_atom_id_t fl_dnd_action;

void fl_sendClientMessage(gi_window_id_t window, gi_atom_id_t message,
                                 unsigned long d0,
                                 unsigned long d1=0,
                                 unsigned long d2=0,
                                 unsigned long d3=0,
                                 unsigned long d4=0)
{
  gi_msg_t e;
  uint32_t *data;

  printf("fl_sendClientMessage: send to %d\n", window);

  data = &e.params[0];
  e.type = GI_MSG_CLIENT_MSG;
  e.ev_window = window;
  e.body.client.client_type = message;
  e.body.client.client_format = 32;
  data[0] = (long)d0;
  data[1] = (long)d1;
  data[2] = (long)d2;
  data[3] = (long)d3;
  data[0] = (long)d4;
  gi_send_event( window, 0, 0, &e);
}


/**
 * convert the current mouse chord into the FLTK modifier state
 */
static int mods_to_key_state( long mods )
{
  int state = 0;
  if ( mods & G_MODIFIERS_NUMLOCK ) state |= FL_NUM_LOCK;
  if ( mods & G_MODIFIERS_META ) state |= FL_META;
  if ( mods & (G_MODIFIERS_ALT) ) state |= FL_ALT;
  if ( mods & G_MODIFIERS_CTRL ) state |= FL_CTRL;
  if ( mods & G_MODIFIERS_SHIFT ) state |= FL_SHIFT;
  if ( mods & G_MODIFIERS_CAPSLOCK ) state |= FL_CAPS_LOCK;
  return state;
}

static int mods_to_mouse_state( long mods ){
  int state = 0;
  if (mods & GI_BUTTON_L) state |= FL_BUTTON1;
  if (mods & GI_BUTTON_M) state |= FL_BUTTON2;
  if (mods & GI_BUTTON_R) state |= FL_BUTTON3;
  return state;
}


////////////////////////////////////////////////////////////////
// Code for copying to clipboard and DnD out of the program:

void Fl::copy(const char *stuff, int len, int clipboard) {
  if (!stuff || len<0) return;
  if (len+1 > fl_selection_buffer_length[clipboard]) {
    delete[] fl_selection_buffer[clipboard];
    fl_selection_buffer[clipboard] = new char[len+100];
    fl_selection_buffer_length[clipboard] = len+100;
  }
  memcpy(fl_selection_buffer[clipboard], stuff, len);
  fl_selection_buffer[clipboard][len] = 0; // needed for direct paste
  fl_selection_length[clipboard] = len;
  fl_i_own_selection[clipboard] = 1;
  gi_atom_id_t property = clipboard ? CLIPBOARD : GA_PRIMARY;
  gi_set_selection_owner( property, fl_message_window, fl_event_time);
}

////////////////////////////////////////////////////////////////

const gi_msg_t* fl_xevent; // the current x event
ulong fl_event_time; // the last timestamp from an x event

char fl_key_vector[32]; // used by Fl::get_key()

// Record event mouse position and state from an gi_msg_t:

static int px, py;
static ulong ptime;

static void set_event_xy(long key, int mouse) {
#  if CONSOLIDATE_MOTION
  send_motion = 0;
#  endif
  Fl::e_x_root  = fl_xevent->params[0];
  Fl::e_y_root  = fl_xevent->params[1];
  Fl::e_x       = fl_xevent->body.rect.x;
  Fl::e_y       = fl_xevent->body.rect.y;

  
  Fl::e_state   = mods_to_key_state(key); //
  Fl::e_state   |= mods_to_mouse_state(mouse); //fixme dpp
  fl_event_time = fl_xevent->time;

  // turn off is_click if enough time or mouse movement has passed:
  if (abs(Fl::e_x_root-px)+abs(Fl::e_y_root-py) > 3 ||
      fl_event_time >= ptime+1000)
    Fl::e_is_click = 0;
}

// if this is same event as last && is_click, increment click count:
static inline void checkdouble() {
  if (Fl::e_is_click == Fl::e_keysym)
    Fl::e_clicks++;
  else {
    Fl::e_clicks = 0;
    Fl::e_is_click = Fl::e_keysym;
  }
  px = Fl::e_x_root;
  py = Fl::e_y_root;
  ptime = fl_event_time;
}

static Fl_Window* resize_bug_fix;

////////////////////////////////////////////////////////////////

static char unknown[] = "<unknown>";
const int unknown_len = 10;

int gix2fltk(int vk) ;


static int get_mouse_button(int flags)
{
  if (flags & GI_BUTTON_L)
    return 1;
  if (flags & GI_BUTTON_M)
    return 2;
  if (flags & GI_BUTTON_R)
    return 3;
  if (flags & GI_BUTTON_WHEEL_UP)
    return 4;
  if (flags & GI_BUTTON_WHEEL_DOWN)
    return 5;
  return 0;
}



int fl_handle(const gi_msg_t& thisevent)
{
  gi_msg_t xevent = thisevent;
  fl_xevent = &thisevent;
  gi_window_id_t xid = xevent.ev_window;
  //static gi_window_id_t xim_win = 0;
  int xbutton;

  if(xevent.type == GI_MSG_SELECTIONNOTIFY)
  {
	switch (xevent.params[0])	
	{

    case G_SEL_NOTIFY: {
    if (!fl_selection_requestor) return 0;
    static unsigned char* buffer = 0;
    if (buffer) {free(buffer); buffer = 0;}
    long bytesread = 0;
    if (gi_message_notify_selection_property(fl_xevent)) for (;;) {
      // The Xdnd code pastes 64K chunks together, possibly to avoid
      // bugs in X servers, or maybe to avoid an extra round-trip to
      // get the property length.  I copy this here:
      gi_atom_id_t actual; int format; unsigned long count, remaining;
      unsigned char* portion = NULL;
      if (gi_get_window_property(
                             gi_message_notify_selection_requestor(fl_xevent),
                             gi_message_notify_selection_property(fl_xevent),
                             bytesread/4, 65536, 1, 0,
                             &actual, &format, &count, &remaining,
                             &portion)) break; // quit on error
      if (actual == TARGETS || actual == GA_ATOM) {
	gi_atom_id_t type = GA_STRING;
	for (unsigned i = 0; i<count; i++) {
	  gi_atom_id_t t = ((gi_atom_id_t*)portion)[i];
	    if (t == fl_Xatextplainutf ||
		  t == fl_Xatextplain ||
		  t == GA_UTF8_STRING) {type = t; break;}
	    // rest are only used if no utf-8 available:
	    if (t == fl_XaText ||
		  t == fl_XaTextUriList ||
		  t == fl_XaCompoundText) type = t;
	}
	free(portion);
	gi_atom_id_t property = gi_message_notify_selection_property(&xevent);
	gi_convert_selection( property, type, property,
	      fl_xid(Fl::first_window()),
	      fl_event_time);
	return true;
      }
      // Make sure we got something sane...
      if ((portion == NULL) || (format != 8) || (count == 0)) {
	if (portion) free(portion);
        return true;
	}
      buffer = (unsigned char*)realloc(buffer, bytesread+count+remaining+1);
      memcpy(buffer+bytesread, portion, count);
      free(portion);
      bytesread += count;
      // Cannot trust data to be null terminated
      buffer[bytesread] = '\0';
      if (!remaining) break;
    }
    if (buffer) {
      buffer[bytesread] = 0;
      convert_crlf(buffer, bytesread);
    }
    Fl::e_text = buffer ? (char*)buffer : (char *)"";
    Fl::e_length = bytesread;
    int old_event = Fl::e_number;
    fl_selection_requestor->handle(Fl::e_number = FL_PASTE);
    Fl::e_number = old_event;
    // Detect if this paste is due to Xdnd by the property name (I use
    // GA_SECONDARY for that) and send an XdndFinished message. It is not
    // clear if this has to be delayed until now or if it can be done
    // immediatly after calling XConvertSelection.
    if (gi_message_notify_selection_property(fl_xevent) == GA_SECONDARY &&
        fl_dnd_source_window) {
      fl_sendClientMessage(fl_dnd_source_window, fl_XdndFinished,
                           gi_message_notify_selection_requestor(fl_xevent));
      fl_dnd_source_window = 0; // don't send a second time
    }
    return 1;}

  case G_SEL_CLEAR: {
    int clipboard = gi_message_clear_selection_property(fl_xevent) == CLIPBOARD;
    fl_i_own_selection[clipboard] = 0;
    return 1;}

  case G_SEL_REQUEST: {
    gi_msg_t e;
    e.type = GI_MSG_SELECTIONNOTIFY;
    e.params[0] = G_SEL_NOTIFY;
    gi_message_notify_selection_requestor(&e) = gi_message_request_selection_requestor(fl_xevent);
    gi_message_notify_selection_selection(&e) = gi_message_request_selection_selection(fl_xevent);
    int clipboard = gi_message_notify_selection_selection(&e) == CLIPBOARD;
    gi_message_notify_selection_target(&e) = gi_message_request_selection_target(fl_xevent);
    gi_message_notify_selection_time(&e) = gi_message_request_selection_time(fl_xevent);
    gi_message_notify_selection_property(&e) = gi_message_request_selection_property(fl_xevent);
    if (gi_message_notify_selection_target(&e) == TARGETS) {
      gi_atom_id_t a[3] = {GA_UTF8_STRING, GA_STRING, fl_XaText};
      gi_change_property( gi_message_notify_selection_requestor(&e), gi_message_notify_selection_property(&e),
                      GA_ATOM, atom_bits, 0, (unsigned char*)a, 3);
    } else if (/*gi_message_notify_selection_target(&e) == GA_STRING &&*/ fl_selection_length[clipboard]) {
    if (gi_message_notify_selection_target(&e) == GA_UTF8_STRING ||
	     gi_message_notify_selection_target(&e) == GA_STRING ||
	     gi_message_notify_selection_target(&e) == fl_XaCompoundText ||
	     gi_message_notify_selection_target(&e) == fl_XaText ||
	     gi_message_notify_selection_target(&e) == fl_Xatextplain ||
	     gi_message_notify_selection_target(&e) == fl_Xatextplainutf) {
	// clobber the target type, this seems to make some applications
	// behave that insist on asking for GA_TEXT instead of UTF8_STRING
	// Does not change GA_STRING as that breaks xclipboard.
	if (gi_message_notify_selection_target(&e) != GA_STRING) gi_message_notify_selection_target(&e) = GA_UTF8_STRING;
	gi_change_property( gi_message_notify_selection_requestor(&e), gi_message_notify_selection_property(&e),
			 gi_message_notify_selection_target(&e), 8, 0,
			 (unsigned char *)fl_selection_buffer[clipboard],
			 fl_selection_length[clipboard]);
      }
    } else {
//    char* x = gi_get_atom_name(fl_display,gi_message_notify_selection_target(&e));
//    fprintf(stderr,"selection request of %s\n",x);
//    free(x);
      gi_message_notify_selection_property(&e) = 0;
    }
    gi_send_event( gi_message_notify_selection_requestor(&e), 0, 0, (gi_msg_t *)&e);}
    return 1;

  // events where interesting window id is in a different place:  
  }

  }


  int event = 0;
  Fl_Window* window = fl_find(xid);

  if (window) switch (xevent.type) {

    case GI_MSG_WINDOW_DESTROY: { // an X11 window was closed externally from the program
      Fl::handle(FL_CLOSE, window);
      Fl_X* X = Fl_X::i(window);
      if (X) { // indicates the FLTK window was not closed
	X->xid = (gi_window_id_t)0; // indicates the X11 window was already destroyed
	window->hide();
	int oldx = window->x(), oldy = window->y();
	window->position(0, 0);
	window->position(oldx, oldy);
	window->show(); // recreate the X11 window in support of the FLTK window
	}
      return 1;
    }
  case GI_MSG_CLIENT_MSG: {
    gi_atom_id_t message = fl_xevent->body.client.client_type;
    const uint32_t* data = (const uint32_t*)fl_xevent->params;
    if ((gi_atom_id_t)(data[0]) == GA_WM_DELETE_WINDOW) {
      event = FL_CLOSE;
    } else if (message == fl_XdndEnter) {
      fl_xmousewin = window;
      in_a_window = true;
      fl_dnd_source_window = data[0];
      // version number is data[1]>>24
//      printf("XdndEnter, version %ld\n", data[1] >> 24);
      if (data[1]&1) {
        // get list of data types:
        gi_atom_id_t actual; int format; unsigned long count, remaining;
        unsigned char *buffer = 0;
        gi_get_window_property( fl_dnd_source_window, fl_XdndTypeList,
                           0, 0x8000000L, FALSE, GA_ATOM, &actual, &format,
                           &count, &remaining, &buffer);
        if (actual != GA_ATOM || format != 32 || count<4 || !buffer)
          goto FAILED;
        delete [] fl_dnd_source_types;
        fl_dnd_source_types = new gi_atom_id_t[count+1];
        for (unsigned i = 0; i < count; i++) {
          fl_dnd_source_types[i] = ((gi_atom_id_t*)buffer)[i];
        }
        fl_dnd_source_types[count] = 0;
      } else {
      FAILED:
        // less than four data types, or if the above messes up:
        if (!fl_dnd_source_types) fl_dnd_source_types = new gi_atom_id_t[4];
        fl_dnd_source_types[0] = data[2];
        fl_dnd_source_types[1] = data[3];
        fl_dnd_source_types[2] = data[4];
        fl_dnd_source_types[3] = 0;
      }

      // Loop through the source types and pick the first text type...
      int i;

      for (i = 0; fl_dnd_source_types[i]; i ++)
      {
//        printf("fl_dnd_source_types[%d] = %ld (%s)\n", i,
//             fl_dnd_source_types[i],
//             gi_get_atom_name(fl_display, fl_dnd_source_types[i]));

        if (!strncmp(gi_get_atom_name( fl_dnd_source_types[i]),
                     "text/", 5))
          break;
      }

      if (fl_dnd_source_types[i])
        fl_dnd_type = fl_dnd_source_types[i];
      else
        fl_dnd_type = fl_dnd_source_types[0];

      event = FL_DND_ENTER;
      Fl::e_text = unknown;
      Fl::e_length = unknown_len;
      break;

    } else if (message == fl_XdndPosition) {
      fl_xmousewin = window;
      in_a_window = true;
      fl_dnd_source_window = data[0];
      Fl::e_x_root = data[2]>>16;
      Fl::e_y_root = data[2]&0xFFFF;
      if (window) {
        Fl::e_x = Fl::e_x_root-window->x();
        Fl::e_y = Fl::e_y_root-window->y();
      }
      fl_event_time = data[3];
      fl_dnd_source_action = data[4];
      fl_dnd_action = fl_XdndActionCopy;
      Fl::e_text = unknown;
      Fl::e_length = unknown_len;
      int accept = Fl::handle(FL_DND_DRAG, window);
      fl_sendClientMessage(data[0], fl_XdndStatus,
                           fl_xevent->ev_window,
                           accept ? 1 : 0,
                           0, // used for xy rectangle to not send position inside
                           0, // used for width+height of rectangle
                           accept ? fl_dnd_action : 0);
      return 1;

    } else if (message == fl_XdndLeave) {
      fl_dnd_source_window = 0; // don't send a finished message to it
      event = FL_DND_LEAVE;
      Fl::e_text = unknown;
      Fl::e_length = unknown_len;
      break;

    } else if (message == fl_XdndDrop) {
      fl_xmousewin = window;
      in_a_window = true;
      fl_dnd_source_window = data[0];
      fl_event_time = data[2];
      gi_window_id_t to_window = fl_xevent->ev_window;
      Fl::e_text = unknown;
      Fl::e_length = unknown_len;
      if (Fl::handle(FL_DND_RELEASE, window)) {
        fl_selection_requestor = Fl::belowmouse();
        gi_convert_selection( fl_XdndSelection,
                          fl_dnd_type, GA_SECONDARY,
                          to_window, fl_event_time);
      } else {
        // Send the finished message if I refuse the drop.
        // It is not clear whether I can just send finished always,
        // or if I have to wait for the SelectionNotify event as the
        // code is currently doing.
        fl_sendClientMessage(fl_dnd_source_window, fl_XdndFinished, to_window);
        fl_dnd_source_window = 0;
      }
      return 1;

    }
    break;}

  case GI_MSG_WINDOW_HIDE:
    event = FL_HIDE;
    break;

  case GI_MSG_EXPOSURE:
	  {
	Fl_X *i = Fl_X::i(window);
    i->wait_for_expose = 0;
	  }

  case GI_MSG_GRAPHICSEXPOSURE:
	//window->clear_damage((uchar)(window->damage()|FL_DAMAGE_EXPOSE));
    window->damage(FL_DAMAGE_EXPOSE, xevent.body.rect.x, xevent.body.rect.y,
                   xevent.body.rect.w, xevent.body.rect.h);	
	//i->flush();
    return 1;

  case GI_MSG_FOCUS_IN:
	  {
#if 1
	gi_composition_form_t attr;
	gi_window_info_t winfo;

	gi_get_window_info(xevent.ev_window,&winfo);

	attr.x = winfo.x  + 10;
	attr.y = winfo.y + winfo.height - 30;
	gi_ime_associate_window(xevent.ev_window, &attr);
#endif	
    event = FL_FOCUS;
	  }
    break;

  case GI_MSG_FOCUS_OUT:
    Fl::flush(); // it never returns to main loop when deactivated...
	gi_ime_associate_window(xevent.ev_window,NULL);
    event = FL_UNFOCUS;
    break;

  case GI_MSG_KEY_DOWN:
  case GI_MSG_KEY_UP: {
	  ulong state;	  

  //KEYPRESS:
    int keycode = xevent.params[3];
    fl_key_vector[keycode/8] |= (1 << (keycode%8));
    static char *buffer = NULL;
    static int buffer_len = 0;
    int len;
    KeySym keysym;
    if (buffer_len == 0) {
      buffer_len = 4096;
      buffer = (char*) malloc(buffer_len);
    }

	keysym = keycode;
    if (xevent.type == GI_MSG_KEY_DOWN) { //down
      int len = 0;
      event = FL_KEYBOARD;//FL_KEYDOWN;
     
		if (xevent.attribute_1 && xevent.params[3] > 127)
		{
		memset(buffer,0,buffer_len);
		len = fl_utf8encode(keycode, buffer);
		}
		else
		 {
			
			Fl::e_keysym = Fl::e_original_keysym = gix2fltk(keycode);
			buffer[0] = keycode;
			len = 1;        
      }
      // MRS: Can't use Fl::event_state(FL_CTRL) since the state is not
      //      set until set_event_xy() is called later...
      //if ((xevent.xkey.state & ControlMask) && keysym == '-') buffer[0] = 0x1f; // ^_
      buffer[len] = 0;
      Fl::e_text = buffer;
      Fl::e_length = len;
	  state = Fl::e_state & 0xff000000;
	  state |= mods_to_key_state(xevent.body.message[3]);
	  Fl::e_state = state;
    } else { //up  
	
	  state = Fl::e_state & 0xff000000;
	  state |= mods_to_key_state(xevent.body.message[3]);

      event = FL_KEYUP;
	  Fl::e_keysym = Fl::e_original_keysym = gix2fltk(keycode);
			buffer[0] = keycode;
			len = 1; 
      fl_key_vector[keycode/8] &= ~(1 << (keycode%8));

	  Fl::e_state = state;
      // keyup events just get the unshifted keysym:
    }  

    //set_event_xy();
    Fl::e_is_click = 0;

  }
    break;

  case GI_MSG_BUTTON_DOWN:
	xbutton = get_mouse_button(xevent.params[2]);
    Fl::e_keysym = FL_Button + xbutton;
    
    if (xbutton == 4) {
      Fl::e_dy = -1; // Up
      event = FL_MOUSEWHEEL;
    } else if (xbutton == 5) {
      Fl::e_dy = +1; // Down
      event = FL_MOUSEWHEEL;
    } else {
      //Fl::e_state |= (FL_BUTTON1 << (xbutton-1));
	  set_event_xy(xevent.body.message[3], xevent.params[2]);
      event = FL_PUSH;
	  //gi_set_focus_window(xevent.ev_window);
      checkdouble();
    }

    fl_xmousewin = window;
    in_a_window = true;
    break;

  case GI_MSG_MOUSE_MOVE:
    set_event_xy(xevent.params[3], xevent.params[2]);
#  if CONSOLIDATE_MOTION
    send_motion = fl_xmousewin = window;
    in_a_window = true;
    return 0;
#  else
    event = FL_MOVE;
    fl_xmousewin = window;
    in_a_window = true;
    break;
#  endif

  case GI_MSG_BUTTON_UP:
	xbutton = get_mouse_button(xevent.params[3]);
    Fl::e_keysym = FL_Button + xbutton;
    set_event_xy(xevent.body.message[3], xevent.params[3]);
	//Fl::e_state = mods_to_mouse_state(xevent.body.message[3];
    Fl::e_state &= ~(FL_BUTTON1 << (xbutton-1));
    if (xbutton == 4 ||
        xbutton == 5) return 0;
    event = FL_RELEASE;

    fl_xmousewin = window;
    in_a_window = true;
    break;

  case GI_MSG_MOUSE_ENTER:
    //set_event_xy();
    //Fl::e_state = xevent.xcrossing.state << 16; //fixme
    event = FL_ENTER;

    fl_xmousewin = window;
    in_a_window = true;   
    break;

  case GI_MSG_MOUSE_EXIT:
    //if (xevent.xcrossing.detail == NotifyInferior) break;
    //set_event_xy();
    //Fl::e_state = xevent.xcrossing.state << 16;
    fl_xmousewin = 0;
    in_a_window = false; // make do_queued_events produce FL_LEAVE event
    return 0;

  // We cannot rely on the x,y position in the configure notify event.
  // I now think this is an unavoidable problem with X: it is impossible
  // for a window manager to prevent the "real" notify event from being
  // sent when it resizes the contents, even though it can send an
  // artificial event with the correct position afterwards (and some
  // window managers do not send this fake event anyway)
  // So anyway, do a round trip to find the correct x,y:
  case GI_MSG_WINDOW_SHOW:
    event = FL_SHOW;
  //printf("GI_MSG_WINDOW_SHOW event got for %d\n", xevent.ev_window);
  break;

  case GI_MSG_CONFIGURENOTIFY: {
    if (window->parent()) break; // ignore child windows

	if (xevent.params[0] > GI_STRUCT_CHANGE_RESIZE )
	{
	break;
	}

    // figure out where OS really put window
    gi_window_info_t actual;
    gi_get_window_info( fl_xid(window), &actual);
    gi_window_id_t cr; int X, Y, W = actual.width, H = actual.height;
    gi_translate_coordinates( fl_xid(window), GI_DESKTOP_WINDOW_ID,
                          0, 0, &X, &Y, &cr);

	//printf("CONFIGURENOTIFY: got line %d (%d,%d %d,%d)\n",__LINE__,
	//	X,Y, xevent.body.rect.x, xevent.body.rect.y);
    // tell Fl_Window about it and set flag to prevent echoing:
    resize_bug_fix = window;
    window->resize(X, Y, W, H);
    break; // allow add_handler to do something too
    }

  case GI_MSG_REPARENT: {
    int xpos, ypos;
    gi_window_id_t junk;

    // on some systems, the ReparentNotify event is not handled as we would expect.
   // XErrorHandler oldHandler = XSetErrorHandler(catchXExceptions());

    //ReparentNotify gives the new position of the window relative to
    //the new parent. FLTK cares about the position on the root window.
	gi_window_id_t parent = xevent.params[1];
    gi_translate_coordinates( parent,
                          GI_DESKTOP_WINDOW_ID,
                          xevent.body.rect.x, xevent.body.rect.y,
                          &xpos, &ypos, &junk);
    // tell Fl_Window about it and set flag to prevent echoing:
    if ( 1 ) {
      resize_bug_fix = window;
      window->position(xpos, ypos);
    }
    break;
    }
  }

  return Fl::handle(event, window);
}

////////////////////////////////////////////////////////////////

void Fl_Window::resize(int X,int Y,int W,int H) {
  int is_a_move = (X != x() || Y != y());
  int is_a_resize = (W != w() || H != h());
  int is_a_enlarge = (W > w() || H > h());
  int resize_from_program = (this != resize_bug_fix);
  if (!resize_from_program) resize_bug_fix = 0;
  if (is_a_move && resize_from_program) set_flag(FORCE_POSITION);
  else if (!is_a_resize && !is_a_move) return;
  if (is_a_resize) {
    Fl_Group::resize(X,Y,W,H);
    if (shown()) {redraw(); if(is_a_enlarge) i->wait_for_expose = 1;}
  } else {
    x(X); y(Y);
  }

  if (resize_from_program && is_a_resize && !resizable()) {
    size_range(w(), h(), w(), h());
  }

  if (resize_from_program && shown()) {
    if (is_a_resize) {
      if (!resizable()) size_range(w(),h(),w(),h());
      if (is_a_move) {
        gi_move_window( i->xid, X, Y);
        gi_resize_window( i->xid,  W>0 ? W : 1, H>0 ? H : 1);
      } else {
        gi_resize_window( i->xid, W>0 ? W : 1, H>0 ? H : 1);
      }
    } else
      gi_move_window( i->xid, X, Y);
  }
}

////////////////////////////////////////////////////////////////

// A subclass of Fl_Window may call this to associate an X window it
// creates with the Fl_Window:

void fl_fix_focus(); // in Fl.cxx

Fl_X* Fl_X::set_xid(Fl_Window* win, gi_window_id_t winxid) {
  Fl_X* xp = new Fl_X;
  xp->xid = winxid;
  xp->other_xid = 0;
  xp->setwindow(win);
  xp->next = Fl_X::first;
  xp->region = 0;
  xp->wait_for_expose = 1;
  xp->backbuffer_bad = 1;
  Fl_X::first = xp;
  if (win->modal()) {Fl::modal_ = win; fl_fix_focus();}
  return xp;
}

// More commonly a subclass calls this, because it hides the really
// ugly parts of X and sets all the stuff for a window that is set
// normally.  The global variables like fl_show_iconic are so that
// subclasses of *that* class may change the behavior...

char fl_show_iconic = 0;    // hack for iconize()
int fl_background_pixel = -1; // hack to speed up bg box drawing
int fl_disable_transient_for = 0; // secret method of removing TRANSIENT_FOR

static const int childEventMask = GI_MASK_EXPOSURE;
//KeymapStateMask
static const int XEventMask = GI_MASK_SELECTIONNOTIFY | GI_MASK_PROPERTYNOTIFY |
	  GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT| GI_MASK_MOUSE_MOVE|
	  GI_MASK_FOCUS_IN | GI_MASK_EXPOSURE | GI_MASK_BUTTON_DOWN|
	  GI_MASK_BUTTON_UP|GI_MASK_KEY_UP|GI_MASK_KEY_DOWN|
	   GI_MASK_CLIENT_MSG| GI_MASK_CONFIGURENOTIFY | GI_MASK_REPARENT;

	   //GI_MASK_WINDOW_HIDE|GI_MASK_WINDOW_SHOW |

void Fl_X::make_xid(Fl_Window* win, gi_screen_info_t *visual)
{
  Fl_Group::current(0); // get rid of very common user bug: forgot end()
  long bgcolor=0, style = 0;
  long event_mask = XEventMask;
  gi_window_id_t newwin;

  bgcolor = GI_RGB(240,240,242);

  int X = win->x();
  int Y = win->y();
  int W = win->w();
  if (W <= 0) W = 1; // X don't like zero...
  int H = win->h();
  if (H <= 0) H = 1; // X don't like zero...
  if (!win->parent() && !Fl::grab()) {
    // center windows in case window manager does not do anything:
#ifdef FL_CENTER_WINDOWS
    if (!(win->flags() & Fl_Widget::FORCE_POSITION)) {
      win->x(X = scr_x+(scr_w-W)/2);
      win->y(Y = scr_y+(scr_h-H)/2);
    }
#endif // FL_CENTER_WINDOWS

    // force the window to be on-screen.  Usually the X window manager
    // does this, but a few don't, so we do it here for consistency:
    int scr_x, scr_y, scr_w, scr_h;
    Fl::screen_xywh(scr_x, scr_y, scr_w, scr_h, X, Y);

    if (win->border()) {
      // ensure border is on screen:
      // (assume extremely minimal dimensions for this border)
      const int top = 20;
      const int left = 1;
      const int right = 1;
      const int bottom = 1;
      if (X+W+right > scr_x+scr_w) X = scr_x+scr_w-right-W;
      if (X-left < scr_x) X = scr_x+left;
      if (Y+H+bottom > scr_y+scr_h) Y = scr_y+scr_h-bottom-H;
      if (Y-top < scr_y) Y = scr_y+top;
    }
    // now insure contents are on-screen (more important than border):
    if (X+W > scr_x+scr_w) X = scr_x+scr_w-W;
    if (X < scr_x) X = scr_x;
    if (Y+H > scr_y+scr_h) Y = scr_y+scr_h-H;
    if (Y < scr_y) Y = scr_y;
  }

  // if the window is a subwindow and our parent is not mapped yet, we
  // mark this window visible, so that mapping the parent at a later
  // point in time will call this function again to finally map the subwindow.
  if (win->parent() && !Fl_X::i(win->window())) {
    win->set_visible();
    return;
  }

  ulong root = win->parent() ?
    fl_xid(win->window()) : GI_DESKTOP_WINDOW_ID;
  
  if (win->override()) {
	  style|=(GI_FLAGS_TEMP_WINDOW);
	   printf("override got ################\n");    
  } //else attr.override_redirect = 0;
 
  if (!win->resizable())
  {
	  style |=  GI_FLAGS_NORESIZE;
  }

  if (win->modal()){
	  style |= ( GI_FLAGS_MENU_WINDOW | GI_FLAGS_TEMP_WINDOW);
  }
  else if (!win->border()) {
	style |= (GI_FLAGS_NON_FRAME);
  }//



  if (fl_background_pixel >= 0) {
    //bgcolor = fl_background_pixel;
    fl_background_pixel = -1;
    //mask |= CWBackPixel;
  }


  if (win->menu_window() || win->tooltip_window()) {
	  style |= (GI_FLAGS_NON_FRAME|GI_FLAGS_MENU_WINDOW);
  }

  //uchar r,g,b;
  //get_rgb_color( fl_xpixel(fl_color()), r, g, b );
  //get_rgb_color( fl_background_pixel, r, g, b );
  //bgcolor = GI_RGB(r,g,b);

  newwin = gi_create_window( root,
                               X, Y, W, H,
                               bgcolor, // borderwidth
                               style);  

  Fl_X* xp =  set_xid(win, newwin);
  gi_set_events_mask(newwin, event_mask);
  gi_set_window_background(newwin,WINDOW_COLOR,GI_BG_USE_NONE);

  //printf("%s: newwin=%d, root=%d %lx\n",__FUNCTION__, newwin, root, style);
  int showit = 1;
  int override_redirect = 0;

  if (!win->parent() && !override_redirect) {
    // Communicate all kinds 'o junk to the X gi_window_id_t Manager:

    win->label(win->label(), win->iconlabel());
    
    // send size limits and border:
    xp->sendxjunk();

#if 0
    // set the class property, which controls the icon used:
    if (win->xclass()) {
      char buffer[1024];
      char *p; const char *q;
      // truncate on any punctuation, because they break XResource lookup:
      for (p = buffer, q = win->xclass(); isalnum(*q)||(*q&128);) *p++ = *q++;
      *p++ = 0;
      // create the capitalized version:
      q = buffer;
      *p = toupper(*q++); if (*p++ == 'X') *p++ = toupper(*q++);
      while ((*p++ = *q++));
      gi_change_property( xp->xid, GA_WM_CLASS, GA_STRING, 8, 0,
                      (unsigned char *)buffer, p-buffer-1);
    }
#endif

#if 0
    if (win->non_modal() && xp->next && !fl_disable_transient_for) {
      // find some other window to be "transient for":
      Fl_Window* wp = xp->next->w;
      while (wp->parent()) wp = wp->window();
      //XSetTransientForHint(fl_display, xp->xid, fl_xid(wp));
      if (!wp->visible()) showit = 0; // guess that wm will not show it
    }

    // Make sure that borderless windows do not show in the task bar
    if (!win->border()) {
      gi_atom_id_t net_wm_state = gi_intern_atom ( "_NET_WM_STATE", 0);
      gi_atom_id_t net_wm_state_skip_taskbar = gi_intern_atom ( "_NET_WM_STATE_SKIP_TASKBAR", 0);
      gi_change_property( xp->xid, net_wm_state, GA_ATOM, 32,
          G_PROP_MODE_Append, (unsigned char*) &net_wm_state_skip_taskbar, 1);
    }
#endif

    // Make it receptive to DnD:
    long version = 4;
    gi_change_property( xp->xid, fl_XdndAware,
                    GA_ATOM, sizeof(int)*8, 0, (unsigned char*)&version, 1);

   
    if (fl_show_iconic) {      
      fl_show_iconic = 0;
      showit = 0;
    }
    if (win->icon()) {
		gi_image_t *img;
		img = (gi_image_t *)win->icon();
		gi_set_window_icon(xp->xid,(uint32_t*)img->rgba,img->w,img->h);      
    }
   
  }

#if 0
  // set the window type for menu and tooltip windows to avoid animations (compiz)
  if (win->menu_window() || win->tooltip_window()) {
    gi_atom_id_t net_wm_type = gi_intern_atom( "_NET_WM_WINDOW_TYPE", FALSE);
    gi_atom_id_t net_wm_type_kind = gi_intern_atom( "_NET_WM_WINDOW_TYPE_MENU", FALSE);
    gi_change_property( xp->xid, net_wm_type, GA_ATOM, 32, G_PROP_MODE_Replace, (unsigned char*)&net_wm_type_kind, 1);
  }
#endif

   XMapWindow(fl_display, xp->xid); 
  if (showit) {
    win->set_visible();
    int old_event = Fl::e_number;
    win->handle(Fl::e_number = FL_SHOW); // get child windows to appear
    Fl::e_number = old_event;
    win->redraw();
  }
}

////////////////////////////////////////////////////////////////
// Send X window stuff that can be changed over time:

void Fl_X::sendxjunk() {
  if (w->parent() || w->override()) return; // it's not a window manager window!

  if (!w->size_range_set) { // default size_range based on resizable():
    if (w->resizable()) {
      Fl_Widget *o = w->resizable();
      int minw = o->w(); if (minw > 100) minw = 100;
      int minh = o->h(); if (minh > 100) minh = 100;
      w->size_range(w->w() - o->w() + minw, w->h() - o->h() + minh, 0, 0);
    } else {
      w->size_range(w->w(), w->h(), w->w(), w->h());
    }
    return; // because this recursively called here
  }

  //fixme dpp

}

void Fl_Window::size_range_() {
  size_range_set = 1;
  if (shown()) i->sendxjunk();
}

////////////////////////////////////////////////////////////////

// returns pointer to the filename, or null if name ends with '/'
const char *fl_filename_name(const char *name) {
  const char *p,*q;
  if (!name) return (0);
  for (p=q=name; *p;) if (*p++ == '/') q = p;
  return q;
}

void Fl_Window::label(const char *name,const char *iname) {
  Fl_Widget::label(name);
  iconlabel_ = iname;
  if (shown() && !parent()) {
    if (!name) {
		name = "";
	}
	else{
	gi_set_window_utf8_caption(i->xid, name);
	//printf("set window name %s\n",name);
	}
    if (!iname) iname = fl_filename_name(name);
	gi_set_window_icon_name(i->xid, iname);
    }
}

////////////////////////////////////////////////////////////////
// Implement the virtual functions for the base Fl_Window class:

// If the box is a filled rectangle, we can make the redisplay *look*
// faster by using X's background pixel erasing.  We can make it
// actually *be* faster by drawing the frame only, this is done by
// setting fl_boxcheat, which is seen by code in fl_drawbox.cxx:
//
// On XFree86 (and prehaps all X's) this has a problem if the window
// is resized while a save-behind window is atop it.  The previous
// contents are restored to the area, but this assumes the area
// is cleared to background color.  So this is disabled in this version.
// Fl_Window *fl_boxcheat;
static inline int can_boxcheat(uchar b) {return (b==1 || ((b&2) && b<=15));}

void Fl_Window::show() {
  image(Fl::scheme_bg_);
  if (Fl::scheme_bg_) {
    labeltype(FL_NORMAL_LABEL);
    align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
  } else {
    labeltype(FL_NO_LABEL);
  }
  Fl_Tooltip::exit(this);
  if (!shown()) {
    fl_open_display();
    // Don't set background pixel for double-buffered windows...
    if (type() == FL_WINDOW && can_boxcheat(box())) {
      fl_background_pixel = int(fl_xpixel(color()));
    }
    Fl_X::make_xid(this);
  } else {
    gi_raise_window( i->xid);
	 XMapWindow(fl_display, i->xid); 
  }

  
#ifdef USE_PRINT_BUTTON
void preparePrintFront(void);
preparePrintFront();
#endif
}

gi_window_id_t fl_window;
Fl_Window *Fl_Window::current_;
gi_gc_ptr_t fl_gc;

// make X drawing go into this window (called by subclass flush() impl.)
void Fl_Window::make_current() {
  static gi_gc_ptr_t gc; // the gi_gc_ptr_t used by all X windows
  if (!gc) gc = gi_create_gc(i->xid, NULL);
  fl_window = i->xid;
  fl_gc = gc;
  current_ = this;
  fl_clip_region(0);

#ifdef FLTK_USE_CAIRO
  // update the cairo_t context
  if (Fl::cairo_autolink_context()) Fl::cairo_make_current(this);
#endif
}

gi_window_id_t fl_xid_(const Fl_Window *w) {
  Fl_X *temp = Fl_X::i(w);
  return temp ? temp->xid : 0;
}

static void decorated_win_size(Fl_Window *win, int &w, int &h)
{
  w = win->w();
  h = win->h();
  if (!win->shown() || win->parent() || !win->border() || !win->visible()) return;
  gi_window_id_t root = GI_DESKTOP_WINDOW_ID, parent, *children;
  int n = 0;
  int status = gi_query_child_tree( Fl_X::i(win)->xid,  &parent, &children, &n); 
  if (status != 0 && n) free(children);
  // when compiz is used, root and parent are the same window 
  // and I don't know where to find the window decoration
  if (status == 0 || root == parent) return; 
  gi_window_info_t attributes;
  gi_get_window_info( parent, &attributes);
  w = attributes.width;
  h = attributes.height;
}

int Fl_Window::decorated_h()
{
  int w, h;
  decorated_win_size(this, w, h);
  return h;
}

int Fl_Window::decorated_w()
{
  int w, h;
  decorated_win_size(this, w, h);
  return w;
}

void Fl_Paged_Device::print_window(Fl_Window *win, int x_offset, int y_offset)
{
  if (!win->shown() || win->parent() || !win->border() || !win->visible()) {
    this->print_widget(win, x_offset, y_offset);
    return;
  }
  Fl_Display_Device::display_device()->set_current();
  win->show();
  Fl::check();
  win->make_current();
  gi_window_id_t root = GI_DESKTOP_WINDOW_ID, parent, *children, child_win, from;
  int n = 0;
  int bx, bt, do_it;
  from = fl_window;
  do_it = (gi_query_child_tree( fl_window, &parent, &children, &n) != 0 && 
	   gi_translate_coordinates( fl_window, parent, 0, 0, &bx, &bt, &child_win) == TRUE);
  if (n) free(children);
  // hack to bypass STR #2648: when compiz is used, root and parent are the same window 
  // and I don't know where to find the window decoration
  if (do_it && root == parent) do_it = 0; 
  if (!do_it) {
    this->set_current();
    this->print_widget(win, x_offset, y_offset);
    return;
  }
  fl_window = parent;
  uchar *top_image = 0, *left_image = 0, *right_image = 0, *bottom_image = 0;
  top_image = fl_read_image(NULL, 0, 0, - (win->w() + 2 * bx), bt);
  if (bx) {
    left_image = fl_read_image(NULL, 0, bt, -bx, win->h() + bx);
    right_image = fl_read_image(NULL, win->w() + bx, bt, -bx, win->h() + bx);
    bottom_image = fl_read_image(NULL, 0, bt + win->h(), -(win->w() + 2*bx), bx);
  }
  fl_window = from;
  this->set_current();
  if (top_image) {
    fl_draw_image(top_image, x_offset, y_offset, win->w() + 2 * bx, bt, 3);
    delete[] top_image;
  }
  if (bx) {
    if (left_image) fl_draw_image(left_image, x_offset, y_offset + bt, bx, win->h() + bx, 3);
    if (right_image) fl_draw_image(right_image, x_offset + win->w() + bx, y_offset + bt, bx, win->h() + bx, 3);
    if (bottom_image) fl_draw_image(bottom_image, x_offset, y_offset + bt + win->h(), win->w() + 2*bx, bx, 3);
    if (left_image) delete[] left_image;
    if (right_image) delete[] right_image;
    if (bottom_image) delete[] bottom_image;
  }
  this->print_widget( win, x_offset + bx, y_offset + bt );
}

#ifdef USE_PRINT_BUTTON
// to test the Fl_Printer class creating a "Print front window" button in a separate window
// contains also preparePrintFront call above
#include <FL/Fl_Printer.H>
#include <FL/Fl_Button.H>
void printFront(Fl_Widget *o, void *data)
{
  Fl_Printer printer;
  o->window()->hide();
  Fl_Window *win = Fl::first_window();
  if(!win) return;
  int w, h;
  if( printer.start_job(1) ) { o->window()->show(); return; }
  if( printer.start_page() ) { o->window()->show(); return; }
  printer.printable_rect(&w,&h);
  // scale the printer device so that the window fits on the page
  float scale = 1;
  int ww = win->decorated_w();
  int wh = win->decorated_h();
  if (ww > w || wh > h) {
    scale = (float)w/ww;
    if ((float)h/wh < scale) scale = (float)h/wh;
    printer.scale(scale, scale);
  }

// #define ROTATE 20.0
#ifdef ROTATE
  printer.scale(scale * 0.8, scale * 0.8);
  printer.printable_rect(&w, &h);
  printer.origin(w/2, h/2 );
  printer.rotate(ROTATE);
  printer.print_widget( win, - win->w()/2, - win->h()/2 );
  //printer.print_window_part( win, 0,0, win->w(), win->h(), - win->w()/2, - win->h()/2 );
#else  
  printer.print_window(win);
#endif

  printer.end_page();
  printer.end_job();
  o->window()->show();
}

void preparePrintFront(void)
{
  static int first=1;
  if(!first) return;
  first=0;
  static Fl_Window w(0,0,150,30);
  static Fl_Button b(0,0,w.w(),w.h(), "Print front window");
  b.callback(printFront);
  w.end();
  w.show();
}
#endif // USE_PRINT_BUTTON

#endif

//
// End of "$Id: Fl_x.cxx 8764 2011-05-30 16:47:48Z manolo $".
//
