lib_LTLIBRARIES = playerdb.la

playerdb_la_SOURCES = playerdb.cpp
playerdb_la_CXXFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
playerdb_la_LDFLAGS = -module -avoid-version -shared
playerdb_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

EXTRA_DIST = \
	README.txt \
	playerdb.def \
	playerdb.sln \
	playerdb.vcproj

MAINTAINERCLEANFILES =	\
	Makefile.in
