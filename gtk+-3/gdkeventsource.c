


#include "config.h"
#include "gdkinternals.h"
#include "gdkprivate-gix.h"



typedef struct _GdkGixEventSource {
  GSource source;
  GPollFD event_poll_fd;
  uint32_t mask;
  GdkDisplay *display;
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
append_event (GdkEvent *event, gboolean  windowing)
{
  GList *link;
  
  fixup_event (event);
#if 1
  link = _gdk_event_queue_append (gdk_display_get_default (), event);

  /* event morphing, the passed in may not be valid afterwards */
  if(windowing)
  _gdk_windowing_got_event (gdk_display_get_default (), link, event, 0);
#else
  _gdk_event_queue_append (gdk_display_get_default (), event);
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
      append_event(event,TRUE);         
    }
  } 
}



static GdkEvent *
generate_focus_event (GdkDeviceManager *device_manager,
                      GdkWindow        *window,
                      gboolean          in)
{
  GdkDevice *device;
  GdkEvent *event;

  device = GDK_DEVICE_MANAGER_GIX (device_manager)->core_keyboard;

  event = gdk_event_new (GDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;
  gdk_event_set_device (event, device);
  return (event);
}


static void
generate_grab_broken_event (GdkDeviceManager *device_manager,
                            GdkWindow        *window,
                            gboolean          keyboard,
                            GdkWindow        *grab_window)
{
  GdkEvent *event = gdk_event_new (GDK_GRAB_BROKEN);
  GdkDevice *device;

  if (keyboard)
    device = GDK_DEVICE_MANAGER_GIX (device_manager)->core_keyboard;
  else
    device = GDK_DEVICE_MANAGER_GIX (device_manager)->core_pointer;

  event->grab_broken.window = window;
  event->grab_broken.send_event = 0;
  event->grab_broken.keyboard = keyboard;
  event->grab_broken.implicit = FALSE;
  event->grab_broken.grab_window = grab_window;
  gdk_event_set_device (event, device);

  append_event (event,FALSE);
}


static GdkEvent *
generate_configure_event (gi_msg_t       *msg,
      GdkWindow *window)
{

  if (msg->params[0] != GI_STRUCT_CHANGE_RESIZE
    && msg->params[0] != GI_STRUCT_CHANGE_MOVE )
  {
    return NULL;
  }

  window->x = msg->body.rect.x;
  window->y = msg->body.rect.y;
  window->width = msg->body.rect.w;
  window->height = msg->body.rect.h;

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


static GdkModifierType  _gdk_gix_modifiers = 0;


static gint
build_pointer_event_state (unsigned button_flags, unsigned key_flags)
{
  gint state;
  gint ks;
  
  ks = state = 0;

  if   (button_flags & GI_BUTTON_L)
    state |= GDK_BUTTON1_MASK;

  if (button_flags & GI_BUTTON_M)
    state |= GDK_BUTTON2_MASK;

  if (button_flags & GI_BUTTON_R)
    state |= GDK_BUTTON3_MASK;

  if (button_flags & GI_BUTTON_WHEEL_UP)
    state |= GDK_BUTTON4_MASK;

  if (button_flags & GI_BUTTON_WHEEL_DOWN)
    state |= GDK_BUTTON5_MASK;


#if 1
  if (key_flags & G_MODIFIERS_SHIFT)
    ks |= GDK_SHIFT_MASK;

  if (key_flags & G_MODIFIERS_META)
    ks |= GDK_MOD1_MASK;

  if (key_flags & G_MODIFIERS_CAPSLOCK)
    ks |= GDK_LOCK_MASK;

  if (key_flags & G_MODIFIERS_CTRL)
    ks |= GDK_CONTROL_MASK;
#endif
  //printf("build_pointer_event_state state = %x %x\n", state,ks);
  return state|ks;
}


static void
fill_key_event_string (GdkEvent *event)
{
  gunichar c;
  gchar buf[256];

  /* Fill in event->string crudely, since various programs
   * depend on it.
   */
  
  c = 0;
  if (event->key.keyval != GDK_KEY_VoidSymbol)
    c = gdk_keyval_to_unicode (event->key.keyval);

  if (c)
    {
      gsize bytes_written;
      gint len;
      
      /* Apply the control key - Taken from Xlib
       */
      if (event->key.state & GDK_CONTROL_MASK)
	{
	  if ((c >= '@' && c < '\177') || c == ' ')
	    c &= 0x1F;
	  else if (c == '2')
	    {
	      event->key.string = g_memdup ("\0\0", 2);
	      event->key.length = 1;
	      return;
	    }
	  else if (c >= '3' && c <= '7')
	    c -= ('3' - '\033');
	  else if (c == '8')
	    c = '\177';
	  else if (c == '/')
	    c = '_' & 0x1F;
	}
      
      len = g_unichar_to_utf8 (c, buf);
      buf[len] = '\0';
	  
      event->key.string = g_locale_from_utf8 (buf, len,
					      NULL, &bytes_written,
					      NULL);
      if (event->key.string)
	event->key.length = bytes_written;
    }
  else if (event->key.keyval == GDK_KEY_Escape)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\033");
    }
  else if (event->key.keyval == GDK_KEY_Return ||
	   event->key.keyval == GDK_KEY_KP_Enter)
    {
      event->key.length = 1;
      event->key.string = g_strdup ("\r");
    }
  
  if (!event->key.string)
    {
      event->key.length = 0;
      event->key.string = g_strdup ("");
    }
}


gulong
_gdk_gix_get_next_tick (gulong suggested_tick)
{
  static gulong cur_tick = 0;

  //if (suggested_tick == 0)
  //  suggested_tick = GetTickCount ();
  if (suggested_tick <= cur_tick)
    return cur_tick;
  else{
    cur_tick = suggested_tick;
  }
  return cur_tick;
}


static void
build_key_event_state (GdkEvent *event,
		       gi_msg_t      *msg)
{
  unsigned mk = msg->body.message[3];
  //unsigned mm = msg->body.message[3];
  event->key.state = 0;

  if (G_MODIFIERS_SHIFT & mk)
    event->key.state |= GDK_SHIFT_MASK;

  if (G_MODIFIERS_CAPSLOCK & 0x01)
    event->key.state |= GDK_LOCK_MASK;

  if (GI_BUTTON_L & mk)
    event->key.state |= GDK_BUTTON1_MASK;
  if (GI_BUTTON_M & mk)
    event->key.state |= GDK_BUTTON2_MASK;
  if (GI_BUTTON_R & mk)
    event->key.state |= GDK_BUTTON3_MASK;
  if (GI_BUTTON_WHEEL_UP & mk)
    event->key.state |= GDK_BUTTON4_MASK;
  if (GI_BUTTON_WHEEL_DOWN & mk)
    event->key.state |= GDK_BUTTON5_MASK;

  /*if (_gdk_keyboard_has_altgr &&
      (key_state[VK_LCONTROL] & mk) &&
      (key_state[VK_RMENU] & mk))
    {
      event->key.group = 1;
      event->key.state |= GDK_MOD2_MASK;
      if (key_state[VK_RCONTROL] & mk)
	event->key.state |= GDK_CONTROL_MASK;
      if (key_state[VK_LMENU] & mk)
	event->key.state |= GDK_MOD1_MASK;
    }
  else*/
    {
      event->key.group = 0;
      if (G_MODIFIERS_CTRL & mk)
	event->key.state |= GDK_CONTROL_MASK;
      if (G_MODIFIERS_META & mk)
	event->key.state |= GDK_MOD1_MASK;
    }
}

static void
build_wm_ime_composition_event (GdkEvent *event,
				gi_msg_t      *msg,
				unsigned   wc)
{
  event->key.time = _gdk_gix_get_next_tick (msg->time);
  
  build_key_event_state (event, msg);

  event->key.hardware_keycode = 0; /* FIXME: What should it be? */
  event->key.string = NULL;
  event->key.length = 0;
  event->key.keyval = gdk_unicode_to_keyval (wc);
  //fill_key_event_string (event); 
}


static GdkEvent *
generate_key_event(GdkDeviceManager *device_manager, gi_msg_t *msg, GdkWindow *window)
{
  GdkEvent *event;
  uint32_t code, modifier, level;
  GdkKeymap *keymap;
  GdkDevice *device;

  keymap = gdk_keymap_get_for_display (_gdk_display);
  device = GDK_DEVICE_MANAGER_GIX (device_manager)->core_keyboard;

  if (msg->attribute_1 && msg->params[3] > 127)
  {
	/* Build a key press event */
	//event = gdk_event_new (GDK_KEY_PRESS);
	event = gdk_event_new (msg->type == GI_MSG_KEY_DOWN ? 
      GDK_KEY_PRESS : GDK_KEY_RELEASE);

	event->key.window = window;
	//gdk_event_set_device (event, GDK_DEVICE_MANAGER_WIN32 (device_manager)->core_keyboard);
	build_wm_ime_composition_event (event, msg, msg->params[3]);
	gdk_event_set_device (event, device);
	return event;
  }

  event = gdk_event_new (msg->type == GI_MSG_KEY_DOWN ? 
    GDK_KEY_PRESS : GDK_KEY_RELEASE);
  event->key.window =  (window);
  event->button.time =_gdk_gix_get_next_tick( msg->time);
  build_key_event_state (event, msg);
  //event->key.state = build_pointer_event_state(msg->params[2], msg->body.message[3]);
  //event->key.group = 0;   
  event->key.hardware_keycode = 0; /* FIXME: What should it be? */
  event->key.string = NULL;
  event->key.length = 0;
  
  event->key.hardware_keycode =  ( msg->params[3]);
  gdk_keymap_translate_keyboard_state ( keymap,
					     event->key.hardware_keycode,
					     event->key.state,
					     event->key.group,
					     &event->key.keyval,
					     NULL, NULL, NULL);
  printf("do get hardware_keycode %d,%d,%d\n",
	   msg->params[3],event->key.hardware_keycode, event->key.keyval);

  gdk_event_set_device (event, device);
  fill_key_event_string (event); 
  return event;
}


static void
assign_object (gpointer lhsp,
	       gpointer rhs)
{
  if (*(gpointer *)lhsp != rhs)
    {
      if (*(gpointer *)lhsp != NULL)
	g_object_unref (*(gpointer *)lhsp);
      *(gpointer *)lhsp = rhs;
      if (rhs != NULL)
	g_object_ref (rhs);
    }
}



static GdkWindow *
find_window_for_mouse_event (GdkWindow* reported_window,
			     gi_msg_t*       msg)
{
  gi_window_id_t hwnd;
  int xroot_x, xroot_y, xwin_x, xwin_y;
  int err;
  GdkWindow* other_window = NULL;
  GdkDeviceManagerGix *device_manager;
  unsigned int xmask;

  device_manager = GDK_DEVICE_MANAGER_GIX (gdk_display_get_device_manager (_gdk_display));

  if (!_gdk_display_get_last_device_grab (_gdk_display, device_manager->core_pointer))
    return reported_window;

  err = gi_query_pointer ( GI_DESKTOP_WINDOW_ID,
				 &hwnd,
				 &xroot_x, &xroot_y,
				 &xwin_x, &xwin_y,
				 &xmask);
  if (err <= 0) {
	  return _gdk_root;
  }

  other_window = gdk_gix_window_lookup_for_display (_gdk_display, hwnd);

  if (other_window == NULL)
    return _gdk_root;

  return other_window;
}



static GdkEvent *
gdk_event_translate (GdkDisplay *display, GdkEventSource *event_source,
  gi_msg_t *g_event, gboolean return_exposes)
{
  GdkEvent        *event    = NULL;
  GdkWindow *window;
  GdkDeviceManager *device_manager;
  GdkDeviceGrabInfo *keyboard_grab = NULL;
  GdkDeviceGrabInfo *pointer_grab = NULL;
  GdkWindow *grab_window = NULL;


  device_manager = gdk_display_get_device_manager (display);
  g_assert(device_manager != NULL);

  g_return_val_if_fail (g_event != NULL, NULL);

  window = gdk_gix_window_lookup_for_display (display, g_event->ev_window);
  g_return_val_if_fail (window != NULL, NULL);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  keyboard_grab = _gdk_display_get_last_device_grab (_gdk_display,
		GDK_DEVICE_MANAGER_GIX (device_manager)->core_keyboard);
  pointer_grab = _gdk_display_get_last_device_grab (_gdk_display,
		GDK_DEVICE_MANAGER_GIX (device_manager)->core_pointer);


  int root_x = g_event->params[0];
  int root_y = g_event->params[1];

  g_object_ref (window); 

  switch (g_event->type)
  {
  case GI_MSG_EXPOSURE :
  {
    GdkRectangle expose_rect;
    int xoffset = 0;
    int yoffset = 0;
	
	//event_data.attribute_1
	if (g_event->attribute_1)
	{
		//double buffer
		printf("GI_MSG_EXPOSURE double buffer\n");
		goto got_event;
	}

    expose_rect.x = g_event->body.rect.x + xoffset;
    expose_rect.y = g_event->body.rect.y + yoffset;
    expose_rect.width = g_event->body.rect.w;
    expose_rect.height = g_event->body.rect.h;

    if (return_exposes) 
    {
    cairo_region_t *result;
    GList *list = display->queued_events;

    event = gdk_event_new (GDK_EXPOSE);

    result = cairo_region_create ();

    cairo_region_union_rectangle (result, &expose_rect);

    event->expose.window = (window);
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
    guint            button_gdk = 0;
    guint			 button_gix = 0;

	//assign_object (&window, find_window_for_mouse_event (window, g_event));

    if (g_event->type == GI_MSG_BUTTON_DOWN){

      button_gix = g_event->params[2] ;
      mask = build_pointer_event_state(button_gix, g_event->body.message[3]);
      _gdk_gix_modifiers |= mask;      

      if (button_gix & (GI_BUTTON_WHEEL_UP | GI_BUTTON_WHEEL_DOWN
        | GI_BUTTON_WHEEL_LEFT | GI_BUTTON_WHEEL_RIGHT))
      { 
   
      event = gdk_event_new ( GDK_SCROLL);
      event->scroll.type = GDK_SCROLL;	  

      if (button_gix & GI_BUTTON_WHEEL_UP){
        event->scroll.direction = GDK_SCROLL_UP;
	  }
      else if (button_gix & GI_BUTTON_WHEEL_DOWN){
        event->scroll.direction = GDK_SCROLL_DOWN;
	  }
      else if (button_gix & GI_BUTTON_WHEEL_LEFT)
        event->scroll.direction = GDK_SCROLL_LEFT;
      else
        event->scroll.direction = GDK_SCROLL_RIGHT;

      event->scroll.window =  (window);
      event->scroll.x = g_event->body.rect.x ;
      event->scroll.y = g_event->body.rect.y ;
      event->scroll.x_root = (gfloat)root_x;
      event->scroll.y_root = (gfloat)root_y;
      event->scroll.state = (GdkModifierType) _gdk_gix_modifiers;
      event->scroll.device =  _gdk_display->core_pointer;
      gdk_event_set_device (event, _gdk_display->core_pointer);

      goto got_event;    
      } //scroll
	  else{
		  _gdk_gix_modifiers &= ~(GDK_BUTTON4_MASK|GDK_BUTTON5_MASK);
	  }
    }
    else{
      button_gix = g_event->params[3] ;
	  mask = build_pointer_event_state(button_gix, g_event->body.message[3]);   
      _gdk_gix_modifiers &= ~mask;
    }    

	if(button_gix & GI_BUTTON_L){
	  button_gdk = 1;
	}
	else if(button_gix &  GI_BUTTON_M){
		button_gdk = 2;
	}
	else if(button_gix &  GI_BUTTON_R){
		button_gdk = 3;
	}
	else{
    //break;
    }
	  
	printf("%s: mask=%x,%x, button_gdk=%d %p\n",
		__FUNCTION__, mask,_gdk_gix_modifiers, button_gdk,window);


    event = gdk_event_new ( g_event->type == GI_MSG_BUTTON_DOWN ?
		GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);	

    event->button.x_root = root_x;
    event->button.y_root = root_y;
	event->button.window = window;
	event->button.axes = NULL;
	event->button.time = _gdk_gix_get_next_tick(g_event->time);
    event->button.x = g_event->body.rect.x;
    event->button.y = g_event->body.rect.y;
    event->button.state  = _gdk_gix_modifiers;
    event->button.button = button_gdk;
    event->button.device = _gdk_display->core_pointer;
    gdk_event_set_device (event, _gdk_display->core_pointer);

    //g_print("%s: %d got button message\n",__FUNCTION__,__LINE__);
    //if (g_event->type == GI_MSG_BUTTON_DOWN)
    //_gdk_event_button_generate (gdk_display_get_default (),event);  
  }
  break;

  case GI_MSG_MOUSE_MOVE:
  {
    event = gdk_event_new (GDK_MOTION_NOTIFY);

	//assign_object (&window, find_window_for_mouse_event (window, g_event));
	
    event->motion.window = window;
    event->motion.time = _gdk_gix_get_next_tick(g_event->time);
    event->motion.x = g_event->body.rect.x;
    event->motion.y = g_event->body.rect.y;
    event->motion.x_root = root_y;
    event->motion.y_root = root_x;
    event->motion.axes = NULL;
    _gdk_gix_modifiers = build_pointer_event_state (
      g_event->params[2], g_event->params[3]);
    event->motion.state = _gdk_gix_modifiers;
    event->motion.is_hint = FALSE;
    gdk_event_set_device (event, _gdk_display->core_pointer);  
  }
  break;

  case GI_MSG_FOCUS_IN:
  {
  gi_composition_form_t attr;
  
  attr.x = 0;
  attr.y = 0;
  gi_ime_associate_window(GDK_WINDOW_XID(window), &attr);

  g_print("%s: %d got focus in message\n",__FUNCTION__,__LINE__);
  // gdk_gix_change_focus (window);
  event = generate_focus_event (device_manager,window, TRUE);
  }
  break;

  case GI_MSG_FOCUS_OUT:
  g_print("%s: %d got focus message\n",__FUNCTION__,__LINE__);
  //gdk_gix_change_focus (_gdk_parent_root);
  gi_ime_associate_window(GDK_WINDOW_XID(window), NULL);

  //generate_grab_broken_event();
  event = generate_focus_event (device_manager,window, FALSE);
  break;

  /* fallthru */

  case GI_MSG_CONFIGURENOTIFY:
  {
    g_print("%s: %d got config message\n",__FUNCTION__,__LINE__);
    event = generate_configure_event(g_event, window);
  }
  break;

  case GI_MSG_KEY_DOWN:
  case GI_MSG_KEY_UP:
  {  
    g_print("%s: %d got key message\n",__FUNCTION__,__LINE__);
    event = generate_key_event(device_manager, g_event, window);
  }
  break;

  case GI_MSG_MOUSE_EXIT:
    
    event = gdk_event_new (GDK_LEAVE_NOTIFY);
	
    event->crossing.window =  (window);
	event->crossing.subwindow = NULL;
    event->crossing.time = _gdk_gix_get_next_tick(g_event->time);
    event->crossing.x = g_event->body.rect.x;
    event->crossing.y = g_event->body.rect.y;
    event->crossing.x_root = root_x;
    event->crossing.y_root = root_y;
    event->crossing.state = _gdk_gix_modifiers;
    event->crossing.mode = GDK_CROSSING_NORMAL;
    event->crossing.detail = GDK_NOTIFY_ANCESTOR;
    gdk_event_set_device (event, _gdk_display->core_pointer);
  break;

  case GI_MSG_MOUSE_ENTER:
  {        
    event = gdk_event_new (GDK_LEAVE_NOTIFY);
	
    event->crossing.window =  (window);
	event->crossing.subwindow = NULL;
    event->crossing.time = _gdk_gix_get_next_tick(g_event->time);
    event->crossing.x = g_event->body.rect.x;
    event->crossing.y = g_event->body.rect.y;
    event->crossing.x_root = root_x;
    event->crossing.y_root = root_y;
    event->crossing.state = _gdk_gix_modifiers;
    event->crossing.mode = GDK_CROSSING_NORMAL;
    event->crossing.detail = GDK_NOTIFY_ANCESTOR;
    gdk_event_set_device (event, _gdk_display->core_pointer);
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
	event->any.send_event = FALSE;
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
	  if (window->event_mask & GDK_PROPERTY_CHANGE_MASK)
	{
	  event = gdk_event_new(GDK_NOTHING);
	  event->property.type = GDK_PROPERTY_NOTIFY;
	  event->property.window = window;
	  event->property.atom = gdk_x11_xatom_to_atom_for_display (display, g_event->params[1]);
	  event->property.time = g_event->params[2];
	  event->property.state = g_event->params[0];
	}
	else{
		event = NULL;
	}
  break;
  case GI_MSG_SELECTIONNOTIFY:
	  event = gdk_event_new(GDK_NOTHING);

	  if (g_event->params[0] == G_SEL_NOTIFY){
	  event->selection.type = GDK_SELECTION_NOTIFY;
      event->selection.window = window;
      event->selection.selection = gdk_x11_xatom_to_atom_for_display (display, g_event->params[3]);
      event->selection.target = gdk_x11_xatom_to_atom_for_display (display, g_event->body.message[0]);
      if (g_event->params[3] == 0)
        event->selection.property = GDK_NONE;
      else
        event->selection.property = gdk_x11_xatom_to_atom_for_display (display, g_event->params[3]);
      event->selection.time = g_event->params[1];

		}
	  else if (g_event->params[0] ==G_SEL_REQUEST)
	  {
	  event->selection.type = GDK_SELECTION_REQUEST;
      event->selection.window = window;
      event->selection.selection = gdk_x11_xatom_to_atom_for_display (display, g_event->params[3]);
      event->selection.target = gdk_x11_xatom_to_atom_for_display (display, g_event->body.message[0]);
      event->selection.property = gdk_x11_xatom_to_atom_for_display (display, g_event->body.message[1]);
      if (g_event->params[2] != 0)
        event->selection.requestor = gdk_x11_window_foreign_new_for_display (display,
                                                                             g_event->params[2]);
      else
        event->selection.requestor = NULL;
      event->selection.time = g_event->body.message[2];
	  }
	  else if (g_event->params[0] ==G_SEL_CLEAR)
	  {
		  event->selection.type = GDK_SELECTION_CLEAR;
	  event->selection.window = window;
	  event->selection.selection = gdk_x11_xatom_to_atom_for_display (display, g_event->params[3]);
	  event->selection.time = g_event->params[1];
	  }
	 break;

  case GI_MSG_REPARENT:
  break;

  default:
  g_message ("unhandled Gix windowing event 0x%08x", g_event->type);
  }

got_event:
  if (!event)
  {
	  event = gdk_event_new(GDK_NOTHING);
	  /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = GDK_NOTHING;
  }

  g_object_unref (window);

  return event;
}


