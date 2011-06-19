


#include "config.h"
#include "gdkinternals.h"
#include "gdkprivate-gix.h"



typedef struct _GdkGixEventSource {
  GSource source;
  GPollFD event_poll_fd;
  uint32_t mask;
  GdkDisplay *display;
  //GList *translators;
} GdkGixEventSource;


static GList *event_sources = NULL;


static GdkEvent *
gdk_event_translate (GdkDisplay *display, GdkEventSource *event_source,
	gi_msg_t *g_event, gboolean return_exposes);

static gboolean
gdk_event_source_prepare(GSource *source, gint *timeout)
{
  GdkDisplay *display = ((GdkEventSource*) source)->display;
  gboolean retval;

  GDK_THREADS_ENTER ();

  *timeout = -1;
  retval = (_gdk_event_queue_find_first (display) != NULL ||
            gi_get_event_count ()>0);

  GDK_THREADS_LEAVE ();

  return retval;
}

static gboolean
gdk_event_source_check(GSource *base)
{
  GdkGixEventSource *event_source = (GdkGixEventSource *) base;
  //GdkEventSource *event_source = (GdkEventSource*) source;
  gboolean retval;

  GDK_THREADS_ENTER ();

  if (event_source->event_poll_fd.revents & G_IO_IN)
    retval = (_gdk_event_queue_find_first (event_source->display) != NULL ||
              gi_get_event_count()>0);
  else
    retval = FALSE;

  GDK_THREADS_LEAVE ();

  return retval;

}

static gboolean
gdk_event_source_dispatch(GSource *base,
			  GSourceFunc callback,
			  gpointer data)
{
  GdkGixEventSource *source = (GdkGixEventSource *) base;
  GdkDisplay *display = source->display;
  GdkEvent *event;

  GDK_THREADS_ENTER ();

  event = gdk_display_get_event (display);

  if (event)
    {
      _gdk_event_emit (event);
      gdk_event_free (event);
    }

  GDK_THREADS_LEAVE ();

  return TRUE;
}

static void
gdk_event_source_finalize (GSource *source)
{
  event_sources = g_list_remove (event_sources, source);
}

static GSourceFuncs event_funcs = {
  gdk_event_source_prepare,
  gdk_event_source_check,
  gdk_event_source_dispatch,
  gdk_event_source_finalize
};

static int
gdk_event_source_update(uint32_t mask, void *data)
{
  GdkGixEventSource *source = data;

  source->mask = mask;

  return 0;
}



GSource *
_gdk_gix_display_event_source_new (GdkDisplay *display)
{
  GSource *source;
  GdkGixEventSource *gix_source;
  GdkDisplayGix *display_gix;
  char *name;

  source = g_source_new (&event_funcs,
			 sizeof (GdkGixEventSource));
  name = g_strdup_printf ("GDK Gix Event source (%s)", "display name");
  g_source_set_name (source, name);
  g_free (name);
  gix_source = (GdkGixEventSource *) source;

  display_gix = GDK_DISPLAY_GIX (display);
  gix_source->display = display;
  gix_source->event_poll_fd.fd = gi_get_connection_fd();

  gix_source->event_poll_fd.events = G_IO_IN | G_IO_ERR;
  g_source_add_poll(source, &gix_source->event_poll_fd);

  g_source_set_priority (source, GDK_PRIORITY_EVENTS);
  g_source_set_can_recurse (source, TRUE);
  g_source_attach (source, NULL);

  event_sources = g_list_prepend (event_sources, source);

  return source;
}

void
_gdk_gix_display_flush (GdkDisplay *display, GSource *source)
{
  GdkGixEventSource *gix_source = (GdkGixEventSource *) source;

  if (display->closed)
    return;
    //XFlush (GDK_DISPLAY_XDISPLAY (display));
}


static void
fixup_event (GdkEvent *event)
{
  if (event->any.window)
    g_object_ref (event->any.window);
  if (((event->any.type == GDK_ENTER_NOTIFY) ||
       (event->any.type == GDK_LEAVE_NOTIFY)) &&
      (event->crossing.subwindow != NULL))
    g_object_ref (event->crossing.subwindow);
  event->any.send_event = FALSE;
}

static void
append_event (GdkEvent *event)
{
  GList *link;
  
  fixup_event (event);
#if 1
  link = _gdk_event_queue_append (gdk_display_get_default (), event);
  //GDK_NOTE (EVENTS, _gdk_win32_print_event (event));
  /* event morphing, the passed in may not be valid afterwards */
  _gdk_windowing_got_event (gdk_display_get_default (), link, event, 0);
#else
  _gdk_event_queue_append (gdk_display_get_default (), event);
  GDK_NOTE (EVENTS, _gdk_win32_print_event (event));
#endif
}


void
_gdk_gix_display_queue_events (GdkDisplay *display)
{
  GdkDisplayGix *display_gix;
  GdkGixEventSource *source;
  gi_msg_t xevent;
  GdkEvent *event;

  display_gix = GDK_DISPLAY_GIX (display);
  source = (GdkGixEventSource *) display_gix->event_source;

  while (!_gdk_event_queue_find_first (display) && gi_get_event_count()>0)
  {
	  int err;

	  err = gi_get_message(&xevent,MSG_ALWAYS_WAIT);
	  if (err <= 0)  {
		  return FALSE;
	  }
	  event = gdk_event_translate (display, source, &xevent, TRUE);
      if (event){
		  append_event(event);         
        }
  } 
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




static GdkEvent *
generate_focus_event (GdkDeviceManager *device_manager,
                      GdkWindow        *window,
                      gboolean          in)
{
  GdkDevice *device;
  GdkEvent *event;

  device = GDK_DEVICE_MANAGER_CORE (device_manager)->core_keyboard;

  event = gdk_event_new (GDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;
  gdk_event_set_device (event, device);
  return (event);
}

#if 0
static GdkEvent *
generate_button_event (GdkEventType      type,
                       gint              button,
                       GdkWindow        *window,
                       gi_msg_t              *msg)
{
  GdkEvent *event = gdk_event_new (type);

  event->button.window = window;
  event->button.time = _gdk_win32_get_next_tick (msg->time);
  event->button.x = msg->body.rect.x;
  event->button.y = msg->body.rect.x;
  event->button.x_root = msg->params[0] ;
  event->button.y_root = msg->params[1] ;
  event->button.axes = NULL;
  event->button.state = 0;//build_pointer_event_state (msg);
  event->button.button = button;
  //gdk_event_set_device (event, display->core_pointer);
  gdk_event_set_device (event, _gdk_display->core_pointer);

  return (event);
}

#endif


static GdkEvent *
handle_configure_event (gi_msg_t       *msg,
			GdkWindow *window)
{

  if (msg->params[0] != GI_STRUCT_CHANGE_RESIZE
	|| msg->params[0] != GI_STRUCT_CHANGE_MOVE )
	{
		return NULL;
	}

  window->width = msg->body.rect.w;
  window->height = msg->body.rect.h;
  
  window->x = msg->body.rect.x;
  window->y = msg->body.rect.y;

  _gdk_window_update_size (window);
  
  if (window->event_mask & GDK_STRUCTURE_MASK)
    {
      GdkEvent *event = gdk_event_new (GDK_CONFIGURE);

      event->configure.window = window;

      event->configure.width = msg->body.rect.w;
      event->configure.height = msg->body.rect.h;
      
      event->configure.x = msg->body.rect.x;
      event->configure.y = msg->body.rect.y;

      return (event);
    }

	return NULL;
}

#if 0
static void
input_handle_key(GdkGixDevice *device , 
		 uint32_t time, uint32_t key, uint32_t state)
{
  //GdkGixDevice *device = data;
  GdkEvent *event;
  uint32_t code, modifier, level;
 // struct xkb_desc *xkb;
  GdkKeymap *keymap;

  keymap = gdk_keymap_get_for_display (device->display);
  //xkb = _gdk_gix_keymap_get_xkb_desc (keymap);

  device->time = time;
  event = gdk_event_new (state ? GDK_KEY_PRESS : GDK_KEY_RELEASE);
  event->key.window = g_object_ref (device->keyboard_focus);
  gdk_event_set_device (event, device->keyboard);
  event->button.time = time;
  event->key.state = device->modifiers;
  event->key.group = 0;
  //code = key + xkb->min_key_code;
  //event->key.hardware_keycode = code;

  level = 0;
  //if (device->modifiers & XKB_COMMON_SHIFT_MASK &&
  //    XkbKeyGroupWidth(xkb, code, 0) > 1)
   // level = 1;

  //event->key.keyval = XkbKeySymEntry(xkb, code, level, 0);

  modifier = 0;//xkb->map->modmap[code];
  if (state)
    device->modifiers |= modifier;
  else
    device->modifiers &= ~modifier;

  event->key.is_modifier = modifier > 0;

  translate_keyboard_string (&event->key);

  _gdk_gix_display_deliver_event (device->display, event);

  GDK_NOTE (EVENTS,
	    g_message ("keyboard event, code %d, sym %d, "
		       "string %s, mods 0x%x",
		       code, event->key.keyval,
		       event->key.string, event->key.state));
}
#endif

int             _gdk_gix_mouse_x   = 0;
int             _gdk_gix_mouse_y   = 0;
GdkModifierType  _gdk_gix_modifiers = 0;


static GdkEvent *
gdk_event_translate (GdkDisplay *display, GdkEventSource *event_source,
	gi_msg_t *g_event, gboolean return_exposes)
{
  GdkEvent        *event    = NULL;
  GdkWindow *window;
  GdkDeviceManager *device_manager;

  device_manager = gdk_display_get_device_manager (display);

  g_return_val_if_fail (g_event != NULL, NULL);

  //GdkEvent *event;
  window = gdk_gix_window_lookup_for_display (display, g_event->ev_window);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  int root_x = g_event->params[0];
  int root_y = g_event->params[1];

  g_object_ref (G_OBJECT (window)); 

  switch (g_event->type)
    {
	 case GI_MSG_EXPOSURE :
		{
	GdkRectangle expose_rect;
	int xoffset = 0;
	int yoffset = 0;     

	expose_rect.x = g_event->body.rect.x + xoffset;
	expose_rect.y = g_event->body.rect.y + yoffset;
	expose_rect.width = g_event->body.rect.w;
	expose_rect.height = g_event->body.rect.h;

	if (return_exposes) 
	  {
		GList *list = display->queued_events;

		event = gdk_event_new (GDK_EXPOSE);

		cairo_region_t *result;

		result = cairo_region_create ();
		
		cairo_region_union_rectangle (result, &expose_rect);

	  event->expose.window = g_object_ref(window);
	   event->expose.area = expose_rect;	  
	   event->expose.region = result;
	  event->expose.count = 0;

	  while (list != NULL)
	    {
	      GdkEventPrivate *evp = list->data;

	      if (evp->event.any.type == GDK_EXPOSE &&
		  evp->event.any.window == window &&
		  !(evp->flags & GDK_EVENT_PENDING))
		evp->event.expose.count++;

	      list = list->next;
	    }
	  }
	else
	  {
		event =NULL;	   
	  }
	
      }	
	break;    

    case GI_MSG_BUTTON_DOWN:
    case GI_MSG_BUTTON_UP:
      {
        gint             wx, wy;
        guint            mask;
        guint            button;
		int button2;

        _gdk_gix_mouse_x = wx = root_x;
        _gdk_gix_mouse_y = wy = root_y;

		if (g_event->type == GI_MSG_BUTTON_DOWN){
		  button2 = g_event->params[2] ;
		  if (button2 & (GI_BUTTON_WHEEL_UP|GI_BUTTON_WHEEL_DOWN|
			  GI_BUTTON_WHEEL_LEFT|GI_BUTTON_WHEEL_RIGHT))
          {
		  event = gdk_event_new ( GDK_SCROLL);
		  event->crossing.window = g_object_ref (window);

		  event->scroll.type = GDK_SCROLL;
          if (button2 & GI_BUTTON_WHEEL_UP)
            event->scroll.direction = GDK_SCROLL_UP;
          else if (button2 & GI_BUTTON_WHEEL_DOWN)
            event->scroll.direction = GDK_SCROLL_DOWN;
          else if (button2 & GI_BUTTON_WHEEL_LEFT)
            event->scroll.direction = GDK_SCROLL_LEFT;
          else
            event->scroll.direction = GDK_SCROLL_RIGHT;
		  event->scroll.x = g_event->body.rect.x ;
		  event->scroll.y = g_event->body.rect.y ;
		  event->scroll.x_root = (gfloat)_gdk_gix_mouse_x;
		  event->scroll.y_root = (gfloat)_gdk_gix_mouse_y;
		  event->scroll.state = (GdkModifierType) _gdk_gix_modifiers;
		  event->scroll.device =  display->core_pointer;
		  gdk_event_set_device (event, display->core_pointer);

		  goto EVENT_OK;	  
		  }
		}
		else{
		  button2 = g_event->params[3] ;
		}	 

		  if(button2 & GI_BUTTON_L){
            button = 1;
            mask   = GDK_BUTTON1_MASK;
		  }
          else if(button2 &  GI_BUTTON_M){
            button = 2;
            mask   = GDK_BUTTON2_MASK;
		  }
          else if(button2 &  GI_BUTTON_R){
            button = 3;
            mask   = GDK_BUTTON3_MASK;
		  }
		  else{
			  break;
		  }

        if (g_event->type == GI_MSG_BUTTON_DOWN)
          _gdk_gix_modifiers |= mask;
        else
          _gdk_gix_modifiers &= ~mask;

	  
		event = gdk_event_new ( g_event->type == GI_MSG_BUTTON_DOWN ?
								   GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);

		event->crossing.window = g_object_ref (window);

		event->button.x_root = _gdk_gix_mouse_x;
		event->button.y_root = _gdk_gix_mouse_y;
		event->button.x = g_event->body.rect.x;
		event->button.y = g_event->body.rect.y;
		event->button.state  = _gdk_gix_modifiers;
		event->button.button = button;
		event->button.device = display->core_pointer;
		gdk_event_set_device (event, display->core_pointer);

		GDK_NOTE (EVENTS, 
				  g_message ("button: %d at %d,%d %s with state 0x%08x",
							 event->button.button,
							 (int)event->button.x, (int)event->button.y,
							 g_event->type == GI_MSG_BUTTON_DOWN ?
							 "pressed" : "released", 
							 _gdk_gix_modifiers));

		g_print("%s: %d got button message\n",__FUNCTION__,__LINE__);

		if (g_event->type == GI_MSG_BUTTON_DOWN)
		  _gdk_event_button_generate (gdk_display_get_default (),event);  
      }
      break;
 
    case GI_MSG_MOUSE_MOVE:
      {
      event = gdk_event_new (GDK_MOTION_NOTIFY);
      event->motion.window = window;
      event->motion.time = (g_event->time);
      event->motion.x = g_event->body.rect.x;
      event->motion.y = g_event->body.rect.y;
      event->motion.x_root = root_y;
      event->motion.y_root = root_x;
      event->motion.axes = NULL;
      event->motion.state = _gdk_gix_modifiers;//build_pointer_event_state (msg);
      event->motion.is_hint = FALSE;
	  gdk_event_set_device (event, display->core_pointer);

      //append_event (event);

	/*if (event_win)
	  {
	    event = gdk_gix_event_make (child, GDK_MOTION_NOTIFY);

	    event->motion.x_root = _gdk_gix_mouse_x;
	    event->motion.y_root = _gdk_gix_mouse_y;

	    //event->motion.x = g_event->body.rect.x;
	    //event->motion.y = g_event->body.rect.y;
	    event->motion.x = wx;
	    event->motion.y = wy;

	    event->motion.state   = _gdk_gix_modifiers;
	    event->motion.is_hint = FALSE;
	    event->motion.device  = display->core_pointer;
	  }*/
         
      }
      break;

    case GI_MSG_FOCUS_IN:
		g_print("%s: %d got focus in message\n",__FUNCTION__,__LINE__);
     // gdk_gix_change_focus (window);
	event = generate_focus_event (device_manager,window, TRUE);
      break;

    case GI_MSG_FOCUS_OUT:
		g_print("%s: %d got focus message\n",__FUNCTION__,__LINE__);
      //gdk_gix_change_focus (_gdk_parent_root);
		event = generate_focus_event (device_manager,window, FALSE);
      break;

      /* fallthru */

    case GI_MSG_CONFIGURENOTIFY:
      {
		g_print("%s: %d got config message\n",__FUNCTION__,__LINE__);
		event = handle_configure_event(g_event, window);
	  }
      break;

    case GI_MSG_KEY_DOWN:
    case GI_MSG_KEY_UP:
      {	
		g_print("%s: %d got key message\n",__FUNCTION__,__LINE__);
      }
      break;

    case GI_MSG_MOUSE_EXIT:
      _gdk_gix_mouse_x = root_x;
      _gdk_gix_mouse_y = root_y;

		event = gdk_event_new (GDK_LEAVE_NOTIFY);
		event->crossing.window = g_object_ref (window);
		event->crossing.time = g_event->time;
		event->crossing.x = g_event->body.rect.x;
		event->crossing.y = g_event->body.rect.y;
		event->crossing.x_root = root_x;
		event->crossing.y_root = root_y;
		event->crossing.state = _gdk_gix_modifiers;
		event->crossing.mode = 0;//g_event->crossing.mode;
		event->crossing.detail = GDK_NOTIFY_ANCESTOR;
		gdk_event_set_device (event, display->core_pointer);
      break;

    case GI_MSG_MOUSE_ENTER:
      {        
        _gdk_gix_mouse_x = root_x;
        _gdk_gix_mouse_y = root_y;

		event = gdk_event_new (GDK_MOTION_NOTIFY);
		event->motion.window = g_object_ref (window);
		event->motion.time = g_event->time;
		event->motion.x = g_event->body.rect.x;
		event->motion.y = g_event->body.rect.y;
		event->motion.x_root = root_x;
		event->motion.y_root = root_y;
		event->motion.state = _gdk_gix_modifiers;
		gdk_event_set_device (event, display->core_pointer);

		//append_event (event); 

        //child = gdk_gix_child_at (window, &g_event->body.rect.x, &g_event->body.rect.y);

        /* this makes sure pointer is set correctly when it previously left
         * a window being not standard shaped
         */
        //gdk_window_set_cursor (window, NULL);
        //gdk_gix_window_send_crossing_events (NULL, child,
         //                                         GDK_CROSSING_NORMAL);

        
      }
      break;

	 case GI_MSG_CLIENT_MSG:
	{
		if(g_event->body.client.client_type == GA_WM_PROTOCOLS
			&&g_event->params[0]  == GA_WM_DELETE_WINDOW ){

		if (GDK_WINDOW_DESTROYED (window))
		  break;

		g_print("%s: %d got exit message\n",__FUNCTION__,__LINE__);

		event = gdk_event_new (GDK_DELETE);
		event->any.window = window;
		}
	}
	break;

    case GI_MSG_WINDOW_DESTROY:
      {
		event = gdk_event_new (GDK_DESTROY);
		event->any.window = window;

		g_print("%s: %d got destroy message\n",__FUNCTION__,__LINE__);

		gdk_window_destroy_notify (window);
      }
      break;

	  case GI_MSG_WINDOW_HIDE:
	  case GI_MSG_WINDOW_SHOW:
		{
		  if (GDK_WINDOW_DESTROYED (window))
			break;

		  g_print("%s: %d got hide/show message\n",__FUNCTION__,__LINE__);
		  event = gdk_event_new (g_event->type == GI_MSG_WINDOW_SHOW ? GDK_MAP : GDK_UNMAP);
		  event->any.window = window;
		}
		  break;

	  case GI_MSG_CREATENOTIFY:
		  break;
	  case GI_MSG_PROPERTYNOTIFY:
		  break;
	  case GI_MSG_REPARENT:
		  break;

    default:
      g_message ("unhandled Gix windowing event 0x%08x", g_event->type);
    }

EVENT_OK:
  g_object_unref (G_OBJECT (window));

  return event;
}


