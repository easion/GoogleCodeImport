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

#include <stdlib.h>
#include "config.h"
#include "gdk.h"
#include "gdkx.h"
#include "gdkprivate.h"
#include "gdkinput.h"


/* Forward declarations */

static gint gdk_input_enable_window (GdkWindow *window,
				     GdkDevicePrivate *gdkdev);
static gint gdk_input_disable_window (GdkWindow *window,
				      GdkDevicePrivate *gdkdev);
static GdkInputWindow *gdk_input_window_find (GdkWindow *window);
static GdkDevicePrivate *gdk_input_find_device (guint32 id);


/* Incorporate the specific routines depending on compilation options */

static const GdkAxisUse gdk_input_core_axes[] = { GDK_AXIS_X, GDK_AXIS_Y };

static const GdkDeviceInfo gdk_input_core_info =
{
  GDK_CORE_POINTER,
  "Core Pointer",
  GDK_SOURCE_MOUSE,
  GDK_MODE_SCREEN,
  TRUE,
  2,
  gdk_input_core_axes
};

/* Global variables  */

GdkInputVTable    gdk_input_vtable;
/* information about network port and host for gxid daemon */
gchar            *gdk_input_gxid_host;
gint              gdk_input_gxid_port;
gint              gdk_input_ignore_core;

/* Local variables */

static GList            *gdk_input_devices;
static GList            *gdk_input_windows;

#include "gdkinputnone.h"
#include "gdkinputcommon.h"
#include "gdkinputxfree.h"
#include "gdkinputgxi.h"

GList *
gdk_input_list_devices (void)
{
	printf("Enter gdk_input_list_devices\n");
}

void
gdk_input_set_source (guint32 deviceid, GdkInputSource source)
{
	printf("Enter gdk_input_set_source\n");
}

gboolean
gdk_input_set_mode (guint32 deviceid, GdkInputMode mode)
{
	printf("Enter gdk_input_set_mode\n");
}

void
gdk_input_set_axes (guint32 deviceid, GdkAxisUse *axes)
{
	printf("Enter gdk_input_set_axes\n");
}

void gdk_input_set_key (guint32 deviceid,
			guint   index,
			guint   keyval,
			GdkModifierType modifiers)
{
	printf("Enter gdk_input_set_key\n");
}

GdkTimeCoord *
gdk_input_motion_events (GdkWindow *window,
			 guint32 deviceid,
			 guint32 start,
			 guint32 stop,
			 gint *nevents_return)
{
	printf("Enter gdk_input_motion_events\n");
}

static gint
gdk_input_enable_window (GdkWindow *window, GdkDevicePrivate *gdkdev)
{
	printf("Enter gdk_input_enable_window\n");
}

static gint
gdk_input_disable_window (GdkWindow *window, GdkDevicePrivate *gdkdev)
{
	printf("Enter gdk_input_disable_window\n");
}


static GdkInputWindow *
gdk_input_window_find(GdkWindow *window)
{
	printf("Enter gdk_input_window_find\n");
}

/* FIXME: this routine currently needs to be called between creation
   and the corresponding configure event (because it doesn't get the
   root_relative_geometry).  This should work with
   gtk_window_set_extension_events, but will likely fail in other
   cases */

void
gdk_input_set_extension_events (GdkWindow *window, gint mask,
				GdkExtensionMode mode)
{
	printf("Enter gdk_input_set_extension_events\n");
}

void
gdk_input_window_destroy (GdkWindow *window)
{
	printf("Enter gdk_input_window_destroy\n");
}

void
gdk_input_exit (void)
{
  GList *tmp_list;
  GdkDevicePrivate *gdkdev;

  for (tmp_list = gdk_input_devices; tmp_list; tmp_list = tmp_list->next)
    {
      gdkdev = (GdkDevicePrivate *)(tmp_list->data);
      if (gdkdev->info.deviceid != GDK_CORE_POINTER)
        {
          gdk_input_set_mode(gdkdev->info.deviceid,GDK_MODE_DISABLED);

          g_free(gdkdev->info.name);
          g_free(gdkdev->info.axes);
          g_free(gdkdev->info.keys);
          g_free(gdkdev);
        }
    }

  g_list_free(gdk_input_devices);

  for (tmp_list = gdk_input_windows; tmp_list; tmp_list = tmp_list->next)
    {
      g_free(tmp_list->data);
    }
  g_list_free(gdk_input_windows);

}

static GdkDevicePrivate *
gdk_input_find_device(guint32 id)
{
	printf("Enter gdk_input_find_device\n");
}

void
gdk_input_window_get_pointer (GdkWindow       *window,
			      guint32	  deviceid,
			      gdouble         *x,
			      gdouble         *y,
			      gdouble         *pressure,
			      gdouble         *xtilt,
			      gdouble         *ytilt,
			      GdkModifierType *mask)
{
	printf("Enter gdk_input_window_get_pointer\n");
}
