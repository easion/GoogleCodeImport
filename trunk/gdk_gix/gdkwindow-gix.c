/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-1999 Tor Lillqvist
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

#include "config.h"
#include "gdk.h"
#include "gdkwindowimpl.h"
#include "gdkwindow.h"

#include "gdkgix.h"
#include "gdkprivate-gix.h"
#include "gdkdisplay-gix.h"

#include "gdkregion-generic.h"

#include "gdkinternals.h"
#include "gdkalias.h"
#include "cairo.h"
#include <assert.h>

#define DFB_RECTANGLE_VALS_FROM_REGION(r)    (r)->x1, (r)->y1, (r)->x2-(r)->x1+1, (r)->y2-(r)->y1+1


#define D_DEBUG_AT //printf

static GdkRegion * gdk_window_impl_gix_get_visible_region (GdkDrawable *drawable);
static void        gdk_window_impl_gix_set_colormap       (GdkDrawable *drawable,
                                                                GdkColormap *colormap);
static void gdk_window_impl_gix_init       (GdkWindowImplGix      *window);
static void gdk_window_impl_gix_class_init (GdkWindowImplGixClass *klass);
static void gdk_window_impl_gix_finalize   (GObject                    *object);

static void gdk_window_impl_iface_init (GdkWindowImplIface *iface);


typedef struct
{
  GdkWindowChildChanged  changed;
  GdkWindowChildGetPos   get_pos;
  gpointer               user_data;
} GdkWindowChildHandlerData;


/* Code for dirty-region queueing
 */
static GSList *update_windows = NULL;
static guint update_idle = 0;

static void
gdk_window_gix_process_all_updates (void)
{
  GSList *tmp_list;
  GSList *old_update_windows = update_windows;

  if (update_idle)
    g_source_remove (update_idle);
  
  update_windows = NULL;
  update_idle = 0;

  D_DEBUG_AT(  "%s()\n", __FUNCTION__ );
  
  g_slist_foreach (old_update_windows, (GFunc)g_object_ref, NULL);
  tmp_list = old_update_windows;
  while (tmp_list)
    {
      GdkWindowObject *private = GDK_WINDOW_OBJECT( tmp_list->data );

      if (private->update_freeze_count)
        {
          
          update_windows = g_slist_prepend (update_windows, private);
        }
      else
        {
          
          gdk_window_process_updates(tmp_list->data,TRUE);
        }

      g_object_unref (tmp_list->data);
      tmp_list = tmp_list->next;
    }

#ifndef GDK_GIX_NO_EXPERIMENTS
  g_slist_foreach (old_update_windows, (GFunc)g_object_ref, NULL);
  tmp_list = old_update_windows;
  while (tmp_list)
    {
      GdkWindowObject *top = GDK_WINDOW_OBJECT( gdk_window_get_toplevel( tmp_list->data ) );

      if (top)
        {
          GdkWindowImplGix *wimpl = GDK_WINDOW_IMPL_GIX (top->impl);

          if (wimpl->flips.num_regions)
            {
              D_DEBUG_AT(  "  -> %p flip   [%4d,%4d-%4dx%4d] (%d boxes)\n",
                          top, DFB_RECTANGLE_VALS_FROM_REGION( &wimpl->flips.bounding ),
                          wimpl->flips.num_regions );

              wimpl->drawable.surface->Flip( wimpl->drawable.surface, &wimpl->flips.bounding, DSFLIP_NONE );

              dfb_updates_reset( &wimpl->flips );
            }
          else
            D_DEBUG_AT(  "  -> %p has no flips!\n", top );
        }
      else
        D_DEBUG_AT(  "  -> %p has no top level window!\n", tmp_list->data );

      g_object_unref (tmp_list->data);
      tmp_list = tmp_list->next;
    }
#endif

  g_slist_free (old_update_windows);
}

static gboolean
gdk_window_update_idle (gpointer data)
{
  gdk_window_gix_process_all_updates ();
  
  return FALSE;
}

static void
gdk_window_schedule_update (GdkWindow *window)
{
  D_DEBUG_AT(  "%s( %p ) <- freeze count %d\n", __FUNCTION__, window,
              window ? GDK_WINDOW_OBJECT (window)->update_freeze_count : -1 );
  
  if (window && GDK_WINDOW_OBJECT (window)->update_freeze_count)
    return;

  if (!update_idle)
    {
      D_DEBUG_AT(  "  -> adding idle callback\n" );

      update_idle = gdk_threads_add_idle_full (GDK_PRIORITY_REDRAW,
                                               gdk_window_update_idle, NULL, NULL);
    }
}


static GdkWindow *gdk_gix_window_containing_pointer = NULL;
static GdkWindow *gdk_gix_focused_window            = NULL;
static gpointer   parent_class                           = NULL;
GdkWindow * _gdk_parent_root = NULL;

static void gdk_window_impl_gix_paintable_init (GdkPaintableIface *iface);


GType
gdk_window_impl_gix_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
        {
          sizeof (GdkWindowImplGixClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) gdk_window_impl_gix_class_init,
          NULL,           /* class_finalize */
          NULL,           /* class_data */
          sizeof (GdkWindowImplGix),
          0,              /* n_preallocs */
          (GInstanceInitFunc) gdk_window_impl_gix_init,
        };

      static const GInterfaceInfo paintable_info =
        {
          (GInterfaceInitFunc) gdk_window_impl_gix_paintable_init,
          NULL,
          NULL
        };

      static const GInterfaceInfo window_impl_info =
        {
          (GInterfaceInitFunc) gdk_window_impl_iface_init,
          NULL,
          NULL
        };

      object_type = g_type_register_static (GDK_TYPE_DRAWABLE_IMPL_GIX,
                                            "GdkWindowImplGix",
                                            &object_info, 0);
      g_type_add_interface_static (object_type,
                                   GDK_TYPE_PAINTABLE,
                                   &paintable_info);

      g_type_add_interface_static (object_type,
                                   GDK_TYPE_WINDOW_IMPL,
                                   &window_impl_info);
    }

  return object_type;
}

GType
_gdk_window_impl_get_type (void)
{
  return gdk_window_impl_gix_get_type ();
}

static void
gdk_window_impl_gix_init (GdkWindowImplGix *impl)
{
  impl->drawable.width  = 1;
  impl->drawable.height = 1;
  //cannot use gdk_cursor_new here since gdk_display_get_default
  //does not work yet.
  impl->cursor          = gdk_cursor_new_for_display (GDK_DISPLAY_OBJECT(_gdk_display),GDK_LEFT_PTR);
  impl->opacity         = 255;
}

static void
gdk_window_impl_gix_class_init (GdkWindowImplGixClass *klass)
{
  GObjectClass     *object_class   = G_OBJECT_CLASS (klass);
  GdkDrawableClass *drawable_class = GDK_DRAWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gdk_window_impl_gix_finalize;

  drawable_class->set_colormap = gdk_window_impl_gix_set_colormap;

  /* Visible and clip regions are the same */

  drawable_class->get_clip_region =
    gdk_window_impl_gix_get_visible_region;

  drawable_class->get_visible_region =
    gdk_window_impl_gix_get_visible_region;
}

static void
g_free_2nd (gpointer a,
            gpointer b,
            gpointer data)
{
  g_free (b);
}

static void
gdk_window_impl_gix_finalize (GObject *object)
{
  GdkWindowImplGix *impl = GDK_WINDOW_IMPL_GIX (object);

  D_DEBUG_AT(  "%s( %p ) <- %dx%d\n", __FUNCTION__, impl, impl->drawable.width, impl->drawable.height );

  if (GDK_WINDOW_IS_MAPPED (impl->drawable.wrapper))
    gdk_window_hide (impl->drawable.wrapper);

  if (impl->cursor)
    gdk_cursor_unref (impl->cursor);

  if (impl->properties)
    {
      g_hash_table_foreach (impl->properties, g_free_2nd, NULL);
      g_hash_table_destroy (impl->properties);
    }
  if (impl->drawable.window_id)
    {
      gdk_gix_window_id_table_remove (impl->drawable.window_id);
	  /* native window resource must be release before we can finalize !*/
      impl->drawable.window_id = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GdkRegion*
gdk_window_impl_gix_get_visible_region (GdkDrawable *drawable)
{
  GdkDrawableImplGix *impl = GDK_DRAWABLE_IMPL_GIX (drawable);
  GdkRectangle result_rect;

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, drawable ); //FIXME DPP

/*  if (priv->surface)
  priv->surface->GetVisibleRectangle (priv->surface, &drect);
  rect.x= drect.x;
  rect.y= drect.y;
  rect.width=drect.w;
  rect.height=drect.h;
  D_DEBUG_AT(  "  -> returning %4d,%4d-%4dx%4d\n", drect.x, drect.y, drect.w, drect.h );
  */
  result_rect.x = 0;
  result_rect.y = 0;
  result_rect.width = impl->width; //这个继续留着？ abs_x,abs_y应该去掉
  result_rect.height = impl->height;


  return gdk_region_rectangle (&result_rect);
}

static void
gdk_window_impl_gix_set_colormap (GdkDrawable *drawable,
                                       GdkColormap *colormap)
{
  GDK_DRAWABLE_CLASS (parent_class)->set_colormap (drawable, colormap);

  if (colormap)
    {
       GdkDrawableImplGix *priv = GDK_DRAWABLE_IMPL_GIX (drawable);
	   D_UNIMPLEMENTED;

       if (0) //fimxe
	 {
	   //IGixPalette *palette = gdk_gix_colormap_get_palette (colormap);

          // if (palette){
//             priv->surface->SetPalette (priv->surface, palette);
         //}
    }
}
}

#if 0
static gboolean
create_gix_window (GdkWindowImplGix *impl,
                        DFBWindowDescription  *desc,
                        DFBWindowOptions       window_options)
{
  int        ret;
  gi_window_id_t window;

  D_DEBUG_AT(  "%s( %4dx%4d, caps 0x%08x )\n", __FUNCTION__, desc->width, desc->height, desc->caps );

  ret = _gdk_display->layer->CreateWindow (_gdk_display->layer, desc, &window);

  if (ret != 0)
    {
      GixError ("gdk_window_new: Layer->CreateWindow failed", ret);
      g_assert (0);
      return FALSE;
    }

  if ((desc->flags & DWDESC_CAPS) && (desc->caps & DWCAPS_INPUTONLY))
  {
    impl->drawable.surface = NULL;
  } else 
    window->GetSurface (window, &impl->drawable.surface);

  if (window_options)
    {
      //DFBWindowOptions options;
      window->GetOptions (window, &options);
      window->SetOptions (window,  options | window_options);
    }

  impl->drawable.window_id = window;

#ifndef GDK_GIX_NO_EXPERIMENTS
  //direct_log_printf( NULL, "Initializing (window %p, wimpl %p)\n", win, impl );

  dfb_updates_init( &impl->flips, impl->flip_regions, G_N_ELEMENTS(impl->flip_regions) );
#endif

  return TRUE;
}
#endif


void
_gdk_windowing_window_init (void)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  gi_screen_info_t  dlc;

  g_assert (_gdk_parent_root == NULL);

  gi_get_screen_info(  &dlc );

  _gdk_parent_root = g_object_new (GDK_TYPE_WINDOW, NULL);
  private = GDK_WINDOW_OBJECT (_gdk_parent_root);
  private->impl = g_object_new (_gdk_window_impl_get_type (), NULL);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);

  private->window_type = GDK_WINDOW_ROOT;
  private->state       = 0;
  private->children    = NULL;
//  impl->drawable.paint_region   = NULL;
  impl->gdkWindow      = _gdk_parent_root;
  impl->drawable.window_id           = NULL;
  impl->drawable.window_id           = GI_DESKTOP_WINDOW_ID;
  impl->drawable.abs_x   = 0;
  impl->drawable.abs_y   = 0;
  impl->drawable.width   = dlc.scr_width;
  impl->drawable.height  = dlc.scr_height;
  impl->drawable.wrapper = GDK_DRAWABLE (private);
  /* custom root window init */
#if 0
  {
    DFBWindowDescription   desc;
    desc.flags = 0;
	/*XXX I must do this now its a bug  ALPHA ROOT*/

    desc.flags = DWDESC_CAPS;
    desc.caps = 0;
    desc.caps  |= DWCAPS_NODECORATION;
    desc.caps  |= DWCAPS_ALPHACHANNEL;
    desc.flags |= ( DWDESC_WIDTH | DWDESC_HEIGHT |
                      DWDESC_POSX  | DWDESC_POSY );
    desc.posx   = 0;
    desc.posy   = 0;
    desc.width  = dlc.width;
    desc.height = dlc.height;
    create_gix_window (impl,&desc,0);
	g_assert(impl->window != NULL);
    g_assert(impl->drawable.surface != NULL );
  }
  impl->drawable.surface->GetPixelFormat(impl->drawable.surface,&impl->drawable.format);
  #endif

  impl->drawable.format = gi_screen_format();

  private->depth = GI_RENDER_FORMAT_BPP(impl->drawable.format);
  /*
	Now we can set up the system colormap
  */
  gdk_drawable_set_colormap (GDK_DRAWABLE (_gdk_parent_root),gdk_colormap_get_system());
}



static const gchar * get_default_title (void)
{
	const char *title;
	
	title = g_get_application_name();
	if (!title) {
		title = g_get_prgname ();
	}
	return title;
}

#if 1
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
      break;

    default:
      break;
    }

  return 0;
}
#endif

GdkWindow *
gdk_gix_window_new (GdkWindow              *parent,
                         GdkWindowAttr          *attributes,
                         gint                    attributes_mask)
{
  GdkWindow             *window;
  GdkWindowObject       *private;
  GdkWindowObject       *parent_private;
  GdkWindowImplGix *impl;
  GdkWindowImplGix *parent_impl;
  GdkVisual             *visual;
  gint x, y;
  gi_window_id_t window_tl;
	char *title;
  unsigned long win_flags = 0;

  g_return_val_if_fail (attributes != NULL, NULL);

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, parent );

  if (!parent || attributes->window_type != GDK_WINDOW_CHILD)
    parent = _gdk_parent_root;

  window = g_object_new (GDK_TYPE_WINDOW, NULL);
  private = GDK_WINDOW_OBJECT (window);
  private->impl = g_object_new (_gdk_window_impl_get_type (), NULL);

  parent_private = GDK_WINDOW_OBJECT (parent);
  parent_impl = GDK_WINDOW_IMPL_GIX (parent_private->impl);
  private->parent = parent_private;

  x = (attributes_mask & GDK_WA_X) ? attributes->x : 0;
  y = (attributes_mask & GDK_WA_Y) ? attributes->y : 0;  

  impl = GDK_WINDOW_IMPL_GIX (private->impl);
  impl->drawable.wrapper = GDK_DRAWABLE (window);
  impl->gdkWindow      = window;

  private->x = x;
  private->y = y;

  _gdk_gix_calc_abs (window);

  impl->drawable.width  = MAX (1, attributes->width);
  impl->drawable.height = MAX (1, attributes->height);

  private->window_type = attributes->window_type;

  //desc.flags = 0;

  if (attributes_mask & GDK_WA_VISUAL)
    visual = attributes->visual;
  else
    visual = gdk_drawable_get_visual (parent);

  switch (attributes->wclass)
    {
    case GDK_INPUT_OUTPUT:
      private->input_only = FALSE;

/*      desc.flags |= DWDESC_PIXELFORMAT;
      desc.pixelformat = ((GdkVisualGix *) visual)->format;

      if (DFB_PIXELFORMAT_HAS_ALPHA (desc.pixelformat))
        {
          desc.flags |= DWDESC_CAPS;
          desc.caps = DWCAPS_ALPHACHANNEL;
        }*/
      break;

    case GDK_INPUT_ONLY:
      private->input_only = TRUE;
#if 1
	if (parent == _gdk_parent_root)
	private->window_type = GDK_WINDOW_TEMP;
      else
	private->window_type = GDK_WINDOW_CHILD;

	  win_flags |= GI_FLAGS_TRANSPARENT;
#endif
	printf("GDK_INPUT_ONLY window\n");
//      desc.flags |= DWDESC_CAPS;
//      desc.caps = DWCAPS_INPUTONLY;
      break;

    default:
      g_warning ("gdk_window_new: unsupported window class\n");
      _gdk_window_destroy (window, FALSE);
      return NULL;
    }


	if (attributes_mask & GDK_WA_TYPE_HINT)
	{
		win_flags |= window_type_hint_to_level(attributes->type_hint);
		printf("try a menu window %d %x\n", GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,win_flags);
	}

  switch (private->window_type)
    {
    case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_DIALOG:
    case GDK_WINDOW_TEMP:
	if (GDK_WINDOW_TYPE (parent) == GDK_WINDOW_FOREIGN){
		win_flags |= GI_FLAGS_NON_FRAME;
	}
	else{
	}

	if (private->window_type == GDK_WINDOW_TEMP)
	{
		win_flags |= GI_FLAGS_TEMP_WINDOW | GI_FLAGS_NORESIZE | GI_FLAGS_TOPMOST_WINDOW;
	}


	window_tl = gi_create_window(parent_impl->drawable.window_id, x,y,
		impl->drawable.width, impl->drawable.height,GI_RGB( 192, 192, 192 ),win_flags);


      if (window_tl <= 0)
        {
	printf("gdk_gix_window_new window_tl = %x  parent=%x (%d,%d,%d,%d) win_flags=%x error\n", 
		window_tl, parent_impl->drawable.window_id,
		x,y,impl->drawable.width, impl->drawable.height,win_flags);
		//D_UNIMPLEMENTED;
          _gdk_window_destroy (window, FALSE);
          return NULL;
        }
	printf("gdk_gix_window_new window_tl = %x %x ### parent=%x succ \n", window_tl,parent_impl, parent_impl->drawable.window_id);

		//gi_set_window_background(window_tl,WINDOW_COLOR,GI_BG_USE_NONE);


		impl->drawable.window_id = window_tl;
		if (attributes->title != NULL) {
			title = attributes->title;
		} else {
			title = get_default_title();
		}

		gdk_window_set_title (window, title);
		impl->drawable.format = gi_screen_format();
	
//      	if( desc.caps != DWCAPS_INPUTONLY )
//			impl->window->SetOpacity(impl->window, 0x00 );
      break;

    case GDK_WINDOW_CHILD:
	   impl->drawable.window_id=NULL;
      if (!private->input_only && parent_impl->drawable.window_id)
        {
		window_tl = gi_create_window(parent_impl->drawable.window_id, x,y,
		impl->drawable.width, impl->drawable.height,GI_RGB( 192, 192, 192 ),win_flags|GI_FLAGS_NON_FRAME);

		if (window_tl <= 0)
        {
		D_UNIMPLEMENTED;
          _gdk_window_destroy (window, FALSE);
          return NULL;
        }

		printf("gdk_gix_window_new GDK_WINDOW_CHILD window_tl = %x parent=%x (%d,%d,%d,%d)\n",
			window_tl,parent_impl->drawable.window_id, x,y,impl->drawable.width, impl->drawable.height);

		impl->drawable.window_id = window_tl;

		//gdk_window_set_events (window, attributes->event_mask | GDK_STRUCTURE_MASK);
		impl->drawable.format = gi_screen_format();

/*          DFBRectangle rect =
          { x, y, impl->drawable.width, impl->drawable.height };
          parent_impl->drawable.surface->GetSubSurface (parent_impl->drawable.surface,
                                                        &rect,
                                                        &impl->drawable.surface);
*/
        }
      break;

    default:
      g_warning ("gdk_window_new: unsupported window type: %d",
                 private->window_type);
      _gdk_window_destroy (window, FALSE);
      return NULL;
    }

  if (impl->drawable.window_id)
    {
      GdkColormap *colormap;

	  impl->drawable.format = gi_screen_format(); //FIXME DPP

      //impl->drawable.surface->GetPixelFormat (impl->drawable.surface,
		//			      &impl->drawable.format);

  	  private->depth = GI_RENDER_FORMAT_BPP(impl->drawable.format);

      if ((attributes_mask & GDK_WA_COLORMAP) && attributes->colormap)
	{
	  colormap = attributes->colormap;
	}
      else
	{
	  if (gdk_visual_get_system () == visual)
	    colormap = gdk_colormap_get_system ();
	  else
	    colormap =gdk_drawable_get_colormap (parent);
	}

      gdk_drawable_set_colormap (GDK_DRAWABLE (window), colormap);
    }
  else
    {
      impl->drawable.format = ((GdkVisualGix *)visual)->format;
  	  private->depth = visual->depth;
    }

  gdk_window_set_cursor (window, ((attributes_mask & GDK_WA_CURSOR) ?
                                  (attributes->cursor) : NULL));

  if (parent_private)
    parent_private->children = g_list_prepend (parent_private->children,
                                               window);

  /* we hold a reference count on ourselves */
  g_object_ref (window);


  if (impl->drawable.window_id)
    {
      //impl->window->GetID (impl->window, &impl->dfb_id);
      gdk_gix_window_id_table_insert (impl->drawable.window_id, window);
      //gdk_gix_event_windows_add (window);
    }

  gdk_window_set_events (window, attributes->event_mask | GDK_STRUCTURE_MASK);
  if (attributes_mask & GDK_WA_TYPE_HINT){
	  printf("calling gdk_window_set_type_hint xxx %x\n",attributes->type_hint);
    gdk_window_set_type_hint (window, attributes->type_hint);
  }

  return window;
}

GdkWindow *
_gdk_window_new (GdkWindow     *parent,
                 GdkWindowAttr *attributes,
                 gint           attributes_mask)
{
  g_return_val_if_fail (attributes != NULL, NULL);

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, parent );

  return gdk_gix_window_new (parent, attributes, attributes_mask);
}
void
_gdk_windowing_window_destroy_foreign (GdkWindow *window)
{
  /* It's somebody else's window, but in our hierarchy,
   * so reparent it to the root window, and then send
   * it a delete event, as if we were a WM
   */
	_gdk_windowing_window_destroy (window,TRUE,TRUE);
}


void
_gdk_windowing_window_destroy (GdkWindow *window,
                               gboolean   recursing,
                               gboolean   foreign_destroy)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  D_DEBUG_AT(  "%s( %p, %srecursing, %sforeign )\n", __FUNCTION__, window,
              recursing ? "" : "not ", foreign_destroy ? "" : "no " );

  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);

  _gdk_selection_window_destroyed (window);
  //gdk_gix_event_windows_remove (window);

  if (window == _gdk_gix_pointer_grab_window)
    gdk_pointer_ungrab (GDK_CURRENT_TIME);
  if (window == _gdk_gix_keyboard_grab_window)
    gdk_keyboard_ungrab (GDK_CURRENT_TIME);

  if (window == gdk_gix_focused_window)
    gdk_gix_change_focus (NULL);


  if (impl->drawable.window_id){
    GdkDrawableImplGix *dimpl = GDK_DRAWABLE_IMPL_GIX (private->impl);
    if(dimpl->cairo_surface) {
      cairo_surface_destroy(dimpl->cairo_surface);
      dimpl->cairo_surface= NULL;
    }

	//FIXME DPP
	//gi_destroy_window(impl->drawable.window_id);
    //impl->drawable.surface->Release (impl->drawable.surface);
    //impl->drawable.surface = NULL;
  }

  if (!recursing && !foreign_destroy && impl->drawable.window_id ) {
    gi_destroy_window (impl->drawable.window_id);
    impl->drawable.window_id = NULL;
  }
}

/* This function is called when the window is really gone.
 */
void
gdk_window_destroy_notify (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, window );

  if (!GDK_WINDOW_DESTROYED (window))
    {
      if (GDK_WINDOW_TYPE(window) != GDK_WINDOW_FOREIGN)
	g_warning ("GdkWindow %p unexpectedly destroyed", window);

      _gdk_window_destroy (window, TRUE);
    }
   g_object_unref (window);
}

/* Focus follows pointer */
GdkWindow *
gdk_gix_window_find_toplevel (GdkWindow *window)
{
  while (window && window != _gdk_parent_root)
    {
      GdkWindow *parent = (GdkWindow *) (GDK_WINDOW_OBJECT (window))->parent;

      if ((parent == _gdk_parent_root) && GDK_WINDOW_IS_MAPPED (window))
        return window;

      window = parent;
    }

  return _gdk_parent_root;
}

GdkWindow *
gdk_gix_window_find_focus (void)
{
  if (_gdk_gix_keyboard_grab_window)
    return _gdk_gix_keyboard_grab_window;

  if (!gdk_gix_focused_window)
    gdk_gix_focused_window = g_object_ref (_gdk_parent_root);

  return gdk_gix_focused_window;
}

void
gdk_gix_change_focus (GdkWindow *new_focus_window)
{
  GdkEventFocus *event;
  GdkWindow     *old_win;
  GdkWindow     *new_win;
  GdkWindow     *event_win;

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, new_focus_window );

  /* No focus changes while the pointer is grabbed */
  if (_gdk_gix_pointer_grab_window)
    return;

  old_win = gdk_gix_focused_window;
  new_win = gdk_gix_window_find_toplevel (new_focus_window);

  if (old_win == new_win)
    return;

  if (old_win)
    {
      event_win = gdk_gix_keyboard_event_window (old_win,
                                                      GDK_FOCUS_CHANGE);
      if (event_win)
        {
          event = (GdkEventFocus *) gdk_gix_event_make (event_win,
                                                             GDK_FOCUS_CHANGE);
          event->in = FALSE;
        }
    }

  event_win = gdk_gix_keyboard_event_window (new_win,
                                                  GDK_FOCUS_CHANGE);
  if (event_win)
    {
      event = (GdkEventFocus *) gdk_gix_event_make (event_win,
                                                         GDK_FOCUS_CHANGE);
      event->in = TRUE;
    }

  if (gdk_gix_focused_window)
    g_object_unref (gdk_gix_focused_window);
  gdk_gix_focused_window = g_object_ref (new_win);
}

void
gdk_window_set_accept_focus (GdkWindow *window,
                             gboolean accept_focus)
{
  GdkWindowObject *private;
  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));
                                                                                                                      
  private = (GdkWindowObject *)window;
                                                                                                                      
  accept_focus = accept_focus != FALSE;
                                                                                                                      
  if (private->accept_focus != accept_focus)
    private->accept_focus = accept_focus;

}

void
gdk_window_set_focus_on_map (GdkWindow *window,
                             gboolean focus_on_map)
{
  GdkWindowObject *private;
  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));
                                                                                                                      
  private = (GdkWindowObject *)window;
                                                                                                                      
  focus_on_map = focus_on_map != FALSE;
                                                                                                                      
  if (private->focus_on_map != focus_on_map)
    private->focus_on_map = focus_on_map;
}

/*

//FIXME DPP 废除此链表
static gboolean
gdk_gix_window_raise (GdkWindow *window)
{
  GdkWindowObject *parent;

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, window );

  parent = GDK_WINDOW_OBJECT (window)->parent;

  if (parent->children->data == window)
    return FALSE;

  parent->children = g_list_remove (parent->children, window);
  parent->children = g_list_prepend (parent->children, window);

  return TRUE;
}

static void
gdk_gix_window_lower (GdkWindow *window)
{
  GdkWindowObject *parent;

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, window );

  parent = GDK_WINDOW_OBJECT (window)->parent;

  parent->children = g_list_remove (parent->children, window);
  parent->children = g_list_append (parent->children, window);
}
*/
static gboolean
all_parents_shown (GdkWindowObject *private)
{
  while (GDK_WINDOW_IS_MAPPED (private))
    {
      if (private->parent)
        private = GDK_WINDOW_OBJECT (private)->parent;
      else
        return TRUE;
    }

  return FALSE;
}

static void
send_map_events (GdkWindowObject *private)
{
  GList     *list;
  GdkWindow *event_win;

  if (!GDK_WINDOW_IS_MAPPED (private))
    return;

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, private );

  event_win = gdk_gix_other_event_window ((GdkWindow *) private, GDK_MAP);
  if (event_win)
    gdk_gix_event_make (event_win, GDK_MAP);

  for (list = private->children; list; list = list->next)
    send_map_events (list->data);
}

static GdkWindow *
gdk_gix_find_common_ancestor (GdkWindow *win1,
                                   GdkWindow *win2)
{
  GdkWindowObject *a;
  GdkWindowObject *b;

  for (a = GDK_WINDOW_OBJECT (win1); a; a = a->parent)
    for (b = GDK_WINDOW_OBJECT (win2); b; b = b->parent)
      {
        if (a == b)
          return GDK_WINDOW (a);
      }

  return NULL;
}

#ifndef GI_CURSOR_USER0
#define GI_CURSOR_USER0 20
#endif


//FIXME DPP
void
gdk_gix_window_send_crossing_events (GdkWindow       *src,
                                          GdkWindow       *dest,
                                          GdkCrossingMode  mode)
{
  GdkWindow       *c;
  GdkWindow       *win, *last, *next;
  GdkEvent        *event;
  gint             x, y, x_int, y_int;
  GdkModifierType  modifiers;
  GSList          *path, *list;
  gboolean         non_linear;
  GdkWindow       *a;
  GdkWindow       *b;
  GdkWindow       *event_win;

  if (!dest)
  {
  printf(  "%s( %p -> %p, %d )\n", __FUNCTION__, src, dest, mode );
  return;
  }


  /* Do a possible cursor change before checking if we need to
     generate crossing events so cursor changes due to pointer
     grabs work correctly. */
  {
    static GdkCursorGix *last_cursor = NULL;

    GdkWindowObject       *private = GDK_WINDOW_OBJECT (dest);
    GdkWindowImplGix *impl    = GDK_WINDOW_IMPL_GIX (private->impl);
    GdkCursorGix     *cursor;

    if (_gdk_gix_pointer_grab_cursor)
      cursor = (GdkCursorGix*) _gdk_gix_pointer_grab_cursor;
    else
      cursor = (GdkCursorGix*) impl->cursor;

    if (cursor != last_cursor)
      {
        win     = gdk_gix_window_find_toplevel (dest);
        private = GDK_WINDOW_OBJECT (win);
        impl    = GDK_WINDOW_IMPL_GIX (private->impl);

     if (impl->drawable.window_id){
      gi_setup_cursor (GI_CURSOR_USER0 , 
		 cursor->hot_x, cursor->hot_y,cursor->shape_img );
	 gi_load_cursor(impl->drawable.window_id,GI_CURSOR_USER0);
	 }
                                        
        last_cursor = cursor;
      }
  }

  if (dest == gdk_gix_window_containing_pointer) {
    D_DEBUG_AT(  "  -> already containing the pointer\n" );
    return;
  }

  if (gdk_gix_window_containing_pointer == NULL)
    gdk_gix_window_containing_pointer = g_object_ref (_gdk_parent_root);

  if (src)
    a = src;
  else
    a = gdk_gix_window_containing_pointer;

  b = dest;

  if (a == b) {
    D_DEBUG_AT(  "  -> src == dest\n" );
    return;
  }

  /* gdk_gix_window_containing_pointer might have been destroyed.
   * The refcount we hold on it should keep it, but it's parents
   * might have died.
   */
  if (GDK_WINDOW_DESTROYED (a)) {
    D_DEBUG_AT(  "  -> src is destroyed!\n" );
    a = _gdk_parent_root;
  }

  gdk_gix_mouse_get_info (&x, &y, &modifiers);

  c = gdk_gix_find_common_ancestor (a, b);

  D_DEBUG_AT(  "  -> common ancestor %p\n", c );

  non_linear = (c != a) && (c != b);

  D_DEBUG_AT(  "  -> non_linear: %s\n", non_linear ? "YES" : "NO" );

  event_win = gdk_gix_pointer_event_window (a, GDK_LEAVE_NOTIFY);
  if (event_win)
    {
      D_DEBUG_AT(  "  -> sending LEAVE to src\n" );

      event = gdk_gix_event_make (event_win, GDK_LEAVE_NOTIFY);
      event->crossing.subwindow = NULL;

      gdk_window_get_origin (a, &x_int, &y_int);

      event->crossing.x      = x - x_int;
      event->crossing.y      = y - y_int;
      event->crossing.x_root = x;
      event->crossing.y_root = y;
      event->crossing.mode   = mode;

      if (non_linear)
        event->crossing.detail = GDK_NOTIFY_NONLINEAR;
      else if (c == a)
        event->crossing.detail = GDK_NOTIFY_INFERIOR;
      else
        event->crossing.detail = GDK_NOTIFY_ANCESTOR;

      event->crossing.focus = FALSE;
      event->crossing.state = modifiers;

      D_DEBUG_AT(  "  => LEAVE (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                  event_win, a,
                  event->crossing.x, event->crossing.y, event->crossing.x_root, event->crossing.y_root,
                  event->crossing.mode, event->crossing.detail );
    }

   /* Traverse up from a to (excluding) c */
  if (c != a)
    {
      last = a;
      win = GDK_WINDOW (GDK_WINDOW_OBJECT (a)->parent);
      while (win != c)
        {
          event_win =
            gdk_gix_pointer_event_window (win, GDK_LEAVE_NOTIFY);

          if (event_win)
            {
              event = gdk_gix_event_make (event_win, GDK_LEAVE_NOTIFY);

              event->crossing.subwindow = g_object_ref (last);

              gdk_window_get_origin (win, &x_int, &y_int);

              event->crossing.x      = x - x_int;
              event->crossing.y      = y - y_int;
              event->crossing.x_root = x;
              event->crossing.y_root = y;
              event->crossing.mode   = mode;

              if (non_linear)
                event->crossing.detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
              else
                event->crossing.detail = GDK_NOTIFY_VIRTUAL;

              event->crossing.focus = FALSE;
              event->crossing.state = modifiers;

              D_DEBUG_AT(  "  -> LEAVE (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                          event_win, win,
                          event->crossing.x, event->crossing.y, event->crossing.x_root, event->crossing.y_root,
                          event->crossing.mode, event->crossing.detail );
            }

          last = win;
          win = GDK_WINDOW (GDK_WINDOW_OBJECT (win)->parent);
        }
    }

  /* Traverse down from c to b */
  if (c != b)
    {
      path = NULL;
      win = GDK_WINDOW (GDK_WINDOW_OBJECT (b)->parent);
      while (win != c)
        {
          path = g_slist_prepend (path, win);
          win = GDK_WINDOW (GDK_WINDOW_OBJECT (win)->parent);
        }

      list = path;
      while (list)
        {
          win = GDK_WINDOW (list->data);
          list = g_slist_next (list);

          if (list)
            next = GDK_WINDOW (list->data);
          else
            next = b;

          event_win =
            gdk_gix_pointer_event_window (win, GDK_ENTER_NOTIFY);

          if (event_win)
            {
              event = gdk_gix_event_make (event_win, GDK_ENTER_NOTIFY);

              event->crossing.subwindow = g_object_ref (next);

              gdk_window_get_origin (win, &x_int, &y_int);

              event->crossing.x      = x - x_int;
              event->crossing.y      = y - y_int;
              event->crossing.x_root = x;
              event->crossing.y_root = y;
              event->crossing.mode   = mode;

              if (non_linear)
                event->crossing.detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
              else
                event->crossing.detail = GDK_NOTIFY_VIRTUAL;

              event->crossing.focus = FALSE;
              event->crossing.state = modifiers;

              D_DEBUG_AT(  "  -> ENTER (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                          event_win, win,
                          event->crossing.x, event->crossing.y, event->crossing.x_root, event->crossing.y_root,
                          event->crossing.mode, event->crossing.detail );
            }
        }

      g_slist_free (path);
    }

  event_win = gdk_gix_pointer_event_window (b, GDK_ENTER_NOTIFY);
  if (event_win)
    {
      event = gdk_gix_event_make (event_win, GDK_ENTER_NOTIFY);

      event->crossing.subwindow = NULL;

      gdk_window_get_origin (b, &x_int, &y_int);

      event->crossing.x      = x - x_int;
      event->crossing.y      = y - y_int;
      event->crossing.x_root = x;
      event->crossing.y_root = y;
      event->crossing.mode   = mode;

      if (non_linear)
        event->crossing.detail = GDK_NOTIFY_NONLINEAR;
      else if (c==a)
        event->crossing.detail = GDK_NOTIFY_ANCESTOR;
      else
        event->crossing.detail = GDK_NOTIFY_INFERIOR;

      event->crossing.focus = FALSE;
      event->crossing.state = modifiers;

      D_DEBUG_AT(  "  => ENTER (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                  event_win, b,
                  event->crossing.x, event->crossing.y, event->crossing.x_root, event->crossing.y_root,
                  event->crossing.mode, event->crossing.detail );
    }

  if (mode != GDK_CROSSING_GRAB)
    {
      //this seems to cause focus to change as the pointer moves yuck
      //gdk_gix_change_focus (b);
      if (b != gdk_gix_window_containing_pointer)
        {
          g_object_unref (gdk_gix_window_containing_pointer);
          gdk_gix_window_containing_pointer = g_object_ref (b);
        }
    }
}

static void
show_window_internal (GdkWindow *window,
                      gboolean   raise)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  GdkWindow             *mousewin;
  int err;

  D_DEBUG_AT(  "%s( %p, %sraise )\n", __FUNCTION__, window, raise ? "" : "no " );

  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);

  if (!private->destroyed && !GDK_WINDOW_IS_MAPPED (private))
    {
      private->state &= ~GDK_WINDOW_STATE_WITHDRAWN;

      if (raise)
        gdk_window_raise (window);

      if (all_parents_shown (GDK_WINDOW_OBJECT (private)->parent))
        {
          send_map_events (private);

          mousewin = gdk_window_at_pointer (NULL, NULL);
          gdk_gix_window_send_crossing_events (NULL, mousewin, GDK_CROSSING_NORMAL);

          if (private->input_only)
            return;

          gdk_window_invalidate_rect (window, NULL, TRUE);
        }
    }

  if (impl->drawable.window_id)
    {
	  printf("show_window_internal: window id %x\n",impl->drawable.window_id);
	  err = gi_show_window_all(impl->drawable.window_id);
	  if (err)
	  {
		  perror("show_window_internal");
	  }

    }
}

static void
gdk_gix_window_show (GdkWindow *window,
                          gboolean   raise)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, window );

  show_window_internal (window, raise);
}

static void
gdk_gix_window_hide (GdkWindow *window)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  GdkWindow             *mousewin;
  GdkWindow             *event_win;

  g_return_if_fail (GDK_IS_WINDOW (window));

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, window );

  private = GDK_WINDOW_OBJECT (window);
  impl    = GDK_WINDOW_IMPL_GIX (private->impl);

  if (impl->drawable.window_id)
    gi_hide_window (impl->drawable.window_id);

  if (!private->destroyed && GDK_WINDOW_IS_MAPPED (private))
    {
      GdkEvent *event;

      private->state |= GDK_WINDOW_STATE_WITHDRAWN;

      if (!private->input_only && private->parent)
        {
          gdk_window_clear_area (GDK_WINDOW (private->parent),
                                 private->x,
                                 private->y,
                                 impl->drawable.width,
                                 impl->drawable.height);
        }

      event_win = gdk_gix_other_event_window (window, GDK_UNMAP);
      if (event_win)
        event = gdk_gix_event_make (event_win, GDK_UNMAP);

      mousewin = gdk_window_at_pointer (NULL, NULL);
      gdk_gix_window_send_crossing_events (NULL,
                                                mousewin,
                                                GDK_CROSSING_NORMAL);

      if (window == _gdk_gix_pointer_grab_window)
        gdk_pointer_ungrab (GDK_CURRENT_TIME);
      if (window == _gdk_gix_keyboard_grab_window)
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);
    }
}

static void
gdk_gix_window_withdraw (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  /* for now this should be enough */
  gdk_window_hide (window);
}

void
_gdk_gix_move_resize_child (GdkWindow *window,
                                 gint       x,
                                 gint       y,
                                 gint       width,
                                 gint       height)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  GdkWindowImplGix *parent_impl;
  GList                 *list;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = GDK_WINDOW_OBJECT (window);
  impl    = GDK_WINDOW_IMPL_GIX (private->impl);

  private->x = x;
  private->y = y;

  D_UNIMPLEMENTED;

  impl->drawable.width  = width;
  impl->drawable.height = height;

  if (!private->input_only)
    {
      if (impl->drawable.window_id)
        {
          if (impl->drawable.cairo_surface)
            {
              cairo_surface_destroy (impl->drawable.cairo_surface);
              impl->drawable.cairo_surface = NULL;
            }

          //impl->drawable.surface->Release (impl->drawable.surface);
          //impl->drawable.surface = NULL;
        }

      parent_impl = GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (private->parent)->impl);

      //if (parent_impl->drawable.surface)
        {
          //DFBRectangle rect = { x, y, width, height };

          //parent_impl->drawable.surface->GetSubSurface (parent_impl->drawable.surface,
           //                                             &rect,
           //                                             &impl->drawable.surface);
        }
    }

  for (list = private->children; list; list = list->next)
    {
      private = GDK_WINDOW_OBJECT (list->data);
      impl  = GDK_WINDOW_IMPL_GIX (private->impl);

      _gdk_gix_move_resize_child (list->data,
                                       private->x, private->y,
                                       impl->drawable.width, impl->drawable.height);
    }
}

static  void
gdk_gix_window_move (GdkWindow *window,
                          gint       x,
                          gint       y)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = GDK_WINDOW_OBJECT (window);
  impl    = GDK_WINDOW_IMPL_GIX (private->impl);

  if (impl->drawable.window_id)
    {
      private->x = x;
      private->y = y;
     // impl->window->MoveTo (impl->drawable.window_id, x, y);
      gi_move_window (impl->drawable.window_id, x, y);
    }
  else
    {
         gint width=impl->drawable.width;
         gint height=impl->drawable.height;
      GdkRectangle  old =
      { private->x, private->y,width,height };

      _gdk_gix_move_resize_child (window, x, y, width, height);
      _gdk_gix_calc_abs (window);

      if (GDK_WINDOW_IS_MAPPED (private))
        {
          GdkWindow    *mousewin;
          GdkRectangle  new = { x, y, width, height };

          gdk_rectangle_union (&new, &old, &new);
          gdk_window_invalidate_rect (GDK_WINDOW (private->parent), &new,TRUE);

          /* The window the pointer is in might have changed */
          mousewin = gdk_window_at_pointer (NULL, NULL);
          gdk_gix_window_send_crossing_events (NULL, mousewin,
                                                    GDK_CROSSING_NORMAL);
        }
    }
}

static void
gdk_gix_window_move_resize (GdkWindow *window,
                                 gboolean   with_move,
                                 gint       x,
                                 gint       y,
                                 gint       width,
                                 gint       height)
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);

  if (with_move && (width < 0 && height < 0))
    {
      gdk_gix_window_move (window, x, y);
      return;
    }

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  if (private->destroyed ||
      (private->x == x  &&  private->y == y  &&
       impl->drawable.width == width  &&  impl->drawable.height == height))
    return;

  if (private->parent && (private->parent->window_type != GDK_WINDOW_CHILD))
    {
      GdkWindowChildHandlerData *data;

      data = g_object_get_data (G_OBJECT (private->parent),
                                "gdk-window-child-handler");

      if (data &&
          (*data->changed) (window, x, y, width, height, data->user_data))
        return;
    }

  if (impl->drawable.width == width  &&  impl->drawable.height == height)
    {
      if (with_move)
        gdk_gix_window_move (window, x, y);
    }
  else if (impl->drawable.window_id) //FIXME DPP
    {
      private->x = x;
      private->y = y;
      impl->drawable.width = width;
      impl->drawable.height = height;

      if (with_move)
      gi_move_window (GDK_WINDOW_GIX_ID(window), x, y);
      gi_resize_window (GDK_WINDOW_GIX_ID(window), width, height);
    }
  else
    {
      GdkRectangle old = { private->x, private->y,
                           impl->drawable.width, impl->drawable.height };
      GdkRectangle new = { x, y, width, height };

      if (! with_move)
        {
          new.x = private->x;
          new.y = private->y;
        }

      _gdk_gix_move_resize_child (window,
                                       new.x, new.y, new.width, new.height);
      _gdk_gix_calc_abs (window);

      if (GDK_WINDOW_IS_MAPPED (private))
        {
          GdkWindow *mousewin;

          gdk_rectangle_union (&new, &old, &new);
          gdk_window_invalidate_rect (GDK_WINDOW (private->parent), &new,TRUE); //FIXME DPP

          /* The window the pointer is in might have changed */
          mousewin = gdk_window_at_pointer (NULL, NULL);
          gdk_gix_window_send_crossing_events (NULL, mousewin,
                                                    GDK_CROSSING_NORMAL);
        }
    }
}

static void
gdk_window_set_static_win_gravity (GdkWindow *window,
                                   gboolean   on)
{
  //XSetWindowAttributes xattributes;
  
  g_return_if_fail (GDK_IS_WINDOW (window));
  
  //xattributes.win_gravity = on ? StaticGravity : NorthWestGravity;
  
  //XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
	//		   GDK_WINDOW_GIX_ID (window),
	//		   CWWinGravity,  &xattributes);
}


//FIXME DPP
static gboolean
gdk_gix_window_reparent (GdkWindow *window,
                              GdkWindow *new_parent,
                              gint       x,
                              gint       y)
{
  GdkWindowObject *window_private;
  GdkWindowObject *parent_private;
  GdkWindowObject *old_parent_private;
  GdkWindowImplGix *impl;
  GdkWindowImplGix *parent_impl;
  GdkVisual             *visual;

  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  if (GDK_WINDOW_DESTROYED (window))
    return FALSE;  

  if (!new_parent)
    new_parent = _gdk_parent_root;

  window_private = (GdkWindowObject *) window;
  old_parent_private = (GdkWindowObject *) window_private->parent;
  parent_private = (GdkWindowObject *) new_parent;
  parent_impl = GDK_WINDOW_IMPL_GIX (parent_private->impl);
  visual = gdk_drawable_get_visual (window);

  /* already parented */
  if( window_private->parent == (GdkWindowObject *)new_parent )
          return FALSE;

  window_private->parent = (GdkWindowObject *) new_parent;

  if (old_parent_private)
    {
      old_parent_private->children =
        g_list_remove (old_parent_private->children, window);
    }

    parent_private->children = g_list_prepend (parent_private->children, window);

    impl = GDK_WINDOW_IMPL_GIX (window_private->impl);  
	

	gi_reparent_window (
		   impl->drawable.window_id,
		   parent_impl->drawable.window_id,
		   x, y);
	window_private->x = x;
    window_private->y = y;

	D_UNIMPLEMENTED;

	switch (GDK_WINDOW_TYPE (new_parent))
    {
    case GDK_WINDOW_ROOT:
    case GDK_WINDOW_FOREIGN:
		/*
	was_toplevel = WINDOW_IS_TOPLEVEL (window);

      if (impl->toplevel_window_type != -1)
	GDK_WINDOW_TYPE (window) = impl->toplevel_window_type;
      else if (GDK_WINDOW_TYPE (window) == GDK_WINDOW_CHILD)
	GDK_WINDOW_TYPE (window) = GDK_WINDOW_TOPLEVEL;

      if (WINDOW_IS_TOPLEVEL (window) && !was_toplevel)
	setup_toplevel_window (window, new_parent);
	*/
		break;
	case GDK_WINDOW_TOPLEVEL:
    case GDK_WINDOW_CHILD:
    case GDK_WINDOW_DIALOG:
    case GDK_WINDOW_TEMP:
    //  if (WINDOW_IS_TOPLEVEL (window))
	{
		/*
		impl->toplevel_window_type = GDK_WINDOW_TYPE (window);
	  GDK_WINDOW_TYPE (window) = GDK_WINDOW_CHILD;
	  if (impl->toplevel)
	    {
	      if (impl->toplevel->focus_window)
		{
		  XDestroyWindow (GDK_WINDOW_XDISPLAY (window), impl->toplevel->focus_window);
		  _gdk_xid_table_remove (GDK_WINDOW_DISPLAY (window), impl->toplevel->focus_window);
		}

	      gdk_toplevel_x11_free_contents (GDK_WINDOW_DISPLAY (window), 
					      impl->toplevel);
	      g_free (impl->toplevel);
	      impl->toplevel = NULL;
	    }
		*/
	}
	break;
	}

	if (old_parent_private)
    old_parent_private->children = g_list_remove (old_parent_private->children, window);

  if ((old_parent_private &&
       (!old_parent_private->guffaw_gravity != !parent_private->guffaw_gravity)) ||
      (!old_parent_private && parent_private->guffaw_gravity))
    gdk_window_set_static_win_gravity (window, parent_private->guffaw_gravity);

  parent_private->children = g_list_prepend (parent_private->children, window);
  _gdk_window_init_position (GDK_WINDOW (window_private));

  return FALSE;



   return TRUE;
}

static void
gdk_gix_window_clear_area (GdkWindow *window,
                                gint       x,
                                gint       y,
                                gint       width,
                                gint       height,
                                gboolean   send_expose)
{
  int err;

  if (GDK_WINDOW_DESTROYED (window))
	  return ;

   err = gi_clear_area ( GDK_WINDOW_GIX_ID (window),
		x, y, width, height, send_expose);

   if (err)
   {
	   perror("gdk_gix_window_clear_area");
	   printf("gdk_gix_window_clear_area: error (%d,%d,%d,%d) %d %x ###\n",
		   x, y, width, height, send_expose,GDK_WINDOW_GIX_ID (window));
   }
}

static void
gdk_window_gix_raise (GdkWindow *window)
{
  GdkWindowImplGix *impl;
  int err;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (!GDK_WINDOW_GIX_ID (window))
  {
  printf(  "%s( %x ) zero!!!\n", __FUNCTION__, GDK_WINDOW_GIX_ID (window) );
  return;
  }

  err = gi_raise_window (GDK_WINDOW_GIX_ID (window));  
  if (err)
  {
  printf(  "%s( %x ) failed\n", __FUNCTION__, GDK_WINDOW_GIX_ID (window) );
  }
}

static void
gdk_window_gix_lower (GdkWindow *window)
{
  GdkWindowImplGix *impl;
  int err;

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, window );

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  err = gi_lower_window ( GDK_WINDOW_GIX_ID (window));  
  if (err)
  {
  printf(  "%s( %x ) failed\n", __FUNCTION__, GDK_WINDOW_GIX_ID (window) );
  }
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
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  D_DEBUG_AT(  "%s( %p, %3d,%3d, min %4dx%4d, max %4dx%4d, flags 0x%08x )\n", __FUNCTION__,
              window, x,y, min_width, min_height, max_width, max_height, flags );
  /* N/A */
}

void
gdk_window_set_geometry_hints (GdkWindow         *window,
                               const GdkGeometry *geometry,
                               GdkWindowHints     geom_mask)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
gdk_window_set_title (GdkWindow   *window,
                      const gchar *title)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  gi_set_window_utf8_caption(GDK_WINDOW_GIX_ID (window), title);

  //D_DEBUG_AT(  "%s( %p, '%s' )\n", __FUNCTION__, window, title );
  /* N/A */
}

void
gdk_window_set_role (GdkWindow   *window,
                     const gchar *role)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

#if 0 //FIXME DPP
  if (role)
	gi_change_property ( GDK_WINDOW_GIX_ID (window),
			 gi_intern_atom ( "WM_WINDOW_ROLE"),
			 GA_STRING, 8, G_PROP_MODE_Replace, (guchar *)role, strlen (role));
      else
	gi_delete_property ( GDK_WINDOW_GIX_ID (window),
			 gi_intern_atom ( "WM_WINDOW_ROLE"));
#endif
}

/**
 * gdk_window_set_startup_id:
 * @window: a toplevel #GdkWindow
 * @startup_id: a string with startup-notification identifier
 *
 * When using GTK+, typically you should use gtk_window_set_startup_id()
 * instead of this low-level function.
 *
 * Since: 2.12
 *
 **/
void
gdk_window_set_startup_id (GdkWindow   *window,
                           const gchar *startup_id)
{
  if (!GDK_WINDOW_DESTROYED (window))
    {
#if 0
      if (startup_id)
	gi_change_property ( GDK_WINDOW_GIX_ID (window),
			 gi_intern_atom ( "_NET_STARTUP_ID"), 
			 gi_intern_atom ( "UTF8_STRING"), 8,
			 G_PROP_MODE_Replace, startup_id, strlen (startup_id));
      else
	gi_delete_property ( GDK_WINDOW_GIX_ID (window),
			 gi_intern_atom ( "_NET_STARTUP_ID"));
#endif
    }
}

void
gdk_window_set_transient_for (GdkWindow *window,
                              GdkWindow *parent)
{
  GdkWindowObject *private;
  GdkWindowObject *root;
  gint i;

  //FIXME dpp

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_WINDOW (parent));

  private = GDK_WINDOW_OBJECT (window);
  root    = GDK_WINDOW_OBJECT (_gdk_parent_root);

  g_return_if_fail (GDK_WINDOW (private->parent) == _gdk_parent_root);
  g_return_if_fail (GDK_WINDOW (GDK_WINDOW_OBJECT (parent)->parent) == _gdk_parent_root);

#if 1
  root->children = g_list_remove (root->children, window);

  i = g_list_index (root->children, parent);
  if (i < 0)
    root->children = g_list_prepend (root->children, window);
  else
    root->children = g_list_insert (root->children, window, i);
  #else
   XSetTransientForHint (GDK_WINDOW_XDISPLAY (window), 
		  GDK_WINDOW_GIX_ID (window),
		  GDK_WINDOW_GIX_ID (parent));
#endif
}

static void
gdk_gix_window_set_background (GdkWindow *window,
                                    const GdkColor  *color)
{
  GdkWindowObject *private;

  g_return_if_fail (GDK_IS_WINDOW (window));

  g_return_if_fail (color != NULL);

  D_DEBUG_AT(  "%s( %p, %d,%d,%d )\n", __FUNCTION__, window, color->red, color->green, color->blue );

  private = GDK_WINDOW_OBJECT (window);
  private->bg_color = *color;

  if (private->bg_pixmap &&
      private->bg_pixmap != GDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != GDK_NO_BG)
    g_object_unref (private->bg_pixmap);

  if (!GDK_WINDOW_DESTROYED (window))
    gi_set_window_background(GDK_WINDOW_GIX_ID (window),
	  GI_RGB(color->red>>8, color->green>>8, color->blue>>8), GI_BG_USE_COLOR);

  private->bg_pixmap = NULL;
}

static void
gdk_gix_window_set_back_pixmap (GdkWindow *window,
                                     GdkPixmap *pixmap,
                                     gboolean   parent_relative)
{
  GdkWindowObject *private;
  GdkPixmap       *old_pixmap;
  gi_window_id_t xpixmap;

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (pixmap == NULL || !parent_relative);

  D_DEBUG_AT(  "%s( %p, %p, %srelative )\n", __FUNCTION__,
              window, pixmap, parent_relative ? "" : "not " );

  private = GDK_WINDOW_OBJECT (window);
  old_pixmap = private->bg_pixmap;

  if (private->bg_pixmap &&
      private->bg_pixmap != GDK_PARENT_RELATIVE_BG &&
      private->bg_pixmap != GDK_NO_BG)
    {
      g_object_unref (private->bg_pixmap);
    }

  if (parent_relative)
    {
      private->bg_pixmap = GDK_PARENT_RELATIVE_BG;
	  xpixmap = 0; //FIXME DPP
    }
  else
    {
      if (pixmap)
        {
          g_object_ref (pixmap);
          private->bg_pixmap = pixmap;
		  xpixmap = GDK_PIXMAP_GIX_ID (pixmap);
        }
      else
        {
          private->bg_pixmap = GDK_NO_BG;
		  xpixmap = 0;
        }
    }

	if (GDK_WINDOW_DESTROYED (window))
		return;
    gi_set_window_background(GDK_WINDOW_GIX_ID (window),
	  xpixmap, GI_BG_USE_PIXMAP);
}

static void
gdk_gix_window_set_cursor (GdkWindow *window,
                                GdkCursor *cursor)
{
  GdkWindowImplGix *impl;
  GdkCursor             *old_cursor;

  g_return_if_fail (GDK_IS_WINDOW (window));

  impl = GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);
  old_cursor = impl->cursor;

  impl->cursor = (cursor ?
                  gdk_cursor_ref (cursor) : gdk_cursor_new (GDK_LEFT_PTR));

 
    {
      GdkCursorGix *dfb_cursor = (GdkCursorGix *) impl->cursor;

	  if (dfb_cursor->shape_img && GDK_WINDOW_GIX_ID(window))
	  {

	  /*printf("gdk_gix_window_set_cursor: window %x, MOUSE%d,%d ON %d,%d\n", 
		  GDK_WINDOW_GIX_ID(window),
		  dfb_cursor->shape_img->w,dfb_cursor->shape_img->h,
		  dfb_cursor->hot_x, dfb_cursor->hot_y);
		  */

      /* this branch takes care of setting the cursor for unmapped windows */
      gi_setup_cursor (GI_CURSOR_USER0 , 
		  dfb_cursor->hot_x, dfb_cursor->hot_y,dfb_cursor->shape_img);
	  gi_load_cursor(GDK_WINDOW_GIX_ID(window),GI_CURSOR_USER0);

	  }

      //impl->window->SetCursorShape (impl->window,
        //                            dfb_cursor->shape,
         //                           dfb_cursor->hot_x, dfb_cursor->hot_y);
    }

  if (old_cursor)
    gdk_cursor_unref (old_cursor);
}

static void
gdk_gix_window_get_geometry (GdkWindow *window,
                                  gint      *x,
                                  gint      *y,
                                  gint      *width,
                                  gint      *height,
                                  gint      *depth)
{
  GdkWindowObject         *private;
  GdkDrawableImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = GDK_WINDOW_OBJECT (window);
  impl    = GDK_DRAWABLE_IMPL_GIX (private->impl);

  if (!GDK_WINDOW_DESTROYED (window))
    {
      if (x)
	*x = private->x;

      if (y)
	*y = private->y;

      if (width)
	*width = impl->width;

      if (height)
	*height = impl->height;

      if (depth)
	*depth = GI_RENDER_FORMAT_BPP(impl->format);
    }
}

void
_gdk_gix_calc_abs (GdkWindow *window)
{
  GdkWindowObject         *private;
  GdkDrawableImplGix *impl;
  GList                   *list;

  g_return_if_fail (GDK_IS_WINDOW (window));

  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_DRAWABLE_IMPL_GIX (private->impl);

  impl->abs_x = private->x;
  impl->abs_y = private->y;

  if (private->parent)
    {
      GdkDrawableImplGix *parent_impl =
        GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (private->parent)->impl);

      impl->abs_x += parent_impl->abs_x;
      impl->abs_y += parent_impl->abs_y;
    }

  D_DEBUG_AT(  "%s( %p ) -> %4d,%4d\n", __FUNCTION__, window, impl->abs_x, impl->abs_y );

  for (list = private->children; list; list = list->next)
    {
      _gdk_gix_calc_abs (list->data);
    }
}

//FIXME DPP
static gboolean
gdk_gix_window_get_origin (GdkWindow *window,
                                gint      *x,
                                gint      *y)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  if (!GDK_WINDOW_DESTROYED (window))
    {
      GdkDrawableImplGix *impl;
	  gi_window_info_t info;
	  //int err;

      impl = GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);
	  //gi_get_window_info (
		//			  GDK_WINDOW_GIX_ID (window),					  
		//			   &info);

      if (x)
	*x = impl->abs_x;
      if (y)
	*y = impl->abs_y;
      return TRUE;
    }

  return FALSE;
}

gboolean
gdk_window_get_deskrelative_origin (GdkWindow *window,
                                    gint      *x,
                                    gint      *y)
{
  return gdk_window_get_origin (window, x, y);
}

//FIXME DPP
void
gdk_window_get_root_origin (GdkWindow *window,
                            gint      *x,
                            gint      *y)
{
  GdkWindowObject *rover;

  g_return_if_fail (GDK_IS_WINDOW (window));

  rover = (GdkWindowObject*) window;
  if (x)
    *x = 0;
  if (y)
    *y = 0;

  if (GDK_WINDOW_DESTROYED (window))
    return;

  while (rover->parent && ((GdkWindowObject*) rover->parent)->parent)
    rover = (GdkWindowObject *) rover->parent;
  if (rover->destroyed)
    return;

  if (x)
    *x = rover->x;
  if (y)
    *y = rover->y;
}

GdkWindow *
_gdk_windowing_window_get_pointer (GdkDisplay      *display,
                                   GdkWindow       *window,
				   gint            *x,
				   gint            *y,
				   GdkModifierType *mask)
{
  GdkWindow               *retval = NULL;
  gint                     rx, ry, wx, wy;
  GdkDrawableImplGix *impl;

  g_return_val_if_fail (window == NULL || GDK_IS_WINDOW (window), NULL);

  if (!window)
    window = _gdk_parent_root;

  gdk_gix_mouse_get_info (&rx, &ry, mask);

  wx = rx;
  wy = ry;
  retval = gdk_gix_child_at (_gdk_parent_root, &wx, &wy);

  impl = GDK_DRAWABLE_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);

  if (x)
    *x = rx - impl->abs_x;
  if (y)
    *y = ry - impl->abs_y;

  return retval;
}

GdkWindow *
_gdk_windowing_window_at_pointer (GdkDisplay *display,
                                  gint       *win_x,
				  gint       *win_y)
{
  GdkWindow *retval;
  gint       wx, wy;

  if (!win_x || !win_y)
  gdk_gix_mouse_get_info (&wx, &wy, NULL);

  if (win_x)
    wx = *win_x;

  if (win_y)
    wy = *win_y;

  retval = gdk_gix_child_at (_gdk_parent_root, &wx, &wy);

  if (win_x)
    *win_x = wx;

  if (win_y)
    *win_y = wy;

  return retval;
}

void
_gdk_windowing_get_pointer (GdkDisplay       *display,
                            GdkScreen       **screen,
                            gint             *x,
                            gint             *y,
                            GdkModifierType  *mask)
{
(void)screen;
if(screen) {
	*screen = gdk_display_get_default_screen  (display);
}
_gdk_windowing_window_get_pointer (display,
				   _gdk_windowing_window_at_pointer(display,NULL,NULL),x,y,mask);

}

static GdkEventMask
gdk_gix_window_get_events (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), 0);

  if (GDK_WINDOW_DESTROYED (window))
    return 0;
  else
    return GDK_WINDOW_OBJECT (window)->event_mask;
}

const int _gdk_event_mask_table[21] =
{
  GI_MASK_EXPOSURE,
  GI_MASK_MOUSE_MOVE,//PointerMotionMask,
  0,//PointerMotionHintMask,
  GI_MASK_MOUSE_MOVE,//ButtonMotionMask,
  0,//Button1MotionMask,
  0,//Button2MotionMask,
  0,//Button3MotionMask,
  GI_MASK_BUTTON_DOWN,
  GI_MASK_BUTTON_UP,
  GI_MASK_KEY_DOWN,
  GI_MASK_KEY_UP,
  GI_MASK_MOUSE_ENTER,
  GI_MASK_MOUSE_EXIT,
  GI_MASK_FOCUS_IN|GI_MASK_FOCUS_OUT,
  GI_MASK_CONFIGURENOTIFY,//StructureNotifyMask,
  GI_MASK_PROPERTYNOTIFY,
  GI_MASK_WINDOW_HIDE|GI_MASK_WINDOW_SHOW,//VisibilityChangeMask,
  0,				/* PROXIMITY_IN */
  0,				/* PROXIMTY_OUT */
  GI_MASK_CONFIGURENOTIFY,//SubstructureNotifyMask,
  0,//ButtonPressMask      /* SCROLL; on X mouse wheel events is treated as mouse button 4/5 */
};
const int _gdk_nenvent_masks = sizeof (_gdk_event_mask_table) / sizeof (int);

static void
gdk_gix_window_set_events (GdkWindow    *window,
                                GdkEventMask  event_mask)
{
  long xevent_mask = 0;
  int i;
  int err;
  gi_window_id_t window_id = 0;

  uint32_t event_flags =	(GI_MASK_EXPOSURE    |     GI_MASK_FOCUS_OUT   |
            GI_MASK_BUTTON_DOWN  | GI_MASK_BUTTON_UP  |  GI_MASK_CLIENT_MSG|           
            GI_MASK_KEY_DOWN     | GI_MASK_KEY_UP     |     GI_MASK_FOCUS_IN|       
             GI_MASK_CONFIGURENOTIFY|	GI_MASK_WINDOW_HIDE|GI_MASK_WINDOW_SHOW|GI_MASK_SELECTIONNOTIFY
					|  GI_MASK_MOUSE_MOVE     |   GI_MASK_WINDOW_DESTROY);
  
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (event_mask & GDK_BUTTON_MOTION_MASK)
    event_mask |= (GDK_BUTTON1_MOTION_MASK |
                   GDK_BUTTON2_MOTION_MASK |
                   GDK_BUTTON3_MOTION_MASK);

  GDK_WINDOW_OBJECT (window)->event_mask = event_mask;
  if (!GDK_WINDOW_DESTROYED (window))
  {
    GDK_WINDOW_OBJECT (window)->event_mask = event_mask;
    xevent_mask = GI_MASK_CLIENT_MSG| GI_MASK_PROPERTYNOTIFY;
    for (i = 0; i < _gdk_nenvent_masks; i++)
	{
	  if (event_mask & (1 << (i + 1)))
	    xevent_mask |= _gdk_event_mask_table[i];
	}

	xevent_mask |= event_flags;   
	if(window_id)
      err = gi_set_events_mask (GDK_WINDOW_GIX_ID(window) , xevent_mask);
	else
	  printf("GDK_WINDOW_GIX_ID(window) = %p empty\n", (window));
  }
}

static void
gdk_gix_window_shape_combine_mask (GdkWindow *window,
                                        GdkBitmap *mask,
                                        gint       x,
                                        gint       y)
{
}

void
gdk_window_input_shape_combine_mask (GdkWindow *window,
                                     GdkBitmap *mask,
                                     gint       x,
                                     gint       y)
{
}

static void
gdk_gix_window_shape_combine_region (GdkWindow       *window,
                                          const GdkRegion *shape_region,
                                          gint             offset_x,
                                          gint             offset_y)
{
}

void
gdk_window_input_shape_combine_region (GdkWindow       *window,
                                       const GdkRegion *shape_region,
                                       gint             offset_x,
                                       gint             offset_y)
{
}

void
gdk_window_set_override_redirect (GdkWindow *window,
                                  gboolean   override_redirect)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  /*if (!GDK_WINDOW_DESTROYED (window))
    {
      GdkWindowObject *private = (GdkWindowObject *)window;
      GdkWindowImplX11 *impl = GDK_WINDOW_IMPL_X11 (private->impl);

      attr.override_redirect = (override_redirect? True : False);
      XChangeWindowAttributes (GDK_WINDOW_XDISPLAY (window),
			       GDK_WINDOW_GIX_ID (window),
			       CWOverrideRedirect,
			       &attr);

      impl->override_redirect = attr.override_redirect;
    }
	*/
}

#define GDK_SELECTION_MAX_SIZE 262144
//FIXME DPP
void
gdk_window_set_icon_list (GdkWindow *window,
                          GList     *pixbufs)
{
  gulong *data;
  guchar *pixels;
  gulong *p;
  gint size;
  GList *l;
  GdkPixbuf *pixbuf_small = NULL;
  GdkPixbuf *pixbuf;
  gint width, height, stride;
  gint x, y;
  gint n_channels;
  GdkDisplay *display;
  gint n;
  
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  display = gdk_drawable_get_display (window);
  
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
      if (size + 2 + width * height > GDK_SELECTION_MAX_SIZE)
	{
	  g_warning ("gdk_window_set_icon_list: icons too large");
	  break;
	}
     
      n++;
      size += 2 + width * height;

	  if (width<=22 && height<=22) //just small icon
	{
		pixbuf_small = pixbuf;
		break;
	}
      
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

	  printf("gdk_window_set_icon_list: %d,%d\n", width, height);
#if 0     
      *p++ = width;
      *p++ = height;
#endif
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
	  gi_set_window_icon(GDK_WINDOW_GIX_ID (window),(guchar*) data,
		  gdk_pixbuf_get_width(pixbuf_small),gdk_pixbuf_get_height(pixbuf_small));

      /*XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
                       GDK_WINDOW_GIX_ID (window),
		       gi_intern_atom (display, "_NET_WM_ICON"),
                       GA_CARDINAL, 32,
                       G_PROP_MODE_Replace,
                       (guchar*) data, size);
					   */
    }
  else
    {
      gi_delete_property (
                       GDK_WINDOW_GIX_ID (window),
		       gi_intern_atom ( "_NET_WM_ICON",FALSE));
    }
  
  g_free (data);
}
//FIXME DPP

void
gdk_window_set_icon (GdkWindow *window,
                     GdkWindow *icon_window,
                     GdkPixmap *pixmap,
                     GdkBitmap *mask)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
gdk_window_set_icon_name (GdkWindow   *window,
                          const gchar *name)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  gi_change_property (
		   GDK_WINDOW_GIX_ID (window),
		   gi_intern_atom ( "_NET_WM_ICON_NAME", FALSE),
		   gi_intern_atom ( "UTF8_STRING", FALSE), 8,
		   G_PROP_MODE_Replace, (guchar *)name, strlen (name));
  

  /* N/A */
}

void
gdk_window_iconify (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  gdk_window_hide (window);
}

void
gdk_window_deiconify (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  gdk_window_show (window);
}

void
gdk_window_stick (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_WINDOW_IS_MAPPED (window)){
  }
  else{
  }

  /* N/A */
}

void
gdk_window_unstick (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
gdk_gix_window_set_opacity (GdkWindow *window,
                                 guchar     opacity)
{
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);

  impl->opacity = opacity;

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;

#if 1
  guint32 cardinal;
  cardinal = opacity * 0xffffffff;

  if (cardinal == 0xffffffff)
    gi_delete_property (
		     GDK_WINDOW_GIX_ID (window),
		     gi_intern_atom ( "_NET_WM_WINDOW_OPACITY",FALSE));
  else
    gi_change_property (
		     GDK_WINDOW_GIX_ID (window),
		     gi_intern_atom ( "_NET_WM_WINDOW_OPACITY",FALSE),
		     GA_CARDINAL, 32,
		     G_PROP_MODE_Replace,
		     (guchar *) &cardinal, 1);
#endif
}

void
gdk_window_focus (GdkWindow *window,
                  guint32    timestamp)
{
  GdkWindow *toplevel;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  toplevel = gdk_gix_window_find_toplevel (window);
  if (toplevel != _gdk_parent_root)
    {
      GdkWindowImplGix *impl;

      impl = GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (toplevel)->impl);

      gi_set_focus_window (GDK_WINDOW_GIX_ID(window));
    }
}

void
gdk_window_maximize (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  gi_wm_window_maximize(GDK_WINDOW_GIX_ID(window),TRUE);
}

void
gdk_window_unmaximize (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_wm_window_maximize(GDK_WINDOW_GIX_ID(window),FALSE);
}

void
gdk_window_set_type_hint (GdkWindow        *window,
                          GdkWindowTypeHint hint)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  GDK_NOTE (MISC, g_print ("gdk_window_set_type_hint: 0x%x: %d\n",
               GDK_WINDOW_GIX_ID (window), hint));

  ((GdkWindowImplGix *)((GdkWindowObject *)window)->impl)->type_hint = hint;

#if 0
switch (hint)
    {
    case GDK_WINDOW_TYPE_HINT_DIALOG:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_DIALOG");
      break;
    case GDK_WINDOW_TYPE_HINT_MENU:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_MENU");
      break;
    case GDK_WINDOW_TYPE_HINT_TOOLBAR:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_TOOLBAR");
      break;
    case GDK_WINDOW_TYPE_HINT_UTILITY:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_UTILITY");
      break;
    case GDK_WINDOW_TYPE_HINT_SPLASHSCREEN:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_SPLASH");
      break;
    case GDK_WINDOW_TYPE_HINT_DOCK:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_DOCK");
      break;
    case GDK_WINDOW_TYPE_HINT_DESKTOP:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_DESKTOP");
      break;
    case GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU");
      break;
    case GDK_WINDOW_TYPE_HINT_POPUP_MENU:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_POPUP_MENU");
      break;
    case GDK_WINDOW_TYPE_HINT_TOOLTIP:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_TOOLTIP");
      break;
    case GDK_WINDOW_TYPE_HINT_NOTIFICATION:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_NOTIFICATION");
      break;
    case GDK_WINDOW_TYPE_HINT_COMBO:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_COMBO");
      break;
    case GDK_WINDOW_TYPE_HINT_DND:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_DND");
      break;
    default:
      g_warning ("Unknown hint %d passed to gdk_window_set_type_hint", hint);
      /* Fall thru */
    case GDK_WINDOW_TYPE_HINT_NORMAL:
      atom = gi_intern_atom ( "_NET_WM_WINDOW_TYPE_NORMAL");
      break;
    }

  gi_change_property ( GDK_WINDOW_GIX_ID (window),
		   gi_intern_atom ( "_NET_WM_WINDOW_TYPE"),
		   GA_ATOM, 32, G_PROP_MODE_Replace,
		   (guchar *)&atom, 1);
#endif
  /* N/A */
}

GdkWindowTypeHint
gdk_window_get_type_hint (GdkWindow *window)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), GDK_WINDOW_TYPE_HINT_NORMAL);
  
  if (GDK_WINDOW_DESTROYED (window))
    return GDK_WINDOW_TYPE_HINT_NORMAL;

  return GDK_WINDOW_IMPL_GIX (((GdkWindowObject *) window)->impl)->type_hint;
}

void
gdk_window_set_modal_hint (GdkWindow *window,
                           gboolean   modal)
{
  GdkWindowImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (window)->impl);

  if (modal)
    {
	  //FIXME DPP
      //gdk_window_raise (window);
    }
  else
    {
    }

  if (GDK_WINDOW_GIX_ID(window))
    {
	  //FIXME DPP
      //impl->window->SetStackingClass (impl->window,
      //                                modal ? DWSC_UPPER : DWSC_MIDDLE);
    }
}

void
gdk_window_set_skip_taskbar_hint (GdkWindow *window,
				  gboolean   skips_taskbar)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  /* N/A */
}

void
gdk_window_set_skip_pager_hint (GdkWindow *window,
				gboolean   skips_pager)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  /* N/A */
}


void
gdk_window_set_group (GdkWindow *window,
                      GdkWindow *leader)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_WINDOW (leader));
 g_warning(" Gix set_group groups not supported \n");

  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

GdkWindow * gdk_window_get_group (GdkWindow *window)
{
 g_warning(" Gix get_group groups not supported \n");
 return window;	
}

void
gdk_fb_window_set_child_handler (GdkWindow             *window,
                                 GdkWindowChildChanged  changed,
                                 GdkWindowChildGetPos   get_pos,
                                 gpointer               user_data)
{
  GdkWindowChildHandlerData *data;

  g_return_if_fail (GDK_IS_WINDOW (window));

  data = g_new (GdkWindowChildHandlerData, 1);
  data->changed   = changed;
  data->get_pos   = get_pos;
  data->user_data = user_data;

  g_object_set_data_full (G_OBJECT (window), "gdk-window-child-handler",
                          data, (GDestroyNotify) g_free);
}

void
gdk_window_set_decorations (GdkWindow       *window,
                            GdkWMDecoration  decorations)
{
  GdkWMDecoration *dec;

  g_return_if_fail (GDK_IS_WINDOW (window));

  dec = g_new (GdkWMDecoration, 1);
  *dec = decorations;

  g_object_set_data_full (G_OBJECT (window), "gdk-window-decorations",
                          dec, (GDestroyNotify) g_free);
}

gboolean
gdk_window_get_decorations (GdkWindow       *window,
			    GdkWMDecoration *decorations)
{
  GdkWMDecoration *dec;

  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  dec = g_object_get_data (G_OBJECT (window), "gdk-window-decorations");
  if (dec)
    {
      *decorations = *dec;
      return TRUE;
    }
  return FALSE;
}

void
gdk_window_set_functions (GdkWindow     *window,
                          GdkWMFunction  functions)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
  g_message("unimplemented %s", __FUNCTION__);
}

static void
gdk_gix_window_set_child_shapes (GdkWindow *window)
{
}

static void
gdk_gix_window_merge_child_shapes (GdkWindow *window)
{
}

void
gdk_window_set_child_input_shapes (GdkWindow *window)
{
}

void
gdk_window_merge_child_input_shapes (GdkWindow *window)
{
}

static gboolean
gdk_gix_window_set_static_gravities (GdkWindow *window,
                                          gboolean   use_static)
{
  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  if (GDK_WINDOW_DESTROYED (window))
    return FALSE;

  /* N/A */
  g_message("unimplemented %s", __FUNCTION__);

  return FALSE;
}

void
gdk_window_begin_resize_drag (GdkWindow     *window,
                              GdkWindowEdge  edge,
                              gint           button,
                              gint           root_x,
                              gint           root_y,
                              guint32        timestamp)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  g_message("unimplemented %s", __FUNCTION__);
}

void
gdk_window_begin_move_drag (GdkWindow *window,
                            gint       button,
                            gint       root_x,
                            gint       root_y,
                            guint32    timestamp)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  g_message("unimplemented %s", __FUNCTION__);
}

/**
 * gdk_window_get_frame_extents:
 * @window: a #GdkWindow
 * @rect: rectangle to fill with bounding box of the window frame
 *
 * Obtains the bounding box of the window, including window manager
 * titlebar/borders if any. The frame position is given in root window
 * coordinates. To get the position of the window itself (rather than
 * the frame) in root window coordinates, use gdk_window_get_origin().
 *
 **/
void
gdk_window_get_frame_extents (GdkWindow    *window,
                              GdkRectangle *rect)
{
  GdkWindowObject         *private;
  GdkDrawableImplGix *impl;

  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (rect != NULL);

  if (GDK_WINDOW_DESTROYED (window))
    return;

  private = GDK_WINDOW_OBJECT (window);

  while (private->parent && ((GdkWindowObject*) private->parent)->parent)
    private = (GdkWindowObject*) private->parent;
  if (GDK_WINDOW_DESTROYED (window))
    return;

  impl = GDK_DRAWABLE_IMPL_GIX (private->impl);

  rect->x      = impl->abs_x;
  rect->y      = impl->abs_y;
  rect->width  = impl->width;
  rect->height = impl->height;
}




/*
 * The wrapping is not perfect since gix does not give full access
 * to the current state of a window event mask etc need to fix dfb
 */
GdkWindow *
gdk_window_foreign_new_for_display (GdkDisplay* display,GdkNativeWindow anid)
{
    GdkWindow *window = NULL;
    GdkWindow              *parent =NULL;
    GdkWindowObject       *private =NULL;
    GdkWindowObject       *parent_private =NULL;
    GdkWindowImplGix *parent_impl =NULL;
    GdkWindowImplGix *impl =NULL;
    //DFBWindowOptions options;
    int        ret;
    GdkDisplayDFB * gdkdisplay =  _gdk_display;
    //gi_window_id_t dfbwindow;
	gi_window_info_t winfo;

    window = gdk_window_lookup (anid);

    if (window) {
        g_object_ref (window);
        return window;
    }
    if( display != NULL )
            gdkdisplay = GDK_DISPLAY_DFB(display);

    //ret = gdkdisplay->layer->GetWindow (gdkdisplay->layer,
     //               (gi_window_id_t)anid,&dfbwindow); 
	 
	 //FIXME DPP !!!
	 //ret  =-1;

	 ret = gi_get_window_info(anid, &winfo);

    if (ret != 0) {
        GixError ("gdk_window_new: Layer->GetWindow failed", ret);
        return NULL;
    }

    parent = _gdk_parent_root;

    if(parent) {
        parent_private = GDK_WINDOW_OBJECT (parent);
        parent_impl = GDK_WINDOW_IMPL_GIX (parent_private->impl);
    }

    window = g_object_new (GDK_TYPE_WINDOW, NULL);
    /* we hold a reference count on ourselves */
    g_object_ref (window);
    private = GDK_WINDOW_OBJECT (window);
    private->impl = g_object_new (_gdk_window_impl_get_type (), NULL);
    private->parent = parent_private;
    private->window_type = GDK_WINDOW_TOPLEVEL;
    impl = GDK_WINDOW_IMPL_GIX (private->impl);

    impl->drawable.wrapper = GDK_DRAWABLE (window);
    impl->drawable.window_id = anid;

	private->x = winfo.x;
	private->y = winfo.y;
	impl->drawable.width = winfo.width;
	impl->drawable.height = winfo.height;
	impl->drawable.format = winfo.format;
    //dfbwindow->GetOptions(dfbwindow,&options);
    //dfbwindow->GetPosition(dfbwindow,&private->x,&private->y);
    //dfbwindow->GetSize(dfbwindow,&impl->drawable.width,&impl->drawable.height);


    private->input_only = FALSE;

    //if( dfbwindow->GetSurface (dfbwindow, &impl->drawable.surface) == -1 ){
     //   private->input_only = TRUE;
     //   impl->drawable.surface = NULL;
    //}
    /*
     * Position ourselevs
     */
    _gdk_gix_calc_abs (window);

    /* We default to all events least surprise to the user 
     * minus the poll for motion events 
     */
    gdk_window_set_events (window, (GDK_ALL_EVENTS_MASK ^ GDK_POINTER_MOTION_HINT_MASK));

    //if (impl->drawable.surface)
    {
      //impl->drawable.surface->GetPixelFormat (impl->drawable.surface,
		//			      &impl->drawable.format);

		impl->drawable.format = winfo.format;

  	  private->depth = GI_RENDER_FORMAT_BPP(impl->drawable.format);
      if( parent )
        gdk_drawable_set_colormap (GDK_DRAWABLE (window), gdk_drawable_get_colormap (parent));
      else
        gdk_drawable_set_colormap (GDK_DRAWABLE (window), gdk_colormap_get_system());
    }

    //can  be null for the soft cursor window itself when
    //running a gtk gix wm
    if( gdk_display_get_default() != NULL ) {
        gdk_window_set_cursor (window,NULL);
    }

    if (parent_private)
        parent_private->children = g_list_prepend (parent_private->children,
                                               window);
    //impl->dfb_id = (gi_window_id_t)anid; //FIMXE
    gdk_gix_window_id_table_insert (impl->drawable.window_id, window);
    //gdk_gix_event_windows_add (window);

    return window;
}

GdkWindow *
gdk_window_lookup_for_display (GdkDisplay *display,GdkNativeWindow anid)
{
  return gdk_gix_window_id_table_lookup ((gi_window_id_t) anid);
}

GdkWindow *
gdk_window_lookup (GdkNativeWindow anid)
{
  return gdk_gix_window_id_table_lookup ((gi_window_id_t) anid);
}

gi_window_id_t *gdk_gix_window_lookup(GdkWindow *window )
{
  GdkWindowObject       *private;
  GdkWindowImplGix *impl;
  g_return_val_if_fail (GDK_IS_WINDOW (window),NULL);
  private = GDK_WINDOW_OBJECT (window);
  impl = GDK_WINDOW_IMPL_GIX (private->impl);
  return impl->drawable.window_id;
}

void
gdk_window_fullscreen (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (!GDK_WINDOW_IS_MAPPED (window))
	  return;

  gi_fullscreen_window(GDK_WINDOW_GIX_ID (window),TRUE);
  //g_warning ("gdk_window_fullscreen() not implemented.\n");
}

void
gdk_window_unfullscreen (GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;
  gi_fullscreen_window(GDK_WINDOW_GIX_ID (window),FALSE);
  /* g_warning ("gdk_window_unfullscreen() not implemented.\n");*/
}

void
gdk_window_set_keep_above (GdkWindow *window, gboolean setting)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_WINDOW_IS_MAPPED (window)){
  }
  gi_set_window_keep_above(GDK_WINDOW_GIX_ID (window),setting);	
}

void
gdk_window_set_keep_below (GdkWindow *window, gboolean setting)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  if (GDK_WINDOW_DESTROYED (window))
    return;
  
  gi_set_window_keep_below(GDK_WINDOW_GIX_ID (window),setting);
}

void
gdk_window_enable_synchronized_configure (GdkWindow *window)
{
}

void
gdk_window_configure_finished (GdkWindow *window)
{
}

void
gdk_display_warp_pointer (GdkDisplay *display,
                          GdkScreen  *screen,
                          gint        x,
                          gint        y)
{
  g_warning ("gdk_display_warp_pointer() not implemented.\n");
}

void
gdk_window_set_urgency_hint (GdkWindow *window,
                             gboolean   urgent)
{
  g_return_if_fail (GDK_IS_WINDOW (window));
  g_return_if_fail (GDK_WINDOW_TYPE (window) != GDK_WINDOW_CHILD);

  if (GDK_WINDOW_DESTROYED (window))
    return;

  g_warning ("gdk_window_set_urgency_hint() not implemented.\n");

}

static void
gdk_window_impl_gix_invalidate_maybe_recurse (GdkPaintable    *paintable,
                                                   const GdkRegion *region,
                                                   gboolean       (*child_func) (GdkWindow *, gpointer),
                                                   gpointer         user_data)
{
  GdkWindow *window;
  GdkWindowObject *private;
  GdkWindowImplGix *wimpl;
  GdkDrawableImplGix *impl;

  wimpl = GDK_WINDOW_IMPL_GIX (paintable);
  impl = (GdkDrawableImplGix *)wimpl;
  window = wimpl->gdkWindow;
  private = (GdkWindowObject *)window;

  GdkRegion visible_region;
  GList *tmp_list;

  g_return_if_fail (window != NULL);
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;
  
  if (private->input_only || !GDK_WINDOW_IS_MAPPED (window))
    return;

  temp_region_init_rectangle_vals( &visible_region, 0, 0, impl->width, impl->height );
  gdk_region_intersect (&visible_region, region);

  tmp_list = private->children;
  while (tmp_list)
    {
      GdkWindowObject         *child = tmp_list->data;
      GdkDrawableImplGix *cimpl = (GdkDrawableImplGix *) child->impl;
      
      if (!child->input_only)
        {
          GdkRegion child_region;
          
          temp_region_init_rectangle_vals( &child_region, child->x, child->y, cimpl->width, cimpl->height );
          
          /* remove child area from the invalid area of the parent */
          if (GDK_WINDOW_IS_MAPPED (child) && !child->shaped)
            gdk_region_subtract (&visible_region, &child_region);
          
          if (child_func && (*child_func) ((GdkWindow *)child, user_data))
            {
              gdk_region_intersect (&child_region, region);
              gdk_region_offset (&child_region, - child->x, - child->y);
              
              gdk_window_invalidate_maybe_recurse ((GdkWindow *)child,
                                                   &child_region, child_func, user_data);
            }

          temp_region_deinit( &child_region );
        }

      tmp_list = tmp_list->next;
    }
  
  if (!gdk_region_empty (&visible_region))
    {
      
      if (private->update_area)
        {
          gdk_region_union (private->update_area, &visible_region);
        }
      else
        {
          update_windows = g_slist_prepend (update_windows, window);
          private->update_area = gdk_region_copy (&visible_region);
          gdk_window_schedule_update (window);
        }
    }

  temp_region_deinit( &visible_region );
}


static void
gdk_window_impl_gix_process_updates (GdkPaintable *paintable,
                                          gboolean      update_children)
{
  GdkWindowImplGix *wimpl;
  GdkDrawableImplGix *impl;
  GdkWindow               *window;
  GdkWindowObject         *private;
  GdkRegion               *update_area;

  wimpl = GDK_WINDOW_IMPL_GIX (paintable);
  impl = (GdkDrawableImplGix *)wimpl;
  window = wimpl->gdkWindow;
  private = (GdkWindowObject *)window;

  D_DEBUG_AT(  "%s(  %schildren )\n", __FUNCTION__,
               update_children ? "update " : "no " );

  /* If an update got queued during update processing, we can get a
   * window in the update queue that has an empty update_area.
   * just ignore it.
   */
  if (!private->update_area)
    return;

  update_area = private->update_area;
  private->update_area = NULL;
      
  D_DEBUG_AT(  "  -> update area %4d,%4d-%4dx%4d\n",
              GDKDFB_RECTANGLE_VALS_FROM_BOX( &update_area->extents ) );

  if (_gdk_event_func && gdk_window_is_viewable (window))
    {
      GdkRegion *expose_region = update_area;
      GdkRegion  window_region;
          
      temp_region_init_rectangle_vals( &window_region, 0, 0, impl->width, impl->height );
      gdk_region_intersect( expose_region, &window_region );
      temp_region_deinit (&window_region);
          
      if (!gdk_region_empty (expose_region) && (private->event_mask & GDK_EXPOSURE_MASK))
        {
          GdkEvent event;
              
          event.expose.type = GDK_EXPOSE;
          event.expose.window = g_object_ref (window);
          event.expose.send_event = FALSE;
          event.expose.count = 0;
          event.expose.region = expose_region;
          gdk_region_get_clipbox (expose_region, &event.expose.area);
          (*_gdk_event_func) (&event, _gdk_event_data);
              
          g_object_unref (window);
        }

      if (expose_region != update_area)
        gdk_region_destroy (expose_region);
    }

  gdk_region_destroy (update_area);
}


static void
gdk_window_impl_gix_begin_paint_region (GdkPaintable    *paintable,
                                             const GdkRegion *region)
{
  GdkDrawableImplGix *impl;
  GdkWindowImplGix   *wimpl;
  gint                     i;


  g_assert (region != NULL );
  wimpl = GDK_WINDOW_IMPL_GIX (paintable);
  impl = (GdkDrawableImplGix *)wimpl;

  if (!region)
    return;

  D_DEBUG_AT(  "%s( %p ) <- %4d,%4d-%4d,%4d (%ld boxes)\n", __FUNCTION__,
              paintable, GDKDFB_RECTANGLE_VALS_FROM_BOX(&region->extents), region->numRects );

  /* When it's buffered... */
  if (impl->buffered)
    {
      /* ...we're already painting on it! */
      g_assert( impl->paint_depth > 0 );
    
      D_DEBUG_AT(  "  -> painted  %4d,%4d-%4dx%4d (%ld boxes)\n",
                  DFB_RECTANGLE_VALS_FROM_REGION( &impl->paint_region.extents ), impl->paint_region.numRects );

      /* Add the new region to the paint region... */
      gdk_region_union (&impl->paint_region, region);
    }
  else
    {
      /* ...otherwise it's the first time! */
      g_assert( impl->paint_depth == 0 );

      /* Generate the clip region for painting around child windows. */
      gdk_gix_clip_region( GDK_DRAWABLE(paintable), NULL, NULL, &impl->clip_region );

      /* Initialize the paint region with the new one... */
      temp_region_init_copy( &impl->paint_region, region );

      impl->buffered = TRUE;
    }

  D_DEBUG_AT(  "  -> painting %4d,%4d-%4dx%4d (%ld boxes)\n",
              DFB_RECTANGLE_VALS_FROM_REGION( &impl->paint_region.extents ), impl->paint_region.numRects );

  /* ...but clip the initial/compound result against the clip region. */
  gdk_region_intersect (&impl->paint_region, &impl->clip_region);

  D_DEBUG_AT(  "  -> clipped  %4d,%4d-%4dx%4d (%ld boxes)\n",
              DFB_RECTANGLE_VALS_FROM_REGION( &impl->paint_region.extents ), impl->paint_region.numRects );

  impl->paint_depth++;

  D_DEBUG_AT(  "  -> depth is now %d\n", impl->paint_depth );

  for (i = 0; i < region->numRects; i++)
    {
      GdkRegionBox *box = &region->rects[i];

      D_DEBUG_AT(  "  -> [%2d] %4d,%4d-%4dx%4d\n", i, GDKDFB_RECTANGLE_VALS_FROM_BOX( box ) );

      gdk_window_clear_area (GDK_WINDOW(wimpl->gdkWindow),
                             box->x1,
                             box->y1,
                             box->x2 - box->x1,
                             box->y2 - box->y1);
    }
}

static void
gdk_window_impl_gix_end_paint (GdkPaintable *paintable)
{
  GdkDrawableImplGix *impl;

  impl = GDK_DRAWABLE_IMPL_GIX (paintable);

  D_DEBUG_AT(  "%s( %p )\n", __FUNCTION__, paintable );

  g_return_if_fail (impl->paint_depth > 0);

  g_assert( impl->buffered );

  impl->paint_depth--;

#ifdef GDK_GIX_NO_EXPERIMENTS
  if (impl->paint_depth == 0)
    {
      impl->buffered = FALSE;

      if (impl->paint_region.numRects)
        {
          gi_boxrec_t reg = { impl->paint_region.extents.x1,
                            impl->paint_region.extents.y1,
                            impl->paint_region.extents.x2-1,
                            impl->paint_region.extents.y2-1 };

          //printf(  "  -> flip %4d,%4d-%4dx%4d (%ld boxes)\n",
          //            DFB_RECTANGLE_VALS_FROM_REGION( &reg ), impl->paint_region.numRects );

		  //gi_set_gc_clip_rectangles( gc,  &reg, 1);

          //impl->surface->Flip( impl->surface, &reg, 0 ); //FIXME DPP

          temp_region_reset( &impl->paint_region );
        }
    }
#else
  if (impl->paint_depth == 0)
    {
      impl->buffered = FALSE;
  
      temp_region_deinit( &impl->clip_region );
  
      if (impl->paint_region.numRects)
        {
          GdkWindow *window = GDK_WINDOW( impl->wrapper );
  
          if (GDK_IS_WINDOW(window))
            {
              GdkWindowObject *top = GDK_WINDOW_OBJECT( gdk_window_get_toplevel( window ) );
  
              if (top)
                {
                  gi_boxrec_t              reg;
                  GdkWindowImplGix *wimpl = GDK_WINDOW_IMPL_GIX (top->impl);
  
                  reg.x1 = impl->abs_x - top->x + impl->paint_region.extents.x1;
                  reg.y1 = impl->abs_y - top->y + impl->paint_region.extents.y1;
                  reg.x2 = impl->abs_x - top->x + impl->paint_region.extents.x2 - 1;
                  reg.y2 = impl->abs_y - top->y + impl->paint_region.extents.y2 - 1;
  
                  D_DEBUG_AT(  "  -> queue flip %4d,%4d-%4dx%4d (%ld boxes)\n",
                              DFB_RECTANGLE_VALS_FROM_REGION( &reg ), impl->paint_region.numRects );
  
                  dfb_updates_add( &wimpl->flips, &reg );
                }
            }
  
          temp_region_reset( &impl->paint_region );
        }
    }
#endif
  else
    D_DEBUG_AT(  "  -> depth is still %d\n", impl->paint_depth );
}


static void
gdk_window_impl_gix_paintable_init (GdkPaintableIface *iface)
{
  iface->begin_paint_region = gdk_window_impl_gix_begin_paint_region;
  iface->end_paint = gdk_window_impl_gix_end_paint;

  iface->invalidate_maybe_recurse = gdk_window_impl_gix_invalidate_maybe_recurse;
  iface->process_updates = gdk_window_impl_gix_process_updates;
}

void
gdk_window_beep (GdkWindow *window)
{
  gdk_display_beep (gdk_display_get_default());
}

void
gdk_window_set_opacity (GdkWindow *window,
			gdouble    opacity)
{
  GdkDisplay *display;
  guint8 cardinal;

  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  display = gdk_drawable_get_display (window);

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;
  cardinal = opacity * 0xff;
  gdk_gix_window_set_opacity(window,cardinal);
}

void
_gdk_windowing_window_set_composited (GdkWindow *window,
                                      gboolean   composited)
{
}

static void
gdk_window_impl_iface_init (GdkWindowImplIface *iface)
{
  iface->show = gdk_gix_window_show;
  iface->hide = gdk_gix_window_hide;
  iface->withdraw = gdk_gix_window_withdraw;
  iface->raise = gdk_window_gix_raise;
  iface->lower = gdk_window_gix_lower;
  iface->move_resize = gdk_gix_window_move_resize;
  iface->move_region = _gdk_gix_window_move_region;
  iface->scroll = _gdk_gix_window_scroll;
  iface->clear_area = gdk_gix_window_clear_area;
  iface->set_background = gdk_gix_window_set_background;
  iface->set_back_pixmap = gdk_gix_window_set_back_pixmap;
  iface->get_events = gdk_gix_window_get_events;
  iface->set_events = gdk_gix_window_set_events;
  iface->reparent = gdk_gix_window_reparent;
  iface->set_cursor = gdk_gix_window_set_cursor;
  iface->get_geometry = gdk_gix_window_get_geometry;
  iface->get_origin = gdk_gix_window_get_origin;
  iface->get_offsets = _gdk_gix_window_get_offsets;
  iface->shape_combine_mask = gdk_gix_window_shape_combine_mask;
  iface->shape_combine_region = gdk_gix_window_shape_combine_region;
  iface->set_child_shapes = gdk_gix_window_set_child_shapes;
  iface->merge_child_shapes = gdk_gix_window_merge_child_shapes;
  iface->set_static_gravities = gdk_gix_window_set_static_gravities;
}

#define __GDK_WINDOW_X11_C__
#include "gdkaliasdef.c"

