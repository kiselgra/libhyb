lib_LTLIBRARIES = libhyb.la

# libconfigfiles_la_SOURCES = grammar.yy  lexer.ll  parser.h  token.h lexer.h config_creator.h configfile.h configfile.cpp
libhyb_la_SOURCES = \
					area-light-rays.cpp area-light-rays.h \
					gpu-triangles.cu \
					rta-cgls-connection.cpp rta-cgls-connection.h \
					pbrt.h \
					shadowrays.cpp shadowrays.h \
					trav-util.h

# for bison/flex
# AM_CPPFLAGS = -Isrc/
# AM_YFLAGS = -d -p__yacc_prefix__
# AM_LFLAGS = -P__lex_prefix__ -o lex.yy.c


library_includedir = $(includedir)/$(PACKAGE)-$(VERSION)
library_include_HEADERS = \
					area-light-rays.h \
					rta-cgls-connection.h \
					pbrt.h \
					shadowrays.h \
					trav-util.h




# setting c++ flags:
# AM_CXXFLAGS = -Wall -ansi -pedantic -ggdb -pg

# setting c preprocessor flags for main project
libhyb_la_CPPFLAGS = -D PACKAGE=\"$(PACKAGE)\" -D VERSION=\"$(VERSION)\" $(RTA_CFLAGS) $(CGLS_CFLAGS)
libhyb_la_LIBADD = $(RTA_LIBS) $(CGLS_LIBS)

SUFFIXES: .x .cu


.cu.lo:
	echo "############### $(LIBTOOL) --tag=CXX --mode=compile $(top_srcdir)/nvcc-wrapper $(DEFAULT_INCLUDES) $(libhyb_la_CXXFLAGS) $(CXXFLAGS) $(libhyb_la_CPPFLAGS) $(CPPFLAGS) -c -o $@ $< -v"
	$(LIBTOOL) --tag=CXX --mode=compile $(top_srcdir)/nvcc-wrapper $(DEFAULT_INCLUDES) $(libhyb_la_CXXFLAGS) $(CXXFLAGS) $(libhyb_la_CPPFLAGS) $(CPPFLAGS) -c -o $@ $< -v -lineinfo

#.cu.o:
#	echo "############### $(LIBTOOL) --tag=CXX --mode=compile $(top_srcdir)/nvcc-wrapper $(DEFAULT_INCLUDES) $(libhyb_la_CXXFLAGS) $(CXXFLAGS) $(libhyb_la_CPPFLAGS) $(CPPFLAGS) -c -o $@ $< -v"
#	$(LIBTOOL) --tag=CXX --mode=compile $(top_srcdir)/nvcc-wrapper $(DEFAULT_INCLUDES) $(libhyb_la_CXXFLAGS) $(CXXFLAGS) $(libhyb_la_CPPFLAGS) $(CPPFLAGS) -c -o $@ $< -v -lineinfo
#
if CAN_USE_GUILE
bin_PROGRAMS = viewer-cgls-rta
BUILT_SOURCES = cmdline.x
viewer_cgls_rta_SOURCES = viewer.cpp cmdline.cpp cmdline.h tracers.h #$(libhyb_la_SOURCES)
viewer_cgls_rta_CPPFLAGS = -D PACKAGE=\"$(PACKAGE)\" -D VERSION=\"$(VERSION)\" -D DATADIR=\"$(cglsdata)\" $(RTA_CFLAGS) $(CGLS_CFLAGS) -DWITH_GUILE $(GUILE_CFLAGS)
viewer_cgls_rta_CXXFLAGS = -fopenmp
viewer_cgls_rta_LDADD = $(RTA_LIBS) $(CGLS_LIBS) .libs/libhyb.a

.libs/libhyb.a: libhyb.la

snarf_opts = @GUILE_DEF@ $(AM_CPPFLAGS) $(CPPFLAGS) $(viewer_cgls_rta_CPPFLAGS)
.c.x:
	guile-snarf $< $(snarf_opts) > $@
.cpp.x:
	guile-snarf $< $(snarf_opts) > $@
endif

# install link
library_nonversioned_includedir = $(includedir)/$(PACKAGE)
install-data-hook:
	@test -L $(library_nonversioned_includedir) && echo "   !!!   The nonversioned library include dir <$(library_nonversioned_includedir)> is already covered by a link. You'll have to update yourself". || true
	test -L $(library_nonversioned_includedir) || ln -s $(library_includedir) $(library_nonversioned_includedir)


