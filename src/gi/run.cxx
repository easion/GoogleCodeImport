
#define CONSOLIDATE_MOTION 0 // this was 1 in fltk 1.0

#define DEBUG_SELECTIONS 0
#define DEBUG_TABLET 0

#include <config.h>
#include <fltk/x.h>
#include <fltk/Window.h>
#include <fltk/Style.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <locale.h>
#include <fltk/visual.h>
#include <fltk/Font.h>
#include <fltk/Browser.h>
#include <fltk/utf.h>



using namespace fltk;

////////////////////////////////////////////////////////////////
// interface to poll/select call:

#if USE_POLL

#include <poll.h>
static pollfd *pollfds = 0;

#else

#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

// The following #define is only needed for HP-UX 9.x and earlier:
//#define select(a,b,c,d,e) select((a),(int *)(b),(int *)(c),(int *)(d),(e))

static fd_set fdsets[3];
static int maxfd;
#define POLLIN 1
#define POLLOUT 4
#define POLLERR 8

#endif /* USE_POLL */

static int nfds = 0;
static int fd_array_size = 0;
static struct FD {
#if !USE_POLL
  int fd;
  short events;
#endif
  void (*cb)(int, void*);
  void* arg;
} *fd = 0;

////////////////////////////////////////////////////////////////
#if USE_GI_IMM

#else // !USE_GI_IMM
void fl_set_spot(fltk::Font *f, Widget *w, int x, int y) {}
#endif

/*!

  Add file descriptor fd to listen to. When the fd becomes ready for
  reading fltk::wait() will call the callback function and then
  return. The callback is passed the fd and the arbitrary void*
  argument.

  The second argument is a bitfield to indicate when the callback
  should be done. You can or these together to make the callback be
  called for multiple conditions:
  - fltk::READ - Call the callback when there is data to be read.
  - fltk::WRITE - Call the callback when data can be written without blocking.
  - fltk::EXCEPT - Call the callback if an exception occurs on the file.

  Under UNIX any file descriptor can be monitored (files, devices,
  pipes, sockets, etc.) Due to limitations in Microsoft Windows, WIN32
  applications can only monitor sockets (? and is the when value
  ignored?)
*/
void fltk::add_fd(int n, int events, FileHandler cb, void *v) {
  remove_fd(n,events);
  int i = nfds++;
  if (i >= fd_array_size) {
    fd_array_size = 2*fd_array_size+1;
    fd = (FD*)realloc(fd, fd_array_size*sizeof(FD));
#if USE_POLL
    pollfds = (pollfd*)realloc(pollfds, fd_array_size*sizeof(pollfd));
#endif
  }
  fd[i].cb = cb;
  fd[i].arg = v;
#if USE_POLL
  pollfds[i].fd = n;
  pollfds[i].events = events;
#else
  fd[i].fd = n;
  fd[i].events = events;
  if (events & POLLIN) FD_SET(n, &fdsets[0]);
  if (events & POLLOUT) FD_SET(n, &fdsets[1]);
  if (events & POLLERR) FD_SET(n, &fdsets[2]);
  if (n > maxfd) maxfd = n;
#endif
}

/*! Same as add_fd(fd, READ, cb, v); */
void fltk::add_fd(int fd, FileHandler cb, void* v) {
  add_fd(fd, POLLIN, cb, v);
}

/*!
  Remove all the callbacks (ie for all different when values) for the
  given file descriptor. It is harmless to call this if there are no
  callbacks for the file descriptor. If when is given then those bits
  are removed from each callback for the file descriptor, and the
  callback removed only if all of the bits turn off.
*/
void fltk::remove_fd(int n, int events) {
  int i,j;
#if !USE_POLL
  maxfd = 0;
#endif
  for (i=j=0; i<nfds; i++) {
#if USE_POLL
    if (pollfds[i].fd == n) {
      int e = pollfds[i].events & ~events;
      if (!e) continue; // if no events left, delete this fd
      pollfds[j].events = e;
    }
#else
    if (fd[i].fd == n) {
      int e = fd[i].events & ~events;
      if (!e) continue; // if no events left, delete this fd
      fd[i].events = e;
    }
#endif
    // move it down in the array if necessary:
    if (j<i) {
#if USE_POLL
      pollfds[j] = pollfds[i];
#else
      fd[j] = fd[i];
#endif
    }
#if !USE_POLL
    if (fd[j].fd > maxfd) maxfd = fd[j].fd;
#endif
    j++;
  }
  nfds = j;
#if !USE_POLL
  if (events & POLLIN) FD_CLR(n, &fdsets[0]);
  if (events & POLLOUT) FD_CLR(n, &fdsets[1]);
  if (events & POLLERR) FD_CLR(n, &fdsets[2]);
#endif
}

#if CONSOLIDATE_MOTION
static Window* send_motion;
#endif
static bool in_a_window; // true if in any of our windows, even destroyed ones
static void do_queued_events(int, void*) {
  in_a_window = true;
  while (!exit_modal_ && gi_get_event_count()>0) {
    gi_next_message( &xevent);
    handle();
  }
  // we send LEAVE only if the mouse did not enter some other window:
  if (!in_a_window) handle(LEAVE, 0);
#if CONSOLIDATE_MOTION
  else if (send_motion == xmousewin) {
    send_motion = 0;
    handle(MOVE, xmousewin);
  }
#endif
}

// these pointers are set by the lock() function:
static void nothing() {}
void (*fl_lock_function)() = nothing;
void (*fl_unlock_function)() = nothing;

// Wait up to the given time for any events or sockets to become ready,
// do the callbacks for the events and sockets:
static inline int fl_wait(float time_to_wait) {

  // OpenGL and other broken libraries call XEventsQueued()
  // and thus cause the file descriptor to not be ready,
  // so we must check for already-read events:
  if ( gi_get_event_count()>0) {do_queued_events(0,0); return 1;}

#if !USE_POLL
  fd_set fdt[3];
  fdt[0] = fdsets[0];
  fdt[1] = fdsets[1];
  fdt[2] = fdsets[2];
#endif

  fl_unlock_function();
#if USE_POLL
  int n = ::poll(pollfds, nfds,
		 (time_to_wait<2147483.648f) ? int(time_to_wait*1000+.5f) : -1);
#else
  int n;
  if (time_to_wait < 2147483.648f) {
    timeval t;
    t.tv_sec = int(time_to_wait);
    t.tv_usec = int(1000000 * (time_to_wait-float(t.tv_sec)));
    n = ::select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],&t);
  } else {
    n = ::select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],0);
  }
#endif
  fl_lock_function();

  if (n > 0) {
    for (int i=0; i<nfds; i++) {
#if USE_POLL
      if (pollfds[i].revents) fd[i].cb(pollfds[i].fd, fd[i].arg);
#else
      int f = fd[i].fd;
      short revents = 0;
      if (FD_ISSET(f,&fdt[0])) revents |= POLLIN;
      if (FD_ISSET(f,&fdt[1])) revents |= POLLOUT;
      if (FD_ISSET(f,&fdt[2])) revents |= POLLERR;
      if (fd[i].events & revents) fd[i].cb(f, fd[i].arg);
#endif
    }
  }
  return n;
}

// ready() is just like wait(0.0) except no callbacks are done:
static inline int fl_ready() {
  if (gi_get_event_count()) return 1;
#if USE_POLL
  return ::poll(pollfds, nfds, 0);
#else
  timeval t;
  t.tv_sec = 0;
  t.tv_usec = 0;
  fd_set fdt[3];
  fdt[0] = fdsets[0];
  fdt[1] = fdsets[1];
  fdt[2] = fdsets[2];
  return ::select(maxfd+1,&fdt[0],&fdt[1],&fdt[2],&t);
#endif
}

////////////////////////////////////////////////////////////////

/**
The open X display.  This is needed as an argument to most Xlib calls.
Don't attempt to change it!  This is NULL before fltk::open_display()
is called.
*/
int fltk::xdisplay = 0;

/**
This dummy 1x1 window is created by fltk::open_display() and is
never destroyed. You can use it to communicate with the window
manager or other programs.
*/
gi_window_id_t fltk::message_window;

/**
Which screen number to use.  This is set by fltk::open_display() to
the default screen.  You can change it by setting this to a different
value immediately afterwards.
*/
int fltk::xscreen;

/**
The X visual that FLTK will use for all windows.  These are set by
fltk::open_display() to the default visual.  You can change them
before calling Window::show() the first time.  Typical code for
changing the default visual is:

\code
fltk::args(argc, argv); // do this first so $DISPLAY is set
fltk::open_display();
fltk::xvisual = find_a_good_visual(fltk:: fltk::xscreen);
if (!fltk::xvisual) fltk::abort(&quot;No good visual&quot;);
fltk::xcolormap = make_a_colormap(fltk:: fltk::xvisual->visual, fltk::xvisual->depth);
// it is now ok to show() windows:
window->show(argc, argv);
\endcode

A portable interface to get a TrueColor visual (which is probably
the only reason to do this) is to call fltk::visual(int).
*/
gi_screen_info_t *fltk::xvisual;

/**
The colormap being used by FLTK. This is needed as an argument for
many Xlib calls. You can also set this immediately after open_display()
is called to your own colormap. The function own_colormap() can be
used to make FLTK create a private one. FLTK uses the same colormap
for all windows and there is no way to change that, sorry.
*/
//Colormap fltk::xcolormap;

/** \fn gi_window_id_t xid(const Window* window);
Returns the XID for a window, or zero if show() has not been called on it.
*/

//static gi_atom_id_t WM_DELETE_WINDOW;
//static gi_atom_id_t WM_PROTOCOLS;
       gi_atom_id_t MOTIF_WM_HINTS;
static gi_atom_id_t FLTKChangeScheme;
static gi_atom_id_t TARGETS;
static gi_atom_id_t CLIPBOARD;
gi_atom_id_t XdndAware;
gi_atom_id_t XdndSelection;
gi_atom_id_t XdndEnter;
gi_atom_id_t XdndTypeList;
gi_atom_id_t XdndPosition;
gi_atom_id_t XdndLeave;
gi_atom_id_t XdndDrop;
gi_atom_id_t XdndStatus;
gi_atom_id_t XdndActionCopy;
gi_atom_id_t XdndFinished;
gi_atom_id_t textplainutf;
gi_atom_id_t textplain;
static gi_atom_id_t GA_TEXT;
gi_atom_id_t texturilist;
//gi_atom_id_t XdndProxy;
static gi_atom_id_t _NET_WM_NAME;
static gi_atom_id_t _NET_WM_ICON_NAME;
//static gi_atom_id_t _NET_WORKAREA;
static gi_atom_id_t _NET_CURRENT_DESKTOP;
gi_atom_id_t UTF8_STRING;




/**
Opens the display.  Does nothing if it is already open.  You should
call this if you wish to do X calls and there is a chance that your
code will be called before the first show() of a window.  This is
called automatically Window::show().

This may call fltk::abort() if there is an error opening the 
display.
*/
void fltk::open_display() {
  if (xdisplay) return;

  setlocale(LC_CTYPE, "");
  //XSetIOErrorHandler(io_error_handler);
  //XSetErrorHandler(xerror_handler);

  xdisplay = gi_init();
  if (xdisplay<=0) fatal("Can't open display \"%s\"","gi");
  open_display_internal();
}

static int
XInternAtoms( char **names, int count,
	     int only_if_exists, gi_atom_id_t * atoms_return)
{
	int ret = 1, i = 0;

	for (i = 0; i < count; i++) {
		atoms_return[i] =
			gi_intern_atom( names[i], only_if_exists);
		if (!atoms_return[i])
			ret = 0;
	}

	return ret;
}


/**
You can make fltk "open" a display that has already been opened,
perhaps by another GUI library.  Calling this will set
xdisplay to the passed display and also read information
FLTK needs from it. <i>Don't call this if the display is already open!</i>
*/
void fltk::open_display_internal() {
  xdisplay =1;
  add_fd(gi_get_connection_fd(), POLLIN, do_queued_events);

#define MAX_ATOMS 30
  gi_atom_id_t* atom_ptr[MAX_ATOMS];
  const char* names[MAX_ATOMS];
  int i = 0;
#define atom(a,b) atom_ptr[i] = &a; names[i] = b; i++
  //atom(	WM_DELETE_WINDOW	, "WM_DELETE_WINDOW");
  //atom(	WM_PROTOCOLS		, "WM_PROTOCOLS");
  atom(	MOTIF_WM_HINTS		, "_MOTIF_WM_HINTS");
  atom(	FLTKChangeScheme	, "FLTKChangeScheme");
  atom(	TARGETS			, "TARGETS");
  atom(	CLIPBOARD		, "CLIPBOARD");
  atom(	XdndAware		, "XdndAware");
  atom(	XdndSelection		, "XdndSelection");
  atom(	XdndEnter		, "XdndEnter");
  atom(	XdndTypeList		, "XdndTypeList");
  atom(	XdndPosition		, "XdndPosition");
  atom(	XdndLeave		, "XdndLeave");
  atom(	XdndDrop		, "XdndDrop");
  atom(	XdndStatus		, "XdndStatus");
  atom(	XdndActionCopy		, "XdndActionCopy");
  atom(	XdndFinished		, "XdndFinished");
  atom(	textplainutf		, "text/plain;charset=UTF-8");
  atom(	textplain		, "text/plain");
  atom(	GA_TEXT			, "TEXT");
  atom(	texturilist		, "text/uri-list");
  //atom(XdndProxy		, "XdndProxy");
  atom( _NET_WM_NAME		, "_NET_WM_NAME");
  atom( _NET_WM_ICON_NAME	, "_NET_WM_ICON_NAME");
  //atom(	_NET_WORKAREA		, "_NET_WORKAREA");
  atom(	_NET_CURRENT_DESKTOP	, "_NET_CURRENT_DESKTOP");
  atom( UTF8_STRING		, "UTF8_STRING");
#undef atom
  gi_atom_id_t atoms[MAX_ATOMS];
  XInternAtoms( (char**)names, i, 0, atoms);
  for (; i--;) *atom_ptr[i] = atoms[i];

 // xscreen = DefaultScreen(d);

  // This window is used for interfaces that need a window id even
  // though there is no real reason to have a window visible:
  message_window = gi_create_window(
		  GI_DESKTOP_WINDOW_ID,
		  0, 0, 1, 1,
	  0,
		  GI_FLAGS_NON_FRAME // borderwidth
		  );
   // XCreateSimpleWindow( GI_DESKTOP_WINDOW_ID, 0,0,1,1,0, 0, 0);

  // construct an XVisualInfo that matches the default Visual:
  //XVisualInfo templt; int num;
  //templt.visualid = XVisualIDFromVisual(DefaultVisual(d, xscreen));
  //xvisual = XGetVisualInfo(d, VisualIDMask, &templt, &num);
  //xcolormap = DefaultColormap(d, xscreen);
  xvisual = (gi_screen_info_t *)malloc(sizeof(gi_screen_info_t));
  gi_get_screen_info(xvisual);
#if USE_GI_IMM
  
#endif

#if !USE_COLORMAP
  visual(RGB);
#endif
}

/**
This closes the X connection.  You do \e not need to call this to 
exit, and in fact it is faster to not do so!  It may be useful to call 
this if you want your program to continue without the X connection. You 
cannot open the display again, and probably cannot call any FLTK 
functions. 
*/
void fltk::close_display() {
  remove_fd(gi_get_connection_fd());
  gi_exit();
  xdisplay = 0;
}

////////////////////////////////////////////////////////////////

static bool reload_info = true;

/*! \class fltk::Monitor
    Structure describing one of the monitors (screens) connected to
    the system. You can ask for one by position with find(), ask for
    all of them with list(), and ask for a fake one that surrounds all
    of them with all(). You then look in the structure to get information
    like the size and bit depth.
*/

/*! \var Monitor::work
  The rectangle of the monitor not covered by tool or menu bars. This
  is not a method because it looks clearer to write "monitor.work.x()"
  than "monitor.work().x()".
*/

/*! Return a "monitor" that surrounds all the monitors.
    If you have a single monitor, this returns a monitor structure that
    defines it. If you have multiple monitors this returns a fake monitor
    that surrounds all of them.
*/
const Monitor& Monitor::all() {
  static Monitor monitor;
  
  if (reload_info) {
    reload_info = false;
    open_display();
    monitor.set(0, 0,
		gi_screen_width(),
		gi_screen_height());

    // initially assume work area is entire monitor:
    monitor.work = monitor;

    // Try to get the work area from the X Desktop standard:
    // First find out what desktop we are on, as it allows the work area
    // to be different (however fltk will return whatever the first answer
    // is even if the user changes the desktop, so this is not exactly
    // right):
    gi_window_id_t root = GI_DESKTOP_WINDOW_ID;
    gi_atom_id_t actual=0; int format; unsigned long count, remaining;
    unsigned char* buffer = 0;
    gi_get_window_property( root, _NET_CURRENT_DESKTOP,
		       0, 1, false, GA_CARDINAL,
		       &actual, &format, &count, &remaining, &buffer);
    int desktop = 0;
    if (buffer && actual == GA_CARDINAL && format == 32 && count > 0)
      desktop = int(*(long*)buffer);
    if (buffer) {free(buffer); buffer = 0;}
    // Now get the workarea, which is an array of workareas for each
    // desktop. The 4*desktop argument makes it index the correct
    // distance into the workarea:
    actual = 0;
    //gi_get_window_property( root, _NET_WORKAREA,
	//	       4*desktop, 4, false, GA_CARDINAL,
	//	       &actual, &format, &count, &remaining, &buffer);
    //if (buffer && actual == GA_CARDINAL && format == 32 && count > 3) {
    //  long* p = (long*)buffer;
     // if (p[2] && p[3])
	//monitor.work.set(int(p[0]),int(p[1]),int(p[2]),int(p[3]));
    //}
    //if (buffer) {free(buffer); buffer = 0;}

    monitor.depth_ = GI_RENDER_FORMAT_BPP(xvisual->format);

    // do any screens really return 0 for MM?

	//fixme
   // int mm = DisplayWidthMM( xscreen);
   // monitor.dpi_x_ = mm ? monitor.w()*25.4f/mm : 100.0f;
    //mm = DisplayHeightMM( xscreen);
   // monitor.dpi_y_ = mm ? monitor.h()*25.4f/mm : monitor.dpi_x_;

  }
  return monitor;
}


static Monitor* monitors = 0;
static int num_monitors=0;

/*! Return an array of all Monitors.
    p is set to point to a static array of Monitor structures describing
    all monitors connected to the system. If there is a "primary" monitor,
    it will be first in the list.

    Subsequent calls will usually
    return the same array, but if a signal comes in indicating a change
    it will probably delete the old array and return a new one.
*/
int Monitor::list(const Monitor** p) {
  if (!num_monitors) {
    open_display();


    num_monitors = 1; // indicate that Xinerama failed
    monitors = (Monitor*)(&all());
    // Guess for dual monitors placed next to each other:
    if (monitors->w() > 2*monitors->h()) {
      int w = monitors[0].w()/2;
      num_monitors = 2;
      monitors = new Monitor[2];
      monitors[0] = monitors[1] = all();
      monitors[0].w(w);
      monitors[1].x(w);
      monitors[1].move_r(-w);
      monitors[0].work.w(w-monitors[0].work.x());
      monitors[1].work.x(w);
      monitors[1].work.move_r(-monitors[0].work.w());
    }


  }
  *p = monitors;
  return num_monitors;
}

/*! Return a pointer to a Monitor structure describing the monitor
    that contains or is closest to the given x,y, position.
*/
const Monitor& Monitor::find(int x, int y) {
  const Monitor* monitors;
  int count = list(&monitors);
  const Monitor* ret = monitors+0;
  if (count > 1) {
    int r = 0;
    for (int i = 0; i < count; i++) {
      const Monitor& m = monitors[i];
      // find distances to nearest edges:
      int rx;
      if (x <= m.x()) rx = m.x()-x;
      else if (x >= m.r()) rx = x-m.r();
      else rx = 0;
      int ry;
      if (y <= m.y()) ry = m.y()-y;
      else if (y >= m.b()) ry = y-m.b();
      else ry = 0;
      // return this screen if inside:
      if (rx <= 0 && ry <= 0) return m;
      // use larger of horizontal and vertical distances:
      if (rx < ry) rx = ry;
      // remember if this is the closest screen:
      if (!i || rx < r) {
	ret = monitors+i;
	r = rx;
      }
    }
  }
  return *ret;
}

////////////////////////////////////////////////////////////////

/*!
  Return where the mouse is on the screen by doing a round-trip query
  to the server. You should use fltk::event_x_root() and
  fltk::event_y_root() if possible, but this is necessary if you are
  not sure if a mouse event has been processed recently (such as to
  position your first window). If the display is not open, this will
  open it.
*/
void fltk::get_mouse(int &x, int &y) {
	gi_window_id_t win;
  open_display();
  gi_window_id_t root = GI_DESKTOP_WINDOW_ID;
  gi_window_id_t c; int mx,my,cx,cy; unsigned int mask;
 // XQueryPointer(root,&root,&c,&mx,&my,&cx,&cy,&mask);
  gi_get_mouse_pos (GI_DESKTOP_WINDOW_ID,&win,&mx,&my) ;
  x = mx;
  y = my;
}

/*!
  Change where the mouse is on the screen.
  Returns true if successful, false on failure (exactly what success
  and failure means depends on the os).
*/
bool fltk::warp_mouse(int x, int y) {
  gi_window_id_t root = GI_DESKTOP_WINDOW_ID;

  gi_move_cursor(x, y) ;

  return true; // always works
}

////////////////////////////////////////////////////////////////
// Tablet initialisation and event handling
const int n_stylus_device = 2;
//static XID stylus_device_id[n_stylus_device] = { 0, 0 };
//static XDevice *stylus_device[n_stylus_device] = { 0, 0 };
static int stylus_motion_event,
	   stylus_proximity_in_event, stylus_proximity_out_event;
static float pressure_mul[n_stylus_device];
static float x_tilt_add, x_tilt_mul;
static float y_tilt_add, y_tilt_mul;

const int tablet_pressure_ix = 2;
const int tablet_x_tilt_ix = 3;
const int tablet_y_tilt_ix = 4;

#define FLTK_T_MAX(a, b) ((a)>(b)?(a):(b))

bool fltk::enable_tablet_events() {
  open_display();
    return false;
 
}

#undef FLTK_T_MAX

////////////////////////////////////////////////////////////////

/**
The dnd_* variables allow your fltk program to use the
Xdnd protocol to manipulate files and interact with file managers. You
can ignore these if you just want to drag & drop blocks of text.  I
have little information on how to use these, I just tried to clean up
the Xlib interface and present the variables nicely.

The program can set this variable before returning non-zero for a
DND_DRAG event to indicate what it will do to the object. Fltk presets
this to <tt>XdndActionCopy</tt> so that is what is returned if you
don't set it.
*/
gi_atom_id_t fltk::dnd_action;

/** The X id of the window being dragged from. */
gi_window_id_t fltk::dnd_source_window;

/**
Zero-terminated list of atoms describing the formats of the source
data. This is set on the DND_ENTER event.  The
following code will print them all as text, a typical value is
<tt>"text/plain;charset=UTF-8"</tt> (gag).

\code
for (int i = 0; dnd_source_types[i]; i++) {
  char* x = XGetAtomName( dnd_source_types[i]);
  puts(x);
  free(x);
}
\endcode

You can set this and #dnd_source_action before calling dnd() to change
information about the source. You must set both of these, if you don't
fltk will default to <tt>"text/plain"</tt> as the type and
<tt>XdndActionCopy</tt> as the action. To set this change it to point
at your own array. Only the first 3 types are sent. Also, FLTK has no
support for reporting back what type the target requested, so all your
types must use the same block of data.
*/
gi_atom_id_t *fltk::dnd_source_types;

/**
The program can set this when returning non-zero for a DND_RELEASE
event to indicate the translation wanted. FLTK presets this to
<tt>"text/plain"</tt> so that is returned if you don't set it
(supposedly it should be limited to one of the values in
dnd_source_types, but <tt>"text/plain"</tt> appears to
always work).
*/
gi_atom_id_t fltk::dnd_type;

/**
The action the source program wants to perform. Due to oddities in the
Xdnd design this variable is \e not set on the fltk::DND_ENTER event,
instead it is set on each DND_DRAG event, and it may change each time.

To print the string value of the gi_atom_id_t use this code:

\code
char* x = XGetAtomName( dnd_source_action);
puts(x);
free(x);
\endcode

You can set this before calling fltk::dnd() to communicate a different
action. See #dnd_source_types, which you must also set.
*/
gi_atom_id_t fltk::dnd_source_action;

gi_atom_id_t *fl_incoming_dnd_source_types;

void fl_sendClientMessage(gi_window_id_t xwindow, gi_atom_id_t message,
			  unsigned long d0,
			  unsigned long d1=0,
			  unsigned long d2=0,
			  unsigned long d3=0,
			  unsigned long d4=0)
{
  gi_msg_t e;
  e.type = GI_MSG_CLIENT_MSG;
  e.ev_window = xwindow;
  e.client.client_type = message;
  e.client.client_format = 32;
  e.params[0] = (long)d0;
  e.params[1] = (long)d1;
  e.params[2] = (long)d2;
  e.params[3] = (long)d3;
  e.params[4] = (long)d4;
  gi_send_event( xwindow, 0, 0, &e);
}

////////////////////////////////////////////////////////////////
// Code used for cut & paste

static Widget *selection_requestor;
static char *selection_buffer[2];
static int selection_length[2];
static int selection_buffer_length[2];
bool fl_i_own_selection[2];

/*!
  Change the current selection. The block of text is copied to an
  internal buffer by FLTK (be careful if doing this in response to an
  fltk::PASTE as this may be the same buffer returned by
  event_text()).

  The block of text may be retrieved (from this program or whatever
  program last set it) with fltk::paste().

  There are actually two buffers. If \a clipboard is true then the text
  goes into the user-visible selection that is moved around with
  cut/copy/paste commands (on X this is the CLIPBOARD selection). If
  \a clipboard is false then the text goes into a less-visible buffer
  used for temporarily selecting text with the mouse and for drag &
  drop (on X this is the GA_PRIMARY selection).
*/
void fltk::copy(const char *stuff, int len, bool clipboard) {
  if (!stuff || len<0) return;
  if (len >= selection_buffer_length[clipboard]) {
	if(selection_buffer[clipboard])
      delete[] selection_buffer[clipboard];
    int n = selection_buffer_length[clipboard]*2;
    if (!n) n = 1024;
    while (len >= n) n *= 2;
    selection_buffer_length[clipboard] = n;
    selection_buffer[clipboard] = new char[n];
  }
  memcpy(selection_buffer[clipboard], stuff, len);
  selection_buffer[clipboard][len] = 0; // needed for direct paste
  selection_length[clipboard] = len;
  fl_i_own_selection[clipboard] = true;
  gi_atom_id_t property = clipboard ? CLIPBOARD : GA_PRIMARY;
  gi_set_selection_owner( property, message_window, event_time);
}

/*!
  This is what a widget does when a "paste" command (like Ctrl+V or
  the middle mouse click) is done to it. Cause an fltk::PASTE event to
  be sent to the receiver with the contents of the current selection
  in the fltk::event_text(). The selection can be set by fltk::copy().

  There are actually two buffers. If \a clipboard is true then the text
  is from the user-visible selection that is moved around with
  cut/copy/paste commands (on X this is the CLIPBOARD selection). If
  \a clipboard is false then the text is from a less-visible buffer
  used for temporarily selecting text with the mouse and for drag &
  drop (on X this is the GA_PRIMARY selection).

  The reciever should be prepared to be called \e directly by this, or
  for it to happen later, or possibly not at all. This allows the
  window system to take as long as necessary to retrieve the paste
  buffer (or even to screw up completely) without complex and
  error-prone synchronization code most toolkits require.
*/
void fltk::paste(Widget &receiver, bool clipboard) {
  if (fl_i_own_selection[clipboard]) {
    // We already have it, do it quickly without window server.
    // Notice that the text is clobbered if set_selection is
    // called in response to PASTE!
    e_text = selection_buffer[clipboard];
    e_length = selection_length[clipboard];
    receiver.handle(PASTE);
    return;
  }
  // otherwise get the window server to return it:
  selection_requestor = &receiver;
  gi_atom_id_t property = clipboard ? CLIPBOARD : GA_PRIMARY;
  gi_convert_selection( property, TARGETS, property,
		    xid(Window::first()), event_time);
}

////////////////////////////////////////////////////////////////

/** The most recent X event. */
gi_msg_t fltk::xevent;

/** The last timestamp from an X event that reported it (not all do).
Many X calls (like cut and paste) need this value. */
ulong fltk::event_time;

char fl_key_vector[32]; // used by get_key()

// Records shift keys that X does not handle:
static int extra_state;

// allows is_click() to be set for KEYUP events:
static unsigned recent_keycode;

// Record event mouse position and state from an gi_msg_t.
static void set_event_xy(bool push, long istate) {
#if CONSOLIDATE_MOTION
  send_motion = 0;
#endif
  e_x_root = xevent.params[0];
  e_y_root = xevent.params[1];
  e_x = xevent.x;
  e_y = xevent.y;

  //printf("set_event_xy() dpp %d,%d\n", event_x(),event_y());

  if(istate!=-1)
  e_state = istate   | extra_state;
  event_time = xevent.time;
  // turn off is_click if enough time or mouse movement has passed:
  static int px, py;
  static ulong ptime;
  if (abs(e_x_root-px)+abs(e_y_root-py) > 3
      || event_time >= ptime+(push?1000:200)) {
    e_is_click = 0;
    recent_keycode = 0;
  }
  if (push) {
    px = e_x_root;
    py = e_y_root;
    ptime = event_time;
  }
}

////////////////////////////////////////////////////////////////



int fl_actual_keysym;

// this little function makes sure that the stylus related event data
// is useful, even if no tablet was discovered, or the mouse was used to
// generate a PUSH, RELEASE, MOVE or DRAG event
static void set_stylus_data() {
  if (e_device==0) {
    e_pressure = e_state&BUTTON1 ? 1.0f : 0.0f;
    e_x_tilt = e_y_tilt = 0.0f;
	// e_device = DEVICE_MOUSE;
  }
}

/**
 * convert the current mouse chord into the FLTK modifier state
 */
static void mods_to_e_state( long mods, bool add )
{
  int state = 0;
  if ( mods & G_MODIFIERS_NUMLOCK ) state |= NUMLOCK;
  if ( mods & G_MODIFIERS_META ) state |= META;
  if ( mods & (G_MODIFIERS_ALT) ) state |= ALT;
  if ( mods & G_MODIFIERS_CTRL ) state |= CTRL;
  if ( mods & G_MODIFIERS_SHIFT ) state |= SHIFT;
  if ( mods & G_MODIFIERS_CAPSLOCK ) state |= CAPSLOCK;

  if(add)
	extra_state |= state;
  else
	  extra_state &= ~state;

  e_state = ( e_state & 0xff000000 ) | state;
  //printf( "State 0x%08x (%04x)\n", e_state, mods );
}

// convert a MSWindows G_KEY_x to an Fltk (X) Keysym:
// See also the inverse converter in get_key_win32.C
// This table is in numeric order by VK:
static const struct {unsigned short vk, fltk, extended;} vktab[] = {
  {G_KEY_BACKSPACE,	BackSpaceKey},
  {G_KEY_TAB,	TabKey},
  {G_KEY_CLEAR,	Keypad5,	ClearKey},
  {G_KEY_RETURN,	ReturnKey,	KeypadEnter},
  {G_KEY_RSHIFT,	RightShiftKey},
  {G_KEY_LSHIFT,	LeftShiftKey},
  {G_KEY_LCTRL,	LeftCtrlKey},
  {G_KEY_RCTRL,	RightCtrlKey},
  {G_KEY_MENU,	LeftAltKey,	RightAltKey},
  {G_KEY_PAUSE,	PauseKey},
  {G_KEY_CAPSLOCK,	CapsLockKey},
  {G_KEY_ESCAPE,	EscapeKey},
  {G_KEY_SPACE,	SpaceKey},
#if IGNORE_NUMLOCK
  {G_KEY_PAGEUP,	Keypad9,	PageUpKey},
  {G_KEY_PAGEDOWN,	Keypad3,	PageDownKey},
  {G_KEY_END,	Keypad1,	EndKey},
  {G_KEY_HOME,	Keypad7,	HomeKey},
  {G_KEY_LEFT,	Keypad4,	LeftKey},
  {G_KEY_UP,	Keypad8,	UpKey},
  {G_KEY_RIGHT,	Keypad6,	RightKey},
  {G_KEY_DOWN,	Keypad2,	DownKey},
  {G_KEY_PRINT, PrintKey},	// does not work on NT
  {G_KEY_INSERT,	Keypad0,	InsertKey},
  {G_KEY_DELETE,	DecimalKey,	DeleteKey},
#else
  {G_KEY_PRIOR,	PageUpKey},
  {G_KEY_NEXT,	PageDownKey},
  {G_KEY_END,	EndKey},
  {G_KEY_HOME,	HomeKey},
  {G_KEY_LEFT,	LeftKey},
  {G_KEY_UP,	UpKey},
  {G_KEY_RIGHT,	RightKey},
  {G_KEY_DOWN,	DownKey},
  {G_KEY_SNAPSHOT, PrintKey},	// does not work on NT
  {G_KEY_INSERT,	InsertKey},
  {G_KEY_DELETE,	DeleteKey},
#endif
  {G_KEY_LMETA,	LeftMetaKey},
  {G_KEY_RMETA,	RightMetaKey},
  {G_KEY_MENU,	MenuKey},
  {G_KEY_KP_MULTIPLY, MultiplyKey},
  {'+',	AddKey},
  {'-', SubtractKey},
  {'*',	DecimalKey},
  {'/',	DivideKey},
  {G_KEY_NUMLOCK,	NumLockKey},
  {G_KEY_SCROLLOCK,	ScrollLockKey},
#if 0
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
#endif
};


unsigned short fltk2gi(unsigned fltk) {
  if (fltk >= '0' && fltk <= '9') return fltk;
  if (fltk >= 'A' && fltk <= 'Z') return fltk;
  if (fltk >= 'a' && fltk <= 'z') return fltk-('a'-'A');
  if (fltk >= F1Key && fltk <= LastFunctionKey) return fltk-F1Key+G_KEY_F1;
  if (event_state(NUMLOCK) ) {
    if (fltk >= Keypad0 && fltk<=Keypad9) return fltk-Keypad0+G_KEY_KP0;
    if (fltk == DecimalKey) return '-';
  }
  if (fltk == KeypadEnter && 0)
    return G_KEY_RETURN;
  int a = 0;
  int b = sizeof(vktab)/sizeof(*vktab);
  while (a < b) {
    int c = (a+b)/2;
    if (vktab[c].fltk == fltk) return vktab[c].vk;
    if (vktab[c].fltk < fltk) a = c+1; else b = c;
  }
  return 0;
}


//bool fl_last_was_extended;
static inline int gi2fltk(int vk, long lParam) {
  static unsigned short vklut[256];
  static unsigned short extendedlut[256];
  if (!vklut[1]) { // init the table
    unsigned int i;
    for (i = 0; i < 256; i++) vklut[i] = tolower(i);
    for (i=G_KEY_F1; i<=G_KEY_F15; i++) vklut[i] = i+(F1Key-G_KEY_F1);
    for (i=G_KEY_KP0; i<=G_KEY_KP9; i++) vklut[i] = i+(Keypad0-G_KEY_KP0);
    for (i = 0; i < sizeof(vktab)/sizeof(*vktab); i++) {
      vklut[vktab[i].vk] = vktab[i].fltk;
      extendedlut[vktab[i].vk] = vktab[i].extended;
    }
    for (i = 0; i < 256; i++) if (!extendedlut[i]) extendedlut[i] = vklut[i];
  }
  if (lParam&(1<<24)) {
    //if (!(lParam&(1<<31))) fl_last_was_extended = true;
    return extendedlut[vk];
  } else {
    //if (!(lParam&(1<<31))) fl_last_was_extended = false;
    return vklut[vk];
  }
}
int has_unicode()
{
	return 0;
}

bool hand_sel_notify()
{
if (!selection_requestor) return false;
    static unsigned char* buffer;
    if (buffer) {free(buffer); buffer = 0;}
    long read = 0;
    if (xevent.params[5]) for (;;) {
      // The Xdnd code pastes 64K chunks together, possibly to avoid
      // bugs in X servers, or maybe to avoid an extra round-trip to
      // get the property length.  I copy this here:
      gi_atom_id_t actual; int format; unsigned long count, remaining;
      unsigned char* portion;
      if (gi_get_window_property(
			     xevent.params[2],
			     xevent.params[5],
			     read/4, 65536, 1, 0,
			     &actual, &format, &count, &remaining,
			     &portion)) break; // quit on error
      if (actual == TARGETS || actual == GA_ATOM) {
	gi_atom_id_t type = GA_STRING;
	// see if it offers a better type:
#if DEBUG_SELECTIONS
	printf("selection source types:\n");
	for (unsigned i = 0; i<count; i++) {
	  gi_atom_id_t t = ((gi_atom_id_t*)portion)[i];
	  char* x = XGetAtomName( t);
	  printf(" %s\n",x);
	  free(x);
	}
#endif
	for (unsigned i = 0; i<count; i++) {
	  gi_atom_id_t t = ((gi_atom_id_t*)portion)[i];
	  if (t == textplainutf ||
	      t == textplain ||
	      t == UTF8_STRING) {type = t; break;}
	  // rest are only used if no utf-8 available:
	  if (t == GA_TEXT || t == texturilist) type = t;
	}
	free(portion);
	gi_atom_id_t property = xevent.params[5];
	gi_convert_selection( property, type, property,
			  xid(Window::first()),
			  event_time);
	return true;
      }
#if DEBUG_SELECTIONS
      char* x = XGetAtomName( actual);
      printf("selection notify of type %s\n",x);
      free(x);
#endif
      if (read) { // append to the accumulated buffer
	buffer = (unsigned char*)realloc(buffer, read+count*format/8+remaining);
	memcpy(buffer+read, portion, count*format/8);
	free(portion);
      } else {	// Use the first section without moving the memory:
	buffer = portion;
      }
      read += count*format/8;

      if (!remaining) {
        e_text = buffer ? (char*)buffer : "";
        e_length = read;

        if (actual == texturilist && strncmp(e_text, "file://", 7) == 0) {
          // to be consistent with windows implementation
          e_text += 7; // skip leading file://
          e_length -= 9; // skip trailing CR+LF
        }

        selection_requestor->handle(PASTE);
        break; // exit for(;;)
      }
    }

    // Detect if this paste is due to Xdnd by the property name (I use
    // GA_SECONDARY for that) and send an XdndFinished message. It is not
    // clear if this has to be delayed until now or if it can be done
    // immediatly after calling gi_convert_selection.
    if (xevent.params[5] == GA_SECONDARY &&
	dnd_source_window) {
      fl_sendClientMessage(dnd_source_window, XdndFinished,
			    xevent.params[2]);
      dnd_source_window = 0; // don't send a second time
    }
}


static void hand_sel_req()
{
	gi_msg_t e;
    e.type = GI_MSG_SELECTIONNOTIFY;
	e.params[0]=G_SEL_NOTIFY;

    e.params[2] = xevent.params[2];//.requestor;
    e.params[3] = xevent.params[3];//.selection;
    bool clipboard = e.params[3] == CLIPBOARD;
    e.params[4] = xevent.params[4];//.target;
    e.params[1] = xevent.params[6];//.time;
    e.params[5] = xevent.params[5];//.property;
#if DEBUG_SELECTIONS
    char* x = gi_get_atom_name(e.params[4]);
    fprintf(stderr,"selection request for %s\n",x);
    free(x);
#endif
    if (!selection_length[clipboard]) {
      e.params[5] = 0;
    } else if (e.params[4] == TARGETS) {
      gi_atom_id_t a[3] = {UTF8_STRING, GA_STRING, GA_TEXT};
      gi_change_property( e.params[2], e.params[5],
		      GA_ATOM, 32, 0, (unsigned char*)a, 3);
    } else if (e.params[4] == UTF8_STRING ||
	       e.params[4] == GA_STRING ||
	       e.params[4] == GA_TEXT ||
	       e.params[4] == textplain ||
	       e.params[4] == textplainutf) {
      // clobber the target type, this seems to make some applications
      // behave that insist on asking for GA_TEXT instead of UTF8_STRING
      // Does not change GA_STRING as that breaks xclipboard.
      if (e.params[4] != GA_STRING) e.params[4] = UTF8_STRING;
      gi_change_property(  e.params[2], e.params[5],
		      e.params[4], 8, 0,
		      (unsigned char *)selection_buffer[clipboard],
		      selection_length[clipboard]);
    } else {
      e.params[5] = 0;
    }
    gi_send_event( e.params[2], 0, 0, (gi_msg_t *)&e);
    return ;
}
/*
static void recursive_expose(CreatedWindow* i) {
  i->wait_for_expose = false;
  i->expose(Rectangle(i->window->w(), i->window->h()));
  for (CreatedWindow* c = i->children; c; c = c->brother) recursive_expose(c);
}
*/
/**
  Make FLTK act as though it just got the event stored in #xevent.
  You can use this to feed artifical X events to it, or to use your
  own code to get events from X.

  Besides feeding events your code should call flush() periodically so
  that FLTK redraws its windows.

  This function will call any widget callbacks from the widget code.
  It will not return until they complete, for instance if it pops up a
  modal window with fltk::ask() it will not return until the user
  clicks yes or no.
*/
bool fltk::handle()
{
	unsigned btn=0;
	 int flbtn=0;
	unsigned long state=0;
  Window* window = find(xevent.ev_window);
  int event = 0;

 KEYPRESS:
#if USE_X11_MULTITHREADING
  fl_unlock_function();
 // if (XFilterEvent((gi_msg_t *)&xevent, 0)) {fl_lock_function(); return 1;}
  fl_lock_function();
#else
 // if (XFilterEvent((gi_msg_t *)&xevent, 0)) return 1;
#endif

	//printf("fltk::handle xevent.type=%d\n",xevent.type);

  switch (xevent.type) {

  //case KeymapNotify:
  //  memcpy(fl_key_vector, xevent.xkeymap.key_vector, 32);
  //  break;

  case GI_MSG_WINDOW_SHOW:
    //XRefreshKeyboardMapping((XMappingEvent*)&xevent.xmapping);
    extra_state = 0;
    break;

  case GI_MSG_CLIENT_MSG: {
    gi_atom_id_t message = xevent.client.client_type;
    const long* data = (const long*)xevent.params;

	printf("fltk GI_MSG_CLIENT_MSG %d,%d\n", GA_WM_DELETE_WINDOW,data[0]);

    if (window && (gi_atom_id_t)(data[0]) == GA_WM_DELETE_WINDOW) {
      if (!grab() && !(modal() && window != modal()))
	window->do_callback();
      return true;

    } else if (message == FLTKChangeScheme) {
      reload_theme();
      return true;

    } else if (message == XdndEnter) {
      xmousewin = window;
      in_a_window = true;
      dnd_source_window = data[0];
      // version number is data[1]>>24
      if (data[1]&1) {
	// get list of data types:
	gi_atom_id_t actual; int format; unsigned long count, remaining;
	unsigned char *buffer = 0;
	gi_get_window_property( dnd_source_window, XdndTypeList,
			   0, 0x8000000L, false, GA_ATOM, &actual, &format,
			   &count, &remaining, &buffer);
	if (actual != GA_ATOM || format != 32 || count<4 || !buffer)
	  goto FAILED;
	delete [] fl_incoming_dnd_source_types;
	fl_incoming_dnd_source_types = new gi_atom_id_t[count+1];
	dnd_source_types = fl_incoming_dnd_source_types;
	for (unsigned i = 0; i < count; i++)
	  dnd_source_types[i] = ((gi_atom_id_t*)buffer)[i];
	dnd_source_types[count] = 0;
      } else {
      FAILED:
	// less than four data types, or if the above messes up:
	if (!fl_incoming_dnd_source_types)
	  fl_incoming_dnd_source_types = new gi_atom_id_t[4];
	dnd_source_types = fl_incoming_dnd_source_types;
	dnd_source_types[0] = data[2];
	dnd_source_types[1] = data[3];
	dnd_source_types[2] = data[4];
	dnd_source_types[3] = 0;
      }
      // This should return one of the dnd_source_types. Unfortunately
      // no way to just force it to cough up whatever data is "most text-like"
      // instead I have to select from a list of known types. We may need
      // to add to this list in the future, turn on the #if to print the
      // types if you get a drop that you think should work.
      dnd_type = textplain; // try this if no matches, it may work
#if DEBUG_SELECTIONS
      printf("dnd source types:\n");
#endif
      for (int i = 0; ; i++) {
	gi_atom_id_t type = dnd_source_types[i]; if (!type) break;
#if DEBUG_SELECTIONS
	char* x = XGetAtomName( type);
	printf(" %s\n",x);
	free(x);
#endif
	if (type == textplainutf ||
	    type == textplain ||
            type == texturilist ||
	    type == UTF8_STRING) {dnd_type = type; break;} // ok
      }
      event = DND_ENTER;
      break;

    } else if (message == XdndPosition) {
      xmousewin = window;
      in_a_window = true;
      dnd_source_window = data[0];
      e_x_root = data[2]>>16;
      e_y_root = data[2]&0xFFFF;
      if (window) {
		  printf("dpp %s: got line %d, %d,%d\n",__FILE__,__LINE__,event_x(),event_y());
	e_x = e_x_root-window->x();
	e_y = e_y_root-window->y();
      }
      event_time = data[3];
      dnd_source_action = data[4];
      dnd_action = XdndActionCopy;
      int accept = handle(DND_DRAG, window);

	  //fixme
      fl_sendClientMessage(data[0], XdndStatus,
			   xevent.ev_window,
			   accept ? 1 : 0,
			   0, // used for xy rectangle to not send position inside
			   0, // used for width+height of rectangle
			   accept ? dnd_action : 0);
      return true;

    } else if (message == XdndLeave) {
      dnd_source_window = 0; // don't send a finished message to it
      event = DND_LEAVE;
      break;

    } else if (message == XdndDrop) {
      xmousewin = window;
      in_a_window = true;
      dnd_source_window = data[0];
      event_time = data[2];
      gi_window_id_t to_window = xevent.ev_window; //fixme
      if (handle(DND_RELEASE, window)) {
	selection_requestor = belowmouse();
	gi_convert_selection( XdndSelection,
			  dnd_type, GA_SECONDARY,
			  to_window, event_time);
      } else {
	// Send the finished message if I refuse the drop.
	// It is not clear whether I can just send finished always,
	// or if I have to wait for the SelectionNotify event as the
	// code is currently doing.
	fl_sendClientMessage(dnd_source_window, XdndFinished, to_window);
	dnd_source_window = 0;
      }
      return true;

    }
    break;}

  // We cannot rely on the x,y position in the configure notify event.
  // I now think this is an unavoidable problem with X: it is impossible
  // for a window manager to prevent the "real" notify event from being
  // sent when it resizes the contents, even though it can send an
  // artificial event with the correct position afterwards. Also I think
  // some window managers are broken and send wrong window locations
  // or don't send the fake ConfigureNotify as all.
  //
  // Furthermore, some window managers send ConfigureNotify for icons
  // or just due to bugs when the window in unmapped. This ignores them.
  //
  // Used to ignore MapNofify as it always came in pairs with Configure
  // Notify so the overhead was doubled. However newer kwin sends the
  // ConfigureNotify before the expose event causing it to be ignored.
  // So I now obey the MapNotify event as well.
  case GI_MSG_CONFIGURENOTIFY:
 // case GI_MSG_WINDOW_SHOW: 
	  {
		  if (xevent.params[0]!=GI_STRUCT_CHANGE_RESIZE && xevent.params[0]!=GI_STRUCT_CHANGE_MOVE)
		  {
			  break;
		  }
    if (!window) break;
    if (window->parent()) break; // ignore child windows
    if (xevent.ev_window != xid(window)) break; // ignore frontbuffer
    if (CreatedWindow::find(window)->wait_for_expose) {
    //  if (xevent.type == GI_MSG_CONFIGURENOTIFY) break; // ignore icons and wm bugs
      CreatedWindow::find(window)->wait_for_expose = false;
    }
    //if (!xevent.xconfigure.send_event) break; // ignore non-wm messages

    // figure out where Window Manager really put window:
    gi_window_info_t actual;
    gi_get_window_info( xid(window), &actual);
    gi_window_id_t junk; int X, Y, W = actual.width, H = actual.height;
    //XTranslateCoordinates( xid(window), actual.root,
	//		  0, 0, &X, &Y, &junk);
	X=actual.x;
	Y=actual.y;
    Rectangle& current = CreatedWindow::find(window)->current_size;
    if (X != current.x() || Y != current.y() || W != current.w() || H != current.h()) {
      window->resize(X, Y, W, H);
      window->layout_damage( window->layout_damage() | LAYOUT_USER );
    }
    current.set(X,Y,W,H);
    break;}

  case GI_MSG_REPARENT:
    if (!window || window->parent()) break;
    // Fix stoopid MetaCity! When you hide a window it reparents it but
    // it trashes the window position. It then uses this bad position
    // when you restore the window. This code detects if this is happening
    // and fixes the position of the hidden window.
    if (!window->visible() &&
	xevent.effect_window == GI_DESKTOP_WINDOW_ID) {
		TRACE_HERE;
      int X = xevent.x;
      int Y = xevent.y;
      Rectangle& current = CreatedWindow::find(window)->current_size;
      // If the x/y seem to be wrong, fix them:
      if (X != current.x() || Y != current.y())
	gi_move_window( xid(window), current.x(), current.y());
    }
    break;

  case GI_MSG_EXPOSURE:

  //case GraphicsExpose:
    if (!window){
	  printf("GI_MSG_EXPOSURE fot %x error\n", xevent.ev_window);
		break;
	}

    // If this window completely fills it's parent, parent will not get
    // an expose event and the wait flag will not turn off. So force this:
    {for (Window* w = window; ;) {
      CreatedWindow::find(w)->wait_for_expose = false;
      if (!w->parent() && (w->x()==USEDEFAULT || w->y()==USEDEFAULT)) {
	// figure out where Window Manager really put window:
	gi_window_info_t actual;
	gi_get_window_info( xid(w), &actual);
	gi_window_id_t junk; int X, Y, W = actual.width, H = actual.height;
	//XTranslateCoordinates( xid(w), actual.root,
		//	      0, 0, &X, &Y, &junk);

	X=actual.x;
	Y=actual.y;
	CreatedWindow::find(w)->current_size.set(X,Y,W,H);
	window->x(X); window->y(Y);
	// Turn on the user-specified position hint so MetaCity won't move it!!
	CreatedWindow::find(w)->sendxjunk();
      }
      w = w->window();
      if (!w) break;
    }}

	  {


	//Rectangle exp_rect(0,0,xevent.width,xevent.height);
	Rectangle exp_rect(xevent.x,xevent.y,xevent.width,xevent.height);


	//fltk::Rectangle exp_rect(xevent.width,xevent.height);
    // Inside of Xexpose event is exactly the same as Rectangle structure,
    // so we pass a pointer.
    CreatedWindow::find(window)->expose(exp_rect);


    // Inside of Xexpose event is exactly the same as Rectangle structure,
    // so we pass a pointer.
    //CreatedWindow::find(window)->expose(*(Rectangle*)(&xevent.x));
	  }
    return true;

  case GI_MSG_WINDOW_HIDE:
    //window = find(xevent.xmapping.window);
    if (!window) break;
    if (window->parent()) break; // ignore child windows
    // turning this flag makes iconic() return true:
    CreatedWindow::find(window)->wait_for_expose = true;
    break; // allow add_handler to do something too

#ifndef WHEEL_DELTA
#  define WHEEL_DELTA 120	// according to MSDN.
#endif


  case GI_MSG_BUTTON_DOWN: {	 
	  
     btn = xevent.params[2];

	 if (btn&GI_BUTTON_WHEEL_UP)
	 {
		 e_dy = -1;
      event = MOUSEWHEEL;
	  e_keysym = 5;
	  //set_event_xy(true,-1);
	 }
	 else if (btn&GI_BUTTON_WHEEL_DOWN)
	 {
		 e_dy = +1;
      event = MOUSEWHEEL;
	  e_keysym = 4;
	  //set_event_xy(true,-1);
	 }
	 else{

		if (btn & GI_BUTTON_L) {
			flbtn=1;
			state |= BUTTON1;
		}
		if (btn & GI_BUTTON_M) {
		  flbtn=2;
		  state |= BUTTON2;
		}
		if (btn & GI_BUTTON_R)
		  {
		  state |= BUTTON3;
		  flbtn=3;
		  }
		set_event_xy(true,state);

		e_keysym = flbtn;
  
		  // turn off is_click if enough time or mouse movement has passed:
		  if (e_is_click == e_keysym) {
			e_clicks++;
		  } else {
			e_clicks = 0;
			e_is_click = e_keysym;
		  }
		  event = PUSH;
		e_state |= (state);
    set_stylus_data();
	 }
    goto J1;
    }

  case GI_MSG_MOUSE_MOVE:
	  {
	  
	  
     btn = xevent.params[2];
	 if (btn & GI_BUTTON_L) {
		flbtn=1;
		state |= BUTTON1;
	}
  if (btn & GI_BUTTON_M) {
	  flbtn=2;
	  state |= BUTTON2;
  }
  if (btn & GI_BUTTON_R)
	  {
	  state |= BUTTON3;
	  flbtn=3;
	  }

	 // if (!window)
	  {
		//  printf("dpp no window in move xevent.ev_window=%d,%d, pid=%d\n",xevent.ev_window,xevent.effect_window,getpid());
	  }
	


    set_event_xy(false,-1);
    set_stylus_data();

	
	  
#if CONSOLIDATE_MOTION
    send_motion = xmousewin = window;
    in_a_window = true;
    return false;
#else
    event = MOVE;
    goto J1;
#endif
	  }

  case GI_MSG_BUTTON_UP: {
	  // printf("dpp GI_MSG_BUTTON_UP xevent.ev_window=%d\n",xevent.ev_window);
	 
	  
     btn = xevent.params[3];
	if (btn & GI_BUTTON_L) {
		flbtn=1;
		state |= BUTTON1;
	}
  if (btn & GI_BUTTON_M) {
	  flbtn=2;
	  state |= BUTTON2;
  }
  if (btn & GI_BUTTON_R)
	  {
	  state |= BUTTON3;
	  flbtn=3;
	  }

    e_keysym = flbtn;
    set_event_xy(false,state);
    //if (n == wheel_up_button || n == wheel_down_button) break;
    e_state &= ~(state);
    event = RELEASE;}
    set_stylus_data();
    goto J1;

  case GI_MSG_MOUSE_ENTER:

	 //   btn = xevent.params[2];
	//if (btn & GI_BUTTON_L) state |= BUTTON1;
  //if (btn & GI_BUTTON_M) state |= BUTTON2;
  //if (btn & GI_BUTTON_R) state |= BUTTON3;

    set_event_xy(false,-1);
  //fixme
   // e_state = state;//xevent.xcrossing.state << 16;
    //if (xevent.xcrossing.detail == NotifyInferior) {event=MOVE; break;}
//      printf("EnterNotify window %s, xmousewin %s\n",
//	   window ? window->label() : "NULL",
//	   xmousewin ? xmousewin->label() : "NULL");
    // XInstallColormap( CreatedWindow::find(window)->colormap);
    event = ENTER;
  J1:
    xmousewin = window;
    in_a_window = true;
    // send a mouse event, with cruft so the grab around modal things works:
    if (grab_) {
      handle(event, window);
     // if (grab_ && !exit_modal_)
	//XAllowEvents( SyncPointer, CurrentTime);
      return true;
    }
    break;

  case GI_MSG_MOUSE_EXIT:
	  e_state=0;
    set_event_xy(false,-1);
   // e_state = xevent.xcrossing.state << 16;
    //if (xevent.xcrossing.detail == NotifyInferior) {event=MOVE; break;}
//      printf("LeaveNotify window %s, xmousewin %s\n",
//	   window ? window->label() : "NULL",
//	   xmousewin ? xmousewin->label() : "NULL");
    xmousewin = 0;
    in_a_window = false; // make do_queued_events produce LEAVE event
    return false;

  case GI_MSG_FOCUS_IN:
#if USE_GI_IMM
   
#endif
    xfocus = window;
    if (window) {fix_focus(); return true;}
    break;

  case GI_MSG_FOCUS_OUT:
    if (window && window == xfocus) {
      xfocus = 0;
      fix_focus();
#if USE_GI_IMM
     // if (fl_xim_ic && !focus()) XUnsetICFocus(fl_xim_ic);
#endif
      return true;
    }
    break;

  case GI_MSG_KEY_DOWN: {
	  int keycode =xevent.params[4];

	  e_keysym = gi2fltk(keycode,0);

	  mods_to_e_state(xevent.params[3],true);
    event = KEY;
	fl_key_vector[keycode/8] |= (1 << (keycode%8));

#if 1
	// if same as last key, increment repeat count:
	if (fl_key_vector[keycode/8]&(1<<(keycode%8))) {
      e_key_repeated++;
      recent_keycode = 0;
    } else {
      e_clicks = 0;
      e_key_repeated = 0;
      recent_keycode = keycode;
    }

	// translate to text:
    static char buffer[31]; // must be big enough for fltk::compose() output
    static char dbcsbuf[3];
    if (1) {
      if (e_keysym==ReturnKey || e_keysym==KeypadEnter) {
	buffer[0] = '\r';
	e_length = 1;
      } else if (has_unicode()) {
	// If we have registered UNICODE window under NT4, 2000, XP
	// We get WCHAR as wParam
	e_length = utf8encode((unsigned short)keycode, &buffer[0]);
      } else {
	if (!dbcsbuf[0] && (unsigned char)keycode>127) {
	  dbcsbuf[0] = (char)keycode;
	  break;
	}
	int dbcsl = 1;
	if(dbcsbuf[0]) {
	  dbcsbuf[1] = (unsigned char) keycode;
	  dbcsl = 2;
	} else {
	  dbcsbuf[0] = (unsigned char) keycode;
	}
	dbcsbuf[2] = 0;
	e_length = utf8frommb(buffer, sizeof(buffer), dbcsbuf, dbcsl);
	if (e_length >= sizeof(buffer)) e_length = sizeof(buffer)-1;
	dbcsbuf[0] = 0;
      }
    } else {
      dbcsbuf[0] = 0;
      e_length = 0;
    }
    buffer[e_length] = 0;
    e_text = buffer;


#else
    
#endif
    break;}

  case GI_MSG_KEY_UP: {

	  int keycode =xevent.params[4];

	  e_keysym = gi2fltk(keycode,0);
	  mods_to_e_state(xevent.params[3],false);

	  fl_key_vector[keycode/8] &= ~(1 << (keycode%8));
    

    //set_event_xy(false);
   /// unsigned keycode = keycode;
   // fl_key_vector[keycode/8] &= ~(1 << (keycode%8));
    // Leave event_is_click() on only if this is the last key pressed:
    e_is_click =0;// (recent_keycode == keycode);
    recent_keycode = 0;
    event = KEYUP;
   // }



    //e_keysym = int(keysym);
    break;}

	case GI_MSG_SELECTIONNOTIFY:
	  {
		switch (xevent.params[0])
		{
		case G_SEL_NOTIFY:
			hand_sel_notify();
			return true;
			break;
		case G_SEL_REQUEST:{
			hand_sel_req();
			return true;
		}

			break;
		case G_SEL_CLEAR:
			{
			bool clipboard = xevent.params[3] == CLIPBOARD;
    fl_i_own_selection[clipboard] = false;
    return true;}
			break;
		default:
			break;		
		}
		break;
	  }




  }
 unknown_device:

  return handle(event, window);
}

////////////////////////////////////////////////////////////////
// Innards of Window::create()

extern bool fl_show_iconic; // In Window.cxx, set by iconize() or -i switch

/**
This virtual function may be overridden to use something other than
FLTK's default code to create the system's window. This must call
either CreatedWindow::create() or CreatedWindow::set_xid().

An example for Xlib (include x.h to make this work):
\code
void MyWindow::create() {
  fltk::open_display();	// necessary if this is first window
  // we only calcualte the necessary visual & colormap once:
  static XVisualInfo* visual;
  static Colormap colormap;
  static int background;
  if (!visual) {
    visual = figure_out_visual();
    colormap = XCreateColormap( RootWindow(xscreen),
			        vis->visual, AllocNone);
    XColor xcol; xcol.red = 1; xcol.green = 2; xcol.blue = 3;
    XAllocColor(fltk::display, colormap, &xcol);
    background = xcol.pixel;
  }
  CreatedWindow::create(this, visual, colormap, background);
}
\endcode
*/
void Window::create() {
  CreatedWindow::create(this,   -1);
}

/**
This function calls XCreateWindow and sets things up so that
xid(window) returns the created X window id.  This also does a lot of
other ugly X stuff, including setting the label, resize limitations,
etc.  The \a background is a pixel to use for X's automatic fill
color, use -1 to indicate that no background filling should be done.
*/
void CreatedWindow::create(Window* window,			  
			   int background)
{
	uint32_t event_mask;
	unsigned style=0;

  //XSetWindowAttributes attr;
  //attr.border_pixel = 0;
  //attr.colormap = colormap;
  //attr.bit_gravity = 0; // StaticGravity;
  //int mask = CWBorderPixel|CWColormap|CWEventMask|CWBitGravity;

  int W = window->w();
  if (W <= 0) W = 1; // X don't like zero...
  int H = window->h();
  if (H <= 0) H = 1; // X don't like zero...
  int X = window->x();
  int Y = window->y();

  ulong root;

  if (window->parent()) {
    // Find the position in the surrounding window, taking into account
    // any intermediate Group widgets:
    for (Widget *o = window->parent(); ; o = o->parent()) {
      if (o->is_window()) {root = ((Window*)o)->i->xid; break;}
      X += o->x();
      Y += o->y();
    }
   // attr.event_mask = ExposureMask;
    event_mask =
      GI_MASK_EXPOSURE;
  } else {

	   if (!window->border()){
      style |= (GI_FLAGS_NON_FRAME);
	   }
   // else if (window->maxw != window->minw || window->maxh != window->minh)
   //   style = WS_THICKFRAME | WS_MAXIMIZEBOX | WS_CAPTION;
   // else
   //   style = WS_DLGFRAME | WS_CAPTION;


    if (X == USEDEFAULT || Y == USEDEFAULT) {
      // Select a window position. Some window managers will ignore this
      // because we do not set the "USPosition" or "PSPosition" bits in
      // the hints, indicating that automatic placement should be done.
      // But it appears that newer window managers use the position,
      // especially for child windows, so we better set it:
      const Window* parent = window->child_of();
      int sw = gi_screen_width();
      int sh = gi_screen_height();
      if (parent) {
	X = parent->x()+((parent->w()-W)>>1);
	if (X <= parent->x()) X = parent->x()+1;
	Y = parent->y()+((parent->h()-H)>>1);
	if (Y < parent->y()+20) Y = parent->y()+20;
      } else {
	X = (sw-W)>>1;
	Y = (sh-H)>>1;
      }
      if (!modal()) {
	static const Window* pp = 0;
	static int delta = 0;
	if (parent != pp) {pp = parent; delta = 0;}
	X += delta;
	Y += delta;
	delta += 16;
      }
      if (X+W >= sw) X = sw-W-1;
      if (X < 1) X = 1;
      if (Y+H >= sh) Y = sh-H-1;
      if (Y < 20) Y = 20;
      // we do not compare to sw/sh here because I think it may mess up
      // some virtual desktop implementations
    }
    root = GI_DESKTOP_WINDOW_ID;

     event_mask =
      GI_MASK_EXPOSURE | GI_MASK_CONFIGURENOTIFY | GI_MASK_CLIENT_MSG
      | GI_MASK_KEY_DOWN | GI_MASK_KEY_UP |  GI_MASK_FOCUS_IN | GI_MASK_FOCUS_OUT
      | GI_MASK_BUTTON_DOWN | GI_MASK_BUTTON_UP|GI_MASK_WINDOW_HIDE|GI_MASK_WINDOW_SHOW
      | GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT|GI_MASK_SELECTIONNOTIFY
      | GI_MASK_MOUSE_MOVE;

    if (window->override()) {
      flush(); // make sure save_under does not undo any drawing!
	  style|=(GI_FLAGS_TEMP_WINDOW);
      
    }
  }

  if (background >= 0) {
    //attr.background_pixel = background;
    //mask |= CWBackPixel;
  }

  if (window->contains(modal())){
	  style|=(GI_FLAGS_TEMP_WINDOW);
  }

	gi_window_id_t id = gi_create_window( root, X, Y, W, H, background, style );

	if(id>0){
	  gi_set_window_background(id,background,GI_BG_USE_COLOR);
	  gi_set_events_mask(id,event_mask);
	}
	else{
	}
	//printf("fl gi_create_window ev_window=%x\n",id);

  CreatedWindow* x = CreatedWindow::set_xid(window,id);
    
  x->current_size.set(X, Y, W, H);

  if (!window->parent() && !window->override()) {
    // send all kinds 'o junk to X Window Manager:
    x->wait_for_expose = true;

    // Set the label:
    window->label(window->label(), window->iconlabel());

	gi_atom_id_t del_id=GA_WM_DELETE_WINDOW;

    // Makes the close button produce an event:
    gi_change_property( x->xid, GA_WM_PROTOCOLS,
		    GA_ATOM, 32, 0, (uchar*)&del_id, 1);

    // send size limits and border:
    x->sendxjunk();
#if 0 //fixme,dpp

    // Setting this allows the window manager to use the window's class
    // to look up things like border colors and icons in the xrdb database:
    if (window->xclass()) {
      char buffer[1024];
      char *p; const char *q;
      // truncate on any punctuation, because they break XResource lookup:
      for (p = buffer, q = window->xclass(); isalnum(*q)||(*q&128);) *p++ = *q++;
      *p++ = 0;
      // create the capitalized version:
      q = buffer;
      *p = toupper(*q++); if (*p++ == 'X') *p++ = toupper(*q++);
      while ((*p++ = *q++));
      gi_change_property( x->xid, GA_WM_CLASS, GA_STRING, 8, 0,
		      (unsigned char *)buffer, p-buffer-1);
    }

    // Send child window information:
  
      if (window->child_of() && window->child_of()->shown())
      XSetTransientForHint( x->xid, window->child_of()->i->xid);
#endif

    // Make it receptive to DnD:
    int version = 4;
    gi_change_property( x->xid, XdndAware, GA_ATOM, 32, 0, (unsigned char*)&version, 1);

	 


    if (window->icon()) {
    
      // FLTK2 uses the freedesktop.org new-style icons. This is an
      // array of 32-bit unsigned values: w,h,(w*h)*argb, repeated
      // for multiple sizes of image. Currently only a single image
      // is allowed since otherwise the size of the data cannot be
      // figured out...
      // warning: this code assumes sizeof(unsigned)==4!
      unsigned* data = (unsigned*)(window->icon());
      unsigned size = data[0]*data[1]+2;
      static gi_atom_id_t _NET_WM_ICON = 0;
      if (!_NET_WM_ICON) _NET_WM_ICON =
			   gi_intern_atom( "_NET_WM_ICON", 0);
      gi_change_property( x->xid, _NET_WM_ICON,
		      GA_CARDINAL, 32, G_PROP_MODE_Replace,
		      (uchar*)data, size);
    }
   // XSetWMHints( x->xid, hints);
    //free(hints);
  }
}

/** Set things up so that xid(window) returns \a winxid. Thus you will
make that Window draw into an existing X window. */
CreatedWindow* CreatedWindow::set_xid(Window* window, gi_window_id_t winxid) {
  CreatedWindow* x = new CreatedWindow;
  x->xid = winxid;
  x->backbuffer = 0;
  x->frontbuffer = 0;
  x->overlay = false;
  x->window = window; window->i = x;
  x->region = 0;
  x->wait_for_expose = false;
  x->cursor = 0;
  x->cursor_for = 0;
  x->next = CreatedWindow::first;
  CreatedWindow::first = x;
  return x;
}

////////////////////////////////////////////////////////////////
// Send X window stuff that can be changed over time:

void CreatedWindow::sendxjunk() {
  if (window->parent() || window->override()) return; // it's not a window manager window!
#if 0
 
#endif
}

void Window::size_range_() {
  size_range_set = 1;
  if (i) i->sendxjunk();
}

/*! Returns true if the window is currently displayed as an
  icon. Returns false if the window is not shown() or hide() has been
  called.

  <i>On X this will return true in the time between when show() is
  called and when the window manager finally puts the window on the
  screen and causes an expose event.</i>
*/
bool Window::iconic() const {
  return (i && visible() && i->wait_for_expose);
}

////////////////////////////////////////////////////////////////

/*! Sets both the label() and the iconlabel() */
void Window::label(const char *name, const char *iname) {
  Widget::label(name);
  iconlabel_ = iname;
  if (i && !parent()) {
    if (!name) name = "";
    int l = strlen(name);

	#if 1
    //if (is_utf8(name,name+l)>=0)
     // gi_change_property( i->xid, _NET_WM_NAME,
	//	      UTF8_STRING, 8, 0, (uchar*)name, l);
    gi_change_property( i->xid, GA_WM_NAME,
		    GA_STRING, 8, 0, (uchar*)name, l+1);
    if (!iname) iname = filename_name(name);
    l = strlen(iname);
    //if (is_utf8(iname,iname+l)>=0)
     // gi_change_property( i->xid, _NET_WM_ICON_NAME,
	//	      UTF8_STRING, 8, 0, (uchar*)iname, l);
    gi_change_property( i->xid, GA_WM_ICON_NAME,
		    GA_STRING, 8, 0, (uchar*)iname, l+1);
	#else
		if (!iname) iname = filename_name(name);
		gi_set_window_utf8_caption(i->xid,(const char*)iname);
		gi_set_window_icon_name(i->xid,iname);
	#endif
  }
}

////////////////////////////////////////////////////////////////
// Drawing context

/*! \fn const Window* Window::drawing_window()
  Returns the Window currently being drawn into. To set this use
  make_current(). It will already be set when draw() is called.
*/
const Window *Window::drawing_window_;

int fl_clip_w, fl_clip_h;

/**
Set by Window::make_current() and/or draw_into() to the window being
drawn into. This may be different than the xid() of the window, as it
may be the back buffer which has a different id.
*/
gi_window_id_t fltk::xwindow;

/**
The single X GC used for all drawing. This is initialized by the first
call to Window::make_current(). This may be removed if we use Cairo or
XRender only.

Most Xlib drawing calls look like this:
\code
XDrawSomething( xwindow, gc, ...);
\endcode
*/
gi_gc_ptr_t fltk::gc;

#if USE_CAIRO
cairo_t* fltk::cr;
static cairo_surface_t* surface;
#endif



/** Make the fltk drawing functions draw into this widget.
    The transformation is set so 0,0 is at the upper-left corner of
    the widget and 1 unit equals one pixel. The transformation stack
    is empied, and all other graphics state is left in unknown
    settings.

    The equivalent of this is already done before a Widget::draw()
    function is called.  The only reason to call this is for
    incremental updating of widgets without using redraw().
    This will crash if the widget is not in a currently shown() window.
    Also this may not work correctly for double-buffered windows.
*/
void Widget::make_current() const {
  int x = 0;
  int y = 0;
  const Widget* widget = this;
  while (!widget->is_window()) {
    x += widget->x();
    y += widget->y();
    widget = widget->parent();
  }
  const Window* window = (const Window*)widget;
  Window::drawing_window_ = window;
  CreatedWindow* i = CreatedWindow::find(window);
  draw_into(i->frontbuffer ? i->frontbuffer : i->xid, window->w(), window->h());
  translate(x,y);
}

namespace fltk {class Image;}
fltk::Image* fl_current_Image;

#if USE_CAIRO
/** Fltk cairo create surface function accepting a Window* as input */
cairo_surface_t* fltk::cairo_create_surface(Window* wi) {
  gi_window_id_t window = fltk::xid(wi);
  cairo_surface_t* surface =  cairo_xlib_surface_create( window, 
				   xvisual->visual, wi->w(), wi->h());
  cairo_xlib_surface_set_size(surface, wi->w(), wi->h());
  return surface;
}
#endif

/**
  Fltk can draw into any X window or pixmap that uses the
  fltk::xvisual.  This will reset the transformation and clip region
  and leave the font, color, etc at unpredictable values. The \a w and
  \a h arguments must be the size of the window and are used by
  fltk::not_clipped().

  Before you destroy the window or pixmap you must call
  fltk::stop_drawing() so that it can destroy any temporary structures
  that were created by this.
*/
void fltk::draw_into(gi_window_id_t window, int w, int h) {
  fl_current_Image = 0;
  fl_clip_w = w;
  fl_clip_h = h;

#if USE_CAIRO
  if (cr) {
    cairo_status_t cstatus = cairo_status(cr);
    if (cstatus) {
      warning("Cairo: %s", cairo_status_to_string(cstatus));
      cairo_destroy(cr); cr = 0;
      cairo_surface_destroy(surface); surface = 0;
      xwindow = 0;
    }
  }
#endif

  if (xwindow != window) {
    xwindow = window;
    
    if (!fltk::gc){
		fltk::gc = gi_create_gc( window,NULL);
	}
	else{
		gi_gc_attch_window(fltk::gc,window);
	}



#if USE_CAIRO
    if (cr) {
      cairo_xlib_surface_set_drawable(surface, window, w, h);
    } else {
      surface = cairo_xlib_surface_create( window, xvisual->visual, w, h);
      cr = cairo_create(surface);
      // emulate line_style(0):
      cairo_set_line_width(cr, 1);
    }
#endif

  }

#if USE_CAIRO
  else cairo_xlib_surface_set_size(surface, w, h);
#endif

  load_identity();
}

/**
  Destroy any "graphics context" structures that point at this window
  or Pixmap. They will be recreated if you call draw_into() again.

  Unfortunately some graphics libraries will crash if you don't do this.
  Even if the graphics context is not used, destroying it after destroying
  it's target will cause a crash. Sigh.
*/
void fltk::stop_drawing(gi_window_id_t window) {
  if (xwindow == window) {
    xwindow = 0;
#if USE_CAIRO
    cairo_destroy(cr); cr = 0;
#endif

  }
}

////////////////////////////////////////////////////////////////
// Window update, double buffering, and overlay:


/**
This virtual function is called by ::flush() to update the
window. You can override it for special window subclasses to change
how they draw.

For FLTK's normal windows this calls make_current(), then perhaps sets
up the clipping if the only damage is expose events, and then draw(),
and then does some extra work to get the back buffer copied or swapped
into the front buffer.

For your own windows you might just want to put all the drawing code
in here.
*/
void Window::flush() {
  drawing_window_ = this;

  unsigned char damage = this->damage();


  gi_window_id_t frontbuffer = i->frontbuffer ? i->frontbuffer : i->xid;

  if (this->double_buffer() || i->overlay) {  // double-buffer drawing

  

    bool eraseoverlay = i->overlay || (damage&DAMAGE_OVERLAY);
    if (eraseoverlay) damage &= ~DAMAGE_OVERLAY;

    if (!i->backbuffer) { // we need to create back buffer

	i->backbuffer =
	  gi_create_pixmap_window(i->xid,  w(), h(), gi_screen_format());
	 // gi_create_pixmap_window( frontbuffer, w(), h(), gi_screen_format());
      set_damage(DAMAGE_ALL); damage = DAMAGE_ALL;
      i->backbuffer_bad = false;
    } else if (i->backbuffer_bad) {
      // the previous draw left garbage in back buffer, so we must redraw:
      set_damage(DAMAGE_ALL); damage = DAMAGE_ALL;
      i->backbuffer_bad = false;
    }

    // draw the back buffer if it needs anything:
    if (damage) {
      // set the graphics context to draw into back buffer:
      draw_into(i->backbuffer, w(), h());
      if (damage & DAMAGE_ALL) {
	draw();
      } else {
	// draw all the changed widgets:
	if (damage & ~DAMAGE_EXPOSE) {
	  set_damage(damage & ~DAMAGE_EXPOSE);
	  draw();
	}
	// redraw(rectangle) will cause this to be executed:
	if (i->region) {
	  clip_region(i->region); i->region = 0;
	  set_damage(DAMAGE_EXPOSE); draw();
	  // keep the clip for the back->front copy if no other damage:
	  if ((damage & ~DAMAGE_EXPOSE) || eraseoverlay) clip_region(0);
	}
      }

      draw_into(frontbuffer, w(), h());
    } else {
      // if damage is zero then expose events were done, just copy
      // the back buffer to the front:
      draw_into(frontbuffer, w(), h());
      if (!eraseoverlay) {
	clip_region(i->region); i->region = 0;
      }
    }

    // Copy the backbuffer to the window:
    // On Irix, at least, it is much slower unless you cut the rectangle
    // down to the clipped area. Seems to be a pretty bad implementation:
    Rectangle r(w(),h());
    intersect_with_clip(r);

    gi_copy_area( i->backbuffer, frontbuffer, gc,
	      r.x(), r.y(), r.w(), r.h(), r.x(), r.y());
    if (i->overlay) draw_overlay();
    clip_region(0);

  }  else {


    // Single buffer drawing
    draw_into(frontbuffer, w(), h());
    unsigned char damage = this->damage();
    if (damage & ~DAMAGE_EXPOSE) {
      set_damage(damage & ~DAMAGE_EXPOSE);
      draw();
      i->backbuffer_bad = true;
    }
    if (i->region && !(damage & DAMAGE_ALL)) {
      clip_region(i->region); i->region = 0;
      set_damage(DAMAGE_EXPOSE); draw();
      clip_region(0);
    }
  }
}

/*! Get rid of extra storage created by drawing when double_buffer() was
  turned on. */
void Window::free_backbuffer() {
  if (!i || !i->backbuffer) return;
  stop_drawing(i->backbuffer);

  gi_destroy_pixmap( i->backbuffer);
  i->backbuffer = 0;
}

////////////////////////////////////////////////////////////////

void Window::borders( fltk::Rectangle *r ) const {
	gi_window_info_t info;
	int err;

  if (!this->border() || this->override() || this->parent()) {
    r->set(0,0,0,0);
    return;
  }

  bool resizable = this->resizable() || maxw != minw || maxh != minh;
  static Rectangle guess_fixed(-4,-21,4+4,21+4);
  static Rectangle guess_resizable(-4,-21,4+4,21+4);

  if ( shown() ) {
    gi_window_id_t parent, root, *children=0;
    int nchildren;
    if ( gi_query_child_tree(  xid(this),  &parent,
		     &children, &nchildren ) ) {
      if ( children ) free( children );
    } else {
      parent = 0;
    }
    if (parent != 0 && parent != root) {
      // Get X and Y relative to parent.
      int tx, ty;
      unsigned int tw,th,bw,wd;

	  err= gi_get_window_info(xid(this),&info);
	  tx = info.x;
	  ty = info.y;
	  tw = info.width;
	  th = info.height;

	  //XGetGeometry(  xid(this), &root, &tx, &ty,
	//		 &tw, &th, &bw, &wd );

      if ( err>=0 ) {
	int wx,wy;
	unsigned int ww,wh;

	err= gi_get_window_info(xid(this),&info);
	  tx = info.x;
	  ty = info.y;
	  tw = info.width;
	  th = info.height;

	//if ( XGetGeometry(  parent, &root, &wx, &wy,
	//		   &ww, &wh, &bw, &wd ) )
	if(err){
	  r->set( -tx, -ty, ww-tw, wh-th );
	  if (resizable)
	    guess_resizable = *r;
	  else
	    guess_fixed = *r;
	  return;
        }
      }
    }
  }

  if (resizable) {
    *r = guess_resizable;
  } else {
    *r = guess_fixed;
  }
}

void Window::layout() {
  // If only the xy position changed, then X does everything for us, so don't
  // call the Group layout.
  if (layout_damage() & ~LAYOUT_XY) Group::layout();
  // Fix the window:
  system_layout();
}

/**
 * Resizes the actual system window to match the current size of the fltk
 * widget. You should call this in your layout() method if xywh have changed.
 * The layout_damage() flags must be on or it won't work.
 */
void Window::system_layout() {
  if (i && (layout_damage()&LAYOUT_XYWH)) {
    // Fix the size/position we recorded for the X window if it is not
    // the same as the fltk window. If we received CONFIGURE_NOTIFY
    // events and the application did not make any further changes to the
    // window size, this will already be correct and no X calls will
    // be made.
    int x = this->x(); int y = this->y();
    for (Widget* p = parent(); p && !p->is_window(); p = p->parent()) {
      x += p->x(); y += p->y();
    }
    int w = this->w(); if (w <= 0) w = 1;
    int h = this->h(); if (h <= 0) h = 1;
    if (w != i->current_size.w() || h != i->current_size.h()) {
      if (!parent()) {
	// Some window managers refuse to allow resizes unless the resize
	// information allows it:
	if (minw == maxw && minh == maxh) size_range(w,h,w,h);
	// Wait for echo (relies on window having StaticGravity!!!)
	i->wait_for_expose = true;
      }

	  //printf("dpp move resize %x %d,%d %d,%d\n", i->xid, x, y,w,h);
     // gi_move_window( i->xid, x, y);
     // gi_resize_window( i->xid,  w, h);
	  gi_wm_set_window_bounds(i->xid, x,y, w, h,0);
      //gi_move_resize_window( i->xid, x, y, w, h);
      i->current_size.set(x,y,w,h);
    } else if (x != i->current_size.x() || y != i->current_size.y()) {
		// printf("dpp move xxx %x %d,%d %d,%d\n", i->xid, x, y,w,h);
      //gi_move_window( i->xid, x, y);
	  gi_wm_set_window_bounds(i->xid, x,y, -1, -1,0);
      i->current_size.x(x);
      i->current_size.y(y);
    }
    // resize the windows used for double-buffering:
    if (layout_damage() & LAYOUT_WH) {

	free_backbuffer();
    }
  }
}

//
// End of "$Id: run.cxx 7473 2010-04-08 20:25:03Z spitzak $".
//
