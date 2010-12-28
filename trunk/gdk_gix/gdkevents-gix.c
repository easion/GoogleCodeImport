/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.
 */

/*
 * GTK+ Gix backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "gdk.h"
#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkinternals.h"

#include "gdkkeysyms.h"

#include "gdkinput-gix.h"
#include <string.h>

#ifndef __GDK_X_H__
#define __GDK_X_H__
gboolean gdk_net_wm_supports (GdkAtom property);
#endif

#include "gdkalias.h"


#include "gdkaliasdef.c"

/*********************************************
 * Functions for maintaining the event queue *
 *********************************************/

static GdkEvent * gdk_event_translate  (gi_msg_t  *g_event,
                                        GdkWindow       *window,gboolean    return_exposes);

/*
 * Private variable declarations
 */
static GList *client_filters;  /* Filters for client messages */


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

static GdkFilterReturn
apply_filters (GdkWindow      *window,
               gi_msg_t *g_event,
               GList          *filters)
{
  GdkFilterReturn result = GDK_FILTER_CONTINUE;
  GdkEvent *event;
  GList *node;
  GList *tmp_list;

  event = gdk_event_new (GDK_NOTHING);
  if (window != NULL)
    event->any.window = g_object_ref (window);
  ((GdkEventPrivate *)event)->flags |= GDK_EVENT_PENDING;

  /* I think GdkFilterFunc semantics require the passed-in event
   * to already be in the queue. The filter func can generate
   * more events and append them after it if it likes.
   */
  node = _gdk_event_queue_append ((GdkDisplay*)_gdk_display, event);
  
  tmp_list = filters;
  while (tmp_list)
    {
      GdkEventFilter *filter = (GdkEventFilter *) tmp_list->data;
      
      tmp_list = tmp_list->next;
      result = filter->function (g_event, event, filter->data);
      if (result != GDK_FILTER_CONTINUE)
        break;
    }

  if (result == GDK_FILTER_CONTINUE || result == GDK_FILTER_REMOVE)
    {
      _gdk_event_queue_remove_link ((GdkDisplay*)_gdk_display, node);
      g_list_free_1 (node);
      gdk_event_free (event);
    }
  else /* GDK_FILTER_TRANSLATE */
    {
      ((GdkEventPrivate *)event)->flags &= ~GDK_EVENT_PENDING;
      fixup_event (event);
    }
  return result;
}

static void
dfb_events_process_window_event (gi_msg_t *event)
{
  GdkWindow *window;

  /*
   * Apply global filters
   *
   * If result is GDK_FILTER_CONTINUE, we continue as if nothing
   * happened. If it is GDK_FILTER_REMOVE or GDK_FILTER_TRANSLATE,
   * we return TRUE and won't dispatch the event.
   */
  if (_gdk_default_filters)
    {
      switch (apply_filters (NULL, event, _gdk_default_filters))
        {
        case GDK_FILTER_REMOVE:
        case GDK_FILTER_TRANSLATE:
          return;

        default:
          break;
        }
    }

  window = gdk_gix_window_id_table_lookup (event->ev_window); //dpp
  if (!window)
    return;

  gdk_event_translate (event, window,TRUE);
}

static gboolean
gdk_event_send_client_message_by_window (GdkEvent *event,
                                         GdkWindow *window)
{
	//dpp

	D_UNIMPLEMENTED;

  /*DFBUserEvent evt;

  g_return_val_if_fail(event != NULL, FALSE);
  g_return_val_if_fail(GDK_IS_WINDOW(window), FALSE);

  evt.clazz = DFEC_USER;
  evt.type = GPOINTER_TO_UINT (GDK_ATOM_TO_POINTER (event->client.message_type));
  evt.data = (void *) event->client.data.l[0];

  _gdk_display->buffer->PostEvent(_gdk_display->buffer, DFB_EVENT (&evt));
  */

  return TRUE;
}

static void
dfb_events_dispatch (void)
{
  GdkDisplay *display = gdk_display_get_default ();
  GdkEvent   *event;

  while ((event = _gdk_event_unqueue (display)) != NULL)
    {
      if (_gdk_event_func)
        (*_gdk_event_func) (event, _gdk_event_data);

      gdk_event_free (event);
    }
}

static gboolean
dfb_events_io_func (GIOChannel   *channel,
                    GIOCondition  condition,
                    gpointer      data)
{
  gsize      i;
  gsize      read;
  GIOStatus  result;
  gi_msg_t   buf[23];
  gi_msg_t  *event;

  result = g_io_channel_read_chars (channel,
                                    (gchar *) buf, sizeof (buf), &read, NULL);

  if (result == G_IO_STATUS_ERROR)
    {
      g_warning ("%s: GIOError occured", __FUNCTION__);
      return TRUE;
    }

  read /= sizeof (gi_msg_t);

  for (i = 0, event = buf; i < read; i++, event++)
    {
	  dfb_events_process_window_event (event);

      /*switch (event->clazz)
        {
        case DFEC_WINDOW:
          if (event->window.type == DWET_ENTER ) {
            if ( i>0 && buf[i-1].window.type != DWET_ENTER )
              dfb_events_process_window_event (&event->window);
          }
          else
            dfb_events_process_window_event (&event->window);
          break;

        case DFEC_USER:
          {
            GList *list;

            GDK_NOTE (EVENTS, g_print (" client_message"));

            for (list = client_filters; list; list = list->next)
              {
                GdkClientFilter *filter     = list->data;
                DFBUserEvent    *user_event = (DFBUserEvent *) event;
                GdkAtom          type;

                type = GDK_POINTER_TO_ATOM (GUINT_TO_POINTER (user_event->type));

                if (filter->type == type)
                  {
                    if (filter->function (user_event,
                                          NULL,
                                          filter->data) != GDK_FILTER_CONTINUE)
                      break;
                  }
              }
          }
          break;

        default:
          break;
        }*/
    }


  dfb_events_dispatch ();

  return TRUE;
}

void
_gdk_events_init (void)
{
  GIOChannel *channel;
  GSource    *source;
  int   ret;
  gint        fd;

  fd = gi_get_connection_fd();
 
  channel = g_io_channel_unix_new (fd);

  g_io_channel_set_encoding (channel, NULL, NULL);
  g_io_channel_set_buffered (channel, FALSE);

  source = g_io_create_watch (channel, G_IO_IN);

  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_set_can_recurse (source, TRUE);
  g_source_set_callback (source, (GSourceFunc) dfb_events_io_func, NULL, NULL);

  g_source_attach (source, NULL);
  g_source_unref (source);
}

gboolean
gdk_events_pending (void)
{
  GdkDisplay *display = gdk_display_get_default ();

  return _gdk_event_queue_find_first (display) ? TRUE : FALSE;
}

GdkEvent *
gdk_event_get_graphics_expose (GdkWindow *window)
{
  GdkDisplay *display;
  GList      *list;

  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  display = gdk_drawable_get_display (GDK_DRAWABLE (window));

  for (list = _gdk_event_queue_find_first (display); list; list = list->next)
    {
      GdkEvent *event = list->data;
      if (event->type == GDK_EXPOSE && event->expose.window == window)
        break;
    }

  if (list)
    {
      GdkEvent *retval = list->data;

      _gdk_event_queue_remove_link (display, list);
      g_list_free_1 (list);

      return retval;
    }

  return NULL;
}

void
_gdk_events_queue (GdkDisplay *display)
{
}

void
gdk_flush (void)
{
gdk_display_flush ( GDK_DISPLAY_OBJECT(_gdk_display));
}

/* Sends a ClientMessage to all toplevel client windows */
gboolean
gdk_event_send_client_message_for_display (GdkDisplay *display,
                                           GdkEvent   *event,
                                           guint32     xid)
{
  GdkWindow *win = NULL;
  gboolean ret = TRUE;

  g_return_val_if_fail(event != NULL, FALSE);

  win = gdk_window_lookup_for_display (display, (GdkNativeWindow) xid);

  g_return_val_if_fail(win != NULL, FALSE);

  if ((GDK_WINDOW_OBJECT(win)->window_type != GDK_WINDOW_CHILD) &&
      (g_object_get_data (G_OBJECT (win), "gdk-window-child-handler")))
    {
      /* Managed window, check children */
      GList *ltmp = NULL;
      for (ltmp = GDK_WINDOW_OBJECT(win)->children; ltmp; ltmp = ltmp->next)
       {
         ret &= gdk_event_send_client_message_by_window (event,
                                                         GDK_WINDOW(ltmp->data));
       }
    }
  else
    {
      ret &= gdk_event_send_client_message_by_window (event, win);
    }

  return ret;
}

/*****/

guint32
gdk_gix_get_time (void)
{
  GTimeVal tv;

  g_get_current_time (&tv);

  return (guint32) tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


GdkWindow *
gdk_gix_child_at (GdkWindow *window,
                       gint      *winx,
                       gint      *winy)
{
  GdkWindowObject *private;
  GList           *list;

  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  private = GDK_WINDOW_OBJECT (window);
  for (list = private->children; list; list = list->next)
    {
      GdkWindowObject *win = list->data;

      if (GDK_WINDOW_IS_MAPPED (win) &&
          *winx >= win->x  &&
          *winx <  win->x + GDK_DRAWABLE_IMPL_GIX (win->impl)->width  &&
          *winy >= win->y  &&
          *winy <  win->y + GDK_DRAWABLE_IMPL_GIX (win->impl)->height)
        {
          *winx -= win->x;
          *winy -= win->y;

          return gdk_gix_child_at (GDK_WINDOW (win), winx, winy );
        }
    }

  return window;
}

static void
get_real_window (GdkDisplay *display,
		 gi_msg_t     *event,
		 gi_window_id_t     *event_window,
		 gi_window_id_t     *filter_window)
{
	*event_window = event->ev_window;
	*filter_window = 0;
}

static void
generate_focus_event (GdkWindow *window,
		      gboolean   in)
{
  GdkEvent event;
  
  event.type = GDK_FOCUS_CHANGE;
  event.focus_change.window = window;
  event.focus_change.send_event = FALSE;
  event.focus_change.in = in;
  
  gdk_event_put (&event);
}

static GdkEvent *
gdk_event_translate (gi_msg_t *g_event,
                     GdkWindow      *window,gboolean    return_exposes)
{
  GdkWindowObject *private;
  GdkDisplay      *display;
  GdkEvent        *event    = NULL;

  g_return_val_if_fail (g_event != NULL, NULL);
  g_return_val_if_fail (GDK_IS_WINDOW (window), NULL);

  int root_x = g_event->params[0];
  int root_y = g_event->params[1];


  private = GDK_WINDOW_OBJECT (window);

  g_object_ref (G_OBJECT (window));

  /*
   * Apply per-window filters
   *
   * If result is GDK_FILTER_CONTINUE, we continue as if nothing
   * happened. If it is GDK_FILTER_REMOVE or GDK_FILTER_TRANSLATE,
   * we return TRUE and won't dispatch the event.
   */
  if (private->filters)
    {
      switch (apply_filters (window, g_event, private->filters))
        {
        case GDK_FILTER_REMOVE:
        case GDK_FILTER_TRANSLATE:
          g_object_unref (G_OBJECT (window));
          return NULL;

        default:
          break;
        }
    }

  display = gdk_drawable_get_display (GDK_DRAWABLE (window));

  switch (g_event->type)
    {
		 case GI_MSG_EXPOSURE :
      GDK_NOTE (EVENTS,
		g_message ("expose:\t\twindow: %ld  %d	x,y: %d %d  w,h: %d %d%s",
			   g_event->window, g_event->count,
			   g_event->x, g_event->y,
			   g_event->width, g_event->height,
			   event->any.send_event ? " (send)" : ""));
      
      if (private == NULL)
        {
          return NULL;
          break;
        }
      
      {
	GdkRectangle expose_rect;
	int xoffset = 0;
	int yoffset = 0;

	expose_rect.x = g_event->x + xoffset;
	expose_rect.y = g_event->y + yoffset;
	expose_rect.width = g_event->width;
	expose_rect.height = g_event->height;

	if (return_exposes) //dpp
	  {
		event = gdk_gix_event_make (window, 
                                       GDK_EXPOSE);

	    event->expose.type = GDK_EXPOSE;
	    event->expose.area = expose_rect;
	    event->expose.region = gdk_region_rectangle (&expose_rect);
	    event->expose.window = window;
	    event->expose.count = 1;
		//_gdk_window_process_expose (window, 0, &expose_rect);

	    //return_val = TRUE;
	  }
	else
	  {
		event =NULL;

	    //_gdk_window_process_expose (window, g_event->serial, &expose_rect);
	    //_gdk_window_process_expose (window, 0, &expose_rect);
	    //return_val = FALSE;
	  }
	
	//return_val = FALSE;
      }	
		 break;

    

    case GI_MSG_BUTTON_DOWN:
    case GI_MSG_BUTTON_UP:
      {
        static gboolean  click_grab = FALSE;
        GdkWindow       *child;
        gint             wx, wy;
        guint            mask;
        guint            button;
		int button2;

        _gdk_gix_mouse_x = wx = root_x;
        _gdk_gix_mouse_y = wy = root_y;

		if (g_event->type == GI_MSG_BUTTON_DOWN){
		  button2 = g_event->params[2] ;
		  if (button2 & (GI_BUTTON_WHEEL_UP|GI_BUTTON_WHEEL_DOWN|
			  GI_BUTTON_WHEEL_LEFT|GI_BUTTON_WHEEL_RIGHT)){
		 GdkWindow *event_win;		

		  if (_gdk_gix_pointer_grab_window) 
          {
            GdkDrawableImplGix *impl;

            event_win = _gdk_gix_pointer_grab_window;
            impl = GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (event_win)->impl);            
            g_event->x = root_x - impl->abs_x; //FIXME
            g_event->y = root_y - impl->abs_y;
          }
        else
          {
            event_win = gdk_gix_child_at (window,
                                               &g_event->x, &g_event->y);
          }

        if (event_win)
          {
		  event = gdk_gix_event_make (event_win, GDK_SCROLL);

		  event->scroll.type = GDK_SCROLL;
          if (button2 & GI_BUTTON_WHEEL_UP)
            event->scroll.direction = GDK_SCROLL_UP;
          else if (button2 & GI_BUTTON_WHEEL_DOWN)
            event->scroll.direction = GDK_SCROLL_DOWN;
          else if (button2 & GI_BUTTON_WHEEL_LEFT)
            event->scroll.direction = GDK_SCROLL_LEFT;
          else
            event->scroll.direction = GDK_SCROLL_RIGHT;
		  event->scroll.x = g_event->x ;
		  event->scroll.y = g_event->y ;
		  event->scroll.x_root = (gfloat)_gdk_gix_mouse_x;
		  event->scroll.y_root = (gfloat)_gdk_gix_mouse_y;
		  event->scroll.state = (GdkModifierType) _gdk_gix_modifiers;
		  event->scroll.device =  display->core_pointer;
		  }

		  //set_screen_from_root (display, event, xevent->xbutton.root);
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

        child = gdk_gix_child_at (_gdk_parent_root, &wx, &wy);

        if (_gdk_gix_pointer_grab_window &&
            (_gdk_gix_pointer_grab_events & (g_event->type == 
                                                  GI_MSG_BUTTON_DOWN ?
                                                  GDK_BUTTON_PRESS_MASK : 
                                                  GDK_BUTTON_RELEASE_MASK)) &&
            (_gdk_gix_pointer_grab_owner_events == FALSE ||
             child == _gdk_parent_root) )
          {
            GdkDrawableImplGix *impl;

            child = _gdk_gix_pointer_grab_window;
            impl  = GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (child)->impl);
            
            g_event->x = g_event->params[0] - impl->abs_x;
            g_event->y = g_event->params[1] - impl->abs_y;
          }
        else if (!_gdk_gix_pointer_grab_window ||
                 (_gdk_gix_pointer_grab_owner_events == TRUE))
          {
            g_event->x = wx;
            g_event->y = wy;
          }
        else
          {
            child = NULL;
          }

        if (g_event->type == GI_MSG_BUTTON_DOWN)
          _gdk_gix_modifiers |= mask;
        else
          _gdk_gix_modifiers &= ~mask;

        if (child)
          {
            event =
              gdk_gix_event_make (child, 
                                       g_event->type == GI_MSG_BUTTON_DOWN ?
                                       GDK_BUTTON_PRESS : GDK_BUTTON_RELEASE);

            event->button.x_root = _gdk_gix_mouse_x;
            event->button.y_root = _gdk_gix_mouse_y;
            event->button.x = g_event->x;
            event->button.y = g_event->y;
            event->button.state  = _gdk_gix_modifiers;
            event->button.button = button;
            event->button.device = display->core_pointer;

            GDK_NOTE (EVENTS, 
                      g_message ("button: %d at %d,%d %s with state 0x%08x",
                                 event->button.button,
                                 (int)event->button.x, (int)event->button.y,
                                 g_event->type == GI_MSG_BUTTON_DOWN ?
                                 "pressed" : "released", 
                                 _gdk_gix_modifiers));

            if (g_event->type == GI_MSG_BUTTON_DOWN)
              _gdk_event_button_generate (gdk_display_get_default (),event);
          }

		  printf("calling grab %x\n", _gdk_gix_modifiers);

        /* Handle implicit button grabs: */
        if (g_event->type == GI_MSG_BUTTON_DOWN  &&  !click_grab  &&  child)
          {
            if (gdk_gix_pointer_grab (child, FALSE,
                                           gdk_window_get_events (child),
                                           NULL, NULL,
                                           GDK_CURRENT_TIME,
                                           TRUE) == GDK_GRAB_SUCCESS)
              click_grab = TRUE;
			printf("calling gdk_gix_pointer_grab\n");
			 //gi_set_focus_window(GDK_WINDOW_GIX_ID(child));
          }
        else if (g_event->type == GI_MSG_BUTTON_UP &&
                 !(_gdk_gix_modifiers & (GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK |
                                              GDK_BUTTON3_MASK)) && click_grab)
          {
			printf("calling gdk_gix_pointer_ungrab\n");
            gdk_gix_pointer_ungrab (GDK_CURRENT_TIME, TRUE);
            click_grab = FALSE;
          }
#if 0
		  if (event->ev_window != event->effect_window){
			gdk_gix_pointer_ungrab (GDK_CURRENT_TIME, TRUE);
            click_grab = FALSE;
		  }
#endif
      }
      break;
 
    case GI_MSG_MOUSE_MOVE:
      {
        GdkWindow *event_win;
        GdkWindow *child;

        _gdk_gix_mouse_x = root_x;
        _gdk_gix_mouse_y = root_y;

	//child = gdk_gix_child_at (window, &g_event->x, &g_event->y);
    /* Go all the way to root to catch popup menus */
    int wx=_gdk_gix_mouse_x;
    int wy=_gdk_gix_mouse_y;
	child = gdk_gix_child_at (_gdk_parent_root, &wx, &wy);

    /* first let's see if any cossing event has to be send */
    gdk_gix_window_send_crossing_events (NULL, child, GDK_CROSSING_NORMAL);

    /* then dispatch the motion event to the window the cursor it's inside */
	event_win = gdk_gix_pointer_event_window (child, GDK_MOTION_NOTIFY);


	if (event_win)
	  {

	    if (event_win == _gdk_gix_pointer_grab_window) {
		GdkDrawableImplGix *impl;

		child = _gdk_gix_pointer_grab_window;
		impl = GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (child)->impl);

		g_event->x = _gdk_gix_mouse_x - impl->abs_x;
		g_event->y = _gdk_gix_mouse_y - impl->abs_y;
	      }

	    event = gdk_gix_event_make (child, GDK_MOTION_NOTIFY);

	    event->motion.x_root = _gdk_gix_mouse_x;
	    event->motion.y_root = _gdk_gix_mouse_y;

	    //event->motion.x = g_event->x;
	    //event->motion.y = g_event->y;
	    event->motion.x = wx;
	    event->motion.y = wy;

	    event->motion.state   = _gdk_gix_modifiers;
	    event->motion.is_hint = FALSE;
	    event->motion.device  = display->core_pointer;

	    if (GDK_WINDOW_OBJECT (event_win)->event_mask &
		GDK_POINTER_MOTION_HINT_MASK)
	      {
			#if 0 //dpp
		while (EventBuffer->PeekEvent (EventBuffer,
					       DFB_EVENT (g_event)) == 0
		       && g_event->type == DWET_MOTION)
		  {
		    EventBuffer->GetEvent (EventBuffer, DFB_EVENT (g_event));
		    event->motion.is_hint = TRUE; //鼠标的双击事件
		  }
		  #endif
	      }
	  }
          /* make sure crossing events go to the event window found */
/*        GdkWindow *ev_win = ( event_win == NULL ) ? gdk_window_at_pointer (NULL,NULL) :event_win;
	     gdk_gix_window_send_crossing_events (NULL,ev_win,GDK_CROSSING_NORMAL);
*/
      }
      break;

    case GI_MSG_FOCUS_IN:
     // gdk_gix_change_focus (window);
	generate_focus_event (window, TRUE);
      break;

    case GI_MSG_FOCUS_OUT:
      //gdk_gix_change_focus (_gdk_parent_root);
		generate_focus_event (window, FALSE);

      break;
#if 0 //dpp
    case DWET_POSITION:
      {
        GdkWindow *event_win;

        private->x = g_event->x;
        private->y = g_event->y;

        event_win = gdk_gix_other_event_window (window, GDK_CONFIGURE);

        if (event_win)
          {
            event = gdk_gix_event_make (event_win, GDK_CONFIGURE);
            event->configure.x = g_event->x;
            event->configure.y = g_event->y;
            event->configure.width =
              GDK_DRAWABLE_IMPL_GIX (private->impl)->width;
            event->configure.height =
              GDK_DRAWABLE_IMPL_GIX (private->impl)->height;
          }

        _gdk_gix_calc_abs (window);
      }
      break;

    case DWET_POSITION_SIZE:
      private->x = g_event->x;
      private->y = g_event->y;
#endif
      /* fallthru */

    case GI_MSG_CONFIGURENOTIFY:
      {
        GdkDrawableImplGix *impl;
        GdkWindow               *event_win;
        GList                   *list;

		if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD ||
          GDK_WINDOW_TYPE (window) == GDK_WINDOW_ROOT)
		{
			break;
		}
        impl = GDK_DRAWABLE_IMPL_GIX (private->impl);

        event_win = gdk_gix_other_event_window (window, GDK_CONFIGURE);

		if (g_event->params[0] == GI_STRUCT_CHANGE_RESIZE){ //resize
        if (event_win)
          {
            event = gdk_gix_event_make (event_win, GDK_CONFIGURE);
				event->configure.x      = private->x;
				event->configure.y      = private->y;
				event->configure.width  = g_event->width;
				event->configure.height = g_event->height;
          }

			impl->width  = g_event->width;
			impl->height = g_event->height;

        for (list = private->children; list; list = list->next)
          {
            GdkWindowObject         *win;
            GdkDrawableImplGix *impl;

            win  = GDK_WINDOW_OBJECT (list->data);
            impl = GDK_DRAWABLE_IMPL_GIX (win->impl);

            _gdk_gix_move_resize_child (GDK_WINDOW (win),
                                             win->x, win->y,
                                             impl->width, impl->height);
          }

        _gdk_gix_calc_abs (window);

        gdk_window_clear (window);
        gdk_window_invalidate_rect (window, NULL, TRUE);  //FIXME DPP
		}
		else if (g_event->params[0] == GI_STRUCT_CHANGE_MOVE){ //move
			private->x = g_event->x;
			private->y = g_event->y;

			event_win = gdk_gix_other_event_window (window, GDK_CONFIGURE);

			if (event_win)
			  {
				event = gdk_gix_event_make (event_win, GDK_CONFIGURE);
				event->configure.x = g_event->x;
				event->configure.y = g_event->y;
				event->configure.width =
				  GDK_DRAWABLE_IMPL_GIX (private->impl)->width;
				event->configure.height =
				  GDK_DRAWABLE_IMPL_GIX (private->impl)->height;
			  }
			
			_gdk_gix_calc_abs (window);  
		}
		else{
		}
      }
      break;

    case GI_MSG_KEY_DOWN:
    case GI_MSG_KEY_UP:
      {
        GdkEventType type = (g_event->type == GI_MSG_KEY_UP ?
                             GDK_KEY_RELEASE : GDK_KEY_PRESS);
        GdkWindow *event_win =
          gdk_gix_keyboard_event_window (gdk_gix_window_find_focus (),
                                              type);
        if (event_win)
          {
            event = gdk_gix_event_make (event_win, type);
            gdk_gix_translate_key_event (g_event, &event->key);
          }
      }
      break;

    case GI_MSG_MOUSE_EXIT:
      _gdk_gix_mouse_x = root_x;
      _gdk_gix_mouse_y = root_y;

      gdk_gix_window_send_crossing_events (NULL, _gdk_parent_root,
                                                GDK_CROSSING_NORMAL);

      if (gdk_gix_apply_focus_opacity)
        {
		  #if 0 //dpp
          if (GDK_WINDOW_IS_MAPPED (window))
            GDK_WINDOW_IMPL_GIX (private->impl)->window->SetOpacity
              (GDK_WINDOW_IMPL_GIX (private->impl)->window,
               (GDK_WINDOW_IMPL_GIX (private->impl)->opacity >> 1) +
               (GDK_WINDOW_IMPL_GIX (private->impl)->opacity >> 2));
		  #endif
        }
      break;

    case GI_MSG_MOUSE_ENTER:
      {
        GdkWindow *child;
        
        _gdk_gix_mouse_x = root_x;
        _gdk_gix_mouse_y = root_y;

        child = gdk_gix_child_at (window, &g_event->x, &g_event->y);

        /* this makes sure pointer is set correctly when it previously left
         * a window being not standard shaped
         */
        gdk_window_set_cursor (window, NULL);
        gdk_gix_window_send_crossing_events (NULL, child,
                                                  GDK_CROSSING_NORMAL);

        if (gdk_gix_apply_focus_opacity)
          {

			 #if 0 //dpp
            GDK_WINDOW_IMPL_GIX (private->impl)->window->SetOpacity
              (GDK_WINDOW_IMPL_GIX (private->impl)->window,
               GDK_WINDOW_IMPL_GIX (private->impl)->opacity);
			#endif
          }
      }
      break;

	 case GI_MSG_CLIENT_MSG:
            {
		 GdkWindow *event_win;   
		 printf("xxx GI_MSG_CLIENT_MSG got message\n");

			if(g_event->client.client_type == GA_WM_PROTOCOLS
					&&g_event->params[0]  == GA_WM_DELETE_WINDOW ){
				event_win = gdk_gix_other_event_window (window, GDK_DELETE);

        if (event_win)
          event = gdk_gix_event_make (event_win, GDK_DELETE);
				printf("xxx GI_MSG_CLIENT_MSG got message\n");
				
                //running=0;
				break;
				}
			}
			break;

    case GI_MSG_WINDOW_DESTROY:
      {
        GdkWindow *event_win;

        event_win = gdk_gix_other_event_window (window, GDK_DESTROY);

        if (event_win)
          event = gdk_gix_event_make (event_win, GDK_DESTROY);

	gdk_window_destroy_notify (window);
      }
      break;

	  case GI_MSG_WINDOW_SHOW:
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

gboolean
gdk_screen_get_setting (GdkScreen   *screen,
                        const gchar *name,
                        GValue      *value)
{
  return FALSE;
}

void
gdk_display_add_client_message_filter (GdkDisplay   *display,
                                       GdkAtom       message_type,
                                       GdkFilterFunc func,
                                       gpointer      data)
{
  /* XXX: display should be used */
  GdkClientFilter *filter = g_new (GdkClientFilter, 1);

  filter->type = message_type;
  filter->function = func;
  filter->data = data;
  client_filters = g_list_append (client_filters, filter);
}


void
gdk_add_client_message_filter (GdkAtom       message_type,
                               GdkFilterFunc func,
                               gpointer      data)
{
  gdk_display_add_client_message_filter (gdk_display_get_default (),
                                         message_type, func, data);
}

void
gdk_screen_broadcast_client_message (GdkScreen *screen,
				     GdkEvent  *sev)
{
  GdkWindow *root_window;
  GdkWindowObject *private;
  GList *top_level = NULL;

  g_return_if_fail (GDK_IS_SCREEN (screen));
  g_return_if_fail(sev != NULL);

  root_window = gdk_screen_get_root_window (screen);

  g_return_if_fail(GDK_IS_WINDOW(root_window));

  private = GDK_WINDOW_OBJECT (root_window);

  for (top_level = private->children; top_level; top_level = top_level->next)
    {
      gdk_event_send_client_message_for_display (gdk_drawable_get_display(GDK_DRAWABLE(root_window)),
                                                sev,
                                                (guint32)(GDK_WINDOW_GIX_ID(GDK_WINDOW(top_level->data))));
   }
}


/**
 * gdk_net_wm_supports:
 * @property: a property atom.
 *
 * This function is specific to the X11 backend of GDK, and indicates
 * whether the window manager for the default screen supports a certain
 * hint from the Extended Window Manager Hints Specification. See
 * gdk_x11_screen_supports_net_wm_hint() for complete details.
 *
 * Return value: %TRUE if the window manager supports @property
 **/


gboolean
gdk_net_wm_supports (GdkAtom property)
{
   return FALSE;
}

void
_gdk_windowing_event_data_copy (const GdkEvent *src,
                                GdkEvent       *dst)
{
}

void
_gdk_windowing_event_data_free (GdkEvent *event)
{
}

#define __GDK_EVENTS_X11_C__
#include "gdkaliasdef.c"
