AC_INIT([libdeltacloud], [0.8], [deltacloud-devel@lists.fedoraproject.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL
AC_PROG_CC

VERSION_SCRIPT_FLAGS=-Wl,--version-script=
`/usr/bin/ld --help 2>&1 | grep -- --version-script >/dev/null` || \
    VERSION_SCRIPT_FLAGS="-Wl,-M -Wl,"
AC_SUBST(VERSION_SCRIPT_FLAGS)

dnl
dnl check for libxml2 library
dnl

LIBXML_REQUIRED=2.6.0
LIBXML_CONFIG="xml2-config"
LIBXML_CFLAGS=""
LIBXML_LIBS=""
LIBXML_FOUND="no"

AC_ARG_WITH([libxml], AC_HELP_STRING([--with-libxml=@<:@PFX@:>@],
            [libxml2 location]))
if test "x$with_libxml" = xno ; then
    AC_MSG_CHECKING([for libxml2 libraries >= $LIBXML_REQUIRED])
    AC_MSG_ERROR([libxml2 >= $LIBXML_REQUIRED is required for libdeltacloud])
elif test "x$with_libxml" = "x" -a "x$PKG_CONFIG" != "x" ; then
    PKG_CHECK_MODULES([LIBXML], [libxml-2.0 >= $LIBXML_REQUIRED],
                      [LIBXML_FOUND=yes], [LIBXML_FOUND=no])
fi
if test "$LIBXML_FOUND" = "no" ; then
    if test "x$with_libxml" != "x" ; then
	LIBXML_CONFIG=$with_libxml/bin/$LIBXML_CONFIG
    fi
    AC_MSG_CHECKING(libxml2 $LIBXML_CONFIG >= $LIBXML_REQUIRED )
    if ! $LIBXML_CONFIG --version > /dev/null 2>&1 ; then
	AC_MSG_ERROR([Could not find libxml2 (see config.log for details).])
    fi
    vers=`$LIBXML_CONFIG --version | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
    minvers=`echo $LIBXML_REQUIRED | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
    if test "$vers" -ge "$minvers" ; then
        LIBXML_LIBS="`$LIBXML_CONFIG --libs`"
        LIBXML_CFLAGS="`$LIBXML_CONFIG --cflags`"
	LIBXML_FOUND="yes"
        AC_MSG_RESULT([yes])
    else
        AC_MSG_ERROR(
          [You need at least libxml2 $LIBXML_REQUIRED for this version of libvir])
    fi
fi
AC_SUBST([LIBXML_CFLAGS])
AC_SUBST([LIBXML_LIBS])

dnl
dnl check for libcurl
dnl

LIBCURL_REQUIRED=7.15
LIBCURL_CONFIG="curl-config"
LIBCURL_CFLAGS=""
LIBCURL_LIBS=""
LIBCURL_FOUND="no"

AC_ARG_WITH([libcurl], AC_HELP_STRING([--with-libcurl=@<:@PFX@:>@],
            [libcurl location]))
if test "x$with_libcurl" = xno ; then
    AC_MSG_CHECKING([for libcurl libraries >= $LIBCURL_REQUIRED])
    AC_MSG_ERROR([libcurl >= $LIBCURL_REQUIRED is required for libdeltacloud])
elif test "x$with_libcurl" = "x" -a "x$PKG_CONFIG" != "x" ; then
    PKG_CHECK_MODULES([LIBCURL], [libcurl >= $LIBCURL_REQUIRED],
                      [LIBCURL_FOUND=yes], [LIBCURL_FOUND=no])
fi
if test "$LIBCURL_FOUND" = "no" ; then
    if test "x$with_libcurl" != "x" ; then
	LIBCURL_CONFIG=$with_libcurl/bin/$LIBCURL_CONFIG
    fi
    AC_MSG_CHECKING(libcurl $LIBCURL_CONFIG >= $LIBCURL_REQUIRED )
    if ! $LIBCURL_CONFIG --version > /dev/null 2>&1 ; then
	AC_MSG_ERROR([Could not find libcurl (see config.log for details).])
    fi
    vers=`$LIBCURL_CONFIG --version | awk '{print $2}' | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
    minvers=`echo $LIBCURL_REQUIRED | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
    if test "$vers" -ge "$minvers" ; then
        LIBCURL_LIBS="`$LIBCURL_CONFIG --libs`"
        LIBCURL_CFLAGS="`$LIBCURL_CONFIG --cflags`"
	LIBCURL_FOUND="yes"
        AC_MSG_RESULT([yes])
    else
        AC_MSG_ERROR(
          [You need at least libcurl $LIBCURL_REQUIRED for this version of libdeltacloud])
    fi
fi
AC_SUBST([LIBCURL_CFLAGS])
AC_SUBST([LIBCURL_LIBS])

AC_CHECK_DECL(CURLOPT_USERNAME, [AC_DEFINE(CURL_HAVE_USERNAME, [], [Description])], ,[#include <curl/curl.h>])

AC_OUTPUT(Makefile include/Makefile include/libdeltacloud/Makefile
	  libdeltacloud.pc)
