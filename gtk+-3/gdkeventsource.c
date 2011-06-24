


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
append_event (GdkEvent *event)
{
  GList *link;
  
  fixup_event (event);
#if 1
  link = _gdk_event_queue_append (gdk_display_get_default (), event);

  /* event morphing, the passed in may not be valid afterwards */
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
      append_event(event);         
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

  device = GDK_DEVICE_MANAGER_CORE (device_manager)->core_keyboard;

  event = gdk_event_new (GDK_FOCUS_CHANGE);
  event->focus_change.window = window;
  event->focus_change.in = in;
  gdk_event_set_device (event, device);
  return (event);
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


static GdkModifierType  _gdk_gix_modifiers = 0;


static gint
build_pointer_event_state (unsigned button_flags, unsigned key_flags)
{
  gint state;
  
  state = 0;

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
    state |= GDK_SHIFT_MASK;

  if (key_flags & G_MODIFIERS_META)
    state |= GDK_MOD1_MASK;

  if (key_flags & G_MODIFIERS_CAPSLOCK)
    state |= GDK_LOCK_MASK;

  if (key_flags & G_MODIFIERS_CTRL)
    state |= GDK_CONTROL_MASK;
#endif
  return state;
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


static GdkEvent *
generate_key_event(GdkDeviceManager *device_manager, gi_msg_t *msg, GdkWindow *window)
{
  GdkEvent *event;
  uint32_t code, modifier, level;
  //GdkKeymap *keymap;
  GdkDevice *device;

  //keymap = gdk_keymap_get_for_display (device->display);

  event = gdk_event_new (msg->type == GI_MSG_KEY_DOWN ? 
    GDK_KEY_PRESS : GDK_KEY_RELEASE);
  event->key.window =  (window);
  event->button.time = msg->time;
  event->key.state = build_pointer_event_state(msg->params[2], msg->body.message[3]);
  event->key.group = 0;   
  event->key.hardware_keycode = 0; /* FIXME: What should it be? */
  event->key.string = NULL;
  event->key.length = 0;

  if (msg->attribute_1 && msg->params[3] > 127)
  {
    //IME
  event->key.keyval = gdk_unicode_to_keyval ( msg->params[3]);
  }
  else{
  //event->key.keyval =  ( msg->params[3]);
  gdk_keymap_translate_keyboard_state (gdk_keymap_get_for_display (_gdk_display),
					     event->key.hardware_keycode,
					     event->key.state,
					     event->key.group,
					     &event->key.keyval,
					     NULL, NULL, NULL);

  }


   fill_key_event_string (event);
  device = GDK_DEVICE_MANAGER_CORE (device_manager)->core_keyboard;
  
  gdk_event_set_device (event, device);



  return event;
}


static GdkEvent *
gdk_event_translate (GdkDisplay *display, GdkEventSource *event_source,
  gi_msg_t *g_event, gboolean return_exposes)
{
  GdkEvent        *event    = NULL;
  GdkWindow *window;
  GdkDeviceManager *device_manager;

  device_manager = gdk_display_get_device_manager (display);
  g_assert(device_manager != NULL);

  g_return_val_if_fail (g_event != NULL, NULL);

  window = gdk_gix_window_lookup_for_display (display, g_event->ev_window);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);
  g_return_val_if_fail (window != NULL, NULL);

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
    guint            button;
    int button2;

    if (g_event->type == GI_MSG_BUTTON_DOWN){
      button2 = g_event->params[2] ;
      if (button2 & (GI_BUTTON_WHEEL_UP|GI_BUTTON_WHEEL_DOWN
        |GI_BUTTON_WHEEL_LEFT|GI_BUTTON_WHEEL_RIGHT))
      {
      mask = build_pointer_event_state(button2, g_event->body.message[3]);

      if (g_event->type == GI_MSG_BUTTON_DOWN)
      _gdk_gix_modifiers |= mask;
      else
      _gdk_gix_modifiers &= ~mask; 
   
      event = gdk_event_new ( GDK_SCROLL);

      event->scroll.type = GDK_SCROLL;

      if (button2 & GI_BUTTON_WHEEL_UP)
        event->scroll.direction = GDK_SCROLL_UP;
      else if (button2 & GI_BUTTON_WHEEL_DOWN)
        event->scroll.direction = GDK_SCROLL_DOWN;
      else if (button2 & GI_BUTTON_WHEEL_LEFT)
        event->scroll.direction = GDK_SCROLL_LEFT;
      else
        event->scroll.direction = GDK_SCROLL_RIGHT;

      event->scroll.window =  (window);
      event->scroll.x = g_event->body.rect.x ;
      event->scroll.y = g_event->body.rect.y ;
      event->scroll.x_root = (gfloat)root_x;
      event->scroll.y_root = (gfloat)root_y;
      event->scroll.state = (GdkModifierType) _gdk_gix_modifiers;
      event->scroll.device =  display->core_pointer;
      gdk_event_set_device (event, display->core_pointer);

      goto got_event;    
      }
    }
    else{
      button2 = g_event->params[3] ;
    }   

    if(button2 & GI_BUTTON_L){
    button = 1;
    }
    else if(button2 &  GI_BUTTON_M){
    button = 2;
    }
    else if(button2 &  GI_BUTTON_R){
    button = 3;
    }
    else{
    break;
    }

    mask = build_pointer_event_state(button2, g_event->body.message[3]);

    if (g_event->type == GI_MSG_BUTTON_DOWN)
      _gdk_gix_modifiers |= mask;
    else
      _gdk_gix_modifiers &= ~mask;

    event = gdk_event_new ( g_event->type == GI_MSG_BUTTON_DOWN ?
		GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);

    event->button.x_root = root_x;
    event->button.y_root = root_y;
	event->button.window = window;
	event->button.time = g_event->time;
    event->button.x = g_event->body.rect.x;
    event->button.y = g_event->body.rect.y;
    event->button.state  = _gdk_gix_modifiers;
    event->button.button = button;
    event->button.device = display->core_pointer;
    gdk_event_set_device (event, display->core_pointer);

    //g_print("%s: %d got button message\n",__FUNCTION__,__LINE__);

    //if (g_event->type == GI_MSG_BUTTON_DOWN)
    //_gdk_event_button_generate (gdk_display_get_default (),event);  
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
    _gdk_gix_modifiers = build_pointer_event_state (
      g_event->params[2], g_event->params[3]);
    event->motion.state = _gdk_gix_modifiers;
    event->motion.is_hint = FALSE;
    gdk_event_set_device (event, display->core_pointer);  
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
    event->crossing.time = g_event->time;
    event->crossing.x = g_event->body.rect.x;
    event->crossing.y = g_event->body.rect.y;
    event->crossing.x_root = root_x;
    event->crossing.y_root = root_y;
    event->crossing.state = _gdk_gix_modifiers;
    event->crossing.mode = GDK_CROSSING_NORMAL;
    event->crossing.detail = GDK_NOTIFY_ANCESTOR;
    gdk_event_set_device (event, display->core_pointer);
  break;

  case GI_MSG_MOUSE_ENTER:
  {        
    event = gdk_event_new (GDK_LEAVE_NOTIFY);
    event->crossing.window =  (window);
	event->crossing.subwindow = NULL;
    event->crossing.time = g_event->time;
    event->crossing.x = g_event->body.rect.x;
    event->crossing.y = g_event->body.rect.y;
    event->crossing.x_root = root_x;
    event->crossing.y_root = root_y;
    event->crossing.state = _gdk_gix_modifiers;
    event->crossing.mode = GDK_CROSSING_NORMAL;
    event->crossing.detail = GDK_NOTIFY_ANCESTOR;
    gdk_event_set_device (event, display->core_pointer);
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
  break;
  case GI_MSG_REPARENT:
  break;

  default:
  g_message ("unhandled Gix windowing event 0x%08x", g_event->type);
  }

got_event:
  g_object_unref (G_OBJECT (window));

  return event;
}


