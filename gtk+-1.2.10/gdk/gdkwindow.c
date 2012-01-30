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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#include <netinet/in.h>
#include "gdk.h"
#include "config.h"
#include "gdkinput.h"
#include "gdkprivate.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


const int nano_event_mask_table[20]=
{
GI_MASK_EXPOSURE,
GI_MASK_MOUSE_MOVE      ,
0,
GI_MASK_MOUSE_MOVE      ,
GI_MASK_MOUSE_MOVE      ,
GI_MASK_MOUSE_MOVE      ,
GI_MASK_MOUSE_MOVE      ,
GI_MASK_BUTTON_DOWN       ,
GI_MASK_BUTTON_UP         ,
GI_MASK_KEY_DOWN          ,
GI_MASK_KEY_UP            ,
GI_MASK_MOUSE_ENTER       ,
GI_MASK_MOUSE_EXIT        ,
GI_MASK_FOCUS_OUT|GI_MASK_FOCUS_IN,
0,
0,
0,
0,
0,
0
};

const int gdk_nevent_masks = sizeof (nano_event_mask_table) / sizeof (int);
extern gi_screen_info_t gdk_screen_info;

/* Forward declarations */
#if NOT_IMPLEMENTED
static gboolean gdk_window_gravity_works (void);
static gboolean gdk_window_have_shape_ext (void);
#endif
static void     gdk_window_set_static_win_gravity (GdkWindow *window, 
						   gboolean   on);

/* internal function created for and used by gdk_window_xid_at_coords */
gi_window_id_t
gdk_window_xid_at (gi_window_id_t base,
		   gint     bx,
		   gint     by,
		   gint     x,
		   gint     y, 
		   GList   *excludes,
		   gboolean excl_child)
{
	g_message("Enter gdk_window_xid_at\n");
	return 0;
}

/* 
 * The following fucntion by The Rasterman <raster@redhat.com>
 * This function returns the X Window ID in which the x y location is in 
 * (x and y being relative to the root window), excluding any windows listed
 * in the GList excludes (this is a list of X Window ID's - gpointer being
 * the Window ID).
 * 
 * This is primarily designed for internal gdk use - for DND for example
 * when using a shaped icon window as the drag object - you exclude the
 * X Window ID of the "icon" (perhaps more if excludes may be needed) and
 * You can get back an X Window ID as to what X Window ID is infact under
 * those X,Y co-ordinates.
 */
gi_window_id_t
gdk_window_xid_at_coords (gint     x,
			  gint     y,
			  GList   *excludes,
			  gboolean excl_child)
{
	g_message("Enter gdk_window_xid_at_coords\n");
	return 0;
}

void
gdk_window_init (void)
{
	extern   gi_screen_info_t gdk_screen_info;


	gdk_root_parent.xwindow = gdk_root_window;
	gdk_root_parent.window_type = GDK_WINDOW_ROOT;
	gdk_root_parent.window.user_data = NULL;
	gdk_root_parent.width = gdk_screen_info.scr_width;
	gdk_root_parent.height = gdk_screen_info.scr_height;
	gdk_root_parent.children = NULL;
	gdk_root_parent.colormap = NULL;
	gdk_root_parent.ref_count = 1;

	gdk_xid_table_insert (&gdk_root_window, &gdk_root_parent);
}

#if NOT_IMPLEMENTED
static GdkAtom wm_client_leader_atom = GDK_NONE;
#endif

GdkWindow*
gdk_window_new (GdkWindow     *parent,
		GdkWindowAttr *attributes,
		gint           attributes_mask)
{
	GdkWindow *window;
	GdkWindowPrivate *private;
	GdkWindowPrivate *parent_private;
	gi_window_id_t xparent;
	long nxattributes;
	long nxattributes_mask;
	unsigned long  eventmask;  
	int x, y;
	int i;
	char * title;
	//GR_WM_PROPERTIES props;

	g_return_val_if_fail (attributes != NULL, NULL);

	if (!parent)
		parent = (GdkWindow*) &gdk_root_parent;

	parent_private = (GdkWindowPrivate*) parent;
	if (parent_private->destroyed)
		return NULL;

	xparent = parent_private->xwindow;

	private = g_new (GdkWindowPrivate, 1);
	window = (GdkWindow*) private;

	private->parent = parent;

	private->destroyed = FALSE;
	private->mapped = FALSE;
	private->guffaw_gravity = FALSE;
	private->resize_count = 0;
	private->ref_count = 1;
	nxattributes_mask = 0;
	eventmask=0; //| GI_MASK_CHLD_UPDATE;  

	if (attributes_mask & GDK_WA_ISENTRY){
		printf("nxattributes_mask |= WMIsEntry\n");
	}

	if (attributes_mask & GDK_WA_X)
	x = attributes->x;
	else
	x = 0;

	if (attributes_mask & GDK_WA_Y)
	y = attributes->y;
	else
	y = 0;

	private->x = x;
	private->y = y;
	private->width = (attributes->width > 1) ? (attributes->width) : (1);
	private->height = (attributes->height > 1) ? (attributes->height) : (1);
	private->window_type = attributes->window_type;
	private->extension_events = FALSE;

	private->filters = NULL;
	private->children = NULL;

	window->user_data = NULL;

	for (i = 0; i < gdk_nevent_masks; i++)
	{
		if (attributes->event_mask & (1 << (i + 1)))
		{
			eventmask |= nano_event_mask_table[i];
		}

	}


	if (attributes_mask & GDK_WA_NOREDIR)
	{
		printf("GDK_WA_NOREDIR not support\n");
		//nxattributes.override_redirect =
		//		(attributes->override_redirect == FALSE)?False:True;
		//nxattributes_mask |= WMOverrideRedirect;
	} 
	else{
		//nxattributes.override_redirect = False;
	}

	if (attributes->wclass == GDK_INPUT_OUTPUT)
	{
		if (attributes_mask & GDK_WA_COLORMAP)
			private->colormap = attributes->colormap;
		else
			private->colormap = gdk_colormap_get_system ();

		//nxattributes.background_pixel = BLACK;
		//nxattributes.border_pixel = BLACK;
		//nxattributes_mask |= WMBorderPixel | WMBackPixel;

		switch (private->window_type)
		{
			case GDK_WINDOW_TOPLEVEL:
			/* FRANKIE
				nxattributes.override_redirect = True;
				nxattributes_mask |= WMOverrideRedirect;
			*/
				eventmask |= GI_MASK_CLIENT_MSG;
				xparent = gdk_root_window;
			break;

			case GDK_WINDOW_CHILD:
				nxattributes |= GI_FLAGS_NON_FRAME;
			break;

			case GDK_WINDOW_DIALOG:
				xparent = gdk_root_window;
				eventmask |= GI_MASK_CLIENT_MSG;
				nxattributes |= (GI_FLAGS_TEMP_WINDOW | GI_FLAGS_TOPMOST_WINDOW|GI_FLAGS_NOMOVE|GI_FLAGS_NORESIZE);
			break;

			case GDK_WINDOW_TEMP:
				nxattributes |= GI_FLAGS_NON_FRAME;
				xparent = gdk_root_window;
			nxattributes |= (GI_FLAGS_TEMP_WINDOW | GI_FLAGS_TOPMOST_WINDOW);

				//nxattributes.save_under = True;
				//nxattributes.override_redirect = True;
				//nxattributes_mask |= WMSaveUnder | WMOverrideRedirect;
			break;
			case GDK_WINDOW_ROOT:
				g_error ("cannot make windows of type GDK_WINDOW_ROOT");
			break;
			case GDK_WINDOW_PIXMAP:
				g_error ("cannot make windows of type GDK_WINDOW_PIXMAP (use gdk_pixmap_new)");
			break;
		}

	}
	else
	{
		private->colormap = NULL;
	}

	if (eventmask)
	{
		//nxattributes.eventmask = eventmask;
		//nxattributes_mask |= WMEventMask;
	}

	// FIXME : BorderSize = 1, BgdColor = White, BorderColor = Black
	if(attributes->wclass == GDK_INPUT_ONLY)
	{
		g_error ("cannot make windows of type GDK_INPUT_ONLY");
		private->xwindow = gi_create_window(xparent,x,y,private->width,private->height,GI_RGB(10,20,20),0);
		gi_set_events_mask(private->xwindow,eventmask);
	}
	else
		private->xwindow = gi_create_window(xparent,x,y,private->width,private->height,GI_RGB(200,200,200),nxattributes);

	/* We don't have any destroy notification from nano-X, so we do not re-ref */
	//gdk_window_ref (window);
	/* .... */
	gdk_xid_table_insert (&(private->xwindow), window);

	if (private->colormap)
		gdk_colormap_ref (private->colormap);

	gdk_window_set_cursor (window, ((attributes_mask & GDK_WA_CURSOR) ?
						(attributes->cursor) :
						NULL));

	if (parent_private)
		parent_private->children = g_list_prepend (parent_private->children, window);

	//props.flags = 0;
	switch (private->window_type)
	{
		case GDK_WINDOW_DIALOG:
			//props.flags |= GR_WM_FLAGS_PROPS;
			//props.props = GR_WM_PROPS_NOAUTOMOVE | GR_WM_PROPS_NOAUTORESIZE;
		case GDK_WINDOW_TOPLEVEL:

			if (attributes_mask & GDK_WA_TITLE)
				title = attributes->title;
			else
				title = g_get_prgname ();

			//props.flags |= GR_WM_FLAGS_TITLE;
			//props.title = title;
			//GrSetWMProperties(GDK_DRAWABLE_XID(window), &props);
			gi_set_window_caption(GDK_DRAWABLE_XID(window), title);
	}

	return window;
}

GdkWindow *
gdk_window_foreign_new (guint32 anid)
{
  GdkWindow *window;
  GdkWindowPrivate *private;
  GdkWindowPrivate *parent_private;
  gi_window_info_t winfo;

        g_message("gdk_window_foreign_new");
  gi_get_window_info(anid, &winfo);
  if(winfo.windowid ==0) return NULL;

  private = g_new (GdkWindowPrivate, 1);
  window = (GdkWindow*) private;
  private->parent = gdk_xid_table_lookup (winfo.parent);

  parent_private = (GdkWindowPrivate *)private->parent;
  if (parent_private)
    parent_private->children = g_list_prepend (parent_private->children, window);

  private->xwindow = anid;
  private->x = winfo.x;
  private->y = winfo.y;
  private->width = winfo.width;
  private->height = winfo.height;
  private->resize_count = 0;
  private->ref_count = 1;
  private->window_type = GDK_WINDOW_FOREIGN;
  private->destroyed = FALSE;
  private->mapped =winfo.mapped;
  private->guffaw_gravity = FALSE;
  private->extension_events = 0;

  private->colormap = NULL;

  private->filters = NULL;
  private->children = NULL;

  window->user_data = NULL;

  gdk_window_ref (window);
  gdk_xid_table_insert (&private->xwindow, window);

  return window;
}

/* Call this function when you want a window and all its children to
 * disappear.  When xdestroy is true, a request to destroy the XWindow
 * is sent out.  When it is false, it is assumed that the XWindow has
 * been or will be destroyed by destroying some ancestor of this
 * window.
 */
static void
gdk_window_internal_destroy (GdkWindow *window,
			     gboolean   xdestroy,
			     gboolean   our_destroy)
{
GdkWindowPrivate *private;
  GdkWindowPrivate *temp_private;
  GdkWindow *temp_window;
  GList *children;
  GList *tmp;

  g_return_if_fail (window != NULL);

  private = (GdkWindowPrivate*) window;

  switch (private->window_type)
    {
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_CHILD:
    case GDK_WINDOW_DIALOG:
    case GDK_WINDOW_TEMP:
    case GDK_WINDOW_FOREIGN:
      if (!private->destroyed)
        {
          if (private->parent)
            {
              GdkWindowPrivate *parent_private = (GdkWindowPrivate
*)private->parent;
              if (parent_private->children)
                parent_private->children = g_list_remove
(parent_private->children, window);
            }

          if (private->window_type != GDK_WINDOW_FOREIGN)
            {
              children = tmp = private->children;
              private->children = NULL;

              while (tmp)
                {
                  temp_window = tmp->data;
                  tmp = tmp->next;

                  temp_private = (GdkWindowPrivate*) temp_window;
                  if (temp_private)
                    gdk_window_internal_destroy (temp_window, FALSE,
                                                 our_destroy);
                }

              g_list_free (children);
	            }
          if (private->extension_events != 0)
            gdk_input_window_destroy (window);

          if (private->filters)
            {
              tmp = private->filters;

              while (tmp)
                {
                  g_free (tmp->data);
                  tmp = tmp->next;
                }

              g_list_free (private->filters);
              private->filters = NULL;
            }

          if (private->window_type == GDK_WINDOW_FOREIGN)
            {
                fprintf(stderr,"Destroy foreign window notimplemented\n");
            }
          else if (xdestroy)
            gi_destroy_window (private->xwindow);
          if (private->colormap)
            gdk_colormap_unref (private->colormap);

          private->mapped = FALSE;
          private->destroyed = TRUE;
          gdk_xid_table_remove (private->xwindow);
        }
      break;

    case GDK_WINDOW_ROOT:
      g_error ("attempted to destroy root window");
      break;

    case GDK_WINDOW_PIXMAP:
      g_error ("called gdk_window_destroy on a pixmap (usegdk_pixmap_unref)");
      break;
    }
}

/* Like internal_destroy, but also destroys the reference created by
   gdk_window_new. */

void
gdk_window_destroy (GdkWindow *window)
{
	gdk_window_internal_destroy (window, TRUE, TRUE);
        gdk_window_unref (window);
}

/* This function is called when the XWindow is really gone.  */

void
gdk_window_destroy_notify (GdkWindow *window)
{
	g_message("Enter gdk_window_destroy_notify\n");
}

GdkWindow*
gdk_window_ref (GdkWindow *window)
{
  GdkWindowPrivate *private = (GdkWindowPrivate *)window;
  g_return_val_if_fail (window != NULL, NULL);
  
  private->ref_count += 1;
  return window;
}

void
gdk_window_unref (GdkWindow *window)
{
  GdkWindowPrivate *private = (GdkWindowPrivate *)window;
  g_return_if_fail (window != NULL);
  g_return_if_fail (private->ref_count > 0);
  
  private->ref_count -= 1;
  if (private->ref_count == 0)
    {
      if (!private->destroyed)
	{
	  if (private->window_type == GDK_WINDOW_FOREIGN)
	    gdk_xid_table_remove (private->xwindow);
	  else
	    g_warning ("losing last reference to undestroyed window\n");
	}
      g_dataset_destroy (window);
      g_free (window);
    }
}

void
gdk_window_show (GdkWindow *window)
{
  GdkWindowPrivate *private;
  
  g_return_if_fail (window != NULL);
  
  private = (GdkWindowPrivate*) window;
  if (!private->destroyed)
    {
      private->mapped = TRUE;
      gi_raise_window(private->xwindow);
      gi_show_window(private->xwindow);
    }
}

void
gdk_window_hide (GdkWindow *window)
{
  GdkWindowPrivate *private;

  g_return_if_fail (window != NULL);

  private = (GdkWindowPrivate*) window;
  if (!private->destroyed)
    {
      private->mapped = FALSE;
        gi_hide_window(private->xwindow);
    }
}

void
gdk_window_withdraw (GdkWindow *window)
{
       g_message("gdk_window_withdraw");
  /* FIXME : */
  if(!GDK_DRAWABLE_DESTROYED(window))
        gi_hide_window(GDK_DRAWABLE_XID(window));
}

void
gdk_window_move (GdkWindow *window,
		 gint       x,
		 gint       y)
{
 GdkWindowPrivate *private;

  g_return_if_fail (window != NULL);

  private = (GdkWindowPrivate*) window;
  if (!private->destroyed  &&( (private->x!=x)||(private->y!=y)))
    {
      gi_move_window(private->xwindow, x , y);

      if (private->window_type == GDK_WINDOW_CHILD)
        {
          private->x = x;
          private->y = y;
        }
    }
}

void
gdk_window_clear_intersect_children(GdkWindow *window, GdkRectangle *rect)
{
        g_return_if_fail(window != NULL);

        if(GDK_DRAWABLE_DESTROYED(window))
            return;
#if 0
        GrClearChildArea(GDK_DRAWABLE_XID(window),
                        rect->x, rect->y, rect->width, rect->height);
#endif
        gdk_flush();
}

void
gdk_window_offset_children (GdkWindow *window,
                 gint       offx,
                 gint       offy)
{
 GdkWindowPrivate *private;
 GList* tmp_list;
 gint real_x;
 gint real_y;

  g_return_if_fail (window != NULL);

  if(GDK_DRAWABLE_DESTROYED(window))
        return;

  private = (GdkWindowPrivate*) window;
  tmp_list= private->children;

        while (tmp_list)
        {
                GdkWindowPrivate *child_private = tmp_list->data;

              real_x = child_private->x +offx;
              real_y = child_private->y +offy;
                if (!child_private->destroyed  &&
			((child_private->x!=real_x)||
			(child_private->y!=real_y)))
                {
                      gi_move_window(child_private->xwindow, real_x , real_y);

                      if (child_private->window_type == GDK_WINDOW_CHILD)
                        {
                          child_private->x = real_x;
                          child_private->y = real_y;
                       }
                }

              tmp_list = tmp_list->next;
            }
}

void
gdk_window_resize (GdkWindow *window,
		   gint       width,
		   gint       height)
{
GdkWindowPrivate *private;

  g_return_if_fail (window != NULL);

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  private = (GdkWindowPrivate*) window;

  if (!private->destroyed &&
      ((private->resize_count > 0) ||
       (private->width != (guint16) width) ||
       (private->height != (guint16) height)))
    {
      gi_resize_window (private->xwindow, width, height);
      private->resize_count += 1;

      if (private->window_type == GDK_WINDOW_CHILD)
        {
          private->width = width;
          private->height = height;
        }
    }
}

void
gdk_window_move_resize (GdkWindow *window,
			gint       x,
			gint       y,
			gint       width,
			gint       height)
{
GdkWindowPrivate *private;

  g_return_if_fail (window != NULL);

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  private = (GdkWindowPrivate*) window;
  if (!private->destroyed)
    {
#if 1
        gi_move_window(GDK_DRAWABLE_XID(window), x, y);
        gi_resize_window(GDK_DRAWABLE_XID(window), width, height);
#else
	GrMoveResizeWindow(GDK_DRAWABLE_XID(window), x,y,width, height);
#endif

      if (private->guffaw_gravity)
        {
          GList *tmp_list = private->children;
          while (tmp_list)
            {
              GdkWindowPrivate *child_private = tmp_list->data;

              child_private->x -= x - private->x;
              child_private->y -= y - private->y;

              tmp_list = tmp_list->next;
            }
        }

      if (private->window_type == GDK_WINDOW_CHILD)
        {
          private->x = x;
          private->y = y;
          private->width = width;
          private->height = height;
        }
    }
}

void
gdk_window_reparent (GdkWindow *window,
		     GdkWindow *new_parent,
		     gint       x,
		     gint       y)
{
  GdkWindowPrivate *window_private;
  GdkWindowPrivate *parent_private;
  GdkWindowPrivate *old_parent_private;

  g_return_if_fail (window != NULL);

  if (!new_parent)
    new_parent = (GdkWindow*) &gdk_root_parent;

  window_private = (GdkWindowPrivate*) window;
  old_parent_private = (GdkWindowPrivate*)window_private->parent;
  parent_private = (GdkWindowPrivate*) new_parent;

  if (!window_private->destroyed && !parent_private->destroyed)
    gi_reparent_window (window_private->xwindow,
                     parent_private->xwindow,
                     x, y);

  window_private->parent = new_parent;

  if (old_parent_private)
    old_parent_private->children = g_list_remove (old_parent_private->children, window);

  if ((old_parent_private &&
       (!old_parent_private->guffaw_gravity != !parent_private->guffaw_gravity)) ||
      (!old_parent_private && parent_private->guffaw_gravity))
    gdk_window_set_static_win_gravity (window, parent_private->guffaw_gravity);

  parent_private->children = g_list_prepend (parent_private->children, window);

}

void
gdk_window_clear (GdkWindow *window)
{

  g_return_if_fail (window != NULL);

  if (!GDK_DRAWABLE_DESTROYED(window))
	gi_clear_window(GDK_DRAWABLE_XID(window),FALSE);
}

void
gdk_window_clear_area (GdkWindow *window,
		       gint       x,
		       gint       y,
		       gint       width,
		       gint       height)
{

  g_return_if_fail (window != NULL);

  if (!GDK_DRAWABLE_DESTROYED(window))
  {
	if(!x && !y  &&!width && !height)
		gdk_window_clear(window);
	else
	{
		if(!width)
			width = ((GdkWindowPrivate*)window)->width - x ;
		if(!height)
                        height= ((GdkWindowPrivate*)window)->height- y ;

    		gi_clear_area (GDK_DRAWABLE_XID(window),x,y,width,height,FALSE);
	}
  }
}

void
gdk_window_clear_area_e (GdkWindow *window,
		         gint       x,
		         gint       y,
		         gint       width,
		         gint       height)
{
  g_return_if_fail (window != NULL);

  printf("gdk_window_clear_area_e called\n");

  if (!GDK_DRAWABLE_DESTROYED(window))
  {
        if(x == 0 && y ==0 &&!width && !height)
               gi_clear_window(GDK_DRAWABLE_XID(window),TRUE);
 
        else
	{
       		if(!width)
                	width = ((GdkWindowPrivate*)window)->width - x ;
       		if(!height)
       			height= ((GdkWindowPrivate*)window)->height- y ;

		gi_clear_area (GDK_DRAWABLE_XID(window),x,y,width,height,TRUE);
	}
  }

}

void
gdk_window_copy_area (GdkWindow    *window,
		      GdkGC        *gc,
		      gint          x,
		      gint          y,
		      GdkWindow    *source_window,
		      gint          source_x,
		      gint          source_y,
		      gint          width,
		      gint          height)
{
  GdkWindowPrivate *src_private;
  GdkWindowPrivate *dest_private;
  GdkGCPrivate *gc_private;
 
  g_return_if_fail (window != NULL);
  g_return_if_fail (gc != NULL);
 
  if (source_window == NULL)
    source_window = window;
 
  src_private = (GdkWindowPrivate*) source_window;
  dest_private = (GdkWindowPrivate*) window;
  gc_private = (GdkGCPrivate*) gc;

  if(!GDK_DRAWABLE_DESTROYED(source_window)&& !GDK_DRAWABLE_DESTROYED(window) && height!=0 && width!=0)
	//gi_copy_area(GDK_DRAWABLE_XID(window), GDK_GC_XGC(gc), x, y, width, height,
	//		GDK_DRAWABLE_XID(source_window), source_x, source_y);

	gi_copy_area ( src_private->xwindow, dest_private->xwindow,
		 gc_private->xgc,
		 source_x, source_y,
		 width, height,
		 x, y);


}

void
gdk_window_raise (GdkWindow *window)
{
	 g_return_if_fail (window != NULL);

	if(!GDK_DRAWABLE_DESTROYED(window))
		gi_raise_window(GDK_DRAWABLE_XID(window));
}

void
gdk_window_lower (GdkWindow *window)
{
  g_return_if_fail (window != NULL);

  if (!GDK_DRAWABLE_DESTROYED(window))
    gi_lower_window (GDK_DRAWABLE_XID(window));

}

void
gdk_window_set_user_data (GdkWindow *window,
			  gpointer   user_data)
{
  g_return_if_fail (window != NULL);
  
  window->user_data = user_data;
}

void
gdk_window_set_hints (GdkWindow *window,
		      gint       x,
		      gint       y,
		      gint       min_width,
		      gint       min_height,
		      gint       max_width,
		      gint       max_height,
		      gint       flags)
{
        g_message("__Unsupported__ gdk_window_set_hints");
}

void 
gdk_window_set_geometry_hints (GdkWindow      *window,
			       GdkGeometry    *geometry,
			       GdkWindowHints  geom_mask)
{
/*
  GdkWindowPrivate *private;
  XSizeHints size_hints;
  
  g_return_if_fail (window != NULL);
  
  private = (GdkWindowPrivate*) window;
  if (private->destroyed)
    return;
  
  size_hints.flags = 0;
  
  if (geom_mask & GDK_HINT_POS)
    {
      size_hints.flags |= PPosition;
      // We need to initialize the following obsolete fields because KWM 
      // apparently uses these fields if they are non-zero.
      // #@#!#!$!.
      //
      size_hints.x = 0;
      size_hints.y = 0;
    }
  
  if (geom_mask & GDK_HINT_MIN_SIZE)
    {
      size_hints.flags |= PMinSize;
      size_hints.min_width = geometry->min_width;
      size_hints.min_height = geometry->min_height;
    }
  
  if (geom_mask & GDK_HINT_MAX_SIZE)
    {
      size_hints.flags |= PMaxSize;
      size_hints.max_width = MAX (geometry->max_width, 1);
      size_hints.max_height = MAX (geometry->max_height, 1);
    }
  
  if (geom_mask & GDK_HINT_BASE_SIZE)
    {
      size_hints.flags |= PBaseSize;
      size_hints.base_width = geometry->base_width;
      size_hints.base_height = geometry->base_height;
    }
  
  if (geom_mask & GDK_HINT_RESIZE_INC)
    {
      size_hints.flags |= PResizeInc;
      size_hints.width_inc = geometry->width_inc;
      size_hints.height_inc = geometry->height_inc;
    }
  
  if (geom_mask & GDK_HINT_ASPECT)
    {
      size_hints.flags |= PAspect;
      if (geometry->min_aspect <= 1)
	{
	  size_hints.min_aspect.x = 65536 * geometry->min_aspect;
	  size_hints.min_aspect.y = 65536;
	}
      else
	{
	  size_hints.min_aspect.x = 65536;
	  size_hints.min_aspect.y = 65536 / geometry->min_aspect;;
	}
      if (geometry->max_aspect <= 1)
	{
	  size_hints.max_aspect.x = 65536 * geometry->max_aspect;
	  size_hints.max_aspect.y = 65536;
	}
      else
	{
	  size_hints.max_aspect.x = 65536;
	  size_hints.max_aspect.y = 65536 / geometry->max_aspect;;
	}
    }
*/
  /* FIXME: Would it be better to delete this property of
   *        geom_mask == 0? It would save space on the server
   */
//  XSetWMNormalHints (private->xdisplay, private->xwindow, &size_hints);
        g_message("__Unsupport__ gdk_window_set_geometry_hints");
}

void
gdk_window_set_title (GdkWindow   *window,
		      const gchar *title)
{
	//GR_WM_PROPERTIES props;

	//props.flags = GR_WM_FLAGS_TITLE;
	//props.title = (char *)title;
	gi_set_window_caption(GDK_DRAWABLE_XID(window), title);
}

void          
gdk_window_set_role (GdkWindow   *window,
		     const gchar *role)
{
        g_message("__Unsupport__ gdk_window_set_role");
}

void          
gdk_window_set_transient_for (GdkWindow *window, 
			      GdkWindow *parent)
{
        g_message("__Unsupport__ gdk_window_set_transient_for");
}

void
gdk_window_set_background (GdkWindow *window,
			   GdkColor  *color)
{
  g_return_if_fail (window != NULL);
  int r,g,b;

  gi_pixel_to_color(&gdk_screen_info,color->pixel ,&r,&g,&b);

  if (!GDK_DRAWABLE_DESTROYED (window))
	  gi_set_window_background(GDK_DRAWABLE_XID(window), GI_RGB(r,g,b), GI_BG_USE_COLOR);
       // GrSetBackgroundColor(GDK_DRAWABLE_XID(window), color->pixel);
}

void
gdk_window_set_back_pixmap (GdkWindow *window,
			    GdkPixmap *pixmap,
			    gboolean   parent_relative)
{
  GdkPixmapPrivate *pixmap_private;
  gi_window_id_t xpixmap;

  g_return_if_fail (window != NULL);

  pixmap_private = (GdkPixmapPrivate*) pixmap;

  if (pixmap)
    xpixmap = pixmap_private->xwindow;
  else
    xpixmap = 0;

  if (!GDK_DRAWABLE_DESTROYED(window))
        gi_set_window_pixmap(GDK_DRAWABLE_XID(window), xpixmap, 0);
}

void
gdk_window_set_cursor (GdkWindow *window,
		       GdkCursor *cursor)
{
  GdkCursorPrivate *cursor_private;
  GdkCursor *newcursor = NULL;
        int useparent = 0;

  g_return_if_fail (window != NULL);

  cursor_private = (GdkCursorPrivate*) cursor;

  if (!cursor)
  {
        useparent=1; // DEFAULT is using parent cursor
        newcursor = gdk_cursor_new(0);
        cursor_private = (GdkCursorPrivate*) newcursor;
  }

#if 0 //dpp fixme
  GrSetCursor_aux(GDK_DRAWABLE_XID(window),cursor_private->width,
              cursor_private->height,cursor_private->hot_x,
              cursor_private->hot_y, cursor_private->fg, cursor_private->bg,
              useparent, cursor_private->bitmap1fg, cursor_private->bitmap1bg);

#endif

 if(newcursor)
        gdk_cursor_destroy(newcursor);

}

void
gdk_window_set_colormap (GdkWindow   *window,
			 GdkColormap *colormap)
{
        /* should only used system colormap, and no indivisual
         * colormap should be supported.
         */
        g_message("__Unsupported__ Enter gdk_window_set_colormap\n");
}

void
gdk_window_get_user_data (GdkWindow *window,
			  gpointer  *data)
{
  g_return_if_fail (window != NULL);
  
  *data = window->user_data;
}

void
gdk_window_get_geometry (GdkWindow *window,
			 gint      *x,
			 gint      *y,
			 gint      *width,
			 gint      *height,
			 gint      *depth)
{
  gi_window_info_t winfo;
  gi_window_info_t pwinfo;
  g_return_if_fail (window != NULL /*|| GDK_IS_WINDOW (window)*/);
  if (!window)
    window = (GdkWindow*)&gdk_root_parent;

  if (!GDK_DRAWABLE_DESTROYED (window))
    {
      gi_get_window_info(GDK_DRAWABLE_XID(window), &winfo);
	if(winfo.parent)
	{
		gi_get_window_info(winfo.parent, &pwinfo);
	}
	
        if (x)
          *x = winfo.x-pwinfo.x ;
        if (y)
          *y = winfo.y- pwinfo.y;
        if (width)
          *width = winfo.width;
        if (height)
          *height = winfo.height;
        if (depth)
          *depth = 24;
    }
}

void
gdk_window_get_position (GdkWindow *window,
			 gint      *x,
			 gint      *y)
{
  GdkWindowPrivate *window_private;
 
  g_return_if_fail (window != NULL);
 
  window_private = (GdkWindowPrivate*) window;
 
  if (x)
    *x = window_private->x;
  if (y)
    *y = window_private->y;
}

void
gdk_window_get_size (GdkWindow *window,
		     gint       *width,
		     gint       *height)
{
  GdkWindowPrivate *window_private;

  g_return_if_fail (window != NULL);
  
  window_private = (GdkWindowPrivate*) window;

  if (width)
    *width = window_private->width;
  if (height)
    *height = window_private->height;
}

GdkVisual*
gdk_window_get_visual (GdkWindow *window)
{
  GdkWindowPrivate *window_private;

  g_return_val_if_fail (window != NULL, NULL);

  window_private = (GdkWindowPrivate*) window;

  while (window_private && (window_private->window_type == GDK_WINDOW_PIXMAP))
    window_private = (GdkWindowPrivate*) window_private->parent;

  if(window_private && !window_private->destroyed)
  {
        if(window_private->colormap == NULL)
                return gdk_visual_get_system();
        else
                return ((GdkColormapPrivate *)window_private->colormap)->visual;
  }
  return NULL;
}

GdkColormap*
gdk_window_get_colormap (GdkWindow *window)
{
  GdkWindowPrivate *window_private;
  
  g_return_val_if_fail (window != NULL, NULL);
  window_private = (GdkWindowPrivate*) window;
  
  g_return_val_if_fail (window_private->window_type != GDK_WINDOW_PIXMAP, NULL);
  if (!window_private->destroyed)
    {
      if (window_private->colormap == NULL)
	{
/*
	  XGetWindowAttributes (window_private->xdisplay,
				window_private->xwindow,
				&window_attributes);
	  return gdk_colormap_lookup (window_attributes.colormap);
*/
	}
      else
	return window_private->colormap;
    }
  
  return NULL;
}

GdkWindowType
gdk_window_get_type (GdkWindow *window)
{
  GdkWindowPrivate *window_private;

  g_return_val_if_fail (window != NULL, (GdkWindowType) -1);

  window_private = (GdkWindowPrivate*) window;
  return window_private->window_type;
}

gint
gdk_window_get_origin (GdkWindow *window,
		       gint      *x,
		       gint      *y)
{
  gi_window_info_t winfo;
  gi_window_info_t pwinfo;
  gint return_val;

  g_return_val_if_fail (window != NULL, 0);

  if (!GDK_DRAWABLE_DESTROYED(window))
  {
	gi_get_window_info(GDK_DRAWABLE_XID(window), &winfo);
	if(!winfo.windowid)
		return 0;
	gi_get_window_info(gdk_root_window, &pwinfo);
	return_val = TRUE;
  }
  else
	return_val = 0;
  if (x)
    *x = winfo.x- pwinfo.x ;
  if (y)
    *y = winfo.y - pwinfo.y ;

  return return_val;
}

gboolean
gdk_window_get_deskrelative_origin (GdkWindow *window,
				    gint      *x,
				    gint      *y)
{
	g_message("Enter gdk_window_get_deskrelative_origin\n");
	return FALSE;
}

void
gdk_window_get_root_origin (GdkWindow *window,
			    gint      *x,
			    gint      *y)
{
  GdkWindowPrivate *private;
  gi_window_id_t xwindow;
  gi_window_id_t xparent;
  gi_window_id_t root = gdk_root_window;
  gi_window_info_t winfo;

  g_return_if_fail (window != NULL);

        g_message("Enter gdk_window_get_root_origin\n");
  private = (GdkWindowPrivate*) window;
  if (x)
    *x = 0;
  if (y)
    *y = 0;
  if (private->destroyed)
    return;

  while (private->parent && ((GdkWindowPrivate*) private->parent)->parent)
    private = (GdkWindowPrivate*) private->parent;
  if (private->destroyed)
    return;

 xparent = private->xwindow;
  do
    {
      xwindow = xparent;
      gi_get_window_info(xwindow, &winfo);
      if(!winfo.windowid || winfo.parent)
                return;
      xparent = winfo.parent;
    }
  while (xparent != root);

  if(xparent == root)
  {
        if(x)
                *x = winfo.x;
        if(y)
                *y = winfo.y;
  }
}

GdkWindow*
gdk_window_get_pointer (GdkWindow       *window,
			gint            *x,
			gint            *y,
			GdkModifierType *mask)
{
  GdkWindow *return_val;
  int winx = 0;
  int winy = 0;
  gi_window_id_t return_id;

  if (!window)
    window = (GdkWindow*) &gdk_root_parent;

  return_val = NULL;
  if (!GDK_DRAWABLE_DESTROYED(window))
        gi_get_mouse_pos (GDK_DRAWABLE_XID(window), &return_id, &winx, &winy);

  if (x)
    *x = winx;
  if (y)
    *y = winy;
  if (mask)
    *mask =  GDK_BUTTON1_MASK; //FIXME Update GrGetPoinet to return the modifier
  return return_val;
}

GdkWindow*
gdk_window_at_pointer (gint *win_x,
		       gint *win_y)
{
  GdkWindow *window;
  gi_window_id_t wid;
  int winx, winy;

        g_message("Enter gdk_window_at_pointer");
  gi_get_mouse_pos(None, &wid, &winx, &winy);

  window = gdk_window_lookup (wid);
  if (win_x)
    *win_x = window ? winx : -1;
  if (win_y)
    *win_y = window ? winy : -1;

  return window;
}

GdkWindow*
gdk_window_get_parent (GdkWindow *window)
{
  g_return_val_if_fail (window != NULL, NULL);

  return ((GdkWindowPrivate*) window)->parent;
}

GdkWindow*
gdk_window_get_toplevel (GdkWindow *window)
{
GdkWindowPrivate *private;

  g_return_val_if_fail (window != NULL, NULL);

  private = (GdkWindowPrivate*) window;

  while (private->window_type == GDK_WINDOW_CHILD)
    {
      window = ((GdkWindowPrivate*) window)->parent;
      private = (GdkWindowPrivate*) window;
    }

  return window;
}

GList*
gdk_window_get_children (GdkWindow *window)
{
	g_message("Enter gdk_window_get_children\n");
	return NULL;
}

GdkEventMask  
gdk_window_get_events (GdkWindow *window)
{
  gi_window_info_t winfo;
  GdkEventMask event_mask;
  int i;
 
  g_return_val_if_fail (window != NULL, 0);
 
  if (GDK_DRAWABLE_DESTROYED(window))
    return 0;
 
  gi_get_window_info(GDK_DRAWABLE_XID(window), &winfo); 
  event_mask = 0;
  for (i = 0; i < gdk_nevent_masks; i++)
    {
      if (winfo.eventmask & nano_event_mask_table[i])
        event_mask |= 1 << (i + 1);
    }
 
  return event_mask;

}

void          
gdk_window_set_events (GdkWindow       *window,
		       GdkEventMask     event_mask)
{
  long xevent_mask;
  int i;
 
  g_return_if_fail (window != NULL);
 
  if (GDK_DRAWABLE_DESTROYED(window))
    return;
 
  xevent_mask = GI_MASK_CLIENT_MSG;
  for (i = 0; i < gdk_nevent_masks; i++)
    {
      if (event_mask & (1 << (i + 1)))
        xevent_mask |= nano_event_mask_table[i];
    }
 
 gi_set_events_mask(GDK_DRAWABLE_XID(window),xevent_mask);

}

void
gdk_window_add_colormap_windows (GdkWindow *window)
{
        /* Should only used system colormap- no other colormap should allow*/
        g_message("__Unsupported__ Enter gdk_window_add_colormap_windows\n");
}

#if NOT_IMPLEMENTED
static gboolean
gdk_window_have_shape_ext (void)
{
	/* There's no shape_ext support in nano-X*/
        g_message("__Unsupported__ Enter gdk_window_have_shape_ext\n");
	return FALSE;
}
#endif

/*
 * This needs the X11 shape extension.
 * If not available, shaped windows will look
 * ugly, but programs still work.    Stefan Wille
 */
void
gdk_window_shape_combine_mask (GdkWindow *window,
			       GdkBitmap *mask,
			       gint x, gint y)
{
#ifdef HAVE_SHAPE_EXT
	g_message("Enter gdk_window_shape_combine_mask\n");
#endif /* HAVE_SHAPE_EXT */

}

void          
gdk_window_add_filter (GdkWindow     *window,
		       GdkFilterFunc  function,
		       gpointer       data)
{
	GdkWindowPrivate *private;
	GList *tmp_list;
	GdkEventFilter *filter;

	private = (GdkWindowPrivate*) window;
	if (private && private->destroyed)
		return;

	if (private)
		tmp_list = private->filters;
	else
		tmp_list = gdk_default_filters;

	while (tmp_list)
	{
		filter = (GdkEventFilter *)tmp_list->data;
		if ((filter->function == function) && (filter->data == data))
			return;
		tmp_list = tmp_list->next;
	}

	filter = g_new (GdkEventFilter, 1);
	filter->function = function;
	filter->data = data;

	if (private)
		private->filters = g_list_append (private->filters, filter);
	else
		gdk_default_filters = g_list_append (gdk_default_filters, filter);

}

void
gdk_window_remove_filter (GdkWindow     *window,
			  GdkFilterFunc  function,
			  gpointer       data)
{
	GdkWindowPrivate *private;
	GList *tmp_list, *node;
	GdkEventFilter *filter;

	private = (GdkWindowPrivate*) window;

	if (private)
		tmp_list = private->filters;
	else
		tmp_list = gdk_default_filters;

	while (tmp_list)
	{
		filter = (GdkEventFilter *)tmp_list->data;
		node = tmp_list;
		tmp_list = tmp_list->next;

		if ((filter->function == function) && (filter->data == data))
		{
			if (private)
				private->filters = g_list_remove_link (private->filters, node);
			else
				gdk_default_filters = g_list_remove_link (gdk_default_filters, node);
			g_list_free_1 (node);
			g_free (filter);

			return;
		}
	}

}

void
gdk_window_set_override_redirect (GdkWindow *window,
				  gboolean override_redirect)
{
        g_message("Enter gdk_window_set_override_redirect\n");
}

void          
gdk_window_set_icon (GdkWindow *window, 
		     GdkWindow *icon_window,
		     GdkPixmap *pixmap,
		     GdkBitmap *mask)
{
	/* no such property in nano-X window */
	g_message("Enter gdk_window_set_icon\n");
}

void          
gdk_window_set_icon_name (GdkWindow   *window, 
			  const gchar *name)
{
	/* no such property in nano-X window */
	g_message("Enter gdk_window_set_icon_name\n");
}

void          
gdk_window_set_group (GdkWindow *window, 
		      GdkWindow *leader)
{
	g_message("Enter gdk_window_set_group\n");
}

#if NOT_IMPLEMENTED
static void
gdk_window_set_mwm_hints (GdkWindow *window,
			  MotifWmHints *new_hints)
{
	/* no such property in nano-X window */
        g_message("__Unsupported__ Enter gdk_window_set_mwm_hints\n");
}
#endif

void
gdk_window_set_decorations (GdkWindow      *window,
			    GdkWMDecoration decorations)
{
	/* no such property in nano-X window */
        g_message("__Unsupported__ Enter gdk_window_set_decorations\n");
}

void
gdk_window_set_functions (GdkWindow    *window,
			  GdkWMFunction functions)
{
        g_message("__Unsupport__ gdk_window_set_functions\n");
}

GList *
gdk_window_get_toplevels (void)
{
  GList *new_list = NULL;
  GList *tmp_list;

  tmp_list = gdk_root_parent.children;
  while (tmp_list)
    {
      new_list = g_list_prepend (new_list, tmp_list->data);
      tmp_list = tmp_list->next;
    }

  return new_list;
}

/* 
 * propagate the shapes from all child windows of a GDK window to the parent 
 * window. Shamelessly ripped from Enlightenment's code
 * 
 * - Raster
 */

struct _gdk_span
{
  gint                start;
  gint                end;
  struct _gdk_span    *next;
};

#if NOT_IMPLEMENTED
static void
gdk_add_to_span (struct _gdk_span **s,
		 gint               x,
		 gint               xx)
{
#ifdef HAVE_SHAPE_EXT
	g_message("Enter gdk_add_to_span\n");
#endif
}

static void
gdk_add_rectangles (Display           *disp,
		    gi_window_id_t win,
		    struct _gdk_span **spans,
		    gint               basew,
		    gint               baseh,
		    gint               x,
		    gint               y)
{
#ifdef HAVE_SHAPE_EXT
	g_message("Enter gdk_add_rectangles\n");
#endif
}

static void
gdk_propagate_shapes (Display *disp,
		      gi_window_id_t win,
		      gboolean merge)
{
#ifdef HAVE_SHAPE_EXT
	g_message("Enter gdk_propagate_shapes\n");
#endif
}

#endif
void
gdk_window_set_child_shapes (GdkWindow *window)
{
#ifdef HAVE_SHAPE_EXT
	g_message("Enter gdk_window_set_child_shapes\n");
#endif
}

void
gdk_window_merge_child_shapes (GdkWindow *window)
{
#ifdef HAVE_SHAPE_EXT
	g_message("Enter gdk_window_merge_child_shapes\n");
#endif
}

/*************************************************************
 * gdk_window_is_visible:
 *     Check if the given window is mapped.
 *   arguments:
 *     window: 
 *   results:
 *     is the window mapped
 *************************************************************/

gboolean 
gdk_window_is_visible (GdkWindow *window)
{
  GdkWindowPrivate *private = (GdkWindowPrivate *)window;
 
  g_return_val_if_fail (window != NULL, FALSE);

  return private->mapped;

}

/*************************************************************
 * gdk_window_is_viewable:
 *     Check if the window and all ancestors of the window
 *     are mapped. (This is not necessarily "viewable" in
 *     the X sense, since we only check as far as we have
 *     GDK window parents, not to the root window)
 *   arguments:
 *     window:
 *   results:
 *     is the window viewable
 *************************************************************/

gboolean 
gdk_window_is_viewable (GdkWindow *window)
{
  GdkWindowPrivate *private = (GdkWindowPrivate *)window;
 
  g_return_val_if_fail (window != NULL, FALSE);
 
  while (private &&
         (private != &gdk_root_parent) &&
         (private->window_type != GDK_WINDOW_FOREIGN))
    {
      if (!private->mapped)
        return FALSE;

      private = (GdkWindowPrivate *)private->parent;
    }

  return TRUE;
}

void          
gdk_drawable_set_data (GdkDrawable   *drawable,
		       const gchar   *key,
		       gpointer	      data,
		       GDestroyNotify destroy_func)
{
        g_message("gdk_drawable_set_data");
        g_dataset_set_data_full (drawable, key, data, destroy_func);
}

#if NOT_IMPLEMENTED
/* Support for windows that can be guffaw-scrolled
 * (See http://www.gtk.org/~otaylor/whitepapers/guffaw-scrolling.txt)
 */

static gboolean
gdk_window_gravity_works (void)
{
        g_message("__Unsupported__ gdk_window_gravity_works");
        return FALSE;
}
#endif

#if NOT_IMPLEMENTED
static void
gdk_window_set_static_bit_gravity (GdkWindow *window, gboolean on)
{
        g_message("__Unsupported__ gdk_window_set_static_bit_gravity");
}
#endif

static void
gdk_window_set_static_win_gravity (GdkWindow *window, gboolean on)
{
        g_message("__Unsupported__ gdk_window_set_static_win_gravity");
}

/*************************************************************
 * gdk_window_set_static_gravities:
 *     Set the bit gravity of the given window to static,
 *     and flag it so all children get static subwindow
 *     gravity.
 *   arguments:
 *     window: window for which to set static gravity
 *     use_static: Whether to turn static gravity on or off.
 *   results:
 *     Does the XServer support static gravity?
 *************************************************************/

gboolean 
gdk_window_set_static_gravities (GdkWindow *window,
				 gboolean   use_static)
{
        g_message("__Unsupported__ gdk_window_set_static_gravities");
        return FALSE;
}
