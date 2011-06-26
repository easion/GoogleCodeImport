
#include "config.h"

#include <netinet/in.h>
#include <unistd.h>

#include "gdk.h"
#include "gdkgix.h"

#include "gdkwindow.h"
#include "gdkwindowimpl.h"
#include "gdkdisplay-gix.h"
#include "gdkprivate-gix.h"
#include "gdkinternals.h"
#include "gdkdeviceprivate.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_IS_TOPLEVEL_OR_FOREIGN(window) \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD &&   \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

#define WINDOW_IS_TOPLEVEL(window)		     \
  (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD &&   \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_FOREIGN && \
   GDK_WINDOW_TYPE (window) != GDK_WINDOW_OFFSCREEN)

/* Return whether time1 is considered later than time2 as far as xserver
 * time is concerned.  Accounts for wraparound.
 */
#define XSERVER_TIME_IS_LATER(time1, time2)                        \
  ( (( time1 > time2 ) && ( time1 - time2 < ((guint32)-1)/2 )) ||  \
    (( time1 < time2 ) && ( time2 - time1 > ((guint32)-1)/2 ))     \
  )

typedef struct _GdkGixWindow GdkGixWindow;
typedef struct _GdkGixWindowClass GdkGixWindowClass;

struct _GdkGixWindow {
  GdkWindow parent;
};

struct _GdkGixWindowClass {
  GdkWindowClass parent_class;
};

G_DEFINE_TYPE (GdkGixWindow, _gdk_gix_window, GDK_TYPE_WINDOW)

static void
_gdk_gix_window_class_init (GdkGixWindowClass *gix_window_class)
{
}

static void
_gdk_gix_window_init (GdkGixWindow *gix_window)
{
}


G_DEFINE_TYPE (GdkWindowImplGix, _gdk_window_impl_gix, GDK_TYPE_WINDOW_IMPL)



#include <stdio.h>

static guint
gdk_xid_hash (gi_window_id_t *xid)
{
  return *xid;
}

static gboolean
gdk_xid_equal (gi_window_id_t *a, gi_window_id_t *b)
{
  return (*a == *b);
}

static void
_gdk_gix_display_add_window (GdkDisplay *display,
                             gi_window_id_t        *xid,
                             GdkWindow  *data)
{
  GdkDisplayGix *display_gix;

  g_return_if_fail (xid != NULL);
  g_return_if_fail (GDK_IS_DISPLAY (display));

  display_gix = GDK_DISPLAY_GIX (display);

  if (!display_gix->xid_ht)
    display_gix->xid_ht = g_hash_table_new ((GHashFunc) gdk_xid_hash,
                                            (GEqualFunc) gdk_xid_equal);

  if (g_hash_table_lookup (display_gix->xid_ht, xid))
    g_warning ("gi_window_id_t collision, trouble ahead");

  g_hash_table_insert (display_gix->xid_ht, xid, data);
}

static void
_gdk_gix_display_remove_window (GdkDisplay *display,
                                gi_window_id_t         xid)
{
  GdkDisplayGix *display_gix;

  g_return_if_fail (GDK_IS_DISPLAY (display));

  display_gix = GDK_DISPLAY_GIX (display);

  if (display_gix->xid_ht)
    g_hash_table_remove (display_gix->xid_ht, &xid);
}

GdkWindow *
gdk_gix_window_lookup_for_display (GdkDisplay *display,
                                   gi_window_id_t      window)
{
  GdkDisplayGix *display_gix;
  GdkWindow *data = NULL;

  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (display == _gdk_display, NULL);

  display_gix = GDK_DISPLAY_GIX (display);

  if (display_gix->xid_ht)
    data = g_hash_table_lookup (display_gix->xid_ht, &window);

  return data;
}



static void
_gdk_window_impl_gix_init (GdkWindowImplGix *impl)
{
  impl->toplevel_window_type = -1;
}

void
_gdk_gix_window_update_size (GdkWindow *window,
				 int32_t width, int32_t height, uint32_t edges)
{
  GdkWindowImplGix *impl = GDK_WINDOW_IMPL_GIX (window->impl);
  GdkRectangle area;
  cairo_region_t *region;

  if (impl->cairo_surface)
    {
      cairo_surface_destroy (impl->cairo_surface);
      impl->cairo_surface = NULL;
    }

  window->width = width;
  window->height = height;
  impl->resize_edges = edges;

  area.x = 0;
  area.y = 0;
  area.width = window->width;
  area.height = window->height;

  region = cairo_region_create_rectangle (&area);
  _gdk_window_invalidate_for_expose (window, region);
  cairo_region_destroy (region);
}

GdkWindow *
_gdk_gix_screen_create_root_window (GdkScreen *screen,
					int width, int height)
{
  GdkWindow *window;
  GdkWindowImplGix *impl;
  GdkScreenGix *gix_screen;

  gix_screen = GDK_SCREEN_GIX (screen);

  window = _gdk_display_create_window (gdk_screen_get_display (screen));
  _gdk_root = window;

  window->impl = g_object_new (GDK_TYPE_WINDOW_IMPL_GIX, NULL);
  window->impl_window = window;
  window->visual = gdk_screen_get_system_visual (screen);
  g_assert(window->visual != NULL);

  impl = GDK_WINDOW_IMPL_GIX (window->impl);

  impl->wrapper = GDK_WINDOW (window);

  window->window_type = GDK_WINDOW_ROOT;
  //window->depth = gi_screen_format();
  window->depth = GI_RENDER_FORMAT_BPP(gi_screen_format());

  window->x = 0;
  window->y = 0;
  window->abs_x = 0;
  window->abs_y = 0;
  window->width = width;
  window->height = height;
  window->viewable = TRUE;

  impl->window_id = GI_DESKTOP_WINDOW_ID;
  gix_screen->root_window = window;

  /* see init_randr_support() in gdkscreen-gix.c */
  window->event_mask = GDK_STRUCTURE_MASK;

  _gdk_gix_display_add_window (gix_screen->scr_display,
                               &gix_screen->xroot_window,
                               gix_screen->root_window);

  return window;
}

static const gchar *
get_default_title (void)
{
  const char *title;

  title = g_get_application_name ();
  if (!title)
    title = g_get_prgname ();
  if (!title)
    title = "";

  return title;
}

static
gint window_type_hint_to_level (GdkWindowTypeHint hint)
{
  switch (hint)
    {
    case GDK_WINDOW_TYPE_HINT_DOCK:
    case GDK_WINDOW_TYPE_HINT_UTILITY:
      return GI_FLAGS_TASKBAR_WINDOW|GI_FLAGS_NON_FRAME;

    case GDK_WINDOW_TYPE_HINT_MENU: /* Torn-off menu */
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU: /* Menu from menubar */
      return (GI_FLAGS_MENU_WINDOW|GI_FLAGS_NON_FRAME);

    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
      return GI_FLAGS_NON_FRAME;

    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case GDK_WINDOW_TYPE_HINT_COMBO:
    case GDK_WINDOW_TYPE_HINT_DND:
      return GI_FLAGS_NON_FRAME;

    case GDK_WINDOW_TYPE_HINT_DIALOG:  /* Dialog window */
	return GI_FLAGS_TEMP_WINDOW;

    case GDK_WINDOW_TYPE_HINT_DESKTOP: /* N/A */
	return GI_FLAGS_DESKTOP_WINDOW;

    case GDK_WINDOW_TYPE_HINT_NORMAL:  /* Normal toplevel window */
    case GDK_WINDOW_TYPE_HINT_TOOLBAR: /* Window used to implement toolbars */

    default:
		printf("unknow hint = %d\n", hint);
      break;
    }

  return 0;
}


void
_gdk_gix_display_create_window_impl (GdkDisplay    *display,
					 GdkWindow     *window,
					 GdkWindow     *real_parent,
					 GdkScreen     *screen,
					 GdkEventMask   event_mask,
					 GdkWindowAttr *attributes,
					 gint           attributes_mask)
{
  GdkWindowImplGix *impl;
  const char *title;
  gi_window_id_t  mainWin;
  gi_window_id_t xparent;
  unsigned long win_flags = 0;
  GdkScreenGix *gix_screen;
  GdkDisplayGix *display_gix;
  int x,y;
  int window_width,window_height;

  display_gix = GDK_DISPLAY_GIX (display);

  gix_screen = GDK_SCREEN_GIX (screen);
  xparent = GDK_WINDOW_XID (real_parent);
  display_gix = GDK_DISPLAY_GIX (display);

  impl = g_object_new (GDK_TYPE_WINDOW_IMPL_GIX, NULL);
  window->impl = GDK_WINDOW_IMPL (impl);
  impl->wrapper =  (window);

  if (!window->input_only){
  }
  else{
	  g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
	  win_flags |= GI_FLAGS_TRANSPARENT;
  }

  if (window->width > 65535 ||
      window->height > 65535)
    {
      g_warning ("Native Windows wider or taller than 65535 pixels are not supported");

      if (window->width > 65535)
	window->width = 65535;
      if (window->height > 65535)
	window->height = 65535;
    }

  //g_object_ref (window);

  if (attributes_mask & GDK_WA_TYPE_HINT)
	{
		win_flags |= window_type_hint_to_level(attributes->type_hint);
		//g_print("try a menu window %d %x,%x\n", 
		//	attributes->type_hint,GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,win_flags);
	}

  switch (window->window_type)
    {
    case GDK_WINDOW_TOPLEVEL:
      if (GDK_WINDOW_TYPE (window->parent) != GDK_WINDOW_ROOT)
	  {
	    /* The common code warns for this case. */
	    xparent = GI_DESKTOP_WINDOW_ID;
		g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
	  }
      /* Children of foreign windows aren't toplevel windows */
      if (GDK_WINDOW_TYPE (real_parent) == GDK_WINDOW_FOREIGN)
	  {
		  g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
	    //win_flags = WS_CHILDWINDOW | WS_CLIPCHILDREN;
	  }
      else
	  {
	    if (window->window_type == GDK_WINDOW_TOPLEVEL)
		  {
			g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
		  }
		  else{
			  g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
		  }
	    //  win_flags = 0;//GI_FLAGS_NON_FRAME;//WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	    //else
	    //  win_flags = 0;//WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU
			//| WS_CAPTION | WS_THICKFRAME | WS_CLIPCHILDREN;
	  }
      break;

    case GDK_WINDOW_CHILD:
      win_flags |= GI_FLAGS_NON_FRAME;//WS_CHILDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      break;

    case GDK_WINDOW_TEMP:
		g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
      /* A temp window is not necessarily a top level window */
      win_flags |= (_gdk_root == real_parent ? GI_FLAGS_TOPMOST_WINDOW|GI_FLAGS_NON_FRAME : 0);
      win_flags |= GI_FLAGS_TEMP_WINDOW  ;
      //dwExStyle |= WS_EX_TOOLWINDOW;
      //offset_x = _gdk_offset_x;
      //offset_y = _gdk_offset_y;
      break;

    default:
      g_assert_not_reached ();
    }



  if (window->window_type != GDK_WINDOW_CHILD)
    {
      x = window->x  ;
      y = window->y  ;

      /* non child windows are placed by the OS/window manager */
      //x = y = CW_USEDEFAULT;

      window_width = window->width;
      window_height = window->height;
    }
  else
    {
      /* adjust position relative to real_parent */
      window_width = window->width;
      window_height = window->height;
      /* use given position for initial placement, native coordinates */
      x = window->x + window->parent->abs_x ;
      y = window->y + window->parent->abs_y ;
    }

	if (attributes_mask & GDK_WA_TITLE)
	  title = attributes->title;
	else
	  title = get_default_title ();
	if (!title || !*title)
	  title = "";

	mainWin = gi_create_window(xparent, x,y,
		window_width, window_height,
		GI_RGB( 192, 192, 192 ),win_flags|GI_FLAGS_DOUBLE_BUFFER);

	if (mainWin <= 0)
	{
	  _gdk_window_destroy (window, FALSE);
	  return NULL;
	}
	impl->window_id = mainWin;
	
	//impl->drawable.format = gi_screen_format();

	g_object_ref (window);
	_gdk_gix_display_add_window (gix_screen->scr_display, &impl->window_id, window);

  if (attributes_mask & GDK_WA_TYPE_HINT)
    gdk_window_set_type_hint (window, attributes->type_hint);

  gdk_gix_event_source_select_events ((GdkEventSource *) display_gix->event_source,
		  GDK_WINDOW_XID (window), event_mask,
		  GI_MASK_CONFIGURENOTIFY | GI_MASK_PROPERTYNOTIFY);

  /* Add window handle to title */
  //GDK_NOTE (MISC_OR_EVENTS, );
  //gdk_window_set_title (window, title);
  gi_set_window_utf8_caption(mainWin, title);

  if (attributes_mask & GDK_WA_CURSOR)
	gdk_window_set_cursor (window, attributes->cursor);
}

static const cairo_user_data_key_t gdk_gix_cairo_key;

typedef struct _GdkGixCairoSurfaceData {  
  GdkDisplayGix *display;
  int32_t width, height;

} GdkGixCairoSurfaceData;


static void
gdk_window_impl_gix_finalize (GObject *object)
{
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW_IMPL_GIX (object));

  impl = GDK_WINDOW_IMPL_GIX (object);

  if (impl->cursor)
    gdk_cursor_unref (impl->cursor);
 
  G_OBJECT_CLASS (_gdk_window_impl_gix_parent_class)->finalize (object);
}

static void
gdk_gix_cairo_surface_destroy (void *data)
{
  GdkWindowImplGix *impl = data;

  impl->cairo_surface = NULL;
}

static cairo_surface_t *
gdk_gix_create_cairo_surface (GdkDisplayGix *display,
				  int width, int height, gi_window_id_t window_id)
{
  GdkGixCairoSurfaceData *data;
  cairo_surface_t *surface;

  data = g_new (GdkGixCairoSurfaceData, 1);
  data->display = display;
  data->width = width;
  data->height = height;

  surface = cairo_gix_surface_create(window_id);

  if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
    fprintf (stderr, "create gix surface failed\n");

  cairo_surface_set_user_data (surface, &gdk_gix_cairo_key,
			       data, gdk_gix_cairo_surface_destroy);


  return surface;
}

static cairo_surface_t *
gdk_gix_window_ref_cairo_surface (GdkWindow *window)
{
  GdkWindowImplGix *impl = GDK_WINDOW_IMPL_GIX (window->impl);
  GdkDisplayGix *display_gix =
    GDK_DISPLAY_GIX (gdk_window_get_display (impl->wrapper));

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (impl->wrapper))
    return NULL;

  if (!impl->cairo_surface)
    {
      impl->cairo_surface =
	    gdk_gix_create_cairo_surface (display_gix,
				      impl->wrapper->width,
				      impl->wrapper->height,impl->window_id);

	  if(impl->cairo_surface)
	    cairo_surface_set_user_data (impl->cairo_surface, &gdk_gix_cairo_key,
			       impl, gdk_gix_cairo_surface_destroy);
    }

  cairo_surface_reference (impl->cairo_surface);
  return impl->cairo_surface;
}

static void
gdk_gix_window_set_user_time (GdkWindow *window, guint32 user_time)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (window->destroyed)
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_show (GdkWindow *window, gboolean already_mapped)
{
  GdkDisplay *display;
  GdkDisplayGix *display_gix;
  GdkWindowImplGix *impl = GDK_WINDOW_IMPL_GIX (window->impl);

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (window->destroyed)
    return;

  display = gdk_window_get_display (window);
  display_gix = GDK_DISPLAY_GIX (display);

  //if (impl->user_time != 0 &&
  //    display_gix->user_time != 0 &&
   //   XSERVER_TIME_IS_LATER (display_gix->user_time, impl->user_time))
   // gdk_gix_window_set_user_time (window, impl->user_time);

  gi_show_window(impl->window_id);// = wl_compositor_create_surface(display_gix->compositor);
  //wl_surface_set_user_data(impl->window_id, window);

  //_gdk_make_event (window, GDK_MAP, NULL, FALSE);
  //event = _gdk_make_event (window, GDK_VISIBILITY_NOTIFY, NULL, FALSE);
  //event->visibility.state = GDK_VISIBILITY_UNOBSCURED;

  //if (impl->cairo_surface)
    //gdk_gix_window_attach_image (window);
}

static void
gdk_gix_window_hide (GdkWindow *window)
{
  GdkWindowImplGix *impl = GDK_WINDOW_IMPL_GIX (window->impl);

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (window->destroyed)
    return;

  if (impl->window_id)
    {
      gi_hide_window(impl->window_id);
      //impl->mapped = FALSE;
    }

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_window_gix_withdraw (GdkWindow *window)
{
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (window->destroyed)
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);

  //GDK_NOTE (MISC, g_print ("gdk_window_gix_withdraw: %p\n",
	//		   GDK_WINDOW_WIN_ID (impl));

  gdk_window_hide (window);	/* ??? */

}

static void
gdk_window_gix_set_events (GdkWindow    *window,
			       GdkEventMask  event_mask)
{
  long xevent_mask = 0;
   GdkDisplayGix *display_gix;
 
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
	  return;

  GDK_WINDOW (window)->event_mask = event_mask;

  //if (GDK_WINDOW_XID (window) != GDK_WINDOW_XROOTWIN (window))
    //    xevent_mask = StructureNotifyMask | PropertyChangeMask;

  display_gix = GDK_DISPLAY_GIX (gdk_window_get_display (window));
  gdk_gix_event_source_select_events ((GdkEventSource *) display_gix->event_source,
	  GDK_WINDOW_XID (window), event_mask,
	  xevent_mask);	
}



static GdkEventMask
gdk_window_gix_get_events (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return 0;
  else
    return GDK_WINDOW (window)->event_mask;
}

static void
gdk_window_gix_raise (GdkWindow *window)
{
  /* FIXME: wl_shell_raise() */
  gi_raise_window ( GDK_WINDOW_WIN_ID (window));
}

static void
gdk_window_gix_lower (GdkWindow *window)
{
  gi_lower_window ( GDK_WINDOW_WIN_ID (window));
}

static void
gdk_window_gix_restack_under (GdkWindow *window,
			      GList *native_siblings)
{
	// ### TODO
}

static void
gdk_window_gix_restack_toplevel (GdkWindow *window,
				 GdkWindow *sibling,
				 gboolean   above)
{
	// ### TODO
}

static void
gdk_window_gix_move_resize (GdkWindow *window,
				gboolean   with_move,
				gint       x,
				gint       y,
				gint       width,
				gint       height)
{
  window->x = x;
  window->y = y;

  if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD){
  }

  if(with_move)
    gi_move_window(GDK_WINDOW_WIN_ID (window),x,y);

  gi_resize_window(GDK_WINDOW_WIN_ID (window),width,height);

  //_gdk_gix_window_update_size (window, width, height, 0);
}

static void
gdk_window_gix_set_background (GdkWindow      *window,
			       cairo_pattern_t *pattern)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static gboolean
gdk_window_gix_reparent (GdkWindow *window,
			     GdkWindow *new_parent,
			     gint       x,
			     gint       y)
{

  GdkWindowImplGix *impl;

  impl = GDK_WINDOW_IMPL_GIX (window->impl);

  //_gdk_gix_window_tmp_unset_bg (window, TRUE);
  //_gdk_gix_window_tmp_unset_parent_bg (window);
  if (!new_parent)
    new_parent = _gdk_root;

  gi_reparent_window (
		   GDK_WINDOW_XID (window),
		   GDK_WINDOW_XID (new_parent),
		   new_parent->abs_x + x, new_parent->abs_y + y);
  //_gdk_gix_window_tmp_reset_parent_bg (window);
  //_gdk_gix_window_tmp_reset_bg (window, TRUE);
  window->parent = new_parent;	  

  switch (GDK_WINDOW_TYPE (new_parent))
    {
    case GDK_WINDOW_ROOT:
      if (impl->toplevel_window_type != -1)
	GDK_WINDOW_TYPE (window) = impl->toplevel_window_type;
      else if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD)
	GDK_WINDOW_TYPE (window) = GDK_WINDOW_TOPLEVEL;
      break;

    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_CHILD:
    case GDK_WINDOW_TEMP:
      if (WINDOW_IS_TOPLEVEL (window))
	{
	  /* Save the original window type so we can restore it if the
	   * window is reparented back to be a toplevel.
	   */
	  impl->toplevel_window_type = GDK_WINDOW_TYPE (window);
	  GDK_WINDOW_TYPE (window) = GDK_WINDOW_CHILD;
	}
    }

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return FALSE;
}

static void
gdk_window_gix_set_device_cursor (GdkWindow *window,
				      GdkDevice *device,
				      GdkCursor *cursor)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_DEVICE (device));

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (!GDK_WINDOW_DESTROYED (window))
    GDK_DEVICE_GET_CLASS (device)->set_window_cursor (device, window, cursor);
}

static void
gdk_window_gix_get_geometry (GdkWindow *window,
				 gint      *x,
				 gint      *y,
				 gint      *width,
				 gint      *height)
{
  gi_window_info_t info;

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (!GDK_WINDOW_DESTROYED (window))
    {
	  gi_get_window_info(GDK_WINDOW_XID (window), &info);
      if (x)
	*x = info.x;
      if (y)
	*y = info.y;
      if (width)
	*width = info.width;
      if (height)
	*height = info.height;
    }
}

static gint
gdk_window_gix_get_root_coords (GdkWindow *window,
				gint       x,
				gint       y,
				gint      *root_x,
				gint      *root_y)
{
	gi_window_info_t info;

	gi_get_window_info(GDK_WINDOW_XID (window), &info);
  /* We can't do this. */
  if (root_x)
    *root_x = info.x + x;
  if (root_y)
    *root_y = info.y + y;

  return 1;
}

static gboolean
gdk_window_gix_get_device_state (GdkWindow       *window,
				     GdkDevice       *device,
				     gint            *x,
				     gint            *y,
				     GdkModifierType *mask)
{
  gboolean return_val;

  g_return_val_if_fail (window == NULL || GDK_IS_WINDOW (window), FALSE);

  return_val = TRUE;

  if (!GDK_WINDOW_DESTROYED (window))
    {
      GdkWindow *child;

      GDK_DEVICE_GET_CLASS (device)->query_state (device, window,
						  NULL, &child,
						  NULL, NULL,
						  x, y, mask);
      return_val = (child != NULL);
    }

  return return_val;
}

static void
gdk_window_gix_shape_combine_region (GdkWindow       *window,
					 const cairo_region_t *shape_region,
					 gint             offset_x,
					 gint             offset_y)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void 
gdk_window_gix_input_shape_combine_region (GdkWindow       *window,
					       const cairo_region_t *shape_region,
					       gint             offset_x,
					       gint             offset_y)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static gboolean
gdk_window_gix_set_static_gravities (GdkWindow *window,
					 gboolean   use_static)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  return !use_static;
}

static gboolean
gdk_gix_window_queue_antiexpose (GdkWindow *window,
				     cairo_region_t *area)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);
#if 0
  GdkWindowQueueItem *item = g_new (GdkWindowQueueItem, 1);
  item->type = GDK_WINDOW_QUEUE_ANTIEXPOSE;
  item->u.antiexpose.area = area;

  gdk_window_queue (window, item);

  return TRUE;
#endif
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  //return FALSE;
}

static void
gdk_gix_window_translate (GdkWindow      *window,
			      cairo_region_t *area,
			      gint            dx,
			      gint            dy)
{
  cairo_surface_t *surface;
  cairo_t *cr;

  surface = gdk_gix_window_ref_cairo_surface (window->impl_window);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface);

  gdk_cairo_region (cr, area);
  cairo_clip (cr);
  cairo_set_source_surface (cr, cairo_get_target (cr), dx, dy);
  cairo_push_group (cr);
  cairo_paint (cr);
  cairo_pop_group_to_source (cr);
  cairo_paint (cr);
  cairo_destroy (cr);
}

static void
gdk_gix_window_destroy (GdkWindow *window,
			    gboolean   recursing,
			    gboolean   foreign_destroy)
{
  GdkWindowImplGix *impl = GDK_WINDOW_IMPL_GIX (window->impl);

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (impl->cairo_surface)
    {
      cairo_surface_finish (impl->cairo_surface);
      cairo_surface_set_user_data (impl->cairo_surface, &gdk_gix_cairo_key,
				   NULL, NULL);
    }

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);


  if (!recursing && !foreign_destroy)
    {
      if (GDK_WINDOW_IMPL_GIX (window->impl)->window_id)
	    gi_destroy_window(GDK_WINDOW_IMPL_GIX (window->impl)->window_id);
    }
  _gdk_gix_display_remove_window (GDK_WINDOW_DISPLAY (window), impl->window_id);
}

static void
gdk_window_gix_destroy_foreign (GdkWindow *window)
{
  gdk_window_hide (window);
  gdk_window_reparent (window, NULL, 0, 0);

  gi_post_quit_message(GDK_WINDOW_XID (window));
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  
  //PostMessage (GDK_WINDOW_HWND (window), WM_CLOSE, 0, 0);
}

static cairo_surface_t *
gdk_window_gix_resize_cairo_surface (GdkWindow       *window,
					 cairo_surface_t *surface,
					 gint             width,
					 gint             height)
{
  cairo_surface_destroy (surface);

	//cairo_xlib_surface_set_size (surface, width, height);
  //return surface;
  return NULL;
}

static cairo_region_t *
gdk_gix_window_get_shape (GdkWindow *window)
{
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return NULL;
}

static cairo_region_t *
gdk_gix_window_get_input_shape (GdkWindow *window)
{
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return NULL;
}

static void
gdk_gix_window_focus (GdkWindow *window,
			  guint32    timestamp)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;

  gi_raise_window ( GDK_WINDOW_XID (window));
  gi_set_focus_window ( GDK_WINDOW_XID (window));
  /* FIXME: wl_shell_focus() */
}

static void
gdk_gix_window_set_type_hint (GdkWindow        *window,
				  GdkWindowTypeHint hint)
{
  GdkWindowImplGix *impl;
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_WINDOW_IMPL_GIX (window->impl);

  impl->type_hint = hint;

  switch (hint)
    {
    case GDK_WINDOW_TYPE_HINT_DIALOG:
    case GDK_WINDOW_TYPE_HINT_MENU:
    case GDK_WINDOW_TYPE_HINT_TOOLBAR:
    case GDK_WINDOW_TYPE_HINT_UTILITY:
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
    case GDK_WINDOW_TYPE_HINT_DOCK:
    case GDK_WINDOW_TYPE_HINT_DESKTOP:
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU:
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
    case GDK_WINDOW_TYPE_HINT_COMBO:
    case GDK_WINDOW_TYPE_HINT_DND:
      break;
    default:
      g_warning ("Unknown hint %d passed to gdk_window_set_type_hint", hint);
      /* Fall thru */
    case GDK_WINDOW_TYPE_HINT_NORMAL:
      break;
    }
}

static GdkWindowTypeHint
gdk_gix_window_get_type_hint (GdkWindow *window)
{
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return GDK_WINDOW_TYPE_HINT_NORMAL;

  impl = GDK_WINDOW_IMPL_GIX (window->impl);

  return impl->type_hint;
}

void
gdk_gix_window_set_modal_hint (GdkWindow *window,
				   gboolean   modal)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_skip_taskbar_hint (GdkWindow *window,
					  gboolean   skips_taskbar)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_skip_pager_hint (GdkWindow *window,
					gboolean   skips_pager)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_urgency_hint (GdkWindow *window,
				     gboolean   urgent)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_geometry_hints (GdkWindow         *window,
				       const GdkGeometry *geometry,
				       GdkWindowHints     geom_mask)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

 
  if (geom_mask & GDK_HINT_POS)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & GDK_HINT_USER_POS)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & GDK_HINT_USER_SIZE)
    {
      /* FIXME: Implement */
    }
  
  if (geom_mask & GDK_HINT_MIN_SIZE)
    {
      //NSSize size;

      //size.width = geometry->min_width;
      //size.height = geometry->min_height;

      //[impl->toplevel setContentMinSize:size];
    }
  
  if (geom_mask & GDK_HINT_MAX_SIZE)
    {
      //NSSize size;

      //size.width = geometry->max_width;
      //size.height = geometry->max_height;

      //[impl->toplevel setContentMaxSize:size];
    }
  
  if (geom_mask & GDK_HINT_BASE_SIZE)
    {
      /* FIXME: Implement */
    }
  
  if (geom_mask & GDK_HINT_RESIZE_INC)
    {
      //NSSize size;

      //size.width = geometry->width_inc;
      //size.height = geometry->height_inc;

      //[impl->toplevel setContentResizeIncrements:size];
    }
  
  if (geom_mask & GDK_HINT_ASPECT)
    {
      /* FIXME: Implement */
    }

  if (geom_mask & GDK_HINT_WIN_GRAVITY)
    {
      /* FIXME: Implement */
    }
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_title (GdkWindow   *window,
			      const gchar *title)
{
  g_return_if_fail (title != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);

  gi_set_window_utf8_caption(GDK_WINDOW_XID (window), title);
}

static void
gdk_gix_window_set_role (GdkWindow   *window,
			     const gchar *role)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window) ||
      WINDOW_IS_TOPLEVEL (window))
    return;

  /* FIXME: Implement */
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_startup_id (GdkWindow   *window,
				   const gchar *startup_id)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_transient_for (GdkWindow *window,
				      GdkWindow *parent)
{
  gi_window_id_t window_id, parent_id;
  GdkWindowImplGix *window_impl = GDK_WINDOW_IMPL_GIX (window->impl);
  GdkWindowImplGix *parent_impl = NULL;
  GSList *item;

  g_return_if_fail (GDK_IS_WINDOW (window));

  window_id = GDK_WINDOW_XID (window);
  parent_id = parent != NULL ? GDK_WINDOW_XID (parent) : NULL;

  GDK_NOTE (MISC, g_print ("gdk_window_set_transient_for: %p: %p\n", window_id, parent_id));

  if (GDK_WINDOW_DESTROYED (window) || (parent && GDK_WINDOW_DESTROYED (parent)))
    {
      if (GDK_WINDOW_DESTROYED (window))
	GDK_NOTE (MISC, g_print ("... destroyed!\n"));
      else
	GDK_NOTE (MISC, g_print ("... owner destroyed!\n"));

      return;
    }

  if (window->window_type == GDK_WINDOW_CHILD)
    {
      GDK_NOTE (MISC, g_print ("... a child window!\n"));
      return;
    }

  //impl = GDK_WINDOW_IMPL_GIX (window->impl);
  if (!parent)
  {
	  g_print("%s: got line %d\n",__FUNCTION__,__LINE__);
    window_impl->transient_for = NULL;
  }
  else{
    window_impl->transient_for = parent;
  }
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);

}

static void
gdk_gix_window_get_root_origin (GdkWindow *window,
				   gint      *x,
				   gint      *y)
{
  GdkRectangle rect;

  rect.x = 0;
  rect.y = 0;
  
  gdk_window_get_frame_extents (window, &rect);

  if (x)
    *x = rect.x;

  if (y)
    *y = rect.y;
}

//FIXME
static void
gdk_gix_window_get_frame_extents (GdkWindow    *window,
				      GdkRectangle *rect)
{
  gi_window_id_t hwnd;
  gi_window_info_t info;
  int err;

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (rect != NULL);

  rect->x = 0;
  rect->y = 0;
  rect->width = 1;
  rect->height = 1;
  
  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* FIXME: window is documented to be a toplevel GdkWindow, so is it really
   * necessary to walk its parent chain?
   */
  while (window->parent && window->parent->parent)
    window = window->parent;

  hwnd = GDK_WINDOW_XID (window);
  err = gi_get_window_info(hwnd, &info);
  //API_CALL (GetWindowRect, (hwnd, &r));

  rect->x = info.x;
  rect->y = info.y;
  rect->width = info.width;
  rect->height = info.height;

  if (err)
  {
  g_print ("gdk_window_get_frame_extents: failed\n");
  }
 

}

static void
gdk_gix_window_set_override_redirect (GdkWindow *window,
					  gboolean override_redirect)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_accept_focus (GdkWindow *window,
				     gboolean accept_focus)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  accept_focus = accept_focus != FALSE;

  if (window->accept_focus != accept_focus)
    window->accept_focus = accept_focus;
}

static void
gdk_gix_window_set_focus_on_map (GdkWindow *window,
				     gboolean focus_on_map)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  focus_on_map = focus_on_map != FALSE;

  if (window->focus_on_map != focus_on_map)
    window->focus_on_map = focus_on_map;
}

static void
gdk_gix_window_set_icon_list (GdkWindow *window,
				  GList     *pixbufs)
{
  gulong *data;
  guchar *pixels;
  gulong *p;
  gint size;
  GList *l;
  GdkPixbuf *pixbuf;
  gint width, height, stride;
  gint x, y;
  gint n_channels;
  GdkDisplay *display;
  gint n;
  GdkPixbuf *pixbuf_small = NULL;

   g_return_if_fail (GDK_IS_WINDOW (window));
 

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  display = gdk_window_get_display (window);
  
  l = pixbufs;
  size = 0;
  n = 0;
  while (l)
    {
      pixbuf = l->data;
      g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

      width = gdk_pixbuf_get_width (pixbuf);
      height = gdk_pixbuf_get_height (pixbuf);
      
      /* silently ignore overlarge icons */
      //if (size + 2 + width * height > GDK_SELECTION_MAX_SIZE(display))
	{
	 // g_warning ("gdk_window_set_icon_list: icons too large");
	 // break;
	}

	if (width<=22 && height<=22) //just small icon
	{
		pixbuf_small = pixbuf;
		break;
	}
     
      n++;
      size += 2 + width * height;
      
      l = g_list_next (l);
    }

	if (!pixbuf_small)
		return;

  data = g_malloc (size * sizeof (gulong));

  l = pixbufs;
  p = data;
  while (l && n > 0)
    {
      pixbuf = l->data;

	  if (pixbuf != pixbuf_small)
		  goto NEXT;
      
      width = gdk_pixbuf_get_width (pixbuf);
      height = gdk_pixbuf_get_height (pixbuf);
      stride = gdk_pixbuf_get_rowstride (pixbuf);
      n_channels = gdk_pixbuf_get_n_channels (pixbuf);
      
      *p++ = width;
      *p++ = height;

      pixels = gdk_pixbuf_get_pixels (pixbuf);

      for (y = 0; y < height; y++)
	{
	  for (x = 0; x < width; x++)
	    {
	      guchar r, g, b, a;
	      
	      r = pixels[y*stride + x*n_channels + 0];
	      g = pixels[y*stride + x*n_channels + 1];
	      b = pixels[y*stride + x*n_channels + 2];
	      if (n_channels >= 4)
		a = pixels[y*stride + x*n_channels + 3];
	      else
		a = 255;
	      
	      *p++ = a << 24 | r << 16 | g << 8 | b ;
	    }
	}

NEXT:
      l = g_list_next (l);
      n--;
    }

  if (size > 0)
    {
	  gi_set_window_icon(GDK_WINDOW_XID (window),(guchar*) data,
		  gdk_pixbuf_get_width(pixbuf_small),gdk_pixbuf_get_height(pixbuf_small));
    }
  else
    {
      gi_delete_property (
               GDK_WINDOW_XID (window),
		       gi_intern_atom ( "_NET_WM_ICON",FALSE));
    }
  
  g_free (data);

  //gdk_window_update_icon (window, pixbufs);

}

static void
gdk_gix_window_set_icon_name (GdkWindow   *window,
				  const gchar *name)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  

  //gi_set_window_caption();
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_iconify (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_WINDOW_IS_MAPPED (window))
	gi_iconify_window(GDK_WINDOW_XID(window));
  else{
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  }
}

static void
gdk_gix_window_deiconify (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  if (GDK_WINDOW_IS_MAPPED (window))
    {  
      gdk_window_show (window);
    }
  else
    {
      /* Flip our client side flag, the real work happens on map. */
      gdk_synthesize_window_state (window, GDK_WINDOW_STATE_ICONIFIED, 0);
    }
}

static void
gdk_gix_window_stick (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_unstick (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_WINDOW_DESTROYED (window))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_maximize (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_wm_window_maximize(GDK_WINDOW_XID(window), TRUE);
}

static void
gdk_gix_window_unmaximize (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_wm_window_maximize(GDK_WINDOW_XID(window), FALSE);
}

static void
gdk_gix_window_fullscreen (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_fullscreen_window(GDK_WINDOW_XID(window), TRUE);
}

static void
gdk_gix_window_unfullscreen (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_fullscreen_window(GDK_WINDOW_XID(window), FALSE);
}

static void
gdk_gix_window_set_keep_above (GdkWindow *window,
				   gboolean   setting)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_set_window_keep_above(GDK_WINDOW_XID(window), setting);
}

static void
gdk_gix_window_set_keep_below (GdkWindow *window, gboolean setting)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  gi_set_window_keep_below(GDK_WINDOW_XID(window), setting);
}

static GdkWindow *
gdk_gix_window_get_group (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL (window))
    return NULL;

  return NULL;
}

static void
gdk_gix_window_set_group (GdkWindow *window,
			      GdkWindow *leader)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD);
  g_return_if_fail (leader == NULL || GDK_IS_WINDOW (leader));

  if (GDK_WINDOW_DESTROYED (window) || GDK_WINDOW_DESTROYED (leader))
    return;
  
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_decorations (GdkWindow      *window,
				    GdkWMDecoration decorations)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
	g_print("gdk_gix_window_set_decorations called\n");

	GDK_NOTE (MISC, g_print ("gdk_window_set_decorations: %p: %s %s%s%s%s%s%s\n",
			   GDK_WINDOW_XID (window),
			   (decorations & GDK_DECOR_ALL ? "clearing" : "setting"),
			   (decorations & GDK_DECOR_BORDER ? "BORDER " : ""),
			   (decorations & GDK_DECOR_RESIZEH ? "RESIZEH " : ""),
			   (decorations & GDK_DECOR_TITLE ? "TITLE " : ""),
			   (decorations & GDK_DECOR_MENU ? "MENU " : ""),
			   (decorations & GDK_DECOR_MINIMIZE ? "MINIMIZE " : ""),

	decorations_copy = g_malloc (sizeof (GdkWMDecoration));
	*decorations_copy = decorations;
	g_object_set_qdata_full (G_OBJECT (window), get_decorations_quark (), decorations_copy, g_free);
	(decorations & GDK_DECOR_MAXIMIZE ? "MAXIMIZE " : "")));

/*
 
  memset(&hints, 0, sizeof(hints));
  hints.flags = MWM_HINTS_DECORATIONS;
  hints.decorations = decorations;
  
  gdk_window_set_mwm_hints (window, &hints);
  */
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);

}

static gboolean
gdk_gix_window_get_decorations (GdkWindow       *window,
				    GdkWMDecoration *decorations)
{
  GdkWMDecoration* decorations_set;
  
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  decorations_set = g_object_get_qdata (G_OBJECT (window), get_decorations_quark ());
  if (decorations_set)
    *decorations = *decorations_set;

  return (decorations_set != NULL);
}

static void
gdk_gix_window_set_functions (GdkWindow    *window,
				  GdkWMFunction functions)
{
  GdkWMFunction* functions_copy;

  g_return_if_fail (GDK_IS_WINDOW (window));
  
  GDK_NOTE (MISC, g_print ("gdk_window_set_functions: %p: %s %s%s%s%s%s\n",
			   GDK_WINDOW_HWND (window),
			   (functions & GDK_FUNC_ALL ? "clearing" : "setting"),
			   (functions & GDK_FUNC_RESIZE ? "RESIZE " : ""),
			   (functions & GDK_FUNC_MOVE ? "MOVE " : ""),
			   (functions & GDK_FUNC_MINIMIZE ? "MINIMIZE " : ""),
			   (functions & GDK_FUNC_MAXIMIZE ? "MAXIMIZE " : ""),
			   (functions & GDK_FUNC_CLOSE ? "CLOSE " : "")));

  functions_copy = g_malloc (sizeof (GdkWMFunction));
  *functions_copy = functions;
  g_object_set_qdata_full (G_OBJECT (window), get_functions_quark (), functions_copy, g_free);

  //update_system_menu (window);
}


gboolean
_gdk_window_get_functions (GdkWindow     *window,
		           GdkWMFunction *functions)
{
  GdkWMDecoration* functions_set;
  
  functions_set = g_object_get_qdata (G_OBJECT (window), get_functions_quark ());
  if (functions_set)
    *functions = *functions_set;

  return (functions_set != NULL);
}

static void
gdk_gix_window_begin_resize_drag (GdkWindow     *window,
				      GdkWindowEdge  edge,
				      gint           button,
				      gint           root_x,
				      gint           root_y,
				      guint32        timestamp)
{
  GdkDisplay *display = gdk_window_get_display (window);
  GdkDeviceManager *dm;
  GdkWindowImplGix *impl;
  GdkDevice *device;
  uint32_t grab_type;
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window) ||
      !WINDOW_IS_TOPLEVEL_OR_FOREIGN (window))
    return;

  switch (edge)
    {
    case GDK_WINDOW_EDGE_NORTH_WEST:
      //grab_type = WL_SHELL_RESIZE_TOP_LEFT;
      break;

    case GDK_WINDOW_EDGE_NORTH:
     // grab_type = WL_SHELL_RESIZE_TOP;
      break;

    case GDK_WINDOW_EDGE_NORTH_EAST:
     // grab_type = WL_SHELL_RESIZE_RIGHT;
      break;

    case GDK_WINDOW_EDGE_WEST:
      //grab_type = WL_SHELL_RESIZE_LEFT;
      break;

    case GDK_WINDOW_EDGE_EAST:
      //grab_type = WL_SHELL_RESIZE_RIGHT;
      break;

    case GDK_WINDOW_EDGE_SOUTH_WEST:
      //grab_type = WL_SHELL_RESIZE_BOTTOM_LEFT;
      break;

    case GDK_WINDOW_EDGE_SOUTH:
      //grab_type = WL_SHELL_RESIZE_BOTTOM;
      break;

    case GDK_WINDOW_EDGE_SOUTH_EAST:
      //grab_type = WL_SHELL_RESIZE_BOTTOM_RIGHT;
      break;

    default:
      g_warning ("gdk_window_begin_resize_drag: bad resize edge %d!",
                 edge);
      return;
    }

  impl = GDK_WINDOW_IMPL_GIX (window->impl);
  dm = gdk_display_get_device_manager (display);
  device = gdk_device_manager_get_client_pointer (dm);
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);

  //wl_shell_resize(GDK_DISPLAY_GIX (display)->shell, impl->window_id,
	//	  _gdk_gix_device_get_device (device),
	//	  timestamp, grab_type);
}

static void
gdk_gix_window_begin_move_drag (GdkWindow *window,
				    gint       button,
				    gint       root_x,
				    gint       root_y,
				    guint32    timestamp)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  
  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* Tell Windows to start interactively moving the window by pretending that
   * the left pointer button was clicked in the titlebar. This will only work
   * if the button is down when this function is called, and will only work
   * with button 1 (left), since Windows only allows window dragging using the
   * left mouse button.
   */
  if (button != 1)
    return;
  
  /* Must break the automatic grab that occured when the button was pressed,
   * otherwise it won't work.
   */
  gdk_display_pointer_ungrab (_gdk_display, 0);

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  //DefWindowProcW (GDK_WINDOW_HWND (window), WM_NCLBUTTONDOWN, HTCAPTION,
	//	  MAKELPARAM (root_x - _gdk_offset_x, root_y - _gdk_offset_y));
}

static void
gdk_gix_window_enable_synchronized_configure (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_configure_finished (GdkWindow *window)
{
  if (!WINDOW_IS_TOPLEVEL (window))
    return;

  if (!GDK_IS_WINDOW_IMPL_GIX (window->impl))
    return;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_set_opacity (GdkWindow *window,
				gdouble    opacity)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
	//gi_set_window_opacity()
}

static void
gdk_gix_window_set_composited (GdkWindow *window,
				   gboolean   composited)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_destroy_notify (GdkWindow *window)
{
  if (!GDK_WINDOW_DESTROYED (window))
    {
      if (GDK_WINDOW_TYPE(window) != GDK_WINDOW_FOREIGN)
	g_warning ("GdkWindow %p unexpectedly destroyed", window);

    _gdk_gix_display_remove_window (GDK_WINDOW_DISPLAY (window), 
		GDK_WINDOW_XID (window) );
      _gdk_window_destroy (window, TRUE);
    }

  g_object_unref (window);
}

static void
gdk_gix_window_process_updates_recurse (GdkWindow *window,
					    cairo_region_t *region)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;

  _gdk_window_process_updates_recurse (window, region);
}

static void
gdk_gix_window_sync_rendering (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return ;
}

static gboolean
gdk_gix_window_simulate_key (GdkWindow      *window,
				 gint            x,
				 gint            y,
				 guint           keyval,
				 GdkModifierType modifiers,
				 GdkEventType    key_pressrelease)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window),FALSE);
  if (GDK_WINDOW_DESTROYED (window))
    return FALSE;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return FALSE;
}

static gboolean
gdk_gix_window_simulate_button (GdkWindow      *window,
				    gint            x,
				    gint            y,
				    guint           button, /*1..3*/
				    GdkModifierType modifiers,
				    GdkEventType    button_pressrelease)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window),FALSE);
  if (GDK_WINDOW_DESTROYED (window))
    return FALSE;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return FALSE;
}

static gboolean
gdk_gix_window_get_property (GdkWindow   *window,
				 GdkAtom      property,
				 GdkAtom      type,
				 gulong       offset,
				 gulong       length,
				 gint         pdelete,
				 GdkAtom     *actual_property_type,
				 gint        *actual_format_type,
				 gint        *actual_length,
				 guchar     **data)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window),FALSE);
  if (GDK_WINDOW_DESTROYED (window))
    return FALSE;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
  return FALSE;
}

static void
gdk_gix_window_change_property (GdkWindow    *window,
				    GdkAtom       property,
				    GdkAtom       type,
				    gint          format,
				    GdkPropMode   mode,
				    const guchar *data,
				    gint          nelements)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;

  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
gdk_gix_window_delete_property (GdkWindow *window,
				    GdkAtom    property)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return ;
  g_warning ("implemented %s: GOT line %d\n",__FUNCTION__,__LINE__);
}

static void
_gdk_window_impl_gix_class_init (GdkWindowImplGixClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkWindowImplClass *impl_class = GDK_WINDOW_IMPL_CLASS (klass);

  object_class->finalize = gdk_window_impl_gix_finalize;

  impl_class->ref_cairo_surface = gdk_gix_window_ref_cairo_surface;
  impl_class->show = gdk_gix_window_show;
  impl_class->hide = gdk_gix_window_hide;
  impl_class->withdraw = gdk_window_gix_withdraw;
  impl_class->set_events = gdk_window_gix_set_events;
  impl_class->get_events = gdk_window_gix_get_events;
  impl_class->raise = gdk_window_gix_raise;
  impl_class->lower = gdk_window_gix_lower;
  impl_class->restack_under = gdk_window_gix_restack_under;
  impl_class->restack_toplevel = gdk_window_gix_restack_toplevel;
  impl_class->move_resize = gdk_window_gix_move_resize;
  impl_class->set_background = gdk_window_gix_set_background;
  impl_class->reparent = gdk_window_gix_reparent;
  impl_class->set_device_cursor = gdk_window_gix_set_device_cursor;
  impl_class->get_geometry = gdk_window_gix_get_geometry;
  impl_class->get_root_coords = gdk_window_gix_get_root_coords;
  impl_class->get_device_state = gdk_window_gix_get_device_state;
  impl_class->shape_combine_region = gdk_window_gix_shape_combine_region;
  impl_class->input_shape_combine_region = gdk_window_gix_input_shape_combine_region;
  impl_class->set_static_gravities = gdk_window_gix_set_static_gravities;
  impl_class->queue_antiexpose = gdk_gix_window_queue_antiexpose;
  impl_class->translate = gdk_gix_window_translate;
  impl_class->destroy = gdk_gix_window_destroy;
  impl_class->destroy_foreign = gdk_window_gix_destroy_foreign;
  impl_class->resize_cairo_surface = gdk_window_gix_resize_cairo_surface;
  impl_class->get_shape = gdk_gix_window_get_shape;
  impl_class->get_input_shape = gdk_gix_window_get_input_shape;
  /* impl_class->beep */

  impl_class->focus = gdk_gix_window_focus;
  impl_class->set_type_hint = gdk_gix_window_set_type_hint;
  impl_class->get_type_hint = gdk_gix_window_get_type_hint;
  impl_class->set_modal_hint = gdk_gix_window_set_modal_hint;
  impl_class->set_skip_taskbar_hint = gdk_gix_window_set_skip_taskbar_hint;
  impl_class->set_skip_pager_hint = gdk_gix_window_set_skip_pager_hint;
  impl_class->set_urgency_hint = gdk_gix_window_set_urgency_hint;
  impl_class->set_geometry_hints = gdk_gix_window_set_geometry_hints;
  impl_class->set_title = gdk_gix_window_set_title;
  impl_class->set_role = gdk_gix_window_set_role;
  impl_class->set_startup_id = gdk_gix_window_set_startup_id;
  impl_class->set_transient_for = gdk_gix_window_set_transient_for;
  impl_class->get_root_origin = gdk_gix_window_get_root_origin;
  impl_class->get_frame_extents = gdk_gix_window_get_frame_extents;
  impl_class->set_override_redirect = gdk_gix_window_set_override_redirect;
  impl_class->set_accept_focus = gdk_gix_window_set_accept_focus;
  impl_class->set_focus_on_map = gdk_gix_window_set_focus_on_map;
  impl_class->set_icon_list = gdk_gix_window_set_icon_list;
  impl_class->set_icon_name = gdk_gix_window_set_icon_name;
  impl_class->iconify = gdk_gix_window_iconify;
  impl_class->deiconify = gdk_gix_window_deiconify;
  impl_class->stick = gdk_gix_window_stick;
  impl_class->unstick = gdk_gix_window_unstick;
  impl_class->maximize = gdk_gix_window_maximize;
  impl_class->unmaximize = gdk_gix_window_unmaximize;
  impl_class->fullscreen = gdk_gix_window_fullscreen;
  impl_class->unfullscreen = gdk_gix_window_unfullscreen;
  impl_class->set_keep_above = gdk_gix_window_set_keep_above;
  impl_class->set_keep_below = gdk_gix_window_set_keep_below;
  impl_class->get_group = gdk_gix_window_get_group;
  impl_class->set_group = gdk_gix_window_set_group;
  impl_class->set_decorations = gdk_gix_window_set_decorations;
  impl_class->get_decorations = gdk_gix_window_get_decorations;
  impl_class->set_functions = gdk_gix_window_set_functions;
  impl_class->begin_resize_drag = gdk_gix_window_begin_resize_drag;
  impl_class->begin_move_drag = gdk_gix_window_begin_move_drag;
  impl_class->enable_synchronized_configure = gdk_gix_window_enable_synchronized_configure;
  impl_class->configure_finished = gdk_gix_window_configure_finished;
  impl_class->set_opacity = gdk_gix_window_set_opacity;
  impl_class->set_composited = gdk_gix_window_set_composited;
  impl_class->destroy_notify = gdk_gix_window_destroy_notify;
  impl_class->get_drag_protocol = _gdk_gix_window_get_drag_protocol;
  impl_class->register_dnd = _gdk_gix_window_register_dnd;
  impl_class->drag_begin = _gdk_gix_window_drag_begin;
  impl_class->process_updates_recurse = gdk_gix_window_process_updates_recurse;
  impl_class->sync_rendering = gdk_gix_window_sync_rendering;
  impl_class->simulate_key = gdk_gix_window_simulate_key;
  impl_class->simulate_button = gdk_gix_window_simulate_button;
  impl_class->get_property = gdk_gix_window_get_property;
  impl_class->change_property = gdk_gix_window_change_property;
  impl_class->delete_property = gdk_gix_window_delete_property;
}
