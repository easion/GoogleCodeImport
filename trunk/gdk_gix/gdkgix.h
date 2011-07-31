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
#ifndef __GDK_GIX_H__
#define __GDK_GIX_H__

/* This define disables some experimental code
 */
#define GDK_GIX_NO_EXPERIMENTS

#include <cairo.h>
#include <gi/gi.h>
#include <gi/property.h>
#include "gdk/gdkprivate.h"

typedef unsigned GIXSurfacePixelFormat;
typedef struct 
{
	int r,g,b,a;
}GIXColor;

extern GdkWindow * _gdk_parent_root;

G_BEGIN_DECLS

#define GDK_ROOT_WINDOW()      _gdk_parent_root 

//#define GDK_WINDOW_GIX_ID(win) (GDK_WINDOW_IMPL_GIX (GDK_WINDOW_OBJECT (win)->impl)->drawable.window_id)


/* used for the --transparent-unfocused hack */
extern gboolean            gdk_gix_apply_focus_opacity;

/* used for the --enable-color-keying hack */
extern gboolean            gdk_gix_enable_color_keying;
extern GIXColor            gdk_gix_bg_color;
extern GIXColor            gdk_gix_bg_color_key;

/* to disable antialiasing */
extern gboolean            gdk_gix_monochrome_fonts;


/* GTK+-Gix specific functions */

void        gdk_gix_window_set_opacity (GdkWindow             *window,
                                             guchar                 opacity);

GdkWindow * gdk_gix_window_new         (GdkWindow             *parent,
                                             GdkWindowAttr         *attributes,
                                             gint                   attributes_mask);

GdkVisual * gdk_gix_visual_by_format   (GIXSurfacePixelFormat  pixel_format);

gi_window_id_t gdk_gix_window_lookup(GdkWindow *window );

GdkWindow *gdk_gix_create_child_window(GdkWindow *parent);


#define D_UNIMPLEMENTED do{\
		g_print("%s: %s line %d UNIMPLEMENTED !!!\n",__FILE__,__FUNCTION__,__LINE__);\
	}\
	while (0)


#define GixError g_print
G_END_DECLS

#endif /* __GDK_GIX_H__ */
