/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "gdk.h"
#include "gdkx.h"
#include "gdkprivate.h"
#include "gdkkeysyms.h"


#if HAVE_CONFIG_H
#  include <config.h>
#  if STDC_HEADERS
#    include <string.h>
#  endif
#endif

#include "gdkinput.h"

typedef struct _GdkIOClosure GdkIOClosure;
typedef struct _GdkEventPrivate GdkEventPrivate;
#define EVENT_TYPE_GRAPHICS_EXPOSE 15

/* Change by Jason */
#ifndef ABS
	#define ABS(n) (((n) < 0) ? -(n) : (n))
#endif
#define DOUBLE_CLICK_TIME      500
#define TRIPLE_CLICK_TIME      1000
/* End */

#define DOUBLE_CLICK_DIST      5
#define TRIPLE_CLICK_DIST      5
#define RBUTTON		GI_BUTTON_R
#define MBUTTON		GI_BUTTON_M
#define LBUTTON		GI_BUTTON_L

#define GI_BUTTON_TO_GDK(b) b&LBUTTON? 1: (b&MBUTTON? 2: (b&RBUTTON? 3: 0))
int EventReady = 0;
gi_msg_t GrEventCached;
static int event_cached = 0;

//#define USE_SIGNAL

typedef enum
{
  /* Following flag is set for events on the event queue during
   * translation and cleared afterwards.
   */
  GDK_EVENT_PENDING = 1 << 0
} GdkEventFlags;

struct _GdkIOClosure
{
  GdkInputFunction function;
  GdkInputCondition condition;
  GdkDestroyNotify notify;
  gpointer data;
};

struct _GdkEventPrivate
{
  GdkEvent event;
  guint    flags;
};

/* 
 * Private function declarations
 */

static GdkEvent *gdk_event_new		(void);
static gint	 gdk_event_apply_filters (gi_msg_t *nevent,
					  GdkEvent *event,
					  GList    *filters);
gint	 gdk_event_translate	 (GdkEvent *event, 
					  gi_msg_t *nevent);
#if NOT_IMPLEMENTED
static Bool	 gdk_event_get_type	(Display   *display, 
					 XEvent	   *xevent, 
					 XPointer   arg);
#endif
static void      gdk_events_queue       (void);
static GdkEvent* gdk_event_unqueue      (void);

static gboolean  gdk_event_prepare      (gpointer   source_data, 
				 	 GTimeVal  *current_time,
					 gint      *timeout,
					 gpointer   user_data);
static gboolean  gdk_event_check        (gpointer   source_data,
				 	 GTimeVal  *current_time,
					 gpointer   user_data);
gboolean  gdk_event_dispatch     (gpointer   source_data,
					 GTimeVal  *current_time,
					 gpointer   user_data);

static void	 gdk_synthesize_click	(GdkEvent  *event, 
					 gint	    nclicks);

GdkFilterReturn gdk_wm_protocols_filter (GdkXEvent *xev,
					 GdkEvent  *event,
					 gpointer   data);

static long gdk_get_time_ms();
static guint gr_mod_to_gdk(guint mods, guint buttons);
/* Private variable declarations
 */

#ifndef USE_SIGNAL
static int connection_number = 0;	    /* The file descriptor number of our
					     *	connection to the X server. This
					     *	is used so that we may determine
					     *	when events are pending by using
					     *	the "select" system call.
					     */
#endif
static guint32 button_click_time[2];	    /* The last 2 button click times. Used
					     *	to determine if the latest button click
					     *	is part of a double or triple click.
					     */
static GdkWindow *button_window[2];	    /* The last 2 windows to receive button presses.
					     *	Also used to determine if the latest button
					     *	click is part of a double or triple click.
					     */
static guint button_number[2];		    /* The last 2 buttons to be pressed.
					     */

static gdouble button_x[2];		    /* The last 2 x position pressed.
					     */
static gdouble button_y[2];		    /* The last 2 y position pressed.
					     */


static GdkEventFunc   event_func = NULL;    /* Callback for events */
static gpointer       event_data = NULL;
static GDestroyNotify event_notify = NULL;

static GList *client_filters;	            /* Filters for client messages */

/* FIFO's for event queue, and for events put back using
 * gdk_event_put().
 */
static GList *queued_events = NULL;
static GList *queued_tail = NULL;

static GSourceFuncs event_funcs = {
  gdk_event_prepare,
  gdk_event_check,
  gdk_event_dispatch,
  (GDestroyNotify)g_free
};

static guint gr_mod_to_gdk(guint mods, guint buttons) 
{
        guint res=0;
        if (mods & G_MODIFIERS_SHIFT)
                        res |= GDK_SHIFT_MASK;
        if (mods & G_MODIFIERS_CTRL)
                        res |= GDK_CONTROL_MASK;
        if (mods & G_MODIFIERS_META)
                        res |= GDK_MOD1_MASK;
        if (buttons & LBUTTON)
                        res |= GDK_BUTTON1_MASK;
        if (buttons & MBUTTON)
                        res |= GDK_BUTTON2_MASK;
        if (buttons & RBUTTON)
                        res |= GDK_BUTTON3_MASK;
        return res;
}

GPollFD event_poll_fd;

#define PRINTFILE(s) g_message("gdkevents.c:%s()",s);
/*********************************************
 * Functions for maintaining the event queue *
 *********************************************/

/*************************************************************
 * gdk_event_queue_find_first:
 *     Find the first event on the queue that is not still
 *     being filled in.
 *   arguments:
 *     
 *   results:
 *     Pointer to the list node for that event, or NULL
 *************************************************************/

static GList*
gdk_event_queue_find_first (void)
{
  GList *tmp_list = queued_events;

  while (tmp_list)
    {
      GdkEventPrivate *event = tmp_list->data;
      if (!(event->flags & GDK_EVENT_PENDING))
	return tmp_list;

      tmp_list = g_list_next (tmp_list);
    }
	
  return NULL;
}

/*************************************************************
 * gdk_event_queue_remove_link:
 *     Remove a specified list node from the event queue.
 *   arguments:
 *     node: Node to remove.
 *   results:
 *************************************************************/

static void
gdk_event_queue_remove_link (GList *node)
{
  if (node->prev)
    node->prev->next = node->next;
  else
    queued_events = node->next;
  
  if (node->next)
    node->next->prev = node->prev;
  else
    queued_tail = node->prev;
  
}

/*************************************************************
 * gdk_event_queue_append:
 *     Append an event onto the tail of the event queue.
 *   arguments:
 *     event: Event to append.
 *   results:
 *************************************************************/

static void
gdk_event_queue_append (GdkEvent *event)
{
  queued_tail = g_list_append (queued_tail, event);
  
  if (!queued_events)
    queued_events = queued_tail;
  else
    queued_tail = queued_tail->next;
}

#if NOT_IMPLEMENTED
static int
events_idle () {
//    gdk_events_queue();
#ifdef USE_SIGNAL
//	pause();
#endif
    return TRUE;
}
#endif

void
gdk_events_init (void)
{
#ifndef USE_SIGNAL
	//extern int nxSocket;
#endif

  	g_source_add (GDK_PRIORITY_EVENTS, TRUE, &event_funcs, NULL, NULL, NULL);
#ifndef USE_SIGNAL
	connection_number=gi_get_connection_fd();

  	event_poll_fd.fd = connection_number;
  	event_poll_fd.events = G_IO_IN;
  	g_main_add_poll (&event_poll_fd, GDK_PRIORITY_EVENTS);
	GrEventCached.type = GI_MSG_NONE;

#endif
	button_click_time[0] = 0; 
        button_click_time[1] = 0; 
        button_window[0] = NULL; 
        button_window[1] = NULL; 
        button_number[0] = -1; 
        button_number[1] = -1; 
        button_x[0] = -1; 
        button_x[1] = -1; 
        button_y[0] = -1; 
        button_y[1] = -1; 

#if NOT_IMPLEMENTED
  	g_idle_add(events_idle, NULL);
#endif
}


/*
 *--------------------------------------------------------------
 * gdk_events_pending
 *
 *   Returns if events are pending on the queue.
 *
 * Arguments:
 *
 * Results:
 *   Returns TRUE if events are pending
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gboolean
gdk_events_pending (void)
{
#ifndef SIGNAL
	if (event_cached==0)
	{
		gi_get_message(&GrEventCached,MSG_ALWAYS_WAIT);
		if (GrEventCached.type!=GI_MSG_NONE)
		{
			event_cached=1;
#if TEST
			gdk_events_pending_add();
#endif
			return 1;
		}
	}
	else return 1;

	return 0;
#else
	gi_msg_t nevent;
  return (gdk_event_queue_find_first() || gi_peek_message(&nevent));
#endif
}

/*
 *--------------------------------------------------------------
 * gdk_event_get_graphics_expose
 *
 *   Waits for a GraphicsExpose or NoExpose event
 *
 * Arguments:
 *
 * Results: 
 *   For GraphicsExpose events, returns a pointer to the event
 *   converted into a GdkEvent Otherwise, returns NULL.
 *
 * Side effects:
 *
 *-------------------------------------------------------------- */

#if NOT_IMPLEMENTED
#if 0
static Bool
graphics_expose_predicate (Display  *display,
			   XEvent   *xevent,
			   XPointer  arg)
#else
static Bool
graphics_expose_predicate ()
#endif
{
	PRINTFILE("graphics_expose_predicate");
    return False;
}
#endif

GdkEvent*
gdk_event_get_graphics_expose (GdkWindow *window)
{
#if NOT_IMPLEMENTED
  gi_msg_t nevent;
  GdkEvent *event;
  
  g_return_val_if_fail (window != NULL, NULL);
 
  //XIfEvent (gdk_display, &xevent, graphics_expose_predicate, (XPointer) window);
	gi_get_message(&nevent,MSG_ALWAYS_WAIT);
  
  if (nevent.type == GI_MSG_EXPOSURE)
    {
      event = gdk_event_new ();
     
	PRINTFILE("gdk_event_get_graphics_expose");
	nevent.type = EVENT_TYPE_GRAPHICS_EXPOSE; 
      if (gdk_event_translate (event, &nevent))
	return event;
      else
	gdk_event_free (event);
    }
#endif 
  return NULL;	
}

/************************
 * Exposure compression *
 ************************/

/*
 * The following implements simple exposure compression. It is
 * modelled after the way Xt does exposure compression - in
 * particular compress_expose = XtExposeCompressMultiple.
 * It compress consecutive sequences of exposure events,
 * but not sequences that cross other events. (This is because
 * if it crosses a ConfigureNotify, we could screw up and
 * mistakenly compress the exposures generated for the new
 * size - could we just check for ConfigureNotify?)
 *
 * Xt compresses to a region / bounding rectangle, we compress
 * to two rectangles, and try find the two rectangles of minimal
 * area for this - this is supposed to handle the typical
 * L-shaped regions generated by OpaqueMove.
 */

/* Given three rectangles, find the two rectangles that cover
 * them with the smallest area.
 */
static void
gdk_add_rect_to_rects (GdkRectangle *rect1,
		       GdkRectangle *rect2, 
		       GdkRectangle *new_rect)
{
  GdkRectangle t1, t2, t3;
  gint size1, size2, size3;

  gdk_rectangle_union (rect1, rect2, &t1);
  gdk_rectangle_union (rect1, new_rect, &t2);
  gdk_rectangle_union (rect2, new_rect, &t3);

  size1 = t1.width * t1.height + new_rect->width * new_rect->height;
  size2 = t2.width * t2.height + rect2->width * rect2->height;
  size3 = t1.width * t1.height + rect1->width * rect1->height;

  if (size1 < size2)
    {
      if (size1 < size3)
	{
	  *rect1 = t1;
	  *rect2 = *new_rect;
	}
      else
	*rect2 = t3;
    }
  else
    {
      if (size2 < size3)
	*rect1 = t2;
      else
	*rect2 = t3;
    }
}

typedef struct _GdkExposeInfo GdkExposeInfo;

struct _GdkExposeInfo
{
  gi_window_id_t window;
  gboolean seen_nonmatching;
};
#if 1
static Bool
expose_predicate ( gi_msg_t* nevent, void* arg)
{
  GdkExposeInfo *info = (GdkExposeInfo*) arg;

  /* Compressing across GravityNotify events is safe, because
   * we completely ignore them, so they can't change what
   * we are going to draw. Compressing across GravityNotify
   * events is necessay because during window-unshading animation
   * we'll get a whole bunch of them interspersed with
   * expose events.
   */
  if (nevent->type != GI_MSG_EXPOSURE)
    {
      info->seen_nonmatching = TRUE;
    }

  if (info->seen_nonmatching ||
      nevent->type != GI_MSG_EXPOSURE ||
      nevent->ev_window != info->window)
    return FALSE;
  else
    return TRUE;
}

gint
gdk_compress_exposures (gi_msg_t* xevent,
                        GdkWindow *window)
{
  gint nrects = 1;
  GdkRectangle rect1;
  GdkRectangle rect2;
  GdkRectangle tmp_rect;
  gi_msg_t tmp_event;
  GdkExposeInfo info;
  GdkEvent event;

  info.window = xevent->ev_window;
  info.seen_nonmatching = FALSE;

  rect1.x = xevent->x;
  rect1.y = xevent->y;
  rect1.width = xevent->width;
  rect1.height = xevent->height;

  event.any.type = GDK_EXPOSE;
  event.any.window = None;
  event.any.send_event = FALSE;
  while (1)
    {
       gi_peek_message(&tmp_event);
	if(!expose_predicate(&tmp_event, &info))
		break;
     gi_next_message(&tmp_event);
	if(tmp_event.type)
		EventReady --;
	else
		EventReady = 0;

      event.any.window = window;

      if (nrects == 1)
        {
          rect2.x = tmp_event.x;
          rect2.y = tmp_event.y;
          rect2.width = tmp_event.width;
          rect2.height = tmp_event.height;

          nrects++;
        }
      else
        {
          tmp_rect.x = tmp_event.x;
          tmp_rect.y = tmp_event.y;
          tmp_rect.width = tmp_event.width;
          tmp_rect.height = tmp_event.height;

          gdk_add_rect_to_rects (&rect1, &rect2, &tmp_rect);
        }

    }

  if (nrects == 2)
    {
      gdk_rectangle_union (&rect1, &rect2, &tmp_rect);

      if ((tmp_rect.width * tmp_rect.height) <
          2 * (rect1.height * rect1.width +
               rect2.height * rect2.width))
        {
          rect1 = tmp_rect;
          nrects = 1;
        }
    }

  if (nrects == 2)
    {
      event.expose.type = GDK_EXPOSE;
      event.expose.window = window;
      event.expose.area.x = rect2.x;
      event.expose.area.y = rect2.y;
      event.expose.area.width = rect2.width;
      event.expose.area.height = rect2.height;
      event.expose.count = 0;

      gdk_event_put (&event);
    }

//  xevent->expose.count = nrects - 1;
  xevent->x = rect1.x;
  xevent->y = rect1.y;
  xevent->width = rect1.width;
  xevent->height = rect1.height;
	return nrects -1;
}
#endif
/*************************************************************
 * gdk_event_handler_set:
 *     
 *   arguments:
 *     func: Callback function to be called for each event.
 *     data: Data supplied to the function
 *     notify: function called when function is no longer needed
 * 
 *   results:
 *************************************************************/

void 
gdk_event_handler_set (GdkEventFunc   func,
		       gpointer       data,
		       GDestroyNotify notify)
{
  if (event_notify)
    (*event_notify) (event_data);

  event_func = func;
  event_data = data;
  event_notify = notify;
}

/*
 *--------------------------------------------------------------
 * gdk_event_get
 *
 *   Gets the next event.
 *
 * Arguments:
 *
 * Results:
 *   If an event is waiting that we care about, returns 
 *   a pointer to that event, to be freed with gdk_event_free.
 *   Otherwise, returns NULL.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

GdkEvent*
gdk_event_get (void)
{
  gdk_events_queue ();

  return gdk_event_unqueue ();
}

/*
 *--------------------------------------------------------------
 * gdk_event_peek
 *
 *   Gets the next event.
 *
 * Arguments:
 *
 * Results:
 *   If an event is waiting that we care about, returns 
 *   a copy of that event, but does not remove it from
 *   the queue. The pointer is to be freed with gdk_event_free.
 *   Otherwise, returns NULL.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

GdkEvent*
gdk_event_peek (void)
{
  GList *tmp_list;

  tmp_list = gdk_event_queue_find_first ();

  if (tmp_list)
    return gdk_event_copy (tmp_list->data);
  else
    return NULL;
}

void
gdk_event_put (GdkEvent *event)
{
  GdkEvent *new_event;
  
  g_return_if_fail (event != NULL);

  new_event = gdk_event_copy (event);

  gdk_event_queue_append (new_event);
}

/*
 *--------------------------------------------------------------
 * gdk_event_copy
 *
 *   Copy a event structure into new storage.
 *
 * Arguments:
 *   "event" is the event struct to copy.
 *
 * Results:
 *   A new event structure.  Free it with gdk_event_free.
 *
 * Side effects:
 *   The reference count of the window in the event is increased.
 *
 *--------------------------------------------------------------
 */

static GMemChunk *event_chunk = NULL;

static GdkEvent*
gdk_event_new (void)
{
  GdkEventPrivate *new_event;
  
  if (event_chunk == NULL)
    event_chunk = g_mem_chunk_new ("events",
				   sizeof (GdkEventPrivate),
				   4096,
				   G_ALLOC_AND_FREE);
  
  new_event = g_chunk_new (GdkEventPrivate, event_chunk);
  new_event->flags = 0;
  
  return (GdkEvent*) new_event;
}

GdkEvent*
gdk_event_copy (GdkEvent *event)
{
  GdkEvent *new_event;
  
  g_return_val_if_fail (event != NULL, NULL);
  
  new_event = gdk_event_new ();
  
  *new_event = *event;

  gdk_window_ref (new_event->any.window);
  
  switch (event->any.type)
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      new_event->key.string = g_strdup (event->key.string);
      break;
      
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      if (event->crossing.subwindow != NULL)
	gdk_window_ref (event->crossing.subwindow);
      break;
      
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
    case GDK_DRAG_MOTION:
    case GDK_DRAG_STATUS:
    case GDK_DROP_START:
    case GDK_DROP_FINISHED:
      gdk_drag_context_ref (event->dnd.context);
      break;
      
    default:
      break;
    }
  
  return new_event;
}

/*
 *--------------------------------------------------------------
 * gdk_event_free
 *
 *   Free a event structure obtained from gdk_event_copy.  Do not use
 *   with other event structures.
 *
 * Arguments:
 *   "event" is the event struct to free.
 *
 * Results:
 *
 * Side effects:
 *   The reference count of the window in the event is decreased and
 *   might be freed, too.
 *
 *-------------------------------------------------------------- */

void
gdk_event_free (GdkEvent *event)
{
  g_return_if_fail (event != NULL);

  g_assert (event_chunk != NULL); /* paranoid */
  
  if (event->any.window)
    gdk_window_unref (event->any.window);
  
  switch (event->any.type)
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      g_free (event->key.string);
      break;
      
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
      if (event->crossing.subwindow != NULL)
	gdk_window_unref (event->crossing.subwindow);
      break;
      
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
    case GDK_DRAG_MOTION:
    case GDK_DRAG_STATUS:
    case GDK_DROP_START:
    case GDK_DROP_FINISHED:
      gdk_drag_context_unref (event->dnd.context);
      break;
      
    default:
      break;
    }
  
  g_mem_chunk_free (event_chunk, event);
}

/*
 *--------------------------------------------------------------
 * gdk_event_get_time:
 *    Get the timestamp from an event.
 *   arguments:
 *     event:
 *   results:
 *    The event's time stamp, if it has one, otherwise
 *    GDK_CURRENT_TIME.
 *--------------------------------------------------------------
 */

guint32
gdk_event_get_time (GdkEvent *event)
{
  if (event)
    switch (event->type)
      {
      case GDK_MOTION_NOTIFY:
	return event->motion.time;
      case GDK_BUTTON_PRESS:
      case GDK_2BUTTON_PRESS:
      case GDK_3BUTTON_PRESS:
      case GDK_BUTTON_RELEASE:
	return event->button.time;
      case GDK_KEY_PRESS:
      case GDK_KEY_RELEASE:
	return event->key.time;
      case GDK_ENTER_NOTIFY:
      case GDK_LEAVE_NOTIFY:
	return event->crossing.time;
      case GDK_PROPERTY_NOTIFY:
	return event->property.time;
      case GDK_SELECTION_CLEAR:
      case GDK_SELECTION_REQUEST:
      case GDK_SELECTION_NOTIFY:
	return event->selection.time;
      case GDK_PROXIMITY_IN:
      case GDK_PROXIMITY_OUT:
	return event->proximity.time;
      case GDK_DRAG_ENTER:
      case GDK_DRAG_LEAVE:
      case GDK_DRAG_MOTION:
      case GDK_DRAG_STATUS:
      case GDK_DROP_START:
      case GDK_DROP_FINISHED:
	return event->dnd.time;
      default:			/* use current time */
	break;
      }
  
  return GDK_CURRENT_TIME;
}

/*
 *--------------------------------------------------------------
 * gdk_set_show_events
 *
 *   Turns on/off the showing of events.
 *
 * Arguments:
 *   "show_events" is a boolean describing whether or
 *   not to show the events gdk receives.
 *
 * Results:
 *
 * Side effects:
 *   When "show_events" is TRUE, calls to "gdk_event_get"
 *   will output debugging informatin regarding the event
 *   received to stdout.
 *
 *--------------------------------------------------------------
 */

void
gdk_set_show_events (gboolean show_events)
{
  if (show_events)
    gdk_debug_flags |= GDK_DEBUG_EVENTS;
  else
    gdk_debug_flags &= ~GDK_DEBUG_EVENTS;
}

gboolean
gdk_get_show_events (void)
{
  return (gdk_debug_flags & GDK_DEBUG_EVENTS) != 0;
}

static void
gdk_io_destroy (gpointer data)

{
  GdkIOClosure *closure = data;

  if (closure->notify)
    closure->notify (closure->data);

  g_free (closure);
}

/* What do we do with G_IO_NVAL?
 */
#define READ_CONDITION (G_IO_IN | G_IO_HUP | G_IO_ERR)
#define WRITE_CONDITION (G_IO_OUT | G_IO_ERR)
#define EXCEPTION_CONDITION (G_IO_PRI)

static gboolean  
gdk_io_invoke (GIOChannel   *source,
	       GIOCondition  condition,
	       gpointer      data)
{
  GdkIOClosure *closure = data;
  GdkInputCondition gdk_cond = 0;

  if (condition & READ_CONDITION)
    gdk_cond |= GDK_INPUT_READ;
  if (condition & WRITE_CONDITION)
    gdk_cond |= GDK_INPUT_WRITE;
  if (condition & EXCEPTION_CONDITION)
    gdk_cond |= GDK_INPUT_EXCEPTION;

  if (closure->condition & gdk_cond)
    closure->function (closure->data, g_io_channel_unix_get_fd (source), gdk_cond);

  return TRUE;
}

gint
gdk_input_add_full (gint	      source,
		    GdkInputCondition condition,
		    GdkInputFunction  function,
		    gpointer	      data,
		    GdkDestroyNotify  destroy)
{
  guint result;
  GdkIOClosure *closure = g_new (GdkIOClosure, 1);
  GIOChannel *channel;
  GIOCondition cond = 0;

  closure->function = function;
  closure->condition = condition;
  closure->notify = destroy;
  closure->data = data;

  if (condition & GDK_INPUT_READ)
    cond |= READ_CONDITION;
  if (condition & GDK_INPUT_WRITE)
    cond |= WRITE_CONDITION;
  if (condition & GDK_INPUT_EXCEPTION)
    cond |= EXCEPTION_CONDITION;

  channel = g_io_channel_unix_new (source);
  result = g_io_add_watch_full (channel, G_PRIORITY_DEFAULT, cond,
                                gdk_io_invoke,
                                closure, gdk_io_destroy);
  g_io_channel_unref (channel);

  return result;
}

gint
gdk_input_add (gint		 source,
	       GdkInputCondition condition,
	       GdkInputFunction	 function,
	       gpointer		 data)
{
  return gdk_input_add_full (source, condition, function, data, NULL);
}

void
gdk_input_remove (gint tag)
{
  g_source_remove (tag);
}

static gint
gdk_event_apply_filters (gi_msg_t *nevent,
			 GdkEvent *event,
			 GList *filters)
{
  GList *tmp_list;
  GdkFilterReturn result;
  
  tmp_list = filters;
  
  while (tmp_list)
    {
      GdkEventFilter *filter = (GdkEventFilter*) tmp_list->data;
      
      tmp_list = tmp_list->next;
      result = filter->function (nevent, event, filter->data);
      if (result !=  GDK_FILTER_CONTINUE)
	return result;
    }
  
  return GDK_FILTER_CONTINUE;
}

void 
gdk_add_client_message_filter (GdkAtom       message_type,
			       GdkFilterFunc func,
			       gpointer      data)
{
  GdkClientFilter *filter = g_new (GdkClientFilter, 1);

  filter->type = message_type;
  filter->function = func;
  filter->data = data;
  
  client_filters = g_list_prepend (client_filters, filter);
}

/* 30 May 2001 Change by Jason 						*/
/* To consider also the distance between points for double click 	*/
/* rather than only consider the time different.			*/
static void
gdk_event_button_generate (GdkEvent *event)
{
  if ((event->button.time < (button_click_time[1] + TRIPLE_CLICK_TIME)) &&
      (event->button.window == button_window[1]) &&
      (event->button.button == button_number[1]) &&
      (ABS(event->button.x - button_x[1]) < DOUBLE_CLICK_DIST) &&
      (ABS(event->button.y - button_y[1]) < DOUBLE_CLICK_DIST) )
    {
      gdk_synthesize_click (event, 3);
    
      button_click_time[1] = 0;
      button_click_time[0] = 0;
      button_window[1] = NULL;
      button_window[0] = 0;
      button_number[1] = -1;
      button_number[0] = -1;

      button_x[0] = -1; 
      button_x[1] = -1; 
      button_y[0] = -1; 
      button_y[1] = -1; 

    }
  else if ((event->button.time < (button_click_time[0] + DOUBLE_CLICK_TIME)) &&
           (event->button.window == button_window[0]) &&
           (event->button.button == button_number[0]) && 
	   (ABS(event->button.x - button_x[0]) < DOUBLE_CLICK_DIST) &&
	   (ABS(event->button.y - button_y[0]) < DOUBLE_CLICK_DIST) )
    {
      gdk_synthesize_click (event, 2);
    
      button_click_time[1] = button_click_time[0];
      button_click_time[0] = event->button.time;
      button_window[1] = button_window[0];
      button_window[0] = event->button.window;
      button_number[1] = button_number[0];
      button_number[0] = event->button.button;

      button_x[1] = button_x[0];
      button_x[0] = event->button.x;
      button_y[1] = button_y[0];
      button_y[0] = event->button.y;
    }
  else
    {

      button_click_time[1] = 0;
      button_click_time[0] = event->button.time;
      button_window[1] = NULL;
      button_window[0] = event->button.window;
      button_number[1] = -1;
      button_number[0] = event->button.button;
      button_x[1] = -1;
      button_x[0] = event->button.x;
      button_y[1] = -1;
      button_y[0] = event->button.y;
    }
}

static gint esc_key_r = FALSE;
static gint num_pad_key_r = FALSE;
static gint after_num_pad_key_r = FALSE;

static guint
gdk_handle_num_pad_seq(guchar keyval)
{
	guint keysym =0;
	g_return_val_if_fail(keyval != 0, 0);

	if(num_pad_key_r)
	{
		
		switch(keyval)
		{
			case G_KEY_HOME:
				keysym = GDK_Home;
				break;
		
			case G_KEY_INSERT:
				 keysym = GDK_Insert;
                                break;
#ifndef CPU72
			case G_KEY_DELETE:
				keysym = GDK_Delete;
                                break;	
#endif

			 case G_KEY_END:
                                keysym = GDK_End;
                                break;

			 case G_KEY_PAGEUP:
                                keysym = GDK_Page_Up;
                                break;

			 case G_KEY_PAGEDOWN:
                                keysym = GDK_Page_Down;
                                break;

			 case G_KEY_UP:
                                keysym = GDK_Up;
                                break;

			 case G_KEY_DOWN:
                                keysym = GDK_Down;
                                break;
			
			 case G_KEY_RIGHT:
                                keysym = GDK_Right;
                                break;

			 case G_KEY_LEFT:
                                keysym = GDK_Left;
                                break;
		}	
		if( keyval<=G_KEY_LEFT && keyval >=G_KEY_UP)
			num_pad_key_r = FALSE;	
	}
	return keysym;
}

gint
gdk_event_translate (GdkEvent *event, gi_msg_t *nevent)
{
  
  GdkWindow *window=NULL;
  GdkWindowPrivate *window_private;
  guint ch=0;

  gint return_val=0;
  gint bnstr = FALSE;
  
  return_val = FALSE;
  /* Find the GdkWindow that this event occurred in.
   * 
   * We handle events with window=None
   *  specially - they are generated by XFree86's XInput under
   *  some circumstances.
   */
  
  if ((nevent->ev_window == None) &&
      gdk_input_vtable.window_none_event)
    {
      //FIXME return_val = gdk_input_vtable.window_none_event (event,nevent);
      
      if (return_val >= 0)	// was handled 
	return return_val;
      else
	return_val = FALSE;
    }
  window = gdk_window_lookup(nevent->ev_window); 
  window_private = (GdkWindowPrivate *) window;
  if (window != NULL)
    gdk_window_ref (window);
  
  event->any.window = window;
  event->any.send_event = FALSE/*xevent->xany.send_event ? TRUE : FALSE*/;
  
  if (window_private && window_private->destroyed)
    {
      //if (nevent->type != DestroyNotify) no event found in gix
	return FALSE;
    }
  else
    {
      /* Check for filters for this window
       */

      GdkFilterReturn result=GDK_FILTER_CONTINUE;
      result = gdk_event_apply_filters (nevent, event,
					window_private
					?window_private->filters
					:gdk_default_filters);
      if (result != GDK_FILTER_CONTINUE)
	{
	  return (result == GDK_FILTER_TRANSLATE) ? TRUE : FALSE;
	}

    }

  /* We do a "manual" conversion of the XEvent to a
   *  GdkEvent. The structures are mostly the same so
   *  the conversion is fairly straightforward. We also
   *  optionally print debugging info regarding events
   *  received.
   */

  return_val = TRUE;

  switch (nevent->type)
    {
#if 0
    case GI_MSG_SYSTEM_CHANGE :
		_gdk_set_encoding_l(nevent->lang.encoding);
		_gdk_set_encoding_width (nevent->lang.width);
		g_message("Received new lang %d width %d\n",
			_gdk_get_encoding(),
			_gdk_get_encoding_width());
	break;
#endif
    case GI_MSG_KEY_DOWN:
      ch = (guint)nevent->params[4];
	/* Need to map the keysym*/
	//g_message("GI_MSG_KEY_DOWN:-%d",ch);

	switch(nevent->params[4])
	{
		case G_KEY_TAB:
			ch = GDK_Tab;
			break;
		case G_KEY_RETURN:
			ch = GDK_Return; 
			bnstr = TRUE;
			break;
	/* On FB, use several bytes to reprepsent this */
#ifdef X11
		case G_KEY_HOME:
			ch = GDK_Home;
			break;
/*
		case G_KEY_INSERT:
			ch = GDK_Insert;
			break;
*/
		case G_KEY_DELETE:
			ch = GDK_Delete;
			break;

		case G_KEY_END:
			ch = GDK_End;
			break;

		case G_KEY_PAGEUP:
			ch = GDK_Page_Up;
			break;

		case G_KEY_PAGEDOWN:
			ch = GDK_Page_Down;
			break;

		case G_KEY_UP:
			ch = GDK_Up;
			break;

		case G_KEY_DOWN:
			ch = GDK_Down;
			break;

		case G_KEY_RIGHT:
			ch = GDK_Right;
			break;

		case G_KEY_LEFT:
			ch = GDK_Left;
			break;
#endif
#ifdef CPU72
		case G_KEY_DELETE:
			ch = GDK_Delete;
			break;
#endif

		case G_KEY_BACKSPACE:
			ch =  GDK_BackSpace;
			bnstr = TRUE;
			//nevent->ch = '\b';
			break;

		case G_KEY_ESCAPE:
			esc_key_r = TRUE;
			return_val= FALSE;
			goto nullevent;
			//return FALSE;
#if 0
		case G_KEY_ESC_FOLLOW:
			if(esc_key_r)
			{
				num_pad_key_r = TRUE;
				esc_key_r = FALSE;
				return_val= FALSE;
				goto nullevent;
				//return FALSE;
			}
			bnstr = TRUE;
			break;	

		case G_KEY_ESC_END:
			if(num_pad_key_r)
			{
				num_pad_key_r = FALSE;
				after_num_pad_key_r = TRUE;
				return_val= FALSE;
				goto nullevent;
				//return FALSE;
			}
			if(after_num_pad_key_r)
			{
				after_num_pad_key_r =FALSE ;
				return_val= FALSE;
				goto nullevent;
			}
			bnstr = TRUE;
			break;
#endif

		default:
                        bnstr = TRUE;
                        break;
	}
	if(num_pad_key_r)
	{
		ch = gdk_handle_num_pad_seq(ch);
		bnstr = FALSE;
	}
      event->key.keyval = ch;
      event->key.type = GDK_KEY_PRESS;
      event->key.window = window;
      event->key.time = gdk_get_time_ms();
      event->key.state = gr_mod_to_gdk(0, nevent->params[5]);
      if(bnstr)
      {
/* [00/09/08] @@@ NANOGTK: begin modification @@@ */
{
	unsigned char first_char = nevent->params[4] >> 8;
	unsigned char second_char = nevent->params[4] & 0xFF;

	//if (is_cjk_char(first_char, second_char)) {
	if (nevent->params[4] > 0xFF) {
		event->key.string = g_strdup_printf ("%c%c", first_char, second_char);
		event->key.length = 2;
	} else {
		event->key.string = g_strdup_printf ("%c", (gchar)nevent->params[4]);
		event->key.length = 1;
	}
}
/* [00/09/08] @@@ NANOGTK: end modification @@@ */
      }
      else
      {
		event->key.string = NULL; 
      		event->key.length = 0;
	
      }
      break;
      
    case GI_MSG_KEY_UP:
/* [00/08/30] @@@ NANOGTK: begin modification @@@ */
      event->key.keyval = nevent->params[4];       
/* [00/08/30] @@@ NANOGTK: end modification @@@ */
	//printf("GI_MSG_KEY_UP:-%d\n",event->key.keyval);
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS, 
		g_message ("key release:\t\twindow: %ld	 key: %12s  %d",
			   nevent->wid,
			   event->key.keyval,
			   event->key.keyval));
      
      	event->key.type = GDK_KEY_RELEASE;
      	event->key.window = window;
      	event->key.time = gdk_get_time_ms();
	event->key.state = gr_mod_to_gdk(0, nevent->params[5])|GDK_RELEASE_MASK;
      event->key.length = 0;
      event->key.string = NULL;
      
      break;
      
    case GI_MSG_BUTTON_DOWN:
      /* Print debugging info.
       */
	//printf("GI_MSG_BUTTON_DOWN-%d\n",nevent->wid);
      GDK_NOTE (EVENTS, 
		g_message ("button press:\t\twindow: %ld  x,y: %d %d  button: %d",
			   nevent->wid,
			   nevent->x, nevent->y,
			   nevent->params[2]));
      	  event->button.type = GDK_BUTTON_PRESS;
          event->button.window = window;
          event->button.time = gdk_get_time_ms();
          event->button.x = nevent->x;
          event->button.y = nevent->y;
          event->button.x_root = (gfloat)nevent->params[0];
          event->button.y_root = (gfloat)nevent->params[1];
          event->button.pressure = 0.5;
          event->button.xtilt = 0;
          event->button.ytilt = 0;
          event->button.state = gr_mod_to_gdk(0, nevent->params[2]);
          event->button.button = GI_BUTTON_TO_GDK(nevent->params[2]);
          event->button.source = GDK_SOURCE_MOUSE;
          event->button.deviceid = GDK_CORE_POINTER;
          //g_message("button down: %d", event->button.button);
          gdk_event_button_generate (event); 
      break;
      
    case GI_MSG_BUTTON_UP:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS, 
		g_message ("button release:\twindow: %ld  x,y: %d %d  button: %d",
			   nevent->wid,
			   nevent->x, nevent->y,
			   nevent->params[2]));
      
	//printf("GI_MSG_BUTTON_UP-%d\n",nevent->wid);
	 event->button.type = GDK_BUTTON_RELEASE;
          event->button.window = window;
          event->button.time = gdk_get_time_ms();
          event->button.x = nevent->x;
          event->button.y = nevent->y;
          event->button.x_root = (gfloat)nevent->params[0];
          event->button.y_root = (gfloat)nevent->params[1];
          event->button.pressure = 0.5;
          event->button.xtilt = 0;
          event->button.ytilt = 0;
      	  event->button.state = gr_mod_to_gdk(0, nevent->params[3])|GDK_RELEASE_MASK;
          event->button.button = GI_BUTTON_TO_GDK(nevent->params[3]);
          event->button.source = GDK_SOURCE_MOUSE;
          event->button.deviceid = GDK_CORE_POINTER;
          //g_message("button up: %d", event->button.button);
          //gdk_event_button_generate (event);
      break;

#if 0 // Gix no this event      
    case MotionNotify:
      /* Print debugging info.
       */
      
      event->motion.type = GDK_MOTION_NOTIFY;
      event->motion.window = window;
      event->motion.time = xevent->xmotion.time;
      event->motion.x = xevent->xmotion.x;
      event->motion.y = xevent->xmotion.y;
      event->motion.x_root = (gfloat)xevent->xmotion.x_root;
      event->motion.y_root = (gfloat)xevent->xmotion.y_root;
      event->motion.pressure = 0.5;
      event->motion.xtilt = 0;
      event->motion.ytilt = 0;
      event->motion.state = (GdkModifierType) xevent->xmotion.state;
      event->motion.is_hint = xevent->xmotion.is_hint;
      event->motion.source = GDK_SOURCE_MOUSE;
      event->motion.deviceid = GDK_CORE_POINTER;
      break;

    case EnterNotify:
      /* Print debugging info.
       */
      
      /* Tell XInput stuff about it if appropriate */
      if (window_private &&
	  !window_private->destroyed &&
	  (window_private->extension_events != 0) &&
	  gdk_input_vtable.enter_event)
	gdk_input_vtable.enter_event (&xevent->xcrossing, window);
      
      event->crossing.type = GDK_ENTER_NOTIFY;
      event->crossing.window = window;
      
      /* If the subwindow field of the XEvent is non-NULL, then
       *  lookup the corresponding GdkWindow.
       */
      if (xevent->xcrossing.subwindow != None)
	event->crossing.subwindow = gdk_window_lookup (xevent->xcrossing.subwindow);
      else
	event->crossing.subwindow = NULL;
      
      event->crossing.time = xevent->xcrossing.time;
      event->crossing.x = xevent->xcrossing.x;
      event->crossing.y = xevent->xcrossing.y;
      event->crossing.x_root = xevent->xcrossing.x_root;
      event->crossing.y_root = xevent->xcrossing.y_root;
      
      /* Translate the crossing mode into Gdk terms.
       */
      switch (xevent->xcrossing.mode)
	{
	case NotifyNormal:
	  event->crossing.mode = GDK_CROSSING_NORMAL;
	  break;
	case NotifyGrab:
	  event->crossing.mode = GDK_CROSSING_GRAB;
	  break;
	case NotifyUngrab:
	  event->crossing.mode = GDK_CROSSING_UNGRAB;
	  break;
	};
      
      /* Translate the crossing detail into Gdk terms.
       */
      switch (xevent->xcrossing.detail)
	{
	case NotifyInferior:
	  event->crossing.detail = GDK_NOTIFY_INFERIOR;
	  break;
	case NotifyAncestor:
	  event->crossing.detail = GDK_NOTIFY_ANCESTOR;
	  break;
	case NotifyVirtual:
	  event->crossing.detail = GDK_NOTIFY_VIRTUAL;
	  break;
	case NotifyNonlinear:
	  event->crossing.detail = GDK_NOTIFY_NONLINEAR;
	  break;
	case NotifyNonlinearVirtual:
	  event->crossing.detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
	  break;
	default:
	  event->crossing.detail = GDK_NOTIFY_UNKNOWN;
	  break;
	}
      
      event->crossing.focus = xevent->xcrossing.focus;
      event->crossing.state = xevent->xcrossing.state;
  
      break;
      
    case LeaveNotify:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS, 
		g_message ("leave notify:\t\twindow: %ld  detail: %d subwin: %ld",
			   xevent->xcrossing.window,
			   xevent->xcrossing.detail, xevent->xcrossing.subwindow));
      
      event->crossing.type = GDK_LEAVE_NOTIFY;
      event->crossing.window = window;
      
      /* If the subwindow field of the XEvent is non-NULL, then
       *  lookup the corresponding GdkWindow.
       */
      if (xevent->xcrossing.subwindow != None)
	event->crossing.subwindow = gdk_window_lookup (xevent->xcrossing.subwindow);
      else
	event->crossing.subwindow = NULL;
      
      event->crossing.time = xevent->xcrossing.time;
      event->crossing.x = xevent->xcrossing.x;
      event->crossing.y = xevent->xcrossing.y;
      event->crossing.x_root = xevent->xcrossing.x_root;
      event->crossing.y_root = xevent->xcrossing.y_root;
      
      /* Translate the crossing mode into Gdk terms.
       */
      switch (xevent->xcrossing.mode)
	{
	case NotifyNormal:
	  event->crossing.mode = GDK_CROSSING_NORMAL;
	  break;
	case NotifyGrab:
	  event->crossing.mode = GDK_CROSSING_GRAB;
	  break;
	case NotifyUngrab:
	  event->crossing.mode = GDK_CROSSING_UNGRAB;
	  break;
	};
      
      /* Translate the crossing detail into Gdk terms.
       */
      switch (xevent->xcrossing.detail)
	{
	case NotifyInferior:
	  event->crossing.detail = GDK_NOTIFY_INFERIOR;
	  break;
	case NotifyAncestor:
	  event->crossing.detail = GDK_NOTIFY_ANCESTOR;
	  break;
	case NotifyVirtual:
	  event->crossing.detail = GDK_NOTIFY_VIRTUAL;
	  break;
	case NotifyNonlinear:
	  event->crossing.detail = GDK_NOTIFY_NONLINEAR;
	  break;
	case NotifyNonlinearVirtual:
	  event->crossing.detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
	  break;
	default:
	  event->crossing.detail = GDK_NOTIFY_UNKNOWN;
	  break;
	}
      
      event->crossing.focus = xevent->xcrossing.focus;
      event->crossing.state = xevent->xcrossing.state;
      
      break;
#endif // Gix no these events
      
    case GI_MSG_FOCUS_IN:
    case GI_MSG_FOCUS_OUT:
 	  /* gdk_keyboard_grab() causes following events. These events confuse
 	   * the XIM focus, so ignore them.
 	   */
	  event->focus_change.type = GDK_FOCUS_CHANGE;
	  event->focus_change.window = window;
	  event->focus_change.in = (nevent->type == GI_MSG_FOCUS_IN);
	  break;

    case GI_MSG_EXPOSURE:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("expose:\t\twindow: %ld 	x,y: %d %d  w,h: %d %d%s",
			   nevent->wid, 
			   nevent->x, nevent->y,
			   nevent->width, nevent->height,
			   " (send)"));
	/* it seems slower to try to compress here. FIX this or forget it */
	//gdk_compress_exposures (nevent, window);
      

      event->expose.type = GDK_EXPOSE;
      event->expose.window =window ;
      event->expose.area.x = nevent->x;
      event->expose.area.y = nevent->y;
      event->expose.area.width = nevent->width;
      event->expose.area.height = nevent->height;
      event->expose.count = 0; 
      break;

        case GI_MSG_CLIENT_MSG:
			if(nevent->client.client_type==GA_WM_PROTOCOLS
					&&nevent->params[0] == GA_WM_DELETE_WINDOW){
                event->any.type = GDK_DELETE;
                event->any.window = window;
		}
        break;

#if 0
    case EVENT_TYPE_GRAPHICS_EXPOSE:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("graphics expose:\tdrawable: %ld",
			   xevent->xgraphicsexpose.drawable));
      
      event->expose.type = GDK_EXPOSE;
      event->expose.window = window;
      event->expose.area.x = nevent->x;
      event->expose.area.y = nevent->y;
      event->expose.area.width = nevent->width;
      event->expose.area.height = nevent->height;
      event->expose.count = 0;
      
      break;
    case NoExpose:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("no expose:\t\tdrawable: %ld",
			   xevent->xnoexpose.drawable));
      
      event->no_expose.type = GDK_NO_EXPOSE;
      event->no_expose.window = window;
      
      break;

      
    case UnmapNotify:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("unmap notify:\t\twindow: %ld",
			   xevent->xmap.window));
      
      event->any.type = GDK_UNMAP;
      event->any.window = window;
      
      if (gdk_xgrab_window == window_private)
	gdk_xgrab_window = NULL;
      
      break;
      
    case MapNotify:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("map notify:\t\twindow: %ld",
			   xevent->xmap.window));
      
      event->any.type = GDK_MAP;
      event->any.window = window;
      
      break;
      
    case ReparentNotify:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("reparent notify:\twindow: %ld  x,y: %d %d  parent: %ld	ovr: %d",
			   xevent->xreparent.window,
			   xevent->xreparent.x,
			   xevent->xreparent.y,
			   xevent->xreparent.parent,
			   xevent->xreparent.override_redirect));

      /* Not currently handled */
      return_val = FALSE;
      break;
      
    case ConfigureNotify:
      /* Print debugging info.
       */
      while (0 && /* don't reorder ConfigureNotify events at all */
	     XPending (gdk_display) > 0 &&
	     XCheckTypedWindowEvent (gdk_display, xevent->xany.window,
				     ConfigureNotify, xevent))
	{
	  GdkFilterReturn result;
	  
	  GDK_NOTE (EVENTS, 
		    g_message ("configure notify discarded:\twindow: %ld",
			       xevent->xconfigure.window));
	  
	  result = gdk_event_apply_filters (xevent, event,
					    window_private
					    ?window_private->filters
					    :gdk_default_filters);
	  
	  /* If the result is GDK_FILTER_REMOVE, there will be
	   * trouble, but anybody who filtering the Configure events
	   * better know what they are doing
	   */
	  if (result != GDK_FILTER_CONTINUE)
	    {
	      return (result == GDK_FILTER_TRANSLATE) ? TRUE : FALSE;
	    }
	  
	  /*XSync (gdk_display, 0);*/
	}
      
      
      GDK_NOTE (EVENTS,
		g_message ("configure notify:\twindow: %ld  x,y: %d %d	w,h: %d %d  b-w: %d  above: %ld	 ovr: %d%s",
			   xevent->xconfigure.window,
			   xevent->xconfigure.x,
			   xevent->xconfigure.y,
			   xevent->xconfigure.width,
			   xevent->xconfigure.height,
			   xevent->xconfigure.border_width,
			   xevent->xconfigure.above,
			   xevent->xconfigure.override_redirect,
			   !window
			   ? " (discarding)"
			   : window_private->window_type == GDK_WINDOW_CHILD
			   ? " (discarding child)"
			   : ""));
      if (window &&
	  !window_private->destroyed &&
	  (window_private->extension_events != 0) &&
	  gdk_input_vtable.configure_event)
	gdk_input_vtable.configure_event (&xevent->xconfigure, window);

      if (!window || window_private->window_type == GDK_WINDOW_CHILD)
	return_val = FALSE;
      else
	{
	  event->configure.type = GDK_CONFIGURE;
	  event->configure.window = window;
	  event->configure.width = xevent->xconfigure.width;
	  event->configure.height = xevent->xconfigure.height;
	  
	  if (!xevent->xconfigure.x &&
	      !xevent->xconfigure.y &&
	      !window_private->destroyed)
	    {
	      gint tx = 0;
	      gint ty = 0;
	      Window child_window = 0;

	      gdk_error_trap_push ();
	      if (XTranslateCoordinates (window_private->xdisplay,
					 window_private->xwindow,
					 gdk_root_window,
					 0, 0,
					 &tx, &ty,
					 &child_window))
		{
		  if (!gdk_error_trap_pop ())
		    {
		      event->configure.x = tx;
		      event->configure.y = ty;
		    }
		}
	      else
		gdk_error_trap_pop ();
	    }
	  else
	    {
	      event->configure.x = xevent->xconfigure.x;
	      event->configure.y = xevent->xconfigure.y;
	    }
	  window_private->x = event->configure.x;
	  window_private->y = event->configure.y;
	  window_private->width = xevent->xconfigure.width;
	  window_private->height = xevent->xconfigure.height;
	  if (window_private->resize_count > 1)
	    window_private->resize_count -= 1;
	}
      break;
      
    case PropertyNotify:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		gchar *atom = gdk_atom_name (xevent->xproperty.atom);
		g_message ("property notify:\twindow: %ld, atom(%ld): %s%s%s",
			   xevent->xproperty.window,
			   xevent->xproperty.atom,
			   atom ? "\"" : "",
			   atom ? atom : "unknown",
			   atom ? "\"" : "");
		);
      
      event->property.type = GDK_PROPERTY_NOTIFY;
      event->property.window = window;
      event->property.atom = xevent->xproperty.atom;
      event->property.time = xevent->xproperty.time;
      event->property.state = xevent->xproperty.state;
      
      break;
      
    case SelectionClear:
      GDK_NOTE (EVENTS,
		g_message ("selection clear:\twindow: %ld",
			   xevent->xproperty.window));
      
      event->selection.type = GDK_SELECTION_CLEAR;
      event->selection.window = window;
      event->selection.selection = xevent->xselectionclear.selection;
      event->selection.time = xevent->xselectionclear.time;
      
      break;
      
    case SelectionRequest:
      GDK_NOTE (EVENTS,
		g_message ("selection request:\twindow: %ld",
			   xevent->xproperty.window));
      
      event->selection.type = GDK_SELECTION_REQUEST;
      event->selection.window = window;
      event->selection.selection = xevent->xselectionrequest.selection;
      event->selection.target = xevent->xselectionrequest.target;
      event->selection.property = xevent->xselectionrequest.property;
      event->selection.requestor = xevent->xselectionrequest.requestor;
      event->selection.time = xevent->xselectionrequest.time;
      
      break;
      
    case SelectionNotify:
      GDK_NOTE (EVENTS,
		g_message ("selection notify:\twindow: %ld",
			   xevent->xproperty.window));
      
      
      event->selection.type = GDK_SELECTION_NOTIFY;
      event->selection.window = window;
      event->selection.selection = xevent->xselection.selection;
      event->selection.target = xevent->xselection.target;
      event->selection.property = xevent->xselection.property;
      event->selection.time = xevent->xselection.time;
      
      break;
      
    case ColormapNotify:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
		g_message ("colormap notify:\twindow: %ld",
			   xevent->xcolormap.window));
      
      /* Not currently handled */
      return_val = FALSE;
      break;
      
#endif // no gix & to be FIXME      
#if 0
    case GI_MSG_SELECTION_REQUEST:
      GDK_NOTE (EVENTS,
                g_message ("selection request:\twindow: %ld\t target: %d",
                           nevent->owner, nevent->target));

      event->selection.type = GDK_SELECTION_REQUEST;
      event->selection.window = window;
      event->selection.selection = nevent->selection;
      event->selection.target = nevent->target;
      event->selection.property = nevent->property;
      event->selection.requestor = nevent->requestor;
      event->selection.time =  nevent->time;

      break;

    case GI_MSG_SELECTION_NOTIFY:
      GDK_NOTE (EVENTS,
                g_message ("selection notify:\twindow: %ld \t target: %d",
                           nevent->owner, nevent->target));
      event->selection.type = GDK_SELECTION_NOTIFY;
      event->selection.window = window;
      event->selection.selection = nevent->selection;
      event->selection.target = nevent->target;
      event->selection.property = nevent->property;
      event->selection.time = nevent->time;
        break;

    case GI_MSG_SELECTION_CLEAR:
      GDK_NOTE (EVENTS,
                g_message ("selection clear:\twindow: %ld",
                           nevent->owner));

      event->selection.type = GDK_SELECTION_CLEAR;
      event->selection.window = window;
      event->selection.selection = nevent->selection;
      event->selection.time = nevent->time;
      break;

    case GI_MSG_PROPERTY_NOTIFY:
      /* Print debugging info.
       */
      GDK_NOTE (EVENTS,
                gchar *atom = gdk_atom_name (nevent->property.atom);
                g_message ("property notify:\twindow: %ld, atom(%ld): %s%s%s",
                           nevent->property.wid,
                           nevent->property.atom,
                           atom ? "\"" : "",
                           atom ? atom : "unknown",
                           atom ? "\"" : "");
                );

      event->property.type = GDK_PROPERTY_NOTIFY;
      event->property.window = window;
      event->property.atom = nevent->property.atom;
      event->property.time = nevent->property.time;
      event->property.state = nevent->property.state;
      break;
#endif

	case GI_MSG_MOUSE_EXIT:
		//printf("GI_MSG_MOUSE_EXIT\n");
      		event->crossing.type = GDK_LEAVE_NOTIFY;
      		event->crossing.window = window;
      		event->crossing.subwindow = NULL;
      		event->crossing.time = gdk_get_time_ms();
      		event->crossing.mode = GDK_CROSSING_NORMAL;
		event->crossing.x = nevent->x;
                event->crossing.y = nevent->y;
                event->crossing.x_root = (gfloat)nevent->params[0];
                event->crossing.y_root = (gfloat)nevent->params[1];
      		event->crossing.detail = GDK_NOTIFY_UNKNOWN;
		//event->crossing.focus = 1;
      		//event->crossing.state = 0;

      		/* other stuff here , x, y, x_root, y_root */
		break;
	case GI_MSG_MOUSE_ENTER:
		 GDK_NOTE (EVENTS,
                g_message ("enter notify:\t\twindow: %ld  x,y:%d %d",
                           nevent->ev_window, ((GdkWindowPrivate*)window)->x, ((GdkWindowPrivate*)window)->y));

		event->crossing.type = GDK_ENTER_NOTIFY;
                event->crossing.window = window;
                event->crossing.subwindow = NULL;
                event->crossing.time = gdk_get_time_ms();
                event->crossing.mode = GDK_CROSSING_NORMAL;
		event->crossing.x = nevent->x;
      		event->crossing.y = nevent->y;
		event->crossing.x_root = (gfloat)nevent->params[0];
      		event->crossing.y_root = (gfloat)nevent->params[1];
                event->crossing.detail = GDK_NOTIFY_UNKNOWN;
		//event->crossing.focus = 0;
      		//event->crossing.state = 0;
		break;

	case GI_MSG_MOUSE_MOVE:
		//printf("GI_MSG_MOUSE_MOTION-%d\n",nevent->wid);
		event->motion.type = GDK_MOTION_NOTIFY;
      		event->motion.window = window;
      		event->motion.time = gdk_get_time_ms();
          	event->motion.x = nevent->x;
          	event->motion.y = nevent->y;
          	event->motion.x_root = (gfloat)nevent->params[0];
          	event->motion.y_root = (gfloat)nevent->params[1];
      		event->motion.pressure = 0.5;
      		event->motion.xtilt = 0;
      		event->motion.ytilt = 0;
      		event->motion.state = gr_mod_to_gdk(0, nevent->params[2]);
      		event->motion.is_hint = 1;
      		event->motion.source = GDK_SOURCE_MOUSE;
      		event->motion.deviceid = GDK_CORE_POINTER;
		break;

	case GI_MSG_WINDOW_SHOW:
		if (nevent->ev_window == nevent->effect_window)
		{
		return_val = FALSE;      
      break;
		}
		 GDK_NOTE (EVENTS,
                	g_message ("map notify:\t\twindow: %ld", nevent->ev_window));
				event->any.type = GDK_MAP;
      				event->any.window = window;
		break;

	case GI_MSG_WINDOW_HIDE:
		if (nevent->ev_window == nevent->effect_window)
		{
		return_val = FALSE;      
      break;
		}
		GDK_NOTE (EVENTS,
                	g_message ("unmap notify:\t\twindow: %ld", nevent->ev_window));
				event->any.type = GDK_UNMAP;
        			event->any.window = window;
				if (gdk_xgrab_window == window_private)
          				gdk_xgrab_window = NULL;
		break;

	case GI_MSG_CONFIGURENOTIFY:
		if (nevent->ev_window == nevent->effect_window)
		{
		return_val = FALSE;      
      break;
		}
		//printf("GI_MSG_UPDATE-%d %d\n",nevent->utype, nevent->wid);
		//g_message("GI_MSG_UPDATE:-%d\n",nevent->utype);
		switch(nevent->params[0])
		{	
			case GI_STRUCT_CHANGE_MOVE:
				      GDK_NOTE (EVENTS,
                g_message ("configure move notify:\twindow: %ld  x,y: %d %d  w,h: %d %d  %s",
                           nevent->ev_window,
                           nevent->x,
                           nevent->y,
                           nevent->width,
                           nevent->height,
                           !window
                           ? " (discarding)"
                           : window_private->window_type == GDK_WINDOW_CHILD
                           ? " (discarding child)"
                           : ""));

                                //if(!window || window_private->window_type == GDK_WINDOW_CHILD)
                                        return_val=FALSE;
                                /*else
                                {
                                        event->any.type = GDK_CONFIGURE;
                                        event->any.window = window;
                                        event->configure.type = GDK_CONFIGURE;
                                        event->configure.window = window;
                                        event->configure.width = nevent->width;
                                        event->configure.height = nevent->height;
                                        event->configure.x =  nevent->x;
                                        event->configure.y = nevent->y;
                                        window_private->x = nevent->x;
                                        window_private->y = nevent->y;
                                        window_private->width = nevent->width;
                                        window_private->height =nevent->height;
                                        if (window_private->resize_count > 1)
                                                window_private->resize_count -= 1;
                                }*/
				break;

			case GI_STRUCT_CHANGE_RESIZE:
			GDK_NOTE (EVENTS,
                g_message ("configure resize notify:\twindow: %ld  x,y: %d %d  w,h: %d %d  %s",
                           nevent->ev_window,
                           nevent->x,
                           nevent->y,
                           nevent->width,
                           nevent->height,
                           !window
                           ? " (discarding)"
                           : window_private->window_type == GDK_WINDOW_CHILD
                           ? " (discarding child)"
                           : ""));

				if(!window || window_private->window_type == GDK_WINDOW_CHILD)
                                        return_val=FALSE;
                                else
                                {
                                        event->any.type = GDK_CONFIGURE;
                                        event->any.window = window;
                                        event->configure.type = GDK_CONFIGURE;
                                        event->configure.window = window;
                                        event->configure.width = nevent->width;
                                        event->configure.height = nevent->height;
                                        event->configure.x =  nevent->x;
                                        event->configure.y = nevent->y;
                                        window_private->x = nevent->x;
                                        window_private->y = nevent->y;
                                        window_private->width = event->configure.width;
                                        window_private->height = event->configure.height;
                                        if (window_private->resize_count > 1)
                                                window_private->resize_count -= 1;
                                }
				break;
		}
	break; 

    default:
      /* something else - (e.g., a Xinput event) */
      
      /*if (window_private &&
	  !window_private->destroyed &&
	  (window_private->extension_events != 0) &&
	  gdk_input_vtable.other_event)
	return_val = gdk_input_vtable.other_event(event, nevent, window);
      else*/
	return_val = FALSE;
      
      break;
    }
  
  if (return_val)
    {
      if (event->any.window)
	gdk_window_ref (event->any.window);
      if (((event->any.type == GDK_ENTER_NOTIFY) ||
	   (event->any.type == GDK_LEAVE_NOTIFY)) &&
	  (event->crossing.subwindow != NULL))
	gdk_window_ref (event->crossing.subwindow);
    }
  else
    {
nullevent:
      /* Mark this event as having no resources to be freed */
      event->any.window = NULL;
      event->any.type = GDK_NOTHING;
    }
  
  if (window)
    gdk_window_unref (window);
  
  return return_val;
}

GdkFilterReturn
gdk_wm_protocols_filter (GdkXEvent *nev, // void type
			 GdkEvent  *event,
			 gpointer data)
{
#if NOT_IMPLEMENTED // Xlicent Messge event ? no macthing in GIX FIXME
  gi_msg_t *nevent = (gi_msg_t *)nev;
  if ((Atom) nevent->xclient.data.l[0] == gdk_wm_delete_window)
    {
  /* The delete window request specifies a window
   *  to delete. We don't actually destroy the
   *  window because "it is only a request". (The
   *  window might contain vital data that the
   *  program does not want destroyed). Instead
   *  the event is passed along to the program,
   *  which should then destroy the window.
   */
      GDK_NOTE (EVENTS,
		g_message ("delete window:\t\twindow: %ld",
			   xevent->xclient.window));
      
      event->any.type = GDK_DELETE;

      return GDK_FILTER_TRANSLATE;
    }
  else if ((Atom) xevent->xclient.data.l[0] == gdk_wm_take_focus)
    {
    }
#endif 
  return GDK_FILTER_REMOVE;
}

#if 0
static Bool
gdk_event_get_type (Display  *display,
		    XEvent   *xevent,
		    XPointer  arg)
{
  GdkEvent event;
  GdkPredicate *pred;
  
  if (gdk_event_translate (&event, xevent))
    {
      pred = (GdkPredicate*) arg;
      return (* pred->func) (&event, pred->data);
    }
  
  return FALSE;
}
#endif

static long 
gdk_get_time_ms()
{
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv,&tz);

	return tv.tv_sec*1000+tv.tv_usec/1000;
}

#ifdef USE_SIGNAL
static void
gdk_events_queue (void)
{
  GList *node;
  GdkEvent *event;
  gi_msg_t nevent;

  while (1) //!gdk_event_queue_find_first() &&  GrPeekEvent(&nevent))
    {
#if 0 //def USE_SIGNAL
	if (gdk_event_queue_find_first() || (EventReady<=0))
		return;
#else
	if (gdk_event_queue_find_first())
		return;
#endif
	if (GrEventCached.type==GI_MSG_NONE)
	{
		gi_get_message(&nevent,MSG_ALWAYS_WAIT);
		if(!nevent.type)
		{
			EventReady = 0;
			return;
		}
		memcpy(&GrEventCached,&nevent,sizeof(nevent));
	}
#ifdef USE_SIGNAL
		//printf("processing %d\n",EventReady);
        	EventReady --;
#endif
      event = gdk_event_new ();
      event->any.type = GDK_NOTHING;
      event->any.window = NULL;
      event->any.send_event = FALSE; 

      ((GdkEventPrivate *)event)->flags |= GDK_EVENT_PENDING;

      gdk_event_queue_append (event);
      node = queued_tail;

      if (gdk_event_translate (event, &GrEventCached))
	{
	  ((GdkEventPrivate *)event)->flags &= ~GDK_EVENT_PENDING;
	}
      else
	{
	  gdk_event_queue_remove_link (node);
	  g_list_free_1 (node);
	  gdk_event_free (event);
	}
	GrEventCached.type=GI_MSG_NONE;
    }
}
#else
static void
gdk_events_queue (void)
{
  GList *node;
  GdkEvent *event;
  gi_msg_t xevent;

  while (!gdk_event_queue_find_first() && gdk_events_pending())
    {
	if (event_cached)
	{
		xevent = GrEventCached;
		GrEventCached.type = GI_MSG_NONE;
		event_cached = 0;
	}
	else
      		gi_get_message (&xevent,MSG_ALWAYS_WAIT);

#ifdef TEST
	gdk_events_pending_remove();
#endif

      event = gdk_event_new ();
      event->any.type = GDK_NOTHING;
      event->any.window = NULL;
      event->any.send_event = FALSE;

      ((GdkEventPrivate *)event)->flags |= GDK_EVENT_PENDING;

      gdk_event_queue_append (event);
      node = queued_tail;

      if (gdk_event_translate (event, &xevent))
        {
          ((GdkEventPrivate *)event)->flags &= ~GDK_EVENT_PENDING;
        }
      else
        {
          gdk_event_queue_remove_link (node);
          g_list_free_1 (node);
          gdk_event_free (event);
        }
    }
}
#endif


void sigusr1()
{
#ifdef USE_SIGNAL
        (EventReady>=0) ?EventReady ++:1;
	//printf("%d arrived\n",EventReady);
#endif
}

static gboolean  
gdk_event_prepare (gpointer  source_data, 
		   GTimeVal *current_time,
		   gint     *timeout,
		   gpointer  user_data)
{
	gboolean retval=0;

	GDK_THREADS_ENTER ();

	*timeout = -1;

#ifdef USE_SIGNAL
        if (EventReady>0 || (gdk_event_queue_find_first () != NULL))
        {
                return 1;
        }
        else
	{
		gi_get_message(&GrEventCached,MSG_ALWAYS_WAIT);
		if (GrEventCached.type!=GI_MSG_NONE)
			return 1;
		else
		{
			EventReady = 0;
			return 0;
		}
	}		
#else
	retval = (gdk_event_queue_find_first () != NULL) || gdk_events_pending ();
#endif

	GDK_THREADS_LEAVE ();

	return retval;
}

static gboolean  
gdk_event_check (gpointer  source_data,
		 GTimeVal *current_time,
		 gpointer  user_data)
{
  gboolean retval;
  
  GDK_THREADS_ENTER ();

  if (event_poll_fd.revents & G_IO_IN)
	{
#ifdef USE_SIGNAL
  	gi_msg_t nevent;
    retval = (gdk_event_queue_find_first () != NULL) || gi_peek_message(&nevent);
#else
#ifdef TEST
		gdk_events_pending_add();
#endif
    	retval = (gdk_event_queue_find_first () != NULL) || gdk_events_pending();
#endif
	}
  else
    retval = FALSE;

  GDK_THREADS_LEAVE ();

  return retval;
}

static GdkEvent*
gdk_event_unqueue (void)
{
  GdkEvent *event = NULL;
  GList *tmp_list;

  tmp_list = gdk_event_queue_find_first ();

  if (tmp_list)
    {
      event = tmp_list->data;
      gdk_event_queue_remove_link (tmp_list);
      g_list_free_1 (tmp_list);
    }

  return event;
}

gboolean  
gdk_event_dispatch (gpointer  source_data,
		    GTimeVal *current_time,
		    gpointer  user_data)
{
  GdkEvent *event;
 
  GDK_THREADS_ENTER ();

  gdk_events_queue();
  event = gdk_event_unqueue();

  if (event)
    {
      if (event_func)
	(*event_func) (event, event_data);
      
      gdk_event_free (event);
    }
  
  GDK_THREADS_LEAVE ();

  return TRUE;
}

static void
gdk_synthesize_click (GdkEvent *event,
		      gint	nclicks)
{
  GdkEvent temp_event;
  
  g_return_if_fail (event != NULL);
  
  temp_event = *event;
  temp_event.type = (nclicks == 2) ? GDK_2BUTTON_PRESS : GDK_3BUTTON_PRESS;
  
  gdk_event_put (&temp_event);
}

/* Sends a ClientMessage to all toplevel client windows */
gboolean
gdk_event_send_client_message (GdkEvent *event, guint32 xid)
{
	PRINTFILE("gdk_event_prepare");
	return FALSE;
}

/* Sends a ClientMessage to all toplevel client windows */
gboolean
#if 0
gdk_event_send_client_message_to_all_recurse (XEvent  *xev, 
#else
gdk_event_send_client_message_to_all_recurse (
#endif
					      guint32  xid,
					      guint    level)
{
	PRINTFILE("gdk_event_send_client_message_to_all_recurse");
	return False;
}

void
gdk_event_send_clientmessage_toall (GdkEvent *event)
{
	PRINTFILE("gdk_event_send_clientmessage_toall");
}

/*
 *--------------------------------------------------------------
 * gdk_flush
 *
 *   Flushes the Xlib output buffer and then waits
 *   until all requests have been received and processed
 *   by the X server. The only real use for this function
 *   is in dealing with XShm.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
gdk_flush (void)
{
	//GrFlush();	
}


