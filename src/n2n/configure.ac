AC_PREREQ([2.69])
define(_LIBN2N_VERSION_MAJOR, 2)
define(_LIBN2N_VERSION_MINOR, 1)
define(_LIBN2N_VERSION_REVISION, 0)
AC_INIT([libn2n],[_LIBN2N_VERSION_MAJOR._LIBN2N_VERSION_MINOR._LIBN2N_VERSION_REVISION],[https://github.com/wanyvic/n2n])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])
AC_CANONICAL_HOST
AH_TOP([#ifndef LIBN2N_CONFIG_H])
AH_TOP([#define LIBN2N_CONFIG_H])
AH_BOTTOM([#endif /*LIBN2N_CONFIG_H*/])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT

libn2n_version=_LIBN2N_VERSION_MAJOR._LIBN2N_VERSION_MINOR._LIBN2N_VERSION_REVISION
dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PKG_PROG_PKG_CONFIG

AC_PATH_TOOL(AR, ar)
AC_PATH_TOOL(RANLIB, ranlib)
AC_PATH_TOOL(STRIP, strip)
Ax_PROG_CC_FOR_BUILD

if test "x$CFLAGS" = "x"; then
  CFLAGS="-O3 -g"
fi

AM_PROG_CC_C_O

AC_PROG_CC_C99
if test x"$ac_cv_prog_cc_c99" = x"no"; then
  AC_MSG_ERROR([c99 compiler support required])
fi

# Checks for libraries.
if test x$enable_aes != xno; then
AC_CHECK_LIB([crypto],[main],CRYPTO_LIBS=-lcrypto, AC_MSG_ERROR(libcrypto missing))
fi
# Checks for header files.
if test x$enable_aes != xno; then
AC_CHECK_HEADER([openssl/aes.h],,AC_MSG_ERROR(libcrypto headers missing))
fi
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h libintl.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset select socket strchr strdup strerror strpbrk strtol strtoul])
AC_OUTPUT


build_linux=no
build_windows=no
build_mac=no

# Detect the target system
case "${host_os}" in
    linux*)
        build_linux=yes
        ;;
    cygwin*|mingw*)
        build_windows=yes
        ;;
    darwin*)
        build_mac=yes
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported"])
        ;;
esac

# Pass the conditionals to automake
AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])
AM_CONDITIONAL([OSX], [test "$build_mac" = "yes"])

CFLAGS="$CFLAGS -W"

warn_CFLAGS="-pedantic -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wstrict-prototypes -Wno-unused-function -Wno-long-long -Wno-overlength-strings"
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $warn_CFLAGS"
AC_MSG_CHECKING([if ${CC} supports ${warn_CFLAGS}])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden -DN2N_VERSION=\\\"${libn2n_version}\\\" -DN2N_OSNAME=\\\"${host_cpu}-${host_os}\\\""
if test x$enable_aes != xno; then
CFLAGS="$CFLAGS -DN2N_HAVE_AES"
fi
AC_MSG_CHECKING([if ${CC} supports -fvisibility=hidden])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[char foo;]])],
    [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_RESULT([no])
      CFLAGS="$saved_CFLAGS"
    ])

AC_ARG_ENABLE(benchmark,
    AS_HELP_STRING([--enable-benchmark],[compile benchmark (default is no)]),
    [use_benchmark=$enableval],
    [use_benchmark=no])

AC_ARG_ENABLE(test,
    AS_HELP_STRING([--enable-test],[compile test (default is no)]),
    [use_test=$enableval],
    [use_test=no])

AC_ARG_ENABLE([executable],
  [AS_HELP_STRING([--enable-executable],
  [enable build executable (disabled by default)])],
  [use_executable=$enableval],
  [use_executable=no])

AC_ARG_ENABLE([aes],
  [AS_HELP_STRING([--disable-aes],
  [disable aes (enabled by default need openssl)])],
  [enable_aes=$enableval],
  [enable_aes=yes])

AC_MSG_CHECKING([for __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[void myfunc() {__builtin_expect(0,0);}]])],
    [ AC_MSG_RESULT([yes]);AC_DEFINE(HAVE_BUILTIN_EXPECT,1,[Define this symbol if __builtin_expect is available]) ],
    [ AC_MSG_RESULT([no])
    ])

AC_C_BIGENDIAN()

AM_CONDITIONAL([USE_BENCHMARK], [test x"$use_benchmark" = x"yes"])
AM_CONDITIONAL([USE_TEST], [test x"$use_test" = x"yes"])
AM_CONDITIONAL([USE_EXECUTABLE], [test x"$use_executable" = x"yes"])
AC_CONFIG_HEADERS([libn2n-config.h])
AC_CONFIG_FILES([Makefile libn2n.pc])
AC_SUBST(CRYPTO_LIBS)

dnl make sure nothing new is exported so that we don't break the cache
PKGCONFIG_PATH_TEMP="$PKG_CONFIG_PATH"
unset PKG_CONFIG_PATH
PKG_CONFIG_PATH="$PKGCONFIG_PATH_TEMP"

AC_OUTPUT
