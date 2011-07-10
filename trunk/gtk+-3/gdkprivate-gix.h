


/*
 * Private uninstalled header defining things local to X windowing code
 */

#ifndef __GDK_PRIVATE_GIX_H__
#define __GDK_PRIVATE_GIX_H__

#include <gdk/gdk.h>
#include <gdk/gdkcursor.h>
#include <gdk/gdkprivate.h>
#include <gdk/gix/gdkdisplay-gix.h>

#include "config.h"
#include "gdkinternals.h"
#include "gdkscreenprivate.h"

#include "gdkdeviceprivate.h"
#include "gdkdevicemanagerprivate.h"

#include "gdkcursor.h"
#include "gdkcursorprivate.h"

#define GDK_TYPE_GIX_CURSOR              (_gdk_gix_cursor_get_type ())
#define GDK_GIX_CURSOR(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GIX_CURSOR, GdkGixCursor))
#define GDK_GIX_CURSOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GIX_CURSOR, GdkGixCursorClass))
#define GDK_IS_GIX_CURSOR(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GIX_CURSOR))
#define GDK_IS_GIX_CURSOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GIX_CURSOR))
#define GDK_GIX_CURSOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GIX_CURSOR, GdkGixCursorClass))

#define GIX_CURSOR_BASE GI_CURSOR_USER0

typedef struct _GdkGixCursor GdkGixCursor;
typedef struct _GdkGixCursorClass GdkGixCursorClass;

struct _GdkGixCursor
{
  GdkCursor cursor;
  gchar *name;
  int x, y, width, height, size;
  int id;
  //void *map;
  //guint serial;
  //struct wl_buffer *buffer;
};

struct _GdkGixCursorClass
{
  GdkCursorClass cursor_class;
};


#define GDK_TYPE_DEVICE_GIX         (gdk_device_gix_get_type ())
#define GDK_DEVICE_GIX(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_DEVICE_GIX, GdkDeviceCore))
#define GDK_DEVICE_GIX_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DEVICE_GIX, GdkDeviceCoreClass))
#define GDK_IS_DEVICE_GIX(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_DEVICE_GIX))
#define GDK_IS_DEVICE_GIX_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DEVICE_GIX))
#define GDK_DEVICE_GIX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DEVICE_GIX, GdkDeviceCoreClass))

typedef struct _GdkDeviceCore GdkDeviceCore;
typedef struct _GdkDeviceCoreClass GdkDeviceCoreClass;


struct _GdkDeviceCore
{
  GdkDevice parent_instance;
  //GdkGixDevice *device;
};

struct _GdkDeviceCoreClass
{
  GdkDeviceClass parent_class;
};



#define GDK_TYPE_DEVICE_MANAGER_GIX         (gdk_device_manager_gix_get_type ())
#define GDK_DEVICE_MANAGER_GIX(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_DEVICE_MANAGER_GIX, GdkDeviceManagerGix))
#define GDK_DEVICE_MANAGER_GIX_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), GDK_TYPE_DEVICE_MANAGER_GIX, GdkDeviceManagerGixClass))
#define GDK_IS_DEVICE_MANAGER_GIX(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_DEVICE_MANAGER_GIX))
#define GDK_IS_DEVICE_MANAGER_GIX_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), GDK_TYPE_DEVICE_MANAGER_GIX))
#define GDK_DEVICE_MANAGER_GIX_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDK_TYPE_DEVICE_MANAGER_GIX, GdkDeviceManagerGixClass))

typedef struct _GdkDeviceManagerGix GdkDeviceManagerGix;
typedef struct _GdkDeviceManagerGixClass GdkDeviceManagerGixClass;

struct _GdkDeviceManagerGix
{
  GdkDeviceManager parent_object;
  GdkDevice *core_pointer;
  GdkDevice *core_keyboard;
  //GList *devices;
};

struct _GdkDeviceManagerGixClass
{
  GdkDeviceManagerClass parent_class;
};

typedef struct _GdkGixDisplayManager GdkGixDisplayManager;
typedef struct _GdkGixDisplayManagerClass GdkGixDisplayManagerClass;

#define GDK_TYPE_GIX_DISPLAY_MANAGER              (gdk_gix_display_manager_get_type())
#define GDK_GIX_DISPLAY_MANAGER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GIX_DISPLAY_MANAGER, GdkGixDisplayManager))
#define GDK_GIX_DISPLAY_MANAGER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GIX_DISPLAY_MANAGER, GdkGixDisplayManagerClass))
#define GDK_IS_GIX_DISPLAY_MANAGER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GIX_DISPLAY_MANAGER))
#define GDK_IS_GIX_DISPLAY_MANAGER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GIX_DISPLAY_MANAGER))
#define GDK_GIX_DISPLAY_MANAGER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GIX_DISPLAY_MANAGER, GdkGixDisplayManagerClass))

typedef struct _GdkGixEventSource GdkEventSource;

#define GDK_TYPE_WINDOW_IMPL_GIX              (_gdk_window_impl_gix_get_type ())
#define GDK_WINDOW_IMPL_GIX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WINDOW_IMPL_GIX, GdkWindowImplGix))
#define GDK_WINDOW_IMPL_GIX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_WINDOW_IMPL_GIX, GdkWindowImplGixClass))
#define GDK_IS_WINDOW_IMPL_GIX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WINDOW_IMPL_GIX))
#define GDK_IS_WINDOW_IMPL_GIX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_WINDOW_IMPL_GIX))
#define GDK_WINDOW_IMPL_GIX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_WINDOW_IMPL_GIX, GdkWindowImplGixClass))

//#define GDK_WINDOW_WIN_ID(win)          (GDK_WINDOW_IMPL_GIX(win->impl)->window_id)
#define GDK_WINDOW_WIN_ID(win)           (GDK_WINDOW_IMPL_GIX(GDK_WINDOW (win)->impl)->window_id)
#define GDK_WINDOW_XID GDK_WINDOW_WIN_ID
typedef struct _GdkWindowImplGix GdkWindowImplGix;
typedef struct _GdkWindowImplGixClass GdkWindowImplGixClass;

struct _GdkWindowImplGix
{
  GdkWindowImpl parent_instance;
  GdkWindowTypeHint type_hint;

  GdkWindow *wrapper;

  GdkCursor *cursor;

  gint8 toplevel_window_type;

  //struct wl_surface *surface;
  //unsigned int mapped : 1;
  GdkWindow *transient_for;

  cairo_surface_t *cairo_surface;
  //cairo_surface_t *server_surface;

  gi_window_id_t window_id;
  uint32_t resize_edges;

  /* Set if the window, or any descendent of it, is the server's focus window
   */
  guint has_focus_window : 1;

  /* Set if window->has_focus_window and the focus isn't grabbed elsewhere.
   */
  guint has_focus : 1;

  /* Set if the pointer is inside this window. (This is needed for
   * for focus tracking)
   */
  guint has_pointer : 1;
  
  /* Set if the window is a descendent of the focus window and the pointer is
   * inside it. (This is the case where the window will receive keystroke
   * events even window->has_focus_window is FALSE)
   */
  guint has_pointer_focus : 1;

  /* Set if we are requesting these hints */
  guint skip_taskbar_hint : 1;
  guint skip_pager_hint : 1;
  guint urgency_hint : 1;

  guint on_all_desktops : 1;   /* _NET_WM_STICKY == 0xFFFFFFFF */

  guint have_sticky : 1;	/* _NET_WM_STATE_STICKY */
  guint have_maxvert : 1;       /* _NET_WM_STATE_MAXIMIZED_VERT */
  guint have_maxhorz : 1;       /* _NET_WM_STATE_MAXIMIZED_HORZ */
  guint have_fullscreen : 1;    /* _NET_WM_STATE_FULLSCREEN */

  gulong map_serial;	/* Serial of last transition from unmapped */

  cairo_surface_t *icon_pixmap;
  cairo_surface_t *icon_mask;

  /* Time of most recent user interaction. */
  gulong user_time;
};

struct _GdkWindowImplGixClass
{
  GdkWindowImplClass parent_class;
};





typedef struct _GdkScreenGix      GdkScreenGix;
typedef struct _GdkScreenGixClass GdkScreenGixClass;

#define GDK_TYPE_SCREEN_GIX              (_gdk_screen_gix_get_type ())
#define GDK_SCREEN_GIX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_SCREEN_GIX, GdkScreenGix))
#define GDK_SCREEN_GIX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_SCREEN_GIX, GdkScreenGixClass))
#define GDK_IS_SCREEN_GIX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_SCREEN_GIX))
#define GDK_IS_SCREEN_GIX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_SCREEN_GIX))
#define GDK_SCREEN_GIX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_SCREEN_GIX, GdkScreenGixClass))

typedef struct _GdkGixMonitor GdkGixMonitor;

struct _GdkScreenGix
{
  GdkScreen parent_instance;

  //GdkDisplay *scr_display;
  GdkWindow *root_window;

  int width, height;
  int width_mm, height_mm;
  gi_window_id_t xroot_window;

  gint		     n_monitors;
  GdkGixMonitor *monitors;
  gint               primary_monitor;
};

struct _GdkScreenGixClass
{
  GdkScreenClass parent_class;

  void (* window_manager_changed) (GdkScreenGix *screen_gix);
};

struct _GdkGixMonitor
{
  GdkRectangle  geometry;
  int		width_mm;
  int		height_mm;
  char *	output_name;
  char *	manufacturer;
};


#define GDK_SCREEN_XROOTWIN(screen)   (GDK_SCREEN_GIX (screen)->xroot_window)

#define GDK_SCREEN_DISPLAY(screen)    (GDK_SCREEN_GIX (screen)->display)
#define GDK_WINDOW_SCREEN(win)	      (gdk_window_get_screen (win))
//#define GDK_WINDOW_DISPLAY(win)       (GDK_SCREEN_GIX (GDK_WINDOW_SCREEN (win))->scr_display)
#define GDK_WINDOW_IS_GIX(win)    (GDK_IS_WINDOW_IMPL_GIX (((GdkWindow *)win)->impl))

GType _gdk_gix_window_get_type    (void);
void  _gdk_gix_window_update_size (GdkWindow *window,
				       int32_t width,
				       int32_t height,
				       uint32_t edges);

GdkKeymap *_gdk_gix_keymap_new (GdkDisplay *display);
struct xkb_desc *_gdk_gix_keymap_get_xkb_desc (GdkKeymap *keymap);

GdkCursor *_gdk_gix_display_get_cursor_for_type (GdkDisplay    *display,
						     GdkCursorType  cursor_type);
GdkCursor *_gdk_gix_display_get_cursor_for_name (GdkDisplay  *display,
						     const gchar *name);
GdkCursor *_gdk_gix_display_get_cursor_for_pixbuf (GdkDisplay *display,
						       GdkPixbuf  *pixbuf,
						       gint        x,
						       gint        y);
void       _gdk_gix_display_get_default_cursor_size (GdkDisplay *display,
							 guint       *width,
							 guint       *height);
void       _gdk_gix_display_get_maximal_cursor_size (GdkDisplay *display,
							 guint       *width,
							 guint       *height);
gboolean   _gdk_gix_display_supports_cursor_alpha (GdkDisplay *display);
gboolean   _gdk_gix_display_supports_cursor_color (GdkDisplay *display);

int _gdk_gix_cursor_get_id (GdkCursor *cursor,
						  int       *x,
						  int       *y);

GdkDragProtocol _gdk_gix_window_get_drag_protocol (GdkWindow *window,
						       GdkWindow **target);

void            _gdk_gix_window_register_dnd (GdkWindow *window);
GdkDragContext *_gdk_gix_window_drag_begin (GdkWindow *window,
						GdkDevice *device,
						GList     *targets);

void _gdk_gix_display_create_window_impl (GdkDisplay    *display,
					      GdkWindow     *window,
					      GdkWindow     *real_parent,
					      GdkScreen     *screen,
					      GdkEventMask   event_mask,
					      GdkWindowAttr *attributes,
					      gint           attributes_mask);

GdkKeymap *_gdk_gix_display_get_keymap (GdkDisplay *display);

GdkWindow *_gdk_gix_display_get_selection_owner (GdkDisplay *display,
						 GdkAtom     selection);
gboolean   _gdk_gix_display_set_selection_owner (GdkDisplay *display,
						     GdkWindow  *owner,
						     GdkAtom     selection,
						     guint32     time,
						     gboolean    send_event);
void       _gdk_gix_display_send_selection_notify (GdkDisplay *dispay,
						       GdkWindow        *requestor,
						       GdkAtom          selection,
						       GdkAtom          target,
						       GdkAtom          property,
						       guint32          time);
gint       _gdk_gix_display_get_selection_property (GdkDisplay  *display,
							GdkWindow   *requestor,
							guchar     **data,
							GdkAtom     *ret_type,
							gint        *ret_format);
void       _gdk_gix_display_convert_selection (GdkDisplay *display,
						   GdkWindow  *requestor,
						   GdkAtom     selection,
						   GdkAtom     target,
						   guint32     time);
gint        _gdk_gix_display_text_property_to_utf8_list (GdkDisplay    *display,
							     GdkAtom        encoding,
							     gint           format,
							     const guchar  *text,
							     gint           length,
							     gchar       ***list);
gchar *     _gdk_gix_display_utf8_to_string_target (GdkDisplay  *display,
							const gchar *str);

GdkDeviceManager *_gdk_gix_device_manager_new (GdkDisplay *display);
void              _gdk_gix_device_manager_add_device (GdkDeviceManager *device_manager,
							  void  *device);
void  *_gdk_gix_device_get_device (GdkDevice *device);

void     _gdk_gix_display_deliver_event (GdkDisplay *display, GdkEvent *event);
GSource *_gdk_gix_display_event_source_new (GdkDisplay *display);
void     _gdk_gix_display_queue_events (GdkDisplay *display);
void     _gdk_gix_display_flush (GdkDisplay *display, GSource *source);

GdkAppLaunchContext *_gdk_gix_display_get_app_launch_context (GdkDisplay *display);

GdkDisplay *_gdk_gix_display_open (const gchar *display_name);

GdkWindow *_gdk_gix_screen_create_root_window (GdkScreen *screen,
						   int width,
						   int height);

GdkScreen *_gdk_windowing_window_init (GdkDisplay *display, GdkScreen *screen);

void _gdk_gix_display_manager_add_display (GdkDisplayManager *manager,
					       GdkDisplay        *display);
void _gdk_gix_display_manager_remove_display (GdkDisplayManager *manager,
						  GdkDisplay        *display);
gulong
_gdk_gix_get_next_tick (gulong suggested_tick);

GdkWindow *
gdk_gix_window_lookup_for_display (GdkDisplay *display,
                                   gi_window_id_t      window);

GdkAtom
gdk_x11_xatom_to_atom_for_display (GdkDisplay *display,
				   gi_atom_id_t	       xatom);

extern GdkDisplay *_gdk_display;
extern GdkScreen *_gdk_screen;
extern GdkWindow *_gdk_root;

#define unimplemented g_warning ("unimplemented %s: Got Line %d\n",__FUNCTION__,__LINE__);
#endif /* __GDK_PRIVATE_GIX_H__ */
