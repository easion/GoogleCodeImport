## Makefile.am for gtk+/gdk/gdk-gix
include $(top_srcdir)/Makefile.decl

libgdkincludedir = $(includedir)/gtk-2.0/gdk

INCLUDES = 		\
	-DG_LOG_DOMAIN=\"Gdk-Gix\"	\
	-DGDK_COMPILATION \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/gdk		\
	-I$(top_builddir)/gdk		\
	-DG_DISABLE_DEPRECATED		\
	@GTK_DEBUG_FLAGS@		\
	@GDK_DEP_CFLAGS@		

LDADDS = 		\
	@GDK_DEP_LIBS@		

noinst_LTLIBRARIES = libgdk-gix.la

libgdk_gix_la_SOURCES =	\
	gdkapplaunchcontext-gix.c \
	gdkcolor-gix.c	\
	gdkcursor-gix.c	\
	gdkdnd-gix.c	\
	gdkdisplay-gix.c   \
	gdkdisplay-gix.h   \
	gdkdrawable-gix.c	\
	gdkevents-gix.c	\
	gdkfont-gix.c	\
	gdkgc-gix.c	\
	gdkgeometry-gix.c	\
	gdkglobals-gix.c	\
	gdkim-gix.c	\
	gdkimage-gix.c	\
	gdkinput-gix.c	\
	gdkinput-gix.h	\
	gdkkeys-gix.c	\
	gdkmain-gix.c	\
	gdkpixmap-gix.c	\
	gdkprivate-gix.h	\
	gdkproperty-gix.c	\
	gdkscreen-gix.c	\
	gdkselection-gix.c	\
	gdkspawn-gix.c 	\
	gdktestutils-gix.c	\
	gdkvisual-gix.c	\
	gdkgix.h		\
	gdkwindow-gix.c	\
	gdkwindowid.c		\
	x-cursors.xbm

libgdkinclude_HEADERS =		\
	gdkgix.h 		\
	gdkprivate-gix.h


EXTRA_DIST += AUTHORS README TODO 
