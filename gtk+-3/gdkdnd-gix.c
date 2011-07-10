


#include "config.h"

#include "gdkdndprivate.h"

#include "gdkmain.h"
#include "gdkinternals.h"
#include "gdkproperty.h"
#include "gdkprivate-gix.h"
#include "gdkdisplay-gix.h"

#include <string.h>

#define GDK_TYPE_GIX_DRAG_CONTEXT              (gdk_gix_drag_context_get_type ())
#define GDK_GIX_DRAG_CONTEXT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_GIX_DRAG_CONTEXT, GdkGixDragContext))
#define GDK_GIX_DRAG_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_GIX_DRAG_CONTEXT, GdkGixDragContextClass))
#define GDK_IS_GIX_DRAG_CONTEXT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_GIX_DRAG_CONTEXT))
#define GDK_IS_GIX_DRAG_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_GIX_DRAG_CONTEXT))
#define GDK_GIX_DRAG_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_GIX_DRAG_CONTEXT, GdkGixDragContextClass))

typedef struct _GdkGixDragContext GdkGixDragContext;
typedef struct _GdkGixDragContextClass GdkGixDragContextClass;

struct _GdkGixDragContext
{
  GdkDragContext context;
};

struct _GdkGixDragContextClass
{
  GdkDragContextClass parent_class;
};

static GList *contexts;

G_DEFINE_TYPE (GdkGixDragContext, gdk_gix_drag_context, GDK_TYPE_DRAG_CONTEXT)

static void
gdk_gix_drag_context_finalize (GObject *object)
{
  GdkDragContext *context = GDK_DRAG_CONTEXT (object);

  contexts = g_list_remove (contexts, context);

  G_OBJECT_CLASS (gdk_gix_drag_context_parent_class)->finalize (object);
}

static GdkWindow *
gdk_gix_drag_context_find_window (GdkDragContext  *context,
				      GdkWindow       *drag_window,
				      GdkScreen       *screen,
				      gint             x_root,
				      gint             y_root,
				      GdkDragProtocol *protocol)
{
  return NULL;
}

static gboolean
gdk_gix_drag_context_drag_motion (GdkDragContext *context,
				      GdkWindow      *dest_window,
				      GdkDragProtocol protocol,
				      gint            x_root,
				      gint            y_root,
				      GdkDragAction   suggested_action,
				      GdkDragAction   possible_actions,
				      guint32         time)
{
  return FALSE;
}

static void
gdk_gix_drag_context_drag_abort (GdkDragContext *context,
				     guint32         time)
{
}

static void
gdk_gix_drag_context_drag_drop (GdkDragContext *context,
				    guint32         time)
{
}

/* Destination side */

static void
gdk_gix_drag_context_drag_status (GdkDragContext *context,
				      GdkDragAction   action,
				      guint32         time_)
{
}

static void
gdk_gix_drag_context_drop_reply (GdkDragContext *context,
				     gboolean        accepted,
				     guint32         time_)
{
}

static void
gdk_gix_drag_context_drop_finish (GdkDragContext *context,
				      gboolean        success,
				      guint32         time)
{
}

static gboolean
gdk_gix_drag_context_drop_status (GdkDragContext *context)
{
  return FALSE;
}

static GdkAtom
gdk_gix_drag_context_get_selection (GdkDragContext *context)
{
    return GDK_NONE;
}

static void
gdk_gix_drag_context_init (GdkGixDragContext *context)
{
  contexts = g_list_prepend (contexts, context);
}

static void
gdk_gix_drag_context_class_init (GdkGixDragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GdkDragContextClass *context_class = GDK_DRAG_CONTEXT_CLASS (klass);

  object_class->finalize = gdk_gix_drag_context_finalize;

  context_class->find_window = gdk_gix_drag_context_find_window;
  context_class->drag_status = gdk_gix_drag_context_drag_status;
  context_class->drag_motion = gdk_gix_drag_context_drag_motion;
  context_class->drag_abort = gdk_gix_drag_context_drag_abort;
  context_class->drag_drop = gdk_gix_drag_context_drag_drop;
  context_class->drop_reply = gdk_gix_drag_context_drop_reply;
  context_class->drop_finish = gdk_gix_drag_context_drop_finish;
  context_class->drop_status = gdk_gix_drag_context_drop_status;
  context_class->get_selection = gdk_gix_drag_context_get_selection;
}

GdkDragProtocol
_gdk_gix_window_get_drag_protocol (GdkWindow *window, GdkWindow **target)
{
  return 0;
}

void
_gdk_gix_window_register_dnd (GdkWindow *window)
{
}

GdkDragContext *
_gdk_gix_window_drag_begin (GdkWindow *window,
				GdkDevice *device,
				GList     *targets)
{
  GdkDragContext *context;

  context = (GdkDragContext *) g_object_new (GDK_TYPE_GIX_DRAG_CONTEXT, NULL);
  g_assert(context != NULL);

  gdk_drag_context_set_device (context, device);

  return context;
}
