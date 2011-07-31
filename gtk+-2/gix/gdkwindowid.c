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
 * GTK+ Gix backend
 * Copyright (C) 2011 www.hanxuantech.com.
 * Written by Easion <easion@hanxuantech.com> , it's based
 * on DirectFB port.
 */

#include "config.h"

#include "gdkgix.h"
#include "gdkprivate-gix.h"


static GHashTable *window_id_ht = NULL;


void
gdk_gix_window_id_table_insert (gi_window_id_t  gix_id,
                                     GdkWindow   *window)
{
  if (!window_id_ht)
    window_id_ht = g_hash_table_new (g_direct_hash, g_direct_equal);

  g_hash_table_insert (window_id_ht, GUINT_TO_POINTER (gix_id), window);
}

void
gdk_gix_window_id_table_remove (gi_window_id_t gix_id)
{
  if (window_id_ht)
    g_hash_table_remove (window_id_ht, GUINT_TO_POINTER (gix_id));
}

GdkWindow *
gdk_gix_window_id_table_lookup (gi_window_id_t gix_id)
{
  GdkWindow *window = NULL;

  if (window_id_ht)
    window = (GdkWindow *) g_hash_table_lookup (window_id_ht,
                                                GUINT_TO_POINTER (gix_id));

  return window;
}
