

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include "gdkgix.h"
#include "gdkdisplay.h"
#include "gdkdisplay-gix.h"
#include "gdkscreen.h"
#include "gdkinternals.h"
#include "gdkdeviceprivate.h"
#include "gdkdevicemanager.h"
#include "gdkkeysprivate.h"
#include "gdkprivate-gix.h"

void _gdk_visual_init ();

G_DEFINE_TYPE (GdkDisplayGix, _gdk_display_gix, GDK_TYPE_DISPLAY)


GdkDisplay *_gdk_display = NULL;
GdkScreen *_gdk_screen = NULL;
GdkWindow *_gdk_root = NULL;


static void
_gdk_input_init (GdkDisplay *display)
{
  GdkDisplayGix *display_gix;
  //GdkDeviceManager *device_manager;
  GdkDeviceManagerGix *gix_device_manager;
  GdkDevice *device;
  GList *list, *l;

  display_gix = GDK_DISPLAY_GIX (display);
 
  //gdk_display_get_device_manager (display);
  gix_device_manager = g_object_new (GDK_TYPE_DEVICE_MANAGER_GIX,
                       "display", display,
                       NULL);
  //display->device_manager =device_manager;
  display->device_manager = GDK_DEVICE_MANAGER (gix_device_manager);
  display->core_pointer = gix_device_manager->core_pointer;
  
  list = gdk_device_manager_list_devices (display->device_manager, GDK_DEVICE_TYPE_FLOATING);

  for (l = list; l; l = l->next)
    {
      device = l->data;

      if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
	continue;

      display_gix->input_devices = g_list_prepend (display_gix->input_devices, l->data);
    }

  g_list_free (list);
 
  list = gdk_device_manager_list_devices (display->device_manager, GDK_DEVICE_TYPE_MASTER);

  for (l = list; l; l = l->next)
    {
      device = list->data;

      if (gdk_device_get_source (device) != GDK_SOURCE_MOUSE)
	continue;

      display->core_pointer = device;
      break;
    }


  display_gix->input_devices = g_list_prepend (display_gix->input_devices, display->core_pointer);

  g_list_free (list);
  
}


GdkDeviceManager *
_gdk_gix_device_manager_new (GdkDisplay *display)
{
  return g_object_new (GDK_TYPE_DEVICE_MANAGER_GIX,
                       "display", display,
                       NULL);
}

GdkDisplay *
_gdk_gix_display_open (const gchar *display_name)
{
  GdkDisplayGix *display_gix;
  int fd;

  if (_gdk_display != NULL)
	{
	  GDK_NOTE (MISC, g_print ("... return _gdk_display\n"));
	  return _gdk_display;
	}

  if (display_name == NULL ||
      g_ascii_strcasecmp (display_name, gdk_display_get_name (_gdk_display)) == 0)
    {
      
    }
  else
    {
      GDK_NOTE (MISC, g_print ("... return NULL\n"));
      return NULL;
    }

  //g_print("_gdk_gix_display_open %s\n", display_name?display_name:"NULL");

  _gdk_display =  g_object_new (GDK_TYPE_DISPLAY_GIX, NULL);
   _gdk_screen = g_object_new (GDK_TYPE_SCREEN_GIX, NULL);
 
  fd = gi_init();
  if (fd < 0) {
	  return NULL;
  }
  display_gix = GDK_DISPLAY_GIX (_gdk_display);
  display_gix->fd = fd;
  gi_get_screen_info(&display_gix->screen_info);
  display_gix->display_screen = _gdk_screen; 
  display_gix->event_source = _gdk_gix_display_event_source_new (_gdk_display);

  _gdk_visual_init ();
  _gdk_windowing_window_init (_gdk_display,_gdk_screen);
  _gdk_input_init (_gdk_display);

  /* Precalculate display name */
  (void) gdk_display_get_name (_gdk_display);

  g_signal_emit_by_name (_gdk_display, "opened");
  g_signal_emit_by_name (gdk_display_manager_get(), "display_opened", _gdk_display);
  GDK_NOTE (MISC, g_print ("... _gdk_display now set up\n"));

  return _gdk_display;
}

static void
gdk_gix_display_dispose (GObject *object)
{
  GdkDisplayGix *display_gix = GDK_DISPLAY_GIX (object);

  _gdk_gix_display_manager_remove_display (gdk_display_manager_get (),
					       GDK_DISPLAY (display_gix));
  g_list_foreach (display_gix->input_devices,
		  (GFunc) g_object_run_dispose, NULL);

  _gdk_screen_close (display_gix->display_screen);

  if (display_gix->event_source)
    {
      g_source_destroy (display_gix->event_source);
      g_source_unref (display_gix->event_source);
      display_gix->event_source = NULL;
    }

  gi_exit();

  G_OBJECT_CLASS (_gdk_display_gix_parent_class)->dispose (object);
}

static void
gdk_gix_display_finalize (GObject *object)
{
  GdkDisplayGix *display_gix = GDK_DISPLAY_GIX (object);

  /* Keymap */
  if (display_gix->keymap)
    g_object_unref (display_gix->keymap);

  /* input GdkDevice list */
  g_list_foreach (display_gix->input_devices, (GFunc) g_object_unref, NULL);
  g_list_free (display_gix->input_devices);

  g_object_unref (display_gix->display_screen);

  g_free (display_gix->startup_notification_id);

  /* Atom Hashtable */
  g_hash_table_destroy (display_gix->atom_from_virtual);
  g_hash_table_destroy (display_gix->atom_to_virtual);

  G_OBJECT_CLASS (_gdk_display_gix_parent_class)->finalize (object);
}

static G_CONST_RETURN gchar *
gdk_gix_display_get_name (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  return "Gix";
}

static gint
gdk_gix_display_get_n_screens (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), 0);
  return 1;
}

static GdkScreen *
gdk_gix_display_get_screen (GdkDisplay *display, 
				gint        screen_num)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (screen_num == 0, NULL);

  //return GDK_DISPLAY_GIX (display)->display_screen;
  return _gdk_screen;
}

static GdkScreen *
gdk_gix_display_get_default_screen (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  //return GDK_DISPLAY_GIX (display)->display_screen;
  return _gdk_screen;
}

static void
gdk_gix_display_beep (GdkDisplay *display)
{
  g_return_if_fail (GDK_IS_DISPLAY (display));
}


static void
gdk_gix_display_sync (GdkDisplay *display)
{
  GdkDisplayGix *display_gix;
  gboolean done;

  g_return_if_fail (GDK_IS_DISPLAY (display));

  display_gix = GDK_DISPLAY_GIX (display);

  /* Not supported. */

}

static void
gdk_gix_display_flush (GdkDisplay *display)
{
  g_return_if_fail (GDK_IS_DISPLAY (display));

  if (!display->closed)
    _gdk_gix_display_flush (display,
				GDK_DISPLAY_GIX (display)->event_source);
}

static gboolean
gdk_gix_display_has_pending (GdkDisplay *display)
{
  return (_gdk_event_queue_find_first (display) ||
         (gi_get_event_count () > 0 ));
}

static GdkWindow *
gdk_gix_display_get_default_group (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  return NULL;
}


static gboolean
gdk_gix_display_supports_selection_notification (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  return FALSE;
  //return TRUE;
}

static gboolean
gdk_gix_display_request_selection_notification (GdkDisplay *display,
						    GdkAtom     selection)

{
    return FALSE;
}

static gboolean
gdk_gix_display_supports_clipboard_persistence (GdkDisplay *display)
{
  return FALSE;
}

static void
gdk_gix_display_store_clipboard (GdkDisplay    *display,
				     GdkWindow     *clipboard_window,
				     guint32        time_,
				     const GdkAtom *targets,
				     gint           n_targets)
{
}

static gboolean
gdk_gix_display_supports_shapes (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  return TRUE;
  //return TRUE;
}

static gboolean
gdk_gix_display_supports_input_shapes (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);

  /* Not yet implemented. See comment in
   * gdk_window_input_shape_combine_mask().
   */

  return FALSE;
}

static gboolean
gdk_gix_display_supports_composite (GdkDisplay *display)
{
  //return TRUE;
  return FALSE;
}

static GList *
gdk_gix_display_list_devices (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  return GDK_DISPLAY_GIX (display)->input_devices;
}

static void
gdk_gix_display_before_process_all_updates (GdkDisplay *display)
{
}

static void
gdk_gix_display_after_process_all_updates (GdkDisplay *display)
{
  /* Post the damage here instead? */
}

static gulong
gdk_gix_display_get_next_serial (GdkDisplay *display)
{
  return 0;
}



void
gdk_gix_display_broadcast_startup_message (GdkDisplay *display,
					       const char *message_type,
					       ...)
{
  GString *message;
  va_list ap;
  const char *key, *value, *p;

  message = g_string_new (message_type);
  g_string_append_c (message, ':');

  va_start (ap, message_type);
  while ((key = va_arg (ap, const char *)))
    {
      value = va_arg (ap, const char *);
      if (!value)
	continue;

      g_string_append_printf (message, " %s=\"", key);
      for (p = value; *p; p++)
	{
	  switch (*p)
	    {
	    case ' ':
	    case '"':
	    case '\\':
	      g_string_append_c (message, '\\');
	      break;
	    }

	  g_string_append_c (message, *p);
	}
      g_string_append_c (message, '\"');
    }
  va_end (ap);

  //printf ("startup message: %s\n", message->str);

  g_string_free (message, TRUE);
}

static void
gdk_gix_display_notify_startup_complete (GdkDisplay  *display,
					     const gchar *startup_id)
{
  gdk_gix_display_broadcast_startup_message (display, "remove",
						 "ID", startup_id,
						 NULL);
}

static void
gdk_gix_display_event_data_copy (GdkDisplay     *display,
				     const GdkEvent *src,
				     GdkEvent       *dst)
{
}

static void
gdk_gix_display_event_data_free (GdkDisplay *display,
				     GdkEvent   *event)
{
}

static GdkKeymap *
gdk_gix_display_get_keymap (GdkDisplay *display)
{
  GdkDisplayGix *display_gix;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  display_gix = GDK_DISPLAY_GIX (display);

  if (!display_gix->keymap)
    display_gix->keymap = _gdk_gix_keymap_new (display);

  return display_gix->keymap;
}

static void
gdk_gix_display_push_error_trap (GdkDisplay *display)
{
}

static gint
gdk_gix_display_pop_error_trap (GdkDisplay *display,
				    gboolean    ignored)
{
  return 0;
}

static void
_gdk_display_gix_class_init (GdkDisplayGixClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkDisplayClass *display_class = GDK_DISPLAY_CLASS (class);

  object_class->dispose = gdk_gix_display_dispose;
  object_class->finalize = gdk_gix_display_finalize;

  display_class->window_type = _gdk_gix_window_get_type ();
  display_class->get_name = gdk_gix_display_get_name;
  display_class->get_n_screens = gdk_gix_display_get_n_screens;
  display_class->get_screen = gdk_gix_display_get_screen;
  display_class->get_default_screen = gdk_gix_display_get_default_screen;
  display_class->beep = gdk_gix_display_beep;
  display_class->sync = gdk_gix_display_sync;
  display_class->flush = gdk_gix_display_flush;
  display_class->has_pending = gdk_gix_display_has_pending;
  display_class->queue_events = _gdk_gix_display_queue_events;
  display_class->get_default_group = gdk_gix_display_get_default_group;
  display_class->supports_selection_notification = gdk_gix_display_supports_selection_notification;
  display_class->request_selection_notification = gdk_gix_display_request_selection_notification;
  display_class->supports_clipboard_persistence = gdk_gix_display_supports_clipboard_persistence;
  display_class->store_clipboard = gdk_gix_display_store_clipboard;
  display_class->supports_shapes = gdk_gix_display_supports_shapes;
  display_class->supports_input_shapes = gdk_gix_display_supports_input_shapes;
  display_class->supports_composite = gdk_gix_display_supports_composite;
  display_class->list_devices = gdk_gix_display_list_devices;
  display_class->get_app_launch_context = _gdk_gix_display_get_app_launch_context;
  display_class->get_default_cursor_size = _gdk_gix_display_get_default_cursor_size;
  display_class->get_maximal_cursor_size = _gdk_gix_display_get_maximal_cursor_size;
  display_class->get_cursor_for_type = _gdk_gix_display_get_cursor_for_type;
  display_class->get_cursor_for_name = _gdk_gix_display_get_cursor_for_name;
  display_class->get_cursor_for_pixbuf = _gdk_gix_display_get_cursor_for_pixbuf;
  display_class->supports_cursor_alpha = _gdk_gix_display_supports_cursor_alpha;
  display_class->supports_cursor_color = _gdk_gix_display_supports_cursor_color;
  display_class->before_process_all_updates = gdk_gix_display_before_process_all_updates;
  display_class->after_process_all_updates = gdk_gix_display_after_process_all_updates;
  display_class->get_next_serial = gdk_gix_display_get_next_serial;
  display_class->notify_startup_complete = gdk_gix_display_notify_startup_complete;
  display_class->event_data_copy = gdk_gix_display_event_data_copy;
  display_class->event_data_free = gdk_gix_display_event_data_free;
  display_class->create_window_impl = _gdk_gix_display_create_window_impl;
  display_class->get_keymap = gdk_gix_display_get_keymap;
  display_class->push_error_trap = gdk_gix_display_push_error_trap;
  display_class->pop_error_trap = gdk_gix_display_pop_error_trap;
  display_class->get_selection_owner = _gdk_gix_display_get_selection_owner;
  display_class->set_selection_owner = _gdk_gix_display_set_selection_owner;
  display_class->send_selection_notify = _gdk_gix_display_send_selection_notify;
  display_class->get_selection_property = _gdk_gix_display_get_selection_property;
  display_class->convert_selection = _gdk_gix_display_convert_selection;
  display_class->text_property_to_utf8_list = _gdk_gix_display_text_property_to_utf8_list;
  display_class->utf8_to_string_target = _gdk_gix_display_utf8_to_string_target;
}

static void
_gdk_display_gix_init (GdkDisplayGix *display)
{
  _gdk_gix_display_manager_add_display (gdk_display_manager_get (),
					    GDK_DISPLAY (display));
}
