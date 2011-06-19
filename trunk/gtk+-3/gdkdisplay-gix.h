

#ifndef __GDK_DISPLAY_GIX__
#define __GDK_DISPLAY_GIX__

#include <stdint.h>
#include <gi/gi.h>
#include <gi/property.h>
#include <gi/user_font.h>


#include <cairo-gix.h>
#include <glib.h>
#include <gdk/gdkkeys.h>
#include <gdk/gdkwindow.h>
#include <gdk/gdkinternals.h>
#include <gdk/gdk.h>		/* For gdk_get_program_class() */

#include "gdkdisplayprivate.h"

G_BEGIN_DECLS

typedef struct _GdkDisplayGix GdkDisplayGix;
typedef struct _GdkDisplayGixClass GdkDisplayGixClass;

#define GDK_TYPE_DISPLAY_GIX              (_gdk_display_gix_get_type())
#define GDK_DISPLAY_GIX(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_DISPLAY_GIX, GdkDisplayGix))
#define GDK_DISPLAY_GIX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_DISPLAY_GIX, GdkDisplayGixClass))
#define GDK_IS_DISPLAY_GIX(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_DISPLAY_GIX))
#define GDK_IS_DISPLAY_GIX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_DISPLAY_GIX))
#define GDK_DISPLAY_GIX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_DISPLAY_GIX, GdkDisplayGixClass))

struct _GdkDisplayGix
{
  GdkDisplay parent_instance;
  GdkScreen *display_screen;

  /* Keyboard related information */
  GdkKeymap *keymap;

  /* input GdkDevice list */
  GList *input_devices;

  /* Startup notification */
  gchar *startup_notification_id;

  /* Time of most recent user interaction. */
  gulong user_time;

  /* Gix fields below */
 
  GSource *event_source;
 
  gi_screen_info_t screen_info;
  int fd; 

  /* List of functions to go from extension event => X window */
  GSList *event_types;

  /* X ID hashtable */
  GHashTable *xid_ht;

};

struct _GdkDisplayGixClass
{
  GdkDisplayClass parent_class;
};

GType      _gdk_display_gix_get_type            (void);

G_END_DECLS

#endif				/* __GDK_DISPLAY_GIX__ */
