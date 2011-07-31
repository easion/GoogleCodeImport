/*
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

#ifndef GDK_DISPLAY_GIX_H
#define GDK_DISPLAY_GIX_H

#include <gi/gi.h>
#include <gi/property.h>

//#include <gix.h>
#include <gdk/gdkdisplay.h>
#include <gdk/gdkkeys.h>

G_BEGIN_DECLS

typedef struct _GdkDisplayGIX GdkDisplayGIX;
typedef struct _GdkDisplayGIXClass GdkDisplayGIXClass;


#define GDK_TYPE_DISPLAY_GIX              (gdk_display_gix_get_type())
#define GDK_DISPLAY_GIX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DISPLAY_GIX, GdkDisplayGIX))
#define GDK_DISPLAY_GIX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DISPLAY_GIX, GdkDisplayGIXClass))
#define GDK_IS_DISPLAY_GIX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DISPLAY_GIX))
#define GDK_IS_DISPLAY_GIX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DISPLAY_GIX))
#define GDK_DISPLAY_GIX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_DISPLAY_GIX, GdkDisplayGIXClass))

struct _GdkDisplayGIX
{
  GdkDisplay parent;
  int sock_id;  
  GdkKeymap *keymap;

  /* X ID hashtable */
  GHashTable *xid_ht;
  /* translation queue */
  GQueue *translate_queue;

  /* Mapping to/from virtual atoms */
  GHashTable *atom_from_virtual;
  GHashTable *atom_to_virtual;
};

struct _GdkDisplayGIXClass
{
  GdkDisplayClass parent;
};

GType      gdk_display_gix_get_type            (void);

void
_gdk_window_init_position (GdkWindow *window);
G_END_DECLS

#endif /* GDK_DISPLAY_GIX_H */
