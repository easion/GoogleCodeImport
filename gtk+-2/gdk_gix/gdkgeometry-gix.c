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
 * Copyright (C) 2011 www.hanxuantech.com.
 * Written by Easion <easion@hanxuantech.com> , it's based
 * on DirectFB port.
 */

#include "config.h"
#include "gdk.h"        /* For gdk_rectangle_intersect */

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkinternals.h"
#include "gdkalias.h"



struct _GdkWindowParentPos
{
  gint x;
  gint y;
  gint x11_x;
  gint x11_y;
  GdkRectangle clip_rect;
};


typedef enum {
  GDK_WINDOW_QUEUE_TRANSLATE,
  GDK_WINDOW_QUEUE_ANTIEXPOSE
} GdkWindowQueueType;

struct _GdkWindowQueueItem
{
  GdkWindow *window;
  //gulong serial;
  GdkWindowQueueType type;
  union {
    struct {
      GdkRegion *area;
      gint dx;
      gint dy;
    } translate;
    struct {
      GdkRegion *area;
    } antiexpose;
  } u;
};

typedef struct _GdkWindowQueueItem GdkWindowQueueItem;
typedef struct _GdkWindowParentPos GdkWindowParentPos;

void gdk_window_queue_translation (GdkWindow *window,
			      GdkRegion *area,
			      gint       dx,
			      gint       dy);

void            _gdk_x11_window_tmp_reset_bg  (GdkWindow *window,
					       gboolean   recurse);
static void
gdk_window_compute_position (GdkWindowImplGIX   *window,
			     GdkWindowParentPos *parent_pos,
			     GdkXPositionInfo   *info);
static void
gdk_window_guffaw_scroll (GdkWindow    *window,
			  gint          dx,
			  gint          dy);

static void
gdk_window_compute_parent_pos (GdkWindowImplGIX      *window,
			       GdkWindowParentPos *parent_pos);
void
_gdk_window_init_position (GdkWindow *window)
{
  GdkWindowParentPos parent_pos;
  GdkWindowImplGIX *impl;
  
  g_return_if_fail (GDK_IS_WINDOW (window));
  
  impl =  GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);

  //fixme dpp
  
  gdk_window_compute_parent_pos (impl, &parent_pos);
  gdk_window_compute_position (impl, &parent_pos, &impl->position_info);
}


static void
compute_intermediate_position (GdkXPositionInfo *position_info,
			       GdkXPositionInfo *new_info,
			       gint              d_xoffset,
			       gint              d_yoffset,
			       GdkRectangle     *new_position)
{
  gint new_x0, new_x1, new_y0, new_y1;
  
  /* Wrap d_xoffset, d_yoffset into [-32768,32767] range. For the
   * purposes of subwindow movement, it doesn't matter if we are
   * off by a factor of 65536, and if we don't do this range
   * reduction, we'll end up with invalid widths.
   */
  d_xoffset = (gint16)d_xoffset;
  d_yoffset = (gint16)d_yoffset;
  
  if (d_xoffset < 0)
    {
      new_x0 = position_info->x + d_xoffset;
      new_x1 = position_info->x + position_info->width;
    }
  else
    {
      new_x0 = position_info->x;
      new_x1 = position_info->x + new_info->width + d_xoffset;
    }

  new_position->x = new_x0;
  new_position->width = new_x1 - new_x0;
  
  if (d_yoffset < 0)
    {
      new_y0 = position_info->y + d_yoffset;
      new_y1 = position_info->y + position_info->height;
    }
  else
    {
      new_y0 = position_info->y;
      new_y1 = position_info->y + new_info->height + d_yoffset;
    }
  
  new_position->y = new_y0;
  new_position->height = new_y1 - new_y0;
}

static void
translate_pos (GdkWindowParentPos *dest, GdkWindowParentPos *src,
               GdkWindowObject *obj, GdkXPositionInfo *pos_info,
               gboolean set_clip)
{
  dest->x = src->x + obj->x;
  dest->y = src->y + obj->y;
  dest->x11_x = src->x11_x + pos_info->x;
  dest->x11_y = src->x11_y + pos_info->y;

  if (set_clip)
      dest->clip_rect = pos_info->clip_rect;
}


static void
reset_backgrounds (GdkWindow *window)
{
  GdkWindowObject *obj = (GdkWindowObject *)window;

  _gdk_x11_window_tmp_reset_bg (window, FALSE);
  
  if (obj->parent)
    _gdk_x11_window_tmp_reset_bg ((GdkWindow *)obj->parent, FALSE);
}

static void
map_if_needed (GdkWindow *window, GdkXPositionInfo *pos_info)
{
  GdkWindowObject *obj = (GdkWindowObject *) window;
  GdkWindowImplGIX *impl = GDK_WINDOW_IMPL_GIX (obj->impl);

  if (!impl->position_info.mapped && pos_info->mapped && GDK_WINDOW_IS_MAPPED (obj))
    gi_show_window ( GDK_DRAWABLE_GIX_ID (window));
}

static void
unmap_if_needed (GdkWindow *window, GdkXPositionInfo *pos_info)
{
  GdkWindowObject *obj = (GdkWindowObject *) window;
  GdkWindowImplGIX *impl = GDK_WINDOW_IMPL_GIX (obj->impl);

  if (impl->position_info.mapped && !pos_info->mapped)
    gi_hide_window ( GDK_DRAWABLE_GIX_ID (window));
}



static void
gdk_window_compute_position (GdkWindowImplGIX   *window,
			     GdkWindowParentPos *parent_pos,
			     GdkXPositionInfo   *info)
{
  GdkWindowObject *wrapper;
  int parent_x_offset;
  int parent_y_offset;

  g_return_if_fail (GDK_IS_WINDOW_IMPL_GIX (window));

  wrapper = GDK_WINDOW_OBJECT (GDK_DRAWABLE_IMPL_GIX (window)->wrapper);
  
  info->big = FALSE;
  
  if (window->width <= 32767)
    {
      info->width = window->width;
      info->x = parent_pos->x + wrapper->x - parent_pos->x11_x;
    }
  else
    {
      info->big = TRUE;
      info->width = 32767;
      if (parent_pos->x + wrapper->x < -16384)
	{
	  if (parent_pos->x + wrapper->x + window->width < 16384)
	    info->x = parent_pos->x + wrapper->x + window->width - info->width - parent_pos->x11_x;
	  else
	    info->x = -16384 - parent_pos->x11_x;
	}
      else
	info->x = parent_pos->x + wrapper->x - parent_pos->x11_x;
    }

  if (window->height <= 32767)
    {
      info->height = window->height;
      info->y = parent_pos->y + wrapper->y - parent_pos->x11_y;
    }
  else
    {
      info->big = TRUE;
      info->height = 32767;
      if (parent_pos->y + wrapper->y < -16384)
	{
	  if (parent_pos->y + wrapper->y + window->height < 16384)
	    info->y = parent_pos->y + wrapper->y + window->height - info->height - parent_pos->x11_y;
	  else
	    info->y = -16384 - parent_pos->x11_y;
	}
      else
	info->y = parent_pos->y + wrapper->y - parent_pos->x11_y;
    }

  parent_x_offset = parent_pos->x11_x - parent_pos->x;
  parent_y_offset = parent_pos->x11_y - parent_pos->y;
  
  info->x_offset = parent_x_offset + info->x - wrapper->x;
  info->y_offset = parent_y_offset + info->y - wrapper->y;

  /* We don't considering the clipping of toplevel windows and their immediate children
   * by their parents, and simply always map those windows.
   */
  if (parent_pos->clip_rect.width == G_MAXINT)
    info->mapped = TRUE;
  /* Check if the window would wrap around into the visible space in either direction */
  else if (info->x + parent_x_offset < parent_pos->clip_rect.x + parent_pos->clip_rect.width - 65536 ||
      info->x + info->width + parent_x_offset > parent_pos->clip_rect.x + 65536 ||
      info->y + parent_y_offset < parent_pos->clip_rect.y + parent_pos->clip_rect.height - 65536 ||
      info->y + info->height + parent_y_offset  > parent_pos->clip_rect.y + 65536)
    info->mapped = FALSE;
  else
    info->mapped = TRUE;

  info->no_bg = FALSE;

  if (GDK_WINDOW_TYPE (wrapper) == GDK_WINDOW_CHILD)
    {
      info->clip_rect.x = wrapper->x;
      info->clip_rect.y = wrapper->y;
      info->clip_rect.width = window->width;
      info->clip_rect.height = window->height;
      
      gdk_rectangle_intersect (&info->clip_rect, &parent_pos->clip_rect, &info->clip_rect);

      info->clip_rect.x -= wrapper->x;
      info->clip_rect.y -= wrapper->y;
    }
  else
    {
      info->clip_rect.x = 0;
      info->clip_rect.y = 0;
      info->clip_rect.width = G_MAXINT;
      info->clip_rect.height = G_MAXINT;
    }
}

static void
gdk_window_compute_parent_pos (GdkWindowImplGIX      *window,
			       GdkWindowParentPos *parent_pos)
{
  GdkWindowObject *wrapper;
  GdkWindowObject *parent;
  GdkRectangle tmp_clip;
  
  int clip_xoffset = 0;
  int clip_yoffset = 0;

  g_return_if_fail (GDK_IS_WINDOW_IMPL_GIX (window));

  wrapper =
    GDK_WINDOW_OBJECT (GDK_DRAWABLE_IMPL_GIX (window)->wrapper);
  
  parent_pos->x = 0;
  parent_pos->y = 0;
  parent_pos->x11_x = 0;
  parent_pos->x11_y = 0;

  /* We take a simple approach here and simply consider toplevel
   * windows not to clip their children on the right/bottom, since the
   * size of toplevel windows is not directly under our
   * control. Clipping only really matters when scrolling and
   * generally we aren't going to be moving the immediate child of a
   * toplevel beyond the bounds of that toplevel.
   *
   * We could go ahead and recompute the clips of toplevel windows and
   * their descendents when we receive size notification, but it would
   * probably not be an improvement in most cases.
   */
  parent_pos->clip_rect.x = 0;
  parent_pos->clip_rect.y = 0;
  parent_pos->clip_rect.width = G_MAXINT;
  parent_pos->clip_rect.height = G_MAXINT;

  parent = (GdkWindowObject *)wrapper->parent;
  while (parent && parent->window_type == GDK_WINDOW_CHILD)
    {
      GdkWindowImplGIX *impl = GDK_WINDOW_IMPL_GIX (parent->impl);
      
      tmp_clip.x = - clip_xoffset;
      tmp_clip.y = - clip_yoffset;
      tmp_clip.width = impl->width;
      tmp_clip.height = impl->height;

      gdk_rectangle_intersect (&parent_pos->clip_rect, &tmp_clip, &parent_pos->clip_rect);

      translate_pos (parent_pos, parent_pos, parent,
                     &impl->position_info, FALSE);

      clip_xoffset += parent->x;
      clip_yoffset += parent->y;

      parent = (GdkWindowObject *)parent->parent;
    }
}


static void
tmp_unset_bg (GdkWindow *window)
{
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;

  obj = (GdkWindowObject *) window;
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);

  /* For windows without EXPOSURE_MASK, we can't do this
   * unsetting because such windows depend on the drawing
   * that the X server is going to do
   */
  if (!(obj->event_mask & GDK_EXPOSURE_MASK))
    return;
    
  impl->position_info.no_bg = TRUE;

  //if (obj->bg_pixmap != GDK_NO_BG)
   // XSetWindowBackgroundPixmap (GDK_DRAWABLE_XID (window), None);
}

void
_gdk_x11_window_tmp_unset_bg (GdkWindow *window,
			      gboolean   recurse)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));
  
  private = (GdkWindowObject *)window;

  if (private->input_only || private->destroyed ||
      (private->window_type != GDK_WINDOW_ROOT &&
       !GDK_WINDOW_IS_MAPPED (window)))
    {
      return;
    }

  if (private->window_type != GDK_WINDOW_ROOT &&
      private->window_type != GDK_WINDOW_FOREIGN)
    {
      tmp_unset_bg (window);
    }

  if (recurse)
    {
      GList *l;

      for (l = private->children; l != NULL; l = l->next)
	_gdk_x11_window_tmp_unset_bg (l->data, TRUE);
    }
}


static void
tmp_reset_bg (GdkWindow *window)
{
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;

  obj = (GdkWindowObject *) window;
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);

  if (!(obj->event_mask & GDK_EXPOSURE_MASK))
    return;
    
  impl->position_info.no_bg = FALSE;

  if (obj->bg_pixmap == GDK_NO_BG)
    return;
  
  if (obj->bg_pixmap)
    {
      gi_id_t xpixmap;

      if (obj->bg_pixmap == GDK_PARENT_RELATIVE_BG)
	xpixmap = 0;
      else 
	xpixmap = GDK_DRAWABLE_GIX_ID (obj->bg_pixmap);

      //XSetWindowBackgroundPixmap ( GDK_DRAWABLE_GIX_ID (window), xpixmap);
    }
  else
    {
      //XSetWindowBackground ( GDK_DRAWABLE_GIX_ID (window), obj->bg_color.pixel);
    }
}

void
_gdk_x11_window_tmp_reset_bg (GdkWindow *window,
			      gboolean   recurse)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = (GdkWindowObject *)window;

  if (private->input_only || private->destroyed ||
      (private->window_type != GDK_WINDOW_ROOT &&
       !GDK_WINDOW_IS_MAPPED (window)))
    {
      return;
    }

  if (private->window_type != GDK_WINDOW_ROOT &&
      private->window_type != GDK_WINDOW_FOREIGN)
    {
      tmp_reset_bg (window);
    }

  if (recurse)
    {
      GList *l;

      for (l = private->children; l != NULL; l = l->next)
	_gdk_x11_window_tmp_reset_bg (l->data, TRUE);
    }
}
static void
gdk_window_clip_changed (GdkWindow *window, GdkRectangle *old_clip, GdkRectangle *new_clip)
{
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;
  GdkRegion *old_clip_region;
  GdkRegion *new_clip_region;
  
  if (((GdkWindowObject *)window)->input_only)
    return;

  obj = (GdkWindowObject *) window;
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);
  
  old_clip_region = gdk_region_rectangle (old_clip);
  new_clip_region = gdk_region_rectangle (new_clip);

  /* We need to update this here because gdk_window_invalidate_region makes
   * use if it (through gdk_drawable_get_visible_region
   */
  impl->position_info.clip_rect = *new_clip;
  
  /* Trim invalid region of window to new clip rectangle
   */
  if (obj->update_area)
    gdk_region_intersect (obj->update_area, new_clip_region);

  /* Invalidate newly exposed portion of window
   */
  gdk_region_subtract (new_clip_region, old_clip_region);
  if (!gdk_region_empty (new_clip_region))
    {
      _gdk_x11_window_tmp_unset_bg (window, FALSE);;
      gdk_window_invalidate_region (window, new_clip_region, FALSE);
    }

  if (obj->parent)
    _gdk_x11_window_tmp_unset_bg ((GdkWindow *)obj->parent, FALSE);
  
  gdk_region_destroy (new_clip_region);
  gdk_region_destroy (old_clip_region);
}


static void
move_resize (GdkWindow *window, GdkRectangle *pos)
{
  gi_move_window (GDK_WINDOW_GIX_ID (window),pos->x, pos->y);
  gi_resize_window (GDK_WINDOW_GIX_ID (window), pos->width, pos->height);
}

static void
gdk_window_premove (GdkWindow          *window,
		    GdkWindowParentPos *parent_pos)
{
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;
  GdkXPositionInfo new_info;
  gint d_xoffset, d_yoffset;
  GdkWindowParentPos this_pos;

  obj = (GdkWindowObject *) window;
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);
  
  gdk_window_compute_position (impl, parent_pos, &new_info);

  gdk_window_clip_changed (window, &impl->position_info.clip_rect, &new_info.clip_rect);

  translate_pos (&this_pos, parent_pos, obj, &new_info, TRUE);

  unmap_if_needed (window, &new_info);

  d_xoffset = new_info.x_offset - impl->position_info.x_offset;
  d_yoffset = new_info.y_offset - impl->position_info.y_offset;
  
  if (d_xoffset != 0 || d_yoffset != 0)
    {
      GdkRectangle new_position;

      if (d_xoffset < 0 || d_yoffset < 0)
	gdk_window_queue_translation (window, NULL, MIN (d_xoffset, 0), MIN (d_yoffset, 0));

      compute_intermediate_position (&impl->position_info, &new_info, d_xoffset, d_yoffset,
				     &new_position);

      move_resize (window, &new_position);
    }

  g_list_foreach (obj->children, (GFunc) gdk_window_premove, &this_pos);
}

static void
gdk_window_postmove (GdkWindow          *window,
		     GdkWindowParentPos *parent_pos)
{
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;
  GdkXPositionInfo new_info;
  gint d_xoffset, d_yoffset;
  GdkWindowParentPos this_pos;

  obj = (GdkWindowObject *) window;
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);
  
  gdk_window_compute_position (impl, parent_pos, &new_info);

  translate_pos (&this_pos, parent_pos, obj, &new_info, TRUE);

  d_xoffset = new_info.x_offset - impl->position_info.x_offset;
  d_yoffset = new_info.y_offset - impl->position_info.y_offset;
  
  if (d_xoffset != 0 || d_yoffset != 0)
    {
      if (d_xoffset > 0 || d_yoffset > 0)
	gdk_window_queue_translation (window, NULL, MAX (d_xoffset, 0), MAX (d_yoffset, 0));
	
      move_resize (window, (GdkRectangle *) &new_info);
    }

  map_if_needed (window, &new_info);

  reset_backgrounds (window);

  impl->position_info = new_info;

  g_list_foreach (obj->children, (GFunc) gdk_window_postmove, &this_pos);
}

static void
queue_delete_link (GQueue *queue,
		   GList  *link)
{
  if (queue->tail == link)
    queue->tail = link->prev;
  
  queue->head = g_list_remove_link (queue->head, link);
  g_list_free_1 (link);
  queue->length--;
}

static void
queue_item_free (GdkWindowQueueItem *item)
{
  if (item->window)
    {
      g_object_remove_weak_pointer (G_OBJECT (item->window),
				    (gpointer *)&(item->window));
    }
  
  if (item->type == GDK_WINDOW_QUEUE_ANTIEXPOSE)
    gdk_region_destroy (item->u.antiexpose.area);
  else
    {
      if (item->u.translate.area)
	gdk_region_destroy (item->u.translate.area);
    }
  
  g_free (item);
}


static void
gdk_window_queue (GdkWindow          *window,
		  GdkWindowQueueItem *item)
{
  GdkDisplayGIX *display_x11 = GDK_DISPLAY_GIX (_gdk_display);
  
  if (!display_x11->translate_queue)
    display_x11->translate_queue = g_queue_new ();

  /* Keep length of queue finite by, if it grows too long,
   * figuring out the latest relevant serial and discarding
   * irrelevant queue items.
   */
  if (display_x11->translate_queue->length >= 64)
    {
      //gulong serial = find_current_serial ();
      GList *tmp_list = display_x11->translate_queue->head;
      
      while (tmp_list)
	{
	  GdkWindowQueueItem *item = tmp_list->data;
	  GList *next = tmp_list->next;
	  
	  /* an overflow-safe (item->serial < serial) */
	  //if (item->serial - serial > (gulong) G_MAXLONG)
	    if(0){
	      queue_delete_link (display_x11->translate_queue, tmp_list);
	      queue_item_free (item);
	    }

	  tmp_list = next;
	}
    }

  /* Catch the case where someone isn't processing events and there
   * is an event stuck in the event queue with an old serial:
   * If we can't reduce the queue length by the above method,
   * discard anti-expose items. (We can't discard translate
   * items 
   */
  if (display_x11->translate_queue->length >= 64)
    {
      GList *tmp_list = display_x11->translate_queue->head;
      
      while (tmp_list)
	{
	  GdkWindowQueueItem *item = tmp_list->data;
	  GList *next = tmp_list->next;
	  
	  if (item->type == GDK_WINDOW_QUEUE_ANTIEXPOSE)
	    {
	      queue_delete_link (display_x11->translate_queue, tmp_list);
	      queue_item_free (item);
	    }

	  tmp_list = next;
	}
    }

  item->window = window;
  //item->serial = NextRequest (GDK_WINDOW_XDISPLAY (window));
  
  g_object_add_weak_pointer (G_OBJECT (window),
			     (gpointer *)&(item->window));

  g_queue_push_tail (display_x11->translate_queue, item);
}


gboolean
_gdk_windowing_window_queue_antiexpose (GdkWindow *window,
					GdkRegion *area)
{
  GdkWindowQueueItem *item = g_new (GdkWindowQueueItem, 1);
  item->type = GDK_WINDOW_QUEUE_ANTIEXPOSE;
  item->u.antiexpose.area = area;

  gdk_window_queue (window, item);

  return TRUE;
}

void
gdk_window_queue_translation (GdkWindow *window,
			      GdkRegion *area,
			      gint       dx,
			      gint       dy)
{
  GdkWindowQueueItem *item = g_new (GdkWindowQueueItem, 1);
  item->type = GDK_WINDOW_QUEUE_TRANSLATE;
  item->u.translate.area = area ? gdk_region_copy (area) : NULL;
  item->u.translate.dx = dx;
  item->u.translate.dy = dy;

  gdk_window_queue (window, item);
}



void
gdk_window_copy_area_scroll (GdkWindow    *window,
			     GdkRectangle *dest_rect,
			     gint          dx,
			     gint          dy)
{
  GdkWindowObject *obj = GDK_WINDOW_OBJECT (window);
  GList *l;

  if (dest_rect->width > 0 && dest_rect->height > 0)
    {
      GdkGC *gc;

      gc = _gdk_drawable_get_scratch_gc (window, TRUE);
      
      gdk_window_queue_translation (window, NULL, dx, dy);
   
      gi_copy_area (
		 GDK_WINDOW_GIX_ID (window),
		 GDK_WINDOW_GIX_ID (window),
		 gdk_x11_gc_get_xgc (gc),
		 dest_rect->x - dx, dest_rect->y - dy,
		 dest_rect->width, dest_rect->height,
		 dest_rect->x, dest_rect->y);
    }

  for (l = obj->children; l; l = l->next)
    {
      GdkWindow *child = (GdkWindow*) l->data;
      GdkWindowObject *child_obj = GDK_WINDOW_OBJECT (child);

      gdk_window_move (child, child_obj->x + dx, child_obj->y + dy);
    }
}


void
_gdk_window_process_expose (GdkWindow    *window,
			    gulong        serial,
			    GdkRectangle *area)
{
  GdkWindowImplGIX *impl;
  GdkRegion *invalidate_region = gdk_region_rectangle (area);
  GdkRegion *clip_region;
  GdkDisplayGIX *display_x11 = GDK_DISPLAY_GIX (_gdk_display);
  impl = GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);

  if (display_x11->translate_queue)
    {
      GList *tmp_list = display_x11->translate_queue->head;
      
      while (tmp_list)
	{
	  GdkWindowQueueItem *item = tmp_list->data;
          GList *next = tmp_list->next;
	  
	  /* an overflow-safe (serial < item->serial) */
	  if (0)//serial - item->serial > (gulong) G_MAXLONG)
	    {
	      if (item->window == window)
		{
		  if (item->type == GDK_WINDOW_QUEUE_TRANSLATE)
		    {
		      if (item->u.translate.area)
			{
			  GdkRegion *intersection;
			  
			  intersection = gdk_region_copy (invalidate_region);
			  gdk_region_intersect (intersection, item->u.translate.area);
			  gdk_region_subtract (invalidate_region, intersection);
			  gdk_region_offset (intersection, item->u.translate.dx, item->u.translate.dy);
			  gdk_region_union (invalidate_region, intersection);
			  gdk_region_destroy (intersection);
			}
		      else
			gdk_region_offset (invalidate_region, item->u.translate.dx, item->u.translate.dy);
		    }
		  else		/* anti-expose */
		    {
		      gdk_region_subtract (invalidate_region, item->u.antiexpose.area);
		    }
		}
	    }
	  else
	    {
	      queue_delete_link (display_x11->translate_queue, tmp_list);
	      queue_item_free (item);
	    }
	  tmp_list = next;
	}
    }

	GdkRectangle clip_rect;

	clip_rect.x = 0;
	clip_rect.y = 0;
	clip_rect.width = impl->drawable.width;
	clip_rect.height = impl->drawable.height;

	//clip_rect = impl->position_info.clip_rect; //FIXME

  clip_region = gdk_region_rectangle (&clip_rect);
  gdk_region_intersect (invalidate_region, clip_region);

  if (!gdk_region_empty (invalidate_region))
    gdk_window_invalidate_region (window, invalidate_region, FALSE);
  
  gdk_region_destroy (invalidate_region);
  gdk_region_destroy (clip_region);
}



void
_gdk_gix_window_get_offsets (GdkWindow *window,
                                  gint      *x_offset,
                                  gint      *y_offset)
{
  if (x_offset)
    *x_offset = 0;
  if (y_offset)
    *y_offset = 0;
}



/**
 * gdk_window_scroll:
 * @window: a #GdkWindow
 * @dx: Amount to scroll in the X direction
 * @dy: Amount to scroll in the Y direction
 *
 * Scroll the contents of its window, both pixels and children, by
 * the given amount. Portions of the window that the scroll operation
 * brings in from offscreen areas are invalidated.
 **/

 #if 1

void
_gdk_gix_window_scroll (GdkWindow *window,
                        gint       dx,
                        gint       dy)
{
  gboolean can_guffaw_scroll = FALSE;
  GdkRegion *invalidate_region;
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;
  GdkRectangle src_rect, dest_rect;
  
  obj = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);  

  //GdkRectangle  clip_rect = {  0,  0, impl->width, impl->height };

  /* Move the current invalid region */
  if (obj->update_area)
    gdk_region_offset (obj->update_area, dx, dy);
  
  /* impl->position_info.clip_rect isn't meaningful for toplevels */
  if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD)
    src_rect = impl->position_info.clip_rect;
  else
    {
      src_rect.x = 0;
      src_rect.y = 0;
      src_rect.width = impl->width;
      src_rect.height = impl->height;
    }
  
  invalidate_region = gdk_region_rectangle (&src_rect);

  dest_rect = src_rect;
  dest_rect.x += dx;
  dest_rect.y += dy;
  gdk_rectangle_intersect (&dest_rect, &src_rect, &dest_rect);

  if (dest_rect.width > 0 && dest_rect.height > 0)
    {
      GdkRegion *tmp_region;

      tmp_region = gdk_region_rectangle (&dest_rect);
      gdk_region_subtract (invalidate_region, tmp_region);
      gdk_region_destroy (tmp_region);
    }
  
  gdk_window_invalidate_region (window, invalidate_region, TRUE);
  gdk_region_destroy (invalidate_region);

  /* We can guffaw scroll if we are a child window, and the parent
   * does not extend beyond our edges. Otherwise, we use XCopyArea, then
   * move any children later
   */
  if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD)
    {
      GdkWindowImplGIX *parent_impl = GDK_WINDOW_IMPL_GIX (obj->parent->impl);  
      can_guffaw_scroll = ((dx == 0 || (obj->x <= 0 && obj->x + impl->width >= parent_impl->width)) &&
			   (dy == 0 || (obj->y <= 0 && obj->y + impl->height >= parent_impl->height)));
    }

  if (!obj->children || !can_guffaw_scroll)
    gdk_window_copy_area_scroll (window, &dest_rect, dx, dy);
  else
    gdk_window_guffaw_scroll (window, dx, dy);
}

#else
void
_gdk_gix_window_scroll (GdkWindow *window,
                             gint       dx,
                             gint       dy)
{
  GdkWindowObject         *private;
  GdkDrawableImplGix *impl;
  GdkRegion               *invalidate_region = NULL;
  GList                   *list;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_DRAWABLE_IMPL_GIX (private->impl);

  if (dx == 0 && dy == 0)
    return;

  /* Move the current invalid region */
  if (private->update_area)
    gdk_region_offset (private->update_area, dx, dy);

  if (GDK_WINDOW_IS_MAPPED (window))
    {
      GdkRectangle  clip_rect = {  0,  0, impl->width, impl->height };
      GdkRectangle  rect      = { dx, dy, impl->width, impl->height };

      invalidate_region = gdk_region_rectangle (&clip_rect);

      if (gdk_rectangle_intersect (&rect, &clip_rect, &rect) &&
          (!private->update_area ||
           !gdk_region_rect_in (private->update_area, &rect)))
        {
          GdkRegion *region;

          region = gdk_region_rectangle (&rect);
          gdk_region_subtract (invalidate_region, region);
          gdk_region_destroy (region);

         // if (impl->gc)  //FIXME DPP
            {/* dpp
              GIXRegion update = { rect.x, rect.y,
                                   rect.x + rect.width  - 1,
                                   rect.y + rect.height - 1 };

              impl->surface->SetClip (impl->surface, &update);
              impl->surface->Blit (impl->surface, impl->surface, NULL, dx, dy);
              impl->surface->SetClip (impl->surface, NULL);
              impl->surface->Flip(impl->surface,&update,0);
*/
            }
        }
    }

  for (list = private->children; list; list = list->next)
    {
      GdkWindowObject         *obj      = GDK_WINDOW_OBJECT (list->data);
      GdkDrawableImplGix *obj_impl = GDK_DRAWABLE_IMPL_GIX (obj->impl);

      _gdk_gix_move_resize_child (list->data,
                                       obj->x + dx,
                                       obj->y + dy,
                                       obj_impl->width,
                                       obj_impl->height);
    }

  _gdk_gix_calc_abs (window);

  if (invalidate_region)
    {
      gdk_window_invalidate_region (window, invalidate_region, TRUE);
      gdk_region_destroy (invalidate_region);
    }
}
#endif


/**
 * gdk_window_move_region:
 * @window: a #GdkWindow
 * @region: The #GdkRegion to move
 * @dx: Amount to move in the X direction
 * @dy: Amount to move in the Y direction
 * 
 * Move the part of @window indicated by @region by @dy pixels in the Y 
 * direction and @dx pixels in the X direction. The portions of @region 
 * that not covered by the new position of @region are invalidated.
 * 
 * Child windows are not moved.
 * 
 * Since: 2.8
 **/



void
_gdk_gix_window_move_region (GdkWindow       *window,
                             const GdkRegion *region,
                             gint             dx,
                             gint             dy)
{
  GdkWindowImplGIX *impl;
  GdkWindowObject *private;
  GdkRegion *window_clip;
  GdkRegion *src_region;
  GdkRegion *brought_in;
  GdkRegion *dest_region;
  GdkRegion *moving_invalid_region;
  GdkRectangle dest_extents;
  GdkGC *gc;
  
  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);  

  window_clip = gdk_region_rectangle (&impl->position_info.clip_rect);

  /* compute source regions */
  src_region = gdk_region_copy (region);
  brought_in = gdk_region_copy (region);
  gdk_region_intersect (src_region, window_clip);

  gdk_region_subtract (brought_in, src_region);
  gdk_region_offset (brought_in, dx, dy);

  /* compute destination regions */
  dest_region = gdk_region_copy (src_region);
  gdk_region_offset (dest_region, dx, dy);
  gdk_region_intersect (dest_region, window_clip);
  gdk_region_get_clipbox (dest_region, &dest_extents);

  gdk_region_destroy (window_clip);

  /* calculating moving part of current invalid area */
  moving_invalid_region = NULL;
  if (private->update_area)
    {
      moving_invalid_region = gdk_region_copy (private->update_area);
      gdk_region_intersect (moving_invalid_region, src_region);
      gdk_region_offset (moving_invalid_region, dx, dy);
    }
  
  /* invalidate all of the src region */
  gdk_window_invalidate_region (window, src_region, FALSE);

  /* un-invalidate destination region */
  if (private->update_area)
    gdk_region_subtract (private->update_area, dest_region);
  
  /* invalidate moving parts of existing update area */
  if (moving_invalid_region)
    {
      gdk_window_invalidate_region (window, moving_invalid_region, FALSE);
      gdk_region_destroy (moving_invalid_region);
    }

  /* invalidate area brought in from off-screen */
  gdk_window_invalidate_region (window, brought_in, FALSE);
  gdk_region_destroy (brought_in);

  /* Actually do the moving */
  gdk_window_queue_translation (window, src_region, dx, dy);

  gc = _gdk_drawable_get_scratch_gc (window, TRUE);
  gdk_gc_set_clip_region (gc, dest_region);
  
  gi_copy_area (
	     GDK_WINDOW_GIX_ID (window),
	     GDK_WINDOW_GIX_ID (window),
	     GDK_GC_XGC (gc),
	     dest_extents.x - dx, dest_extents.y - dy,
	     dest_extents.width, dest_extents.height,
	     dest_extents.x, dest_extents.y);

  /* Unset clip region of cached GC */
  gdk_gc_set_clip_region (gc, NULL);
  
  gdk_region_destroy (src_region);
  gdk_region_destroy (dest_region);
}




static void
move_relative (GdkWindow *window, GdkRectangle *rect,
               gint dx, gint dy)
{
  gi_move_window (GDK_WINDOW_GIX_ID (window),
               rect->x + dx, rect->y + dy);
}

static void
gdk_window_guffaw_scroll (GdkWindow    *window,
			  gint          dx,
			  gint          dy)
{
  GdkWindowObject *obj = GDK_WINDOW_OBJECT (window);
  GdkWindowImplGIX *impl = GDK_WINDOW_IMPL_GIX (obj->impl);

  gint d_xoffset = -dx;
  gint d_yoffset = -dy;
  GdkRectangle new_position;
  GdkXPositionInfo new_info;
  GdkWindowParentPos parent_pos;
  GList *l;
  
  gdk_window_compute_parent_pos (impl, &parent_pos);
  gdk_window_compute_position (impl, &parent_pos, &new_info);

  translate_pos (&parent_pos, &parent_pos, obj, &new_info, TRUE);

  _gdk_x11_window_tmp_unset_bg (window, FALSE);;

  if (dx > 0 || dy > 0)
    gdk_window_queue_translation (window, NULL, MAX (dx, 0), MAX (dy, 0));
	
  gdk_window_set_static_gravities (window, TRUE);

  compute_intermediate_position (&impl->position_info, &new_info, d_xoffset, d_yoffset,
				 &new_position);
  
  move_resize (window, &new_position);
  
  for (l = obj->children; l; l = l->next)
    {
      GdkWindow *child = (GdkWindow*) l->data;
      GdkWindowObject *child_obj = GDK_WINDOW_OBJECT (child);

      child_obj->x -= d_xoffset;
      child_obj->y -= d_yoffset;

      gdk_window_premove (child, &parent_pos);
    }
  
  move_relative (window, &new_position, -d_xoffset, -d_yoffset);
  
  if (dx < 0 || dy < 0)
    gdk_window_queue_translation (window, NULL, MIN (dx, 0), MIN (dy, 0));
  
  move_resize (window, (GdkRectangle *) &impl->position_info);
  
  if (impl->position_info.no_bg)
    _gdk_x11_window_tmp_reset_bg (window, FALSE);
  
  impl->position_info = new_info;
  
  g_list_foreach (obj->children, (GFunc) gdk_window_postmove, &parent_pos);
}

void
_gdk_x11_window_scroll (GdkWindow *window,
                        gint       dx,
                        gint       dy)
{
  gboolean can_guffaw_scroll = FALSE;
  GdkRegion *invalidate_region;
  GdkWindowImplGIX *impl;
  GdkWindowObject *obj;
  GdkRectangle src_rect, dest_rect;
  
  obj = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (obj->impl);  

  /* Move the current invalid region */
  if (obj->update_area)
    gdk_region_offset (obj->update_area, dx, dy);
  
  /* impl->position_info.clip_rect isn't meaningful for toplevels */
  if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD)
    src_rect = impl->position_info.clip_rect;
  else
    {
      src_rect.x = 0;
      src_rect.y = 0;
      src_rect.width = impl->width;
      src_rect.height = impl->height;
    }
  
  invalidate_region = gdk_region_rectangle (&src_rect);

  dest_rect = src_rect;
  dest_rect.x += dx;
  dest_rect.y += dy;
  gdk_rectangle_intersect (&dest_rect, &src_rect, &dest_rect);

  if (dest_rect.width > 0 && dest_rect.height > 0)
    {
      GdkRegion *tmp_region;

      tmp_region = gdk_region_rectangle (&dest_rect);
      gdk_region_subtract (invalidate_region, tmp_region);
      gdk_region_destroy (tmp_region);
    }
  
  gdk_window_invalidate_region (window, invalidate_region, TRUE);
  gdk_region_destroy (invalidate_region);

  /* We can guffaw scroll if we are a child window, and the parent
   * does not extend beyond our edges. Otherwise, we use XCopyArea, then
   * move any children later
   */
  if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD)
    {
      GdkWindowImplGIX *parent_impl = GDK_WINDOW_IMPL_GIX (obj->parent->impl);  
      can_guffaw_scroll = ((dx == 0 || (obj->x <= 0 && obj->x + impl->width >= parent_impl->width)) &&
			   (dy == 0 || (obj->y <= 0 && obj->y + impl->height >= parent_impl->height)));
    }

  if (!obj->children || !can_guffaw_scroll)
    gdk_window_copy_area_scroll (window, &dest_rect, dx, dy);
  else
    gdk_window_guffaw_scroll (window, dx, dy);
}


#define __GDK_GEOMETRY_X11_C__
#include "gdkaliasdef.c"
