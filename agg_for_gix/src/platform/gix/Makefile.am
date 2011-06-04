if ENABLE_GIX
lib_LTLIBRARIES = libaggplatformGIX.la

libaggplatformGIX_la_LDFLAGS = -version-info @AGG_LIB_VERSION@  -L@x_libraries@
libaggplatformGIX_la_SOURCES = agg_platform_support.cpp
libaggplatformGIX_la_CXXFLAGS =  -I$(top_srcdir)/include -I@x_includes@
libaggplatformGIX_la_LIBADD = -lgix
endif
