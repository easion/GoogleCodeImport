


#include "config.h"

#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gdesktopappinfo.h>

#include "gdkgix.h"
#include "gdkprivate-gix.h"
#include "gdkapplaunchcontextprivate.h"
#include "gdkscreen.h"
#include "gdkinternals.h"
#include "gdkintl.h"

static char *
gdk_gix_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
						      GAppInfo          *info, 
						      GList             *files)
{
  return NULL;
}

static void
gdk_gix_app_launch_context_launch_failed (GAppLaunchContext *context, 
					      const char        *startup_notify_id)
{
}

typedef struct _GdkGixAppLaunchContext GdkGixAppLaunchContext;
typedef struct _GdkGixAppLaunchContextClass GdkGixAppLaunchContextClass;

struct _GdkGixAppLaunchContext
{
  GdkAppLaunchContext base;
  gchar *name;
  guint serial;
};

struct _GdkGixAppLaunchContextClass
{
  GdkAppLaunchContextClass base_class;
};

G_DEFINE_TYPE (GdkGixAppLaunchContext, gdk_gix_app_launch_context, GDK_TYPE_APP_LAUNCH_CONTEXT)

static void
gdk_gix_app_launch_context_class_init (GdkGixAppLaunchContextClass *klass)
{
  GAppLaunchContextClass *ctx_class = G_APP_LAUNCH_CONTEXT_CLASS (klass);

  ctx_class->get_startup_notify_id = gdk_gix_app_launch_context_get_startup_notify_id;
  ctx_class->launch_failed = gdk_gix_app_launch_context_launch_failed;
}

static void
gdk_gix_app_launch_context_init (GdkGixAppLaunchContext *ctx)
{
}

GdkAppLaunchContext *
_gdk_gix_display_get_app_launch_context (GdkDisplay *display)
{
  GdkAppLaunchContext *ctx;

  ctx = g_object_new (gdk_gix_app_launch_context_get_type (),
                      "display", display,
                      NULL);

  return ctx;
}
