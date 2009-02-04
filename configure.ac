AC_INIT([mod_auth_openid], [0.4], [bmuller@butterfat.net])
AC_DEFINE([PACKAGE_URL],["http://trac.butterfat.net/public/mod_auth_openid"],[project url])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE()

AC_CONFIG_MACRO_DIR([acinclude.d])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG_CPLUSPLUS
AC_CANONICAL_HOST
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_HEADER_STDC

# provide flag --enable-debug
AC_ARG_ENABLE(debug, AC_HELP_STRING([[--enable-debug]], [enable debugging output to Apache error log]),
[if test "$enableval" = "yes"; then
    DEBUG="yes"
    CPPFLAGS="$CPPFLAGS -DDEBUG"
fi]
)

# this will look for apxs command - put it in $APXS, fail on failure
AX_WITH_APXS()
# find apr-config binary
AC_ARG_WITH(apr_config, AC_HELP_STRING([[--with-apr-config=FILE]], [Path to apr-config program]),
			[ apr_config="$withval" ],
			[AC_PATH_PROGS(apr_config,
				[apr-config apr-0-config apr-1-config], 
				[no], 
				[$PATH:/usr/sbin/:/usr/local/apache2/bin]
			)]
)

if test "$apr_config" = "no"; then
   AC_MSG_ERROR(Could not find the apr-config program.  You can specify a location with the --with-apr-config=FILE option.  It may be named apr-0-config or apr-1-config and can be found in your apache2 bin directory.)
fi
$apr_config --cppflags &> /dev/null
if test "$?" != "0"; then
  AC_MSG_ERROR($apr_config is not a valid apr-config program)
fi

AX_LIB_SQLITE3([3.3.0])
if test "$SQLITE3_VERSION" == ""; then
  AC_MSG_ERROR([No sqlite 3 (http://www.sqlite.org) library found.])
fi

APACHE_CFLAGS="-I`${APXS} -q INCLUDEDIR` -I`${apr_config} --includedir`"
AC_SUBST(APACHE_CFLAGS)

PKG_CHECK_MODULES([OPKELE],[libopkele >= 2.0],,[
 AC_MSG_ERROR([no libopkele library found (version 2.0 or higher). get one from http://kin.klever.net/libopkele/])
])

# These next few are prerequisites for libopekele, but it's possible they've been removed since that install....
# Check for pcre
AX_PATH_LIB_PCRE([], [ AC_MSG_ERROR([No pcre library found.  You can get it at http://www.pcre.org]) ])
AC_SUBST(PCRE_LIBS)
AC_SUBST(PCRE_CFLAGS)

# Check for curl
AC_CHECK_CURL([7], [], [ AC_MSG_ERROR([No curl library found.  You can get it at http://curl.haxx.se]) ]) 
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

# Idea taken from libopekele
nitpick=false
AC_ARG_ENABLE([nitpicking],
 AC_HELP_STRING([--enable-nitpicking],[make compiler somewhat overly fastidious about the code it deals with]),
 [ test "$enableval" = "no" || nitpick=true ]
)
if $nitpick ; then
 CPP_NITPICK="-Wall -Wextra -Wundef -Wshadow -Wunsafe-loop-optimizations -Wconversion -Wmissing-format-attribute"
 CPP_NITPICK="$CPP_NITPICK -Wredundant-decls -ansi -Wmissing-noreturn"
 CPPFLAGS="$CPPFLAGS $CPP_NITPICK"
# CFLAGS="$CFLAGS $CPP_NITPICK"
# CXXFLAGS="$CXXFLAGS $CPP_NITPICK"
fi

AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT

echo " ***"
echo " *** You are now ready to build mod_auth_openid:"
echo " *** Enter the following commands:"
echo " ***"
echo " *** $> make"
echo " *** $> su root"
echo " *** $> make install"
echo " ***"
echo " *** Report bugs at http://trac.butterfat.net/public/mod_auth_openid"
echo " *** Thanks for using free (as in speech and beer) software."
echo " ***"
echo
