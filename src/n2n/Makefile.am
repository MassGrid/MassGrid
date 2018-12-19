ACLOCAL_AMFLAGS = -I build-aux/m4
lib_LTLIBRARIES =
lib_LTLIBRARIES += libn2n.la
include_HEADERS = include/n2n.h include/n2n_wire.h include/sglib.h
noinst_HEADERS = 
noinst_HEADERS += src/lzoconf.h
noinst_HEADERS += src/lzodefs.h
noinst_HEADERS += src/minilzo.h
noinst_HEADERS += src/n2n_keyfile.h
noinst_HEADERS += src/n2n_transforms.h
noinst_HEADERS += src/scm.h
noinst_HEADERS += src/twofish.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libn2n.pc

libn2n_la_SOURCES = src/n2n.c \
                src/n2n_keyfile.c \
                src/wire.c \
                src/minilzo.c \
                src/twofish.c \
                src/transform_null.c \
                src/transform_tf.c \
                src/transform_aes.c \
                src/tuntap_freebsd.c \
                src/tuntap_netbsd.c \
                src/tuntap_linux.c \
                src/tuntap_osx.c \
                src/version.c
if WINDOWS
    libn2n_la_SOURCES += win32/win-scm.c
else 
    libn2n_la_SOURCES += src/unix-scm.c
endif
libn2n_la_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
libn2n_la_LIBADD =

bin_PROGRAMS =
noinst_PROGRAMS =
if USE_BENCHMARK
noinst_PROGRAMS += benchmark benchmark_hashtable
benchmark_SOURCES = src/benchmark.c
benchmark_LDADD = libn2n.la
benchmark_CFLAGS = -I$(top_srcdir)/include -static 
benchmark_hashtable_SOURCES = src/benchmark_hashtable.c
benchmark_hashtable_LDADD = libn2n.la
benchmark_hashtable_CFLAGS = -I$(top_srcdir)/include -static
endif

if USE_TEST
noinst_PROGRAMS += n2n_test
n2n_test_SOURCES = src/n2n_test.c
n2n_test_LDADD = libn2n.la
n2n_test_CFLAGS = -I$(top_srcdir)/include -static
TESTS = n2n_test
endif

if USE_EXECUTABLE
bin_PROGRAMS += edge supernode
edge_SOURCES = src/edge.c
edge_LDADD = libn2n.la $(CRYPTO_LIBS)
edge_CFLAGS = -I$(top_srcdir)/include -static
supernode_SOURCES = src/sn.c
supernode_LDADD = libn2n.la
supernode_CFLAGS = -I$(top_srcdir)/include -static
endif

EXTRA_DIST = autogen.sh

 
clean-local:
	rm -rf autom4te.cache src/.deps
	rm -f aclocal.m4 config.h.in config.guess config.sub ltmain.sh install-sh depcomp compile src/Makefile Makefile