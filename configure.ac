AC_PREREQ(2.53)
AC_INIT(webarg, 0.1+, http://github.com/Sektor/WebArg)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_HEADERS(config.h)
AM_MAINTAINER_MODE([enable])
LT_INIT(shared)

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_PROG_CC_STDC
AC_HEADER_STDC

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.20.1])
AC_SUBST(GLIB)

PKG_CHECK_MODULES(GLIBJSON, [json-glib-1.0 >= 0.6.2])
AC_SUBST(GLIBJSON)

#
#
#
AC_ARG_ENABLE(openssl,
 [  --enable-openssl  Enables https support and needs openssl libs],
 [ openssl=yes
 ])

if test "x$openssl" = "xyes" ; then
AC_CHECK_LIB([ssl], [SSL_library_init])
CFLAGS="$CFLAGS -DLWS_OPENSSL_SUPPORT"
fi

#
#
#
AC_ARG_ENABLE(libcrypto,
 [  --enable-libcrypto  Use libcrypto MD5 and SHA1 implementations],
 [ libcrypto=yes
 ])

if test "x$libcrypto" = "xyes" ; then
CFLAGS="$CFLAGS -DLWS_LIBCRYPTO"
LDFLAGS="$LDFLAGS -lcrypto"
fi
AM_CONDITIONAL(LIBCRYPTO, test x$libcrypto = xyes)

#
#
#
AC_ARG_ENABLE(x-google-mux,
 [  --enable-x-google-mux  Build experimental x-google-mux],
 [ x_google_mux=yes
 ])
if test "x$x_google_mux" = "xyes" ; then
CFLAGS="$CFLAGS -DLWS_EXT_GOOGLE_MUX"
fi
AM_CONDITIONAL(EXT_GOOGLE_MUX, test x$x_google_mux = xyes)

#
#
#
AC_ARG_WITH([client-cert-dir],
[AS_HELP_STRING([--with-client-cert-dir],[directory containing client certs, defaults to /etc/pki/tls/certs/])],
[clientcertdir=$withval],
[clientcertdir=/etc/pki/tls/certs/]
)
AC_SUBST([clientcertdir])

AC_SUBST([CFLAGS])

# Checks for header files.
AC_CHECK_HEADERS([zlib.h fcntl.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero memset socket strerror])

AC_CONFIG_FILES([
Makefile
libwebsockets/Makefile
src/Makefile
data/Makefile
])
AC_OUTPUT

