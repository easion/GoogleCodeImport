


#include "config.h"

#include "gdkdisplaymanagerprivate.h"
#include "gdkdisplay-gix.h"
#include "gdkprivate-gix.h"

#include "gdkinternals.h"


struct _GdkGixDisplayManager
{
  GdkDisplayManager parent;

  GdkDisplay *default_display;
  GSList *displays;
};

struct _GdkGixDisplayManagerClass
{
  GdkDisplayManagerClass parent_class;
};



G_DEFINE_TYPE (GdkGixDisplayManager, gdk_gix_display_manager, GDK_TYPE_DISPLAY_MANAGER)

static void
gdk_gix_display_manager_finalize (GObject *object)
{
  g_error ("A GdkGixDisplayManager object was finalized. This should not happen");
  G_OBJECT_CLASS (gdk_gix_display_manager_parent_class)->finalize (object);
}

static GdkDisplay *
gdk_gix_display_manager_open_display (GdkDisplayManager *manager,
					  const gchar       *name)
{
  return _gdk_gix_display_open (name);
}

static GSList *
gdk_gix_display_manager_list_displays (GdkDisplayManager *manager)
{
  return g_slist_copy (GDK_GIX_DISPLAY_MANAGER (manager)->displays);
}

static void
gdk_gix_display_manager_set_default_display (GdkDisplayManager *manager,
						 GdkDisplay        *display)
{
  GDK_GIX_DISPLAY_MANAGER (manager)->default_display = display;

  //g_print("gdk_gix_display_manager_set_default_display: _gdk_display =%p display = %p\n", _gdk_display , display);

  //g_assert (display == NULL || _gdk_display == display);
  //g_assert (gdk_display_get_default () == display);

}

static GdkDisplay *
gdk_gix_display_manager_get_default_display (GdkDisplayManager *manager)
{
  GdkDisplay *display = GDK_GIX_DISPLAY_MANAGER (manager)->default_display;

  if (_gdk_display)
  {
	  return _gdk_display;
  }

  display =  _gdk_gix_display_open (NULL);
  //g_assert(display != NULL);
 return display;
}

static GdkAtom
gdk_gix_display_manager_atom_intern (GdkDisplayManager *manager,
					 const gchar       *atom_name,
					 gboolean           dup)
{
  return 0;
}

static gchar *
gdk_gix_display_manager_get_atom_name (GdkDisplayManager *manager,
					   GdkAtom            atom)
{
  return 0;
}

#include "../gdkkeynames.c"

static guint
gdk_gix_display_manager_lookup_keyval (GdkDisplayManager *manager,
					   const gchar       *keyval_name)
{
  g_return_val_if_fail (keyval_name != NULL, 0);

  return _gdk_keyval_from_name (keyval_name);
}

static gchar *
gdk_gix_display_manager_get_keyval_name (GdkDisplayManager *manager,
					     guint              keyval)
{
   return _gdk_keyval_name (keyval);
}

static void
gdk_gix_display_manager_keyval_convert_case (GdkDisplayManager *manager,
						 guint              symbol,
						 guint             *lower,
						 guint             *upper)
{
  /* FIXME implement this */
  if (lower)
    *lower = symbol;
  if (upper)
    *upper = symbol;
}

static void
gdk_gix_display_manager_class_init (GdkGixDisplayManagerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkDisplayManagerClass *manager_class = GDK_DISPLAY_MANAGER_CLASS (class);

  object_class->finalize = gdk_gix_display_manager_finalize;

  manager_class->open_display = gdk_gix_display_manager_open_display;
  manager_class->list_displays = gdk_gix_display_manager_list_displays;
  manager_class->set_default_display = gdk_gix_display_manager_set_default_display;
  manager_class->get_default_display = gdk_gix_display_manager_get_default_display;
  manager_class->atom_intern = gdk_gix_display_manager_atom_intern;
  manager_class->get_atom_name = gdk_gix_display_manager_get_atom_name;
  manager_class->lookup_keyval = gdk_gix_display_manager_lookup_keyval;
  manager_class->get_keyval_name = gdk_gix_display_manager_get_keyval_name;
  manager_class->keyval_convert_case = gdk_gix_display_manager_keyval_convert_case;
}

static void
gdk_gix_display_manager_init (GdkGixDisplayManager *manager)
{
}

void
_gdk_gix_display_manager_add_display (GdkDisplayManager *manager,
					  GdkDisplay        *display)
{
  GdkGixDisplayManager *manager_gix = GDK_GIX_DISPLAY_MANAGER (manager);

  if (manager_gix->displays == NULL)
    gdk_display_manager_set_default_display (manager, display);

  manager_gix->displays = g_slist_prepend (manager_gix->displays, display);
}

void
_gdk_gix_display_manager_remove_display (GdkDisplayManager *manager,
					     GdkDisplay        *display)
{
  GdkGixDisplayManager *manager_gix = GDK_GIX_DISPLAY_MANAGER (manager);

  manager_gix->displays = g_slist_remove (manager_gix->displays, display);

  if (manager_gix->default_display == display)
    {
      if (manager_gix->displays)
        gdk_display_manager_set_default_display (manager, manager_gix->displays->data);
      else
        gdk_display_manager_set_default_display (manager, NULL);
    }
}
