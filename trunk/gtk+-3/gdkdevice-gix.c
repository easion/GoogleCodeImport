
#include "config.h"

#include <string.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdktypes.h>

#include "gdkgix.h"
#include "gdkkeysyms.h"
#include "gdkprivate-gix.h"

G_DEFINE_TYPE (GdkDeviceCore, gdk_device_core, GDK_TYPE_DEVICE)

G_DEFINE_TYPE (GdkDeviceManagerCore,
	       gdk_device_manager_core, GDK_TYPE_DEVICE_MANAGER)



/***********************************************************/

static gboolean
gdk_device_core_get_history (GdkDevice      *device,
                             GdkWindow      *window,
                             guint32         start,
                             guint32         stop,
                             GdkTimeCoord ***events,
                             gint           *n_events)
{
  return FALSE;
}

static void
gdk_device_core_get_state (GdkDevice       *device,
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
gdk_device_core_set_window_cursor (GdkDevice *device,
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
      //wl_input_device_attach(wd->device, wd->time, buffer, x, y);
    }
  else
    {
	  LOG_UNUSED;
	  id = GI_CURSOR_ARROW;
	  //id = GI_CURSOR_NONE;
	  gi_load_cursor(GDK_WINDOW_WIN_ID(window), id);
      //wl_input_device_attach(wd->device, wd->time, NULL, 0, 0);
    }
}


static GdkWindow *
gdk_device_core_window_at_position (GdkDevice       *device,
                                    gint            *win_x,
                                    gint            *win_y,
                                    GdkModifierType *mask,
                                    gboolean         get_toplevel)
{
  GdkDisplay *display;
  GdkScreen *screen;
  //Display *xdisplay;
  GdkWindow *window;
  gi_window_id_t xwindow,  child;
  int xroot_x, xroot_y, xwin_x, xwin_y;
  unsigned int xmask;

  display = gdk_device_get_display (device);
  screen = gdk_display_get_default_screen (display);
  

  //xdisplay = GDK_SCREEN_XDISPLAY (screen);
  xwindow = GDK_SCREEN_XROOTWIN (screen);

  
  gi_query_pointer ( xwindow,
				 &child,
				 &xroot_x, &xroot_y,
				 &xwin_x, &xwin_y,
				 &xmask);

  //if (root == xwindow)
	//xwindow = child;
  //else
	//xwindow = root;

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
gdk_device_core_query_state (GdkDevice        *device,
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
gdk_device_core_warp (GdkDevice *device,
                      GdkScreen *screen,
                      gint       x,
                      gint       y)
{
}

static GdkGrabStatus
gdk_device_core_grab (GdkDevice    *device,
                      GdkWindow    *window,
                      gboolean      owner_events,
                      GdkEventMask  event_mask,
                      GdkWindow    *confine_to,
                      GdkCursor    *cursor,
                      guint32       time_)
{
  return GDK_GRAB_SUCCESS;
}

static void
gdk_device_core_ungrab (GdkDevice *device,
                        guint32    time_)
{
}

static void
gdk_device_core_select_window_events (GdkDevice    *device,
                                      GdkWindow    *window,
                                      GdkEventMask  event_mask)
{
}
/***********************************************************/

static void
gdk_device_core_class_init (GdkDeviceCoreClass *klass)
{
  GdkDeviceClass *device_class = GDK_DEVICE_CLASS (klass);

  device_class->get_history = gdk_device_core_get_history;
  device_class->get_state = gdk_device_core_get_state;
  device_class->set_window_cursor = gdk_device_core_set_window_cursor;
  device_class->warp = gdk_device_core_warp;
  device_class->query_state = gdk_device_core_query_state;
  device_class->grab = gdk_device_core_grab;
  device_class->ungrab = gdk_device_core_ungrab;
  device_class->window_at_position = gdk_device_core_window_at_position;
  device_class->select_window_events = gdk_device_core_select_window_events;
}

static void
gdk_device_core_init (GdkDeviceCore *device_core)
{
  GdkDevice *device;

  device = GDK_DEVICE (device_core);

  _gdk_device_add_axis (device, GDK_NONE, GDK_AXIS_X, 0, 0, 1);
  _gdk_device_add_axis (device, GDK_NONE, GDK_AXIS_Y, 0, 0, 1);
}

/*
void *
_gdk_gix_device_get_device (GdkDevice *device)
{
  return GDK_DEVICE_CORE (device)->device->device;
}


static void
free_device (void *data, void *user_data)
{
  g_object_unref (data);
}
*/

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
gdk_device_manager_core_class_init (GdkDeviceManagerCoreClass *klass)
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
  return g_object_new (GDK_TYPE_DEVICE_CORE,
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
  return g_object_new (GDK_TYPE_DEVICE_CORE,
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
gdk_device_manager_core_init (GdkDeviceManagerCore *device_manager)
{
}


static void
gdk_device_manager_core_finalize (GObject *object)
{
  GdkDeviceManagerCore *device_manager_core;

  device_manager_core = GDK_DEVICE_MANAGER_CORE (object);

  g_object_unref (device_manager_core->core_pointer);
  g_object_unref (device_manager_core->core_keyboard);

  G_OBJECT_CLASS (gdk_device_manager_core_parent_class)->finalize (object);
}


static void
gdk_device_manager_core_constructed (GObject *object)
{
  GdkDeviceManagerCore *device_manager;
  GdkDisplay *display;

  device_manager = GDK_DEVICE_MANAGER_CORE (object);
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
  GdkDeviceManagerCore *device_manager_core;
  GList *devices = NULL;

  if (type == GDK_DEVICE_TYPE_MASTER)
    {
      device_manager_core = (GdkDeviceManagerCore *) device_manager;
	  devices = g_list_prepend (devices, device_manager_core->core_keyboard);
      devices = g_list_prepend (devices, device_manager_core->core_pointer);
    }

  return devices;
}

static GdkDevice *
gdk_device_manager_core_get_client_pointer (GdkDeviceManager *device_manager)
{
  GdkDeviceManagerCore *device_manager_core;

  device_manager_core = (GdkDeviceManagerCore *) device_manager;
  return device_manager_core->core_pointer;
}

