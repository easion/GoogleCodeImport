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
#include "gdk.h"

#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkalias.h"


GdkDisplayGIX *_gdk_display = NULL;
GdkScreen          * _gdk_screen                              = NULL;

gboolean              gdk_gix_apply_focus_opacity        = FALSE;
gboolean              gdk_gix_enable_color_keying        = FALSE;
GIXColor              gdk_gix_bg_color                   = { 0, 0, 0, 0 };
GIXColor              gdk_gix_bg_color_key               = { 0, 0, 0, 0 };
gboolean              gdk_gix_monochrome_fonts           = FALSE;

GdkWindow          * _gdk_gix_pointer_grab_window        = NULL;
GdkWindow          * _gdk_gix_keyboard_grab_window       = NULL;
GdkWindow          * _gdk_gix_pointer_grab_confine       = NULL;
gboolean             _gdk_gix_pointer_grab_owner_events  = FALSE;
gboolean             _gdk_gix_keyboard_grab_owner_events = FALSE;
GdkEventMask         _gdk_gix_pointer_grab_events        = 0;
GdkEventMask         _gdk_gix_keyboard_grab_events       = 0;
GdkCursor          * _gdk_gix_pointer_grab_cursor        = NULL;

GdkAtom              _gdk_selection_property                  = 0;

#include "gdkaliasdef.c"
