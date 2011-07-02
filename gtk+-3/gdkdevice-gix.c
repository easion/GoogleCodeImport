
#include "config.h"

#include <string.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdktypes.h>

#include "gdkgix.h"
#include "gdkkeysyms.h"
#include "gdkprivate-gix.h"

G_DEFINE_TYPE (GdkDeviceCore, gdk_device_gix, GDK_TYPE_DEVICE)

G_DEFINE_TYPE (GdkDeviceManagerGix,
	       gdk_device_manager_core, GDK_TYPE_DEVICE_MANAGER)



/***********************************************************/

static gboolean
gdk_device_gix_get_history (GdkDevice      *device,
                             GdkWindow      *window,
                             guint32         start,
                             guint32         stop,
                             GdkTimeCoord ***events,
                             gint           *n_events)
{
  return FALSE;
}

static void
gdk_device_gix_get_state (GdkDevice       *device,
                           GdkWindow       *window,
                           gdouble         *axes,
                           GdkModifierType *mask)
{
  gint x_int, y_int;

  gdk_window_get_pointer (window, &x_int, &y_int, mask);

  if (axes)
    {
      axes[0] = x_int;
      axes[1] = y_int;
    }
}

static void
gdk_device_gix_set_window_cursor (GdkDevice *device,
                                   GdkWindow *window,
                                   GdkCursor *cursor)
{
  GdkGixCursor *gix_cursor = GDK_GIX_CURSOR (cursor); 
  int id;
  int x, y;

  if (cursor)
    {
      id = _gdk_gix_cursor_get_id(cursor, &x, &y);
	  gi_load_cursor(GDK_WINDOW_WIN_ID(window), id);
	  LOG_UNUSED;
    }
  else
    {
	  LOG_UNUSED;
	  id = GI_CURSOR_ARROW;
	  //id = GI_CURSOR_NONE;
	  gi_load_cursor(GDK_WINDOW_WIN_ID(window), id);
    }
}


static GdkWindow *
gdk_device_gix_window_at_position (GdkDevice       *device,
                                    gint            *win_x,
                                    gint            *win_y,
                                    GdkModifierType *mask,
                                    gboolean         get_toplevel)
{
  GdkDisplay *display;
  GdkScreen *screen;
  GdkWindow *window;
  gi_window_id_t xwindow,  child;
  int xroot_x, xroot_y, xwin_x, xwin_y;
  unsigned int xmask;

  display = gdk_device_get_display (device);
  screen = gdk_display_get_default_screen (display);  

  xwindow = GDK_SCREEN_XROOTWIN (screen);
  
  gi_query_pointer ( xwindow,
				 &child,
				 &xroot_x, &xroot_y,
				 &xwin_x, &xwin_y,
				 &xmask);

  window = gdk_gix_window_lookup_for_display (display, child);

  if (win_x)
    *win_x = (window) ? xwin_x : -1;

  if (win_y)
    *win_y = (window) ? xwin_y : -1;

  if (mask)
    *mask = xmask;

  return window;

}


static gboolean
gdk_device_gix_query_state (GdkDevice        *device,
                             GdkWindow        *window,
                             GdkWindow       **root_window,
                             GdkWindow       **child_window,
                             gint             *root_x,
                             gint             *root_y,
                             gint             *win_x,
                             gint             *win_y,
                             GdkModifierType  *mask)
{
  GdkDisplay *display;
  GdkScreen *default_screen;
  gi_window_id_t  xchild_window;
  int xroot_x, xroot_y, xwin_x, xwin_y;
  unsigned int xmask;

  display = gdk_window_get_display (window);
  default_screen = gdk_display_get_default_screen (display);

  
  if (gi_query_pointer (
		  GDK_WINDOW_XID (window),
		  &xchild_window,
		  &xroot_x, &xroot_y,
		  &xwin_x, &xwin_y,
		  &xmask) <= 0)
	return FALSE;
   

  if (root_window)
    *root_window = gdk_gix_window_lookup_for_display (display, GI_DESKTOP_WINDOW_ID);


  if (child_window)
    *child_window = gdk_gix_window_lookup_for_display (display, xchild_window);

  if (root_x)
    *root_x = xroot_x;

  if (root_y)
    *root_y = xroot_y;

  if (win_x)
    *win_x = xwin_x;

  if (win_y)
    *win_y = xwin_y;

  if (mask)
    *mask = xmask;

  return TRUE;
}


static void
gdk_device_gix_warp (GdkDevice *device,
                      GdkScreen *screen,
                      gint       x,
                      gint       y)
{
}



const int _gdk_gix_event_mask_table[21] =
{
  GI_MASK_EXPOSURE,
  GI_MASK_MOUSE_MOVE,
  GI_MASK_MOUSE_MOVE, //PointerMotionHintMask,
  GI_MASK_MOUSE_MOVE, //ButtonMotionMask,
  GI_MASK_MOUSE_MOVE, //Button1MotionMask,
  GI_MASK_MOUSE_MOVE, //Button2MotionMask,
  GI_MASK_MOUSE_MOVE, //Button3MotionMask,
  GI_MASK_BUTTON_DOWN,
  GI_MASK_BUTTON_UP,
  GI_MASK_KEY_DOWN,

  GI_MASK_KEY_UP,
  GI_MASK_MOUSE_ENTER,
  GI_MASK_MOUSE_EXIT,
  GI_MASK_FOCUS_IN|GI_MASK_FOCUS_OUT,
  GI_MASK_CONFIGURENOTIFY,
  GI_MASK_PROPERTYNOTIFY,
  GI_MASK_WINDOW_SHOW|GI_MASK_WINDOW_HIDE,
  0,                    /* PROXIMITY_IN */
  0,                    /* PROXIMTY_OUT */
  GI_MASK_CONFIGURENOTIFY,

  GI_MASK_BUTTON_DOWN      /* SCROLL; on X mouse wheel events is treated as mouse button 4/5 */
};

const gint _gdk_gix_event_mask_table_size = G_N_ELEMENTS (_gdk_gix_event_mask_table);


void
gdk_gix_event_source_select_events (GdkEventSource *source,
                                    gi_window_id_t          window,
                                    GdkEventMask    event_mask,
                                    unsigned int    extra_x_mask)
{
  unsigned int xmask = extra_x_mask;
  GList *list;
  gint i; 

  for (i = 0; i < _gdk_gix_event_mask_table_size; i++)
    {
      if (event_mask & (1 << (i + 1)))
        xmask |= _gdk_gix_event_mask_table[i];
    }

  gi_set_events_mask ( window, xmask|GI_MASK_CLIENT_MSG);
}




static GdkGrabStatus
gdk_device_gix_grab (GdkDevice    *device,
                      GdkWindow    *window,
                      gboolean      owner_events,
                      GdkEventMask  event_mask,
                      GdkWindow    *confine_to,
                      GdkCursor    *cursor,
                      guint32       time_)
{
  GdkDisplay *display;
  gi_window_id_t xwindow, xconfine_to;
  gint status;

  display = gdk_device_get_display (device);

  xwindow = GDK_WINDOW_XID (window);

  if (confine_to)
    confine_to = _gdk_window_get_impl_window (confine_to);

  if (!confine_to || GDK_WINDOW_DESTROYED (confine_to))
    xconfine_to = 0;
  else
    xconfine_to = GDK_WINDOW_XID (confine_to);


  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      /* Device is a keyboard */
      status = gi_grab_keyboard (
                              xwindow,
                              owner_events,
                              0, 0 );
    }
  else
    {
      int xcursor = 0;
      guint xevent_mask;
      gint i;

      /* Device is a pointer */   
      xevent_mask = 0;

      for (i = 0; i < _gdk_gix_event_mask_table_size; i++)
        {
          if (event_mask & (1 << (i + 1)))
            xevent_mask |= _gdk_gix_event_mask_table[i];
        }

      /* We don't want to set a native motion hint mask, as we're emulating motion
       * hints. If we set a native one we just wouldn't get any events.
       */
      //xevent_mask &= ~PointerMotionHintMask;

      status = gi_grab_pointer (xwindow,
							 TRUE,
                             0, 
                             owner_events,                             
                             xcursor,
                             GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M);
    }

  //_gdk_x11_display_update_grab_info (display, device, status);
  //return _gdk_x11_convert_grab_status (status);
  return GDK_GRAB_SUCCESS;
}

static void
gdk_device_gix_ungrab (GdkDevice *device,
                        guint32    time_)
{
  GdkDisplay *display;
  GdkDeviceGrabInfo *info;

  display = gdk_device_get_display (device);

 info = _gdk_display_get_last_device_grab (display, device);

  if (info)
    info->serial_end = 0;

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    gi_ungrab_keyboard ();
  else
    gi_ungrab_pointer ();

  //_gdk_x11_display_update_grab_info_ungrab (display, device, time_, serial);
  _gdk_display_device_grab_update (display, device, NULL, 0);
}

static void
gdk_device_gix_select_window_events (GdkDevice    *device,
                                      GdkWindow    *window,
                                      GdkEventMask  event_mask)
{
}
/***********************************************************/

static void
gdk_device_gix_class_init (GdkDeviceCoreClass *klass)
{
  GdkDeviceClass *device_class = GDK_DEVICE_CLASS (klass);

  device_class->get_history = gdk_device_gix_get_history;
  device_class->get_state = gdk_device_gix_get_state;
  device_class->set_window_cursor = gdk_device_gix_set_window_cursor;
  device_class->warp = gdk_device_gix_warp;
  device_class->query_state = gdk_device_gix_query_state;
  device_class->grab = gdk_device_gix_grab;
  device_class->ungrab = gdk_device_gix_ungrab;
  device_class->window_at_position = gdk_device_gix_window_at_position;
  device_class->select_window_events = gdk_device_gix_select_window_events;
}

static void
gdk_device_gix_init (GdkDeviceCore *device_core)
{
  GdkDevice *device;

  device = GDK_DEVICE (device_core);

  _gdk_device_add_axis (device, GDK_NONE, GDK_AXIS_X, 0, 0, 1);
  _gdk_device_add_axis (device, GDK_NONE, GDK_AXIS_Y, 0, 0, 1);
}

/***********************************************************/
static void
gdk_device_manager_core_finalize (GObject *object);
static void
gdk_device_manager_core_constructed (GObject *object);
static GList *
gdk_device_manager_core_list_devices (GdkDeviceManager *device_manager,
                                      GdkDeviceType     type);
static GdkDevice *
gdk_device_manager_core_get_client_pointer (GdkDeviceManager *device_manager);

static void
gdk_device_manager_core_class_init (GdkDeviceManagerGixClass *klass)
{
  GdkDeviceManagerClass *device_manager_class = GDK_DEVICE_MANAGER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gdk_device_manager_core_finalize;
  object_class->constructed = gdk_device_manager_core_constructed;
  device_manager_class->list_devices = gdk_device_manager_core_list_devices;
  device_manager_class->get_client_pointer = gdk_device_manager_core_get_client_pointer;
}

static GdkDevice *
create_core_pointer (GdkDeviceManager *device_manager,GdkDisplay       *display)
{
  return g_object_new (GDK_TYPE_DEVICE_GIX,
                       "name", "Core Pointer",
                       "type", GDK_DEVICE_TYPE_MASTER,
                       "input-source", GDK_SOURCE_MOUSE,
                       "input-mode", GDK_MODE_SCREEN,
                       "has-cursor", TRUE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}

static GdkDevice *
create_core_keyboard (GdkDeviceManager *device_manager,GdkDisplay       *display)
{
  return g_object_new (GDK_TYPE_DEVICE_GIX,
                       "name", "Core Keyboard",
                       "type", GDK_DEVICE_TYPE_MASTER,
                       "input-source", GDK_SOURCE_KEYBOARD,
                       "input-mode", GDK_MODE_SCREEN,
                       "has-cursor", FALSE,
                       "display", display,
                       "device-manager", device_manager,
                       NULL);
}



static void
gdk_device_manager_core_init (GdkDeviceManagerGix *device_manager)
{
}


static void
gdk_device_manager_core_finalize (GObject *object)
{
  GdkDeviceManagerGix *device_manager_core;

  device_manager_core = GDK_DEVICE_MANAGER_GIX (object);

  g_object_unref (device_manager_core->core_pointer);
  g_object_unref (device_manager_core->core_keyboard);

  G_OBJECT_CLASS (gdk_device_manager_core_parent_class)->finalize (object);
}


static void
gdk_device_manager_core_constructed (GObject *object)
{
  GdkDeviceManagerGix *device_manager;
  GdkDisplay *display;

  device_manager = GDK_DEVICE_MANAGER_GIX (object);
  display = gdk_device_manager_get_display (GDK_DEVICE_MANAGER (object));
  device_manager->core_pointer = create_core_pointer (GDK_DEVICE_MANAGER (device_manager), display);
  device_manager->core_keyboard = create_core_keyboard (GDK_DEVICE_MANAGER (device_manager), display);

  _gdk_device_set_associated_device (device_manager->core_pointer, device_manager->core_keyboard);
  _gdk_device_set_associated_device (device_manager->core_keyboard, device_manager->core_pointer);
}


static GList *
gdk_device_manager_core_list_devices (GdkDeviceManager *device_manager,
                                      GdkDeviceType     type)
{
  GdkDeviceManagerGix *device_manager_core;
  GList *devices = NULL;

  if (type == GDK_DEVICE_TYPE_MASTER)
    {
      device_manager_core = (GdkDeviceManagerGix *) device_manager;
	  devices = g_list_prepend (devices, device_manager_core->core_keyboard);
      devices = g_list_prepend (devices, device_manager_core->core_pointer);
    }

  return devices;
}

static GdkDevice *
gdk_device_manager_core_get_client_pointer (GdkDeviceManager *device_manager)
{
  GdkDeviceManagerGix *device_manager_core;

  device_manager_core = (GdkDeviceManagerGix *) device_manager;
  return device_manager_core->core_pointer;
}

