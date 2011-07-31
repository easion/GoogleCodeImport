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
#include "gdkscreen.h"
#include "gdkgix.h"
#include "gdkprivate-gix.h"

#include "gdkinternals.h"
#include "gdkalias.h"


static GdkColormap *default_colormap = NULL;

GdkDisplay *
gdk_screen_get_display (GdkScreen *screen)
{
  return GDK_DISPLAY_OBJECT(_gdk_display);
}

GdkWindow *
gdk_screen_get_root_window (GdkScreen *screen)
{
  return _gdk_parent_root;
}

GdkColormap*
gdk_screen_get_default_colormap (GdkScreen *screen)
{
  return default_colormap;
}

void
gdk_screen_set_default_colormap (GdkScreen   *screen,
				 GdkColormap *colormap)
{
  GdkColormap *old_colormap;

  g_return_if_fail (GDK_IS_SCREEN (screen));
  g_return_if_fail (GDK_IS_COLORMAP (colormap));

  old_colormap = default_colormap;

  default_colormap = g_object_ref (colormap);

  if (old_colormap)
    g_object_unref (old_colormap);
}

gint
gdk_screen_get_n_monitors (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return 1;
}

void
gdk_screen_get_monitor_geometry (GdkScreen    *screen,
				 gint          num_monitor,
				 GdkRectangle *dest)
{
  g_return_if_fail (GDK_IS_SCREEN (screen));
  g_return_if_fail (dest != NULL);

  dest->x = 0;
  dest->y = 0;
  dest->width  = gdk_screen_width ();
  dest->height = gdk_screen_height ();
}

gint
gdk_screen_get_monitor_width_mm (GdkScreen *screen,
                                 gint       monitor_num)
{
  return gdk_screen_get_width_mm (screen);
}

gint
gdk_screen_get_monitor_height_mm (GdkScreen *screen,
                                  gint       monitor_num)
{
  return gdk_screen_get_height_mm (screen);
}

gchar *
gdk_screen_get_monitor_plug_name (GdkScreen *screen,
                                  gint       monitor_num)
{
  return g_strdup ("Gix");
}

gint
gdk_screen_get_number (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

  return 0;
}


gchar *
_gdk_windowing_substitute_screen_number (const gchar *display_name,
					 int          screen_number)
{
  return g_strdup (display_name);
}

gchar *
gdk_screen_make_display_name (GdkScreen *screen)
{
  return g_strdup ("Gix");
}

gint
gdk_screen_get_width (GdkScreen *screen)
{
  gi_screen_info_t dlc;

  gi_get_screen_info ( &dlc);

  return dlc.scr_width;
}

gint
gdk_screen_get_height (GdkScreen *screen)
{
  gi_screen_info_t dlc;

  gi_get_screen_info ( &dlc);

  return dlc.scr_height;
}

gint
gdk_screen_get_width_mm (GdkScreen *screen)
{
  static gboolean first_call = TRUE;
  gi_screen_info_t dlc;

  if (first_call)
    {
      g_message
        ("gdk_screen_width_mm() assumes a screen resolution of 72 dpi");
      first_call = FALSE;
    }

  gi_get_screen_info ( &dlc);

  return (dlc.scr_width * 254) / 720;
}

gint
gdk_screen_get_height_mm (GdkScreen *screen)
{
  static gboolean first_call = TRUE;
  gi_screen_info_t dlc;

  if (first_call)
    {
      g_message
        ("gdk_screen_height_mm() assumes a screen resolution of 72 dpi");
      first_call = FALSE;
    }

  gi_get_screen_info ( &dlc);

  return (dlc.scr_height * 254) / 720;
}

GdkVisual *
gdk_screen_get_rgba_visual (GdkScreen *screen)
{
  static GdkVisual *rgba_visual;
  if( !rgba_visual )
    rgba_visual = gdk_gix_visual_by_format(GI_RENDER_a8r8g8b8);
  return rgba_visual;
}

GdkColormap *
gdk_screen_get_rgba_colormap (GdkScreen *screen)
{
  static GdkColormap *rgba_colormap;
  if( !rgba_colormap && gdk_screen_get_rgba_visual(screen) )
    rgba_colormap = gdk_colormap_new (gdk_screen_get_rgba_visual(screen),FALSE);
  return rgba_colormap;
}

GdkWindow *
gdk_screen_get_active_window (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return NULL;
}

GList *
gdk_screen_get_window_stack (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

  return NULL;
}

gboolean
gdk_screen_is_composited (GdkScreen *screen)
{
   g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
   return FALSE;
} 

#define __GDK_SCREEN_X11_C__
#include "gdkaliasdef.c"
