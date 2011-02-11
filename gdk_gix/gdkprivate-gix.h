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

#ifndef __GDK_PRIVATE_GIX_H__
#define __GDK_PRIVATE_GIX_H__

//#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include "gdkinternals.h"
#include "gdkcursor.h"
#include "gdkdisplay-gix.h"
#include "gdkregion-generic.h"
#include <cairo.h>

#include <string.h>

//#include <gix_util.h>

#define WARING_MSG do{\
	printf("%s: Not working now(Line%d)\n",__FUNCTION__, __LINE__);\
	}\
	while (0)

#define GDK_TYPE_DRAWABLE_IMPL_GIX       (gdk_drawable_impl_gix_get_type ())
#define GDK_DRAWABLE_IMPL_GIX(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DRAWABLE_IMPL_GIX, GdkDrawableImplGix))
#define GDK_IS_DRAWABLE_IMPL_GIX(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DRAWABLE_IMPL_GIX))

#define GDK_TYPE_WINDOW_IMPL_GIX         (gdk_window_impl_gix_get_type ())
#define GDK_WINDOW_IMPL_GIX(object)      (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_WINDOW_IMPL_GIX, GdkWindowImplGix))
#define GDK_IS_WINDOW_IMPL_GIX(object)   (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_WINDOW_IMPL_GIX))

#define GDK_TYPE_PIXMAP_IMPL_GIX         (gdk_pixmap_impl_gix_get_type ())
#define GDK_PIXMAP_IMPL_GIX(object)      (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXMAP_IMPL_GIX, GdkPixmapImplGix))
#define GDK_IS_PIXMAP_IMPL_GIX(object)   (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXMAP_IMPL_GIX))

//#define GDK_WINDOW_GIX_ID(win) (GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (win)->impl)->drawable.window_id)
#define GDK_WINDOW_GIX_ID(win)           (GDK_DRAWABLE_IMPL_GIX(((GdkWindowObject *)win)->impl)->window_id)
#define GDK_PIXMAP_GIX_ID(win)           (GDK_DRAWABLE_IMPL_GIX(((GdkPixmapObject *)win)->impl)->window_id)

typedef struct _GdkDrawableImplGix GdkDrawableImplGix;
typedef struct _GdkWindowImplGix   GdkWindowImplGix;
typedef struct _GdkPixmapImplGix   GdkPixmapImplGix;


struct _GdkDrawableImplGix
{
  GdkDrawable             parent_object;

  GdkDrawable            *wrapper;

  gboolean                buffered;

  GdkRegion               paint_region;
  gint                    paint_depth;
  gint                    width;
  gint                    height;
  gint                    abs_x;
  gint                    abs_y;

  GdkRegion               clip_region;

  GdkColormap            *colormap;
  
  gi_window_id_t window_id;

  //IGixSurface       *surface;
  //DFBSurfacePixelFormat   format;
  unsigned format;
  cairo_surface_t *  cairo_surface;

};

typedef struct
{
  GdkDrawableClass  parent_class;
} GdkDrawableImplGixClass;

GType      gdk_drawable_impl_gix_get_type (void);

GdkEvent *  gdk_gix_event_make     (GdkWindow               *window,
                                         GdkEventType             type);


/*
 * Pixmap
 */

struct _GdkPixmapImplGix
{
  GdkDrawableImplGix parent_instance;
};

typedef struct
{
  GdkDrawableImplGixClass parent_class;
} GdkPixmapImplGixClass;

GType gdk_pixmap_impl_gix_get_type (void);



/*
 * Window
 */

typedef struct
{
  gulong   length;
  GdkAtom  type;
  gint     format;
  guchar   data[1];
} GdkWindowProperty;

typedef struct _GdkXPositionInfo GdkXPositionInfo;

struct _GdkXPositionInfo
{
  gint x;
  gint y;
  gint width;
  gint height;
  gint x_offset;		/* Offsets to add to X coordinates within window */
  gint y_offset;		/*   to get GDK coodinates within window */
  guint big : 1;
  guint mapped : 1;
  guint no_bg : 1;	        /* Set when the window background is temporarily
				 * unset during resizing and scaling */
  GdkRectangle clip_rect;	/* visible rectangle of window */
};
struct _GdkWindowImplGix
{
  GdkDrawableImplGix drawable;
  GdkWindow             *gdkWindow;

//  GdkXPositionInfo position_info; 

  GdkCursor              *cursor;
  GHashTable             *properties;

  guint8                  opacity;

  GdkWindowTypeHint       type_hint;

  //DFBUpdates              flips;
  //DFBRegion               flip_regions[4];
};

typedef struct
{
  GdkDrawableImplGixClass parent_class;
} GdkWindowImplGixClass;

GType gdk_window_impl_gix_get_type        (void);

void  gdk_gix_window_send_crossing_events (GdkWindow       *src,
                                                GdkWindow       *dest,
                                                GdkCrossingMode  mode);

void  _gdk_gix_calc_abs                   (GdkWindow       *window);

GdkWindow * gdk_gix_window_find_toplevel  (GdkWindow       *window);


void        gdk_gix_window_id_table_insert (gi_window_id_t  dfb_id,
                                                 GdkWindow   *window);
void        gdk_gix_window_id_table_remove (gi_window_id_t  dfb_id);
GdkWindow * gdk_gix_window_id_table_lookup (gi_window_id_t  dfb_id);

void        _gdk_gix_window_get_offsets    (GdkWindow       *window,
                                                 gint            *x_offset,
                                                 gint            *y_offset);
void        _gdk_gix_window_scroll         (GdkWindow       *window,
                                                 gint             dx,
                                                 gint             dy);
void        _gdk_gix_window_move_region    (GdkWindow       *window,
                                                 const GdkRegion *region,
                                                 gint             dx,
                                                 gint             dy);


typedef struct
{
  GdkCursor         cursor;

  gint              hot_x;
  gint              hot_y;
  gi_image_t *shape_img;
  //IGixSurface *shape;
} GdkCursorGix;

typedef struct
{
  GdkVisual              visual;
  DFBSurfacePixelFormat  format;
} GdkVisualGix;

typedef struct
{
	gi_image_t *surface_img;
  //IGixSurface *surface;
} GdkImageGix;


#define GDK_TYPE_GC_GIX       (gdk_gc_gix_get_type ())
#define GDK_GC_GIX(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GC_GIX, GdkGCGix))
#define GDK_IS_GC_GIX(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GC_GIX))
#define GDK_GC_GET_XGC(gc)	      (GDK_GC_GIX(gc)->gix_gc)

typedef struct
{
  GdkGC             parent_instance;
  guint16 dirty_mask;
#if 0
  guint have_clip_region : 1;
  guint have_clip_mask : 1;
  guint depth : 8;
#else
  GdkRegion         clip_region;
#endif
  GdkGCValuesMask   values_mask;
  GdkGCValues       values;
  gi_gc_ptr_t gix_gc;
} GdkGCGix;

typedef struct
{
  GdkGCClass        parent_class;
} GdkGCGixClass;

GType     gdk_gc_gix_get_type (void);

GdkGC *  _gdk_gix_gc_new      (GdkDrawable     *drawable,
                                    GdkGCValues     *values,
                                    GdkGCValuesMask  values_mask);

GdkImage* _gdk_gix_copy_to_image (GdkDrawable  *drawable,
                                       GdkImage     *image,
                                       gint          src_x,
                                       gint          src_y,
                                       gint          dest_x,
                                       gint          dest_y,
                                       gint          width,
                                       gint          height);

void       gdk_gix_event_windows_add    (GdkWindow *window);
void       gdk_gix_event_windows_remove (GdkWindow *window);

GdkGrabStatus gdk_gix_keyboard_grab  (GdkDisplay          *display,
                                           GdkWindow           *window,
                                           gint                 owner_events,
                                           guint32              time);

void          gdk_gix_keyboard_ungrab(GdkDisplay          *display,
                                           guint32              time);

GdkGrabStatus gdk_gix_pointer_grab   (GdkWindow           *window,
                                           gint                 owner_events,
                                           GdkEventMask         event_mask,
                                           GdkWindow           *confine_to,
                                           GdkCursor           *cursor,
                                           guint32              time,
                                           gboolean             implicit_grab);
void          gdk_gix_pointer_ungrab (guint32              time,
                                           gboolean             implicit_grab);

guint32       gdk_gix_get_time       (void);

GdkWindow * gdk_gix_pointer_event_window  (GdkWindow    *window,
                                                GdkEventType  type);
GdkWindow * gdk_gix_keyboard_event_window (GdkWindow    *window,
                                                GdkEventType  type);
GdkWindow * gdk_gix_other_event_window    (GdkWindow    *window,
                                                GdkEventType  type);
void       _gdk_selection_window_destroyed    (GdkWindow       *window);

void       _gdk_gix_move_resize_child (GdkWindow *window,
                                            gint       x,
                                            gint       y,
                                            gint       width,
                                            gint       height);

GdkWindow * gdk_gix_child_at          (GdkWindow *window,
                                            gint      *x,
                                            gint      *y);

GdkWindow * gdk_gix_window_find_focus (void);

void        gdk_gix_change_focus      (GdkWindow *new_focus_window);

void        gdk_gix_mouse_get_info    (gint            *x,
                                            gint            *y,
                                            GdkModifierType *mask);

/**********************/
/*  Global variables  */
/**********************/

extern GdkDisplayDFB *_gdk_display;

/* Pointer grab info */
extern GdkWindow           * _gdk_gix_pointer_grab_window;
extern gboolean              _gdk_gix_pointer_grab_owner_events;
extern GdkWindow           * _gdk_gix_pointer_grab_confine;
extern GdkEventMask          _gdk_gix_pointer_grab_events;
extern GdkCursor           * _gdk_gix_pointer_grab_cursor;

/* Keyboard grab info */
extern GdkWindow           * _gdk_gix_keyboard_grab_window;
extern GdkEventMask          _gdk_gix_keyboard_grab_events;
extern gboolean              _gdk_gix_keyboard_grab_owner_events;

extern GdkScreen  *  _gdk_screen;

extern GdkAtom               _gdk_selection_property;



typedef struct 
{
int r,g,b;
}IGixPalette;


IGixPalette * gdk_gix_colormap_get_palette (GdkColormap *colormap);


/* these are Linux-FB specific functions used for window decorations */

typedef gboolean (* GdkWindowChildChanged) (GdkWindow *window,
                                            gint       x,
                                            gint       y,
                                            gint       width,
                                            gint       height,
                                            gpointer   user_data);
typedef void     (* GdkWindowChildGetPos)  (GdkWindow *window,
                                            gint      *x,
                                            gint      *y,
                                            gpointer   user_data);

void gdk_fb_window_set_child_handler (GdkWindow              *window,
                                      GdkWindowChildChanged  changed,
                                      GdkWindowChildGetPos   get_pos,
                                      gpointer               user_data);

void gdk_gix_clip_region (GdkDrawable  *drawable,
                               GdkGC        *gc,
                               GdkRectangle *draw_rect,
                               GdkRegion    *ret_clip);


/* Utilities for avoiding mallocs */

static inline void
temp_region_init_copy( GdkRegion       *region, 
                       const GdkRegion *source)
{
  if (region != source) /*  don't want to copy to itself */
    {  
      if (region->size < source->numRects)
        {
          if (region->rects && region->rects != &region->extents)
            g_free( region->rects );

          region->rects = g_new (GdkRegionBox, source->numRects);
          region->size  = source->numRects;
        }

      region->numRects = source->numRects;
      region->extents  = source->extents;

      memcpy( region->rects, source->rects, source->numRects * sizeof (GdkRegionBox) );
    }
}

static inline void
temp_region_init_rectangle( GdkRegion          *region,
                            const GdkRectangle *rect )
{
     region->numRects = 1;
     region->rects = &region->extents;
     region->extents.x1 = rect->x;
     region->extents.y1 = rect->y;
     region->extents.x2 = rect->x + rect->width;
     region->extents.y2 = rect->y + rect->height;
     region->size = 1;
}

static inline void
temp_region_init_rectangle_vals( GdkRegion *region,
                                 int        x,
                                 int        y,
                                 int        w,
                                 int        h )
{
     region->numRects = 1;
     region->rects = &region->extents;
     region->extents.x1 = x;
     region->extents.y1 = y;
     region->extents.x2 = x + w;
     region->extents.y2 = y + h;
     region->size = 1;
}

static inline void
temp_region_reset( GdkRegion *region )
{
     if (region->size > 32 && region->rects && region->rects != &region->extents) {
          g_free( region->rects );

          region->size  = 1;
          region->rects = &region->extents;
     }

     region->numRects = 0;
}

static inline void
temp_region_deinit( GdkRegion *region )
{
     if (region->rects && region->rects != &region->extents) {
          g_free( region->rects );
          region->rects = NULL;
     }

     region->numRects = 0;
}


#define GDKDFB_RECTANGLE_VALS_FROM_BOX(s)   (s)->x1, (s)->y1, (s)->x2-(s)->x1, (s)->y2-(s)->y1


#endif /* __GDK_PRIVATE_GIX_H__ */
