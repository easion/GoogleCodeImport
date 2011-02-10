
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/include \
-I$(top_srcdir)/src/modules/engines \
@FREETYPE_CFLAGS@ \
@EINA_CFLAGS@ \
@evas_engine_gix_cflags@

AM_CFLAGS = @WIN32_CFLAGS@

if BUILD_ENGINE_GIX

GIX_SOURCES = evas_engine.c evas_gix_buffer.c   evas_gix_outbuf.c
GIX_LIBADD = @evas_engine_gix_libs@

pkgdir = $(libdir)/evas/modules/engines/gix/$(MODULE_ARCH)

includes_HEADERS = Evas_Engine_Gix.h
includesdir = $(includedir)/evas-@VMAJ@

if !EVAS_STATIC_BUILD_GIX

pkg_LTLIBRARIES = module.la

module_la_SOURCES  = $(GIX_SOURCES)

module_la_LIBADD = @EINA_LIBS@ $(GIX_LIBADD) $(top_builddir)/src/lib/libevas.la
module_la_LDFLAGS = -module -avoid-version
module_la_LIBTOOLFLAGS = --tag=disable-static

else

noinst_LTLIBRARIES = libevas_engine_gix.la

libevas_engine_gix_la_SOURCES = $(GIX_SOURCES)
libevas_engine_gix_la_LIBADD = $(GIX_LIBADD)

endif
endif

EXTRA_DIST = evas_engine.h
