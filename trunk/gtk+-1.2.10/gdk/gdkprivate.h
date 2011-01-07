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

#ifndef __GDK_PRIVATE_H__
#define __GDK_PRIVATE_H__

#include <gdk/gdktypes.h>

#ifdef NANOGTK
#define NANOGTK_internal
#endif

#include <gi/gi.h>
#include <gi/property.h>
#include <gi/user_font.h>
#include <gi/region.h>
//#include "../../microwin/src/include/cjk.h"

typedef struct 
{
	gi_ufont_t font;
	int baseline;
	int height;
}GR_FONT_INFO;

typedef struct {
    short x, y;
    unsigned short width, height;
} XRectangle1;

typedef struct gi_cliprect_t XRectangle;


//#include <X11/Xlib.h>
#include <sys/time.h>
//#include "gdk/region.h"
typedef struct _XRegion *Region;

#define gdk_window_lookup(xid)	   ((GdkWindow*) gdk_xid_table_lookup (xid))
#define gdk_pixmap_lookup(xid)	   ((GdkPixmap*) gdk_xid_table_lookup (xid))
#define gdk_font_lookup(xid)	   ((GdkFont*) gdk_xid_table_lookup (xid))

#ifndef _XLIB_H_
#define Bool 	int
#define Status 	int
#define True 	1
#define False 	0
#define None 	0L
#define GrabSuccess     0
#define AlreadyGrabbed  1
#define LSBFirst        0
#define MSBFirst        1
#if 0
struct timeval {
    long tv_sec;
    long tv_usec;
};
#endif
#endif //_XLIB_H_

#define GDK_DRAWABLE_DESTROYED(d) (((GdkWindowPrivate *)d)->destroyed)
#define GDK_GC_XGC(gc)            (((GdkGCPrivate*) gc)->xgc)
#define GDK_DRAWABLE_XID(win) 	(((GdkWindowPrivate*)win)->xwindow)
#define GDK_DRAWABLE_CHILDREN(win) (((GdkWindowPrivate*)win)->children)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct _GdkWindowPrivate       GdkWindowPrivate;
typedef struct _GdkWindowPrivate       GdkPixmapPrivate;
typedef struct _GdkImagePrivate	       GdkImagePrivate;
typedef struct _GdkGCPrivate	       GdkGCPrivate;
typedef struct _GdkColormapPrivate     GdkColormapPrivate;
typedef struct _GdkColorInfo           GdkColorInfo;
typedef struct _GdkVisualPrivate       GdkVisualPrivate;
typedef struct _GdkFontPrivate	       GdkFontPrivate;
typedef struct _GdkCursorPrivate       GdkCursorPrivate;
typedef struct _GdkEventFilter	       GdkEventFilter;
typedef struct _GdkClientFilter	       GdkClientFilter;
typedef struct _GdkColorContextPrivate GdkColorContextPrivate;
typedef struct _GdkRegionPrivate       GdkRegionPrivate;


struct _GdkWindowPrivate
{
  GdkWindow window;
  GdkWindow *parent;
  gi_window_id_t xwindow;
  gint16 x;
  gint16 y;
  guint16 width;
  guint16 height;
  guint8 resize_count;
  guint8 window_type;
  guint ref_count;
  guint destroyed : 2;
  guint mapped : 1;
  guint guffaw_gravity : 1;

  gint extension_events;

  GList *filters;
  GdkColormap *colormap;
  GList *children;
};

struct _GdkImagePrivate
{
  GdkImage image;
  gi_image_t* ximage;
  gint	bnImageInServer;
  char *data;	/* pointer to image data */
  gpointer x_shm_info;

  void (*image_put) (GdkDrawable *window,
		     GdkGC	 *gc,
		     GdkImage	 *image,
		     gint	  xsrc,
		     gint	  ysrc,
		     gint	  xdest,
		     gint	  ydest,
		     gint	  width,
		     gint	  height);
};

struct _GdkGCPrivate
{
  GdkGC gc;
  gi_gc_ptr_t xgc;
  gi_window_id_t xwindow; // the window id the GC lie on
  gpointer klass_data;

  guint ref_count;
  gint clip_x_origin;
  gint clip_y_origin;
  gint ts_x_origin;
  gint ts_y_origin;
};

typedef enum {
  GDK_COLOR_WRITEABLE = 1 << 0
} GdkColorInfoFlags;

struct _GdkColorInfo
{
  GdkColorInfoFlags flags;
  guint ref_count;
};

struct _GdkColormapPrivate
{
  GdkColormap colormap;
  gi_color_t xcolormap;
  //Colormap xcolormap;
  //Display *xdisplay;
  GdkVisual *visual;
  gint private_val;

  GHashTable *hash;
  GdkColorInfo *info;
  time_t last_sync_time;
  
  guint ref_count;
};

struct _GdkVisualPrivate
{
  GdkVisual visual;
  gi_screen_info_t *si;
};

struct _GdkFontPrivate
{
  GdkFont font;
  /* generic pointer point to XFontStruct or XFontSet */
  gpointer xfont;
  gi_ufont_t fid;
  guint ref_count;
  gchar *font_name;
  GSList *names;
};

struct _GdkCursorPrivate
{
  GdkCursor cursor;
  int height;
  int width;
  gi_color_t fg;
  gi_color_t bg;
  int hot_x;
  int hot_y;
  gi_image_t       *bitmap1fg;   /* mouse cursor */
  gi_image_t       *bitmap1bg;
};

struct _GdkDndCursorInfo {
  //Cursor	  gdk_cursor_dragdefault, gdk_cursor_dragok;
  GdkWindow	 *drag_pm_default, *drag_pm_ok;
  GdkPoint	  default_hotspot, ok_hotspot;
  GList *xids;
};
typedef struct _GdkDndCursorInfo GdkDndCursorInfo;

struct _GdkDndGlobals {
  GdkAtom	     gdk_XdeEnter, gdk_XdeLeave, gdk_XdeRequest;
  GdkAtom	     gdk_XdeDataAvailable, gdk_XdeDataShow, gdk_XdeCancel;
  GdkAtom	     gdk_XdeTypelist;

  GdkDndCursorInfo  *c;
  GdkWindow	**drag_startwindows;
  guint		  drag_numwindows;
  gboolean	  drag_really, drag_perhaps, dnd_grabbed;
  //Window	  dnd_drag_target;
  GdkPoint	  drag_dropcoords;

  GdkPoint dnd_drag_start, dnd_drag_oldpos;
  GdkRectangle dnd_drag_dropzone;
  GdkWindowPrivate *real_sw;
  //Window dnd_drag_curwin;
  //Time last_drop_time; /* An incredible hack, sosumi miguel */
};
typedef struct _GdkDndGlobals GdkDndGlobals;

struct _GdkEventFilter {
  GdkFilterFunc function;
  gpointer data;
};

struct _GdkClientFilter {
  GdkAtom       type;
  GdkFilterFunc function;
  gpointer      data;
};

#if 0 //def USE_XIM

typedef struct _GdkICPrivate GdkICPrivate;

struct _GdkICPrivate
{
  XIC xic;
  GdkICAttr *attr;
  GdkICAttributesType mask;
};

#endif /* USE_XIM */

struct _GdkColorContextPrivate
{
  GdkColorContext color_context;
  //Display *xdisplay;
//  XStandardColormap std_cmap;
};

struct _GdkRegionPrivate
{
  GdkRegion region;
  Region xregion;
};

typedef enum {
  GDK_DEBUG_MISC          = 1 << 0,
  GDK_DEBUG_EVENTS        = 1 << 1,
  GDK_DEBUG_DND           = 1 << 2,
  GDK_DEBUG_COLOR_CONTEXT = 1 << 3,
  GDK_DEBUG_XIM           = 1 << 4
} GdkDebugFlag;

void gdk_events_init (void);
void gdk_window_init (void);
void gdk_visual_init (void);
void gdk_dnd_init    (void);

void gdk_image_init  (void);
void gdk_image_exit (void);

GdkColormap* gdk_colormap_lookup (gi_color_t  xcolormap);
GdkVisual*   gdk_visual_lookup	 ();

void gdk_window_add_colormap_windows (GdkWindow *window);
void gdk_window_destroy_notify	     (GdkWindow *window);

void	 gdk_xid_table_insert (gi_id_t	*xid,
			       gpointer	 data);
void	 gdk_xid_table_remove (gi_id_t	 xid);
gpointer gdk_xid_table_lookup (gi_id_t	 xid);

gint gdk_send_xevent (gboolean propagate, glong event_mask);

/* If you pass x = y = -1, it queries the pointer
   to find out where it currently is.
   If you pass x = y = -2, it does anything necessary
   to know that the drag is ending.
*/
void gdk_dnd_display_drag_cursor(gint x,
				 gint y,
				 gboolean drag_ok,
				 gboolean change_made);

/* Please see gdkwindow.c for comments on how to use */ 
gi_window_id_t gdk_window_xid_at(gi_window_id_t base, gint bx, gint by, gint x, gint y, GList *excludes, gboolean excl_child);
gi_window_id_t gdk_window_xid_at_coords(gint x, gint y, GList *excludes, gboolean excl_child);

extern gint		 gdk_debug_level;
extern gint		 gdk_show_events;
extern gint		 gdk_use_xshm;
extern gint		 gdk_stack_trace;
extern gchar		*gdk_display_name;
extern gint		 gdk_screen;
extern gi_window_id_t		 gdk_root_window;
extern gi_window_id_t		 gdk_leader_window;
extern GdkWindowPrivate	 gdk_root_parent;
extern gi_window_id_t		 gdk_wm_delete_window;
extern gi_window_id_t		 gdk_wm_take_focus;
extern gi_window_id_t		 gdk_wm_protocols;
extern gi_window_id_t		 gdk_wm_window_protocols[];
extern gi_window_id_t		 gdk_selection_property;
extern GdkDndGlobals	 gdk_dnd;
extern GdkWindow	*selection_owner[];
extern gchar		*gdk_progclass;
extern gint		 gdk_error_code;
extern gint		 gdk_error_warnings;
extern gint              gdk_null_window_warnings;
extern GList            *gdk_default_filters;
extern const int         gdk_nevent_masks;
extern const int         gdk_event_mask_table[];

extern GdkWindowPrivate *gdk_xgrab_window;  /* Window that currently holds the
					     * x pointer grab
					     */

#if 0 //def USE_XIM
/* XIM support */
gint   gdk_im_open		 (void);
void   gdk_im_close		 (void);
void   gdk_ic_cleanup		 (void);

extern GdkICPrivate *gdk_xim_ic;		/* currently using IC */
extern GdkWindow *gdk_xim_window;	        /* currently using Window */
#endif /* USE_XIM */

/* Debugging support */

#ifdef G_ENABLE_DEBUG

#define GDK_NOTE(type,action)		     G_STMT_START { \
    if (gdk_debug_flags & GDK_DEBUG_##type)		    \
       { action; };			     } G_STMT_END

#else /* !G_ENABLE_DEBUG */

#define GDK_NOTE(type,action)
      
#endif /* G_ENABLE_DEBUG */

extern guint gdk_debug_flags;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GDK_PRIVATE_H__ */
