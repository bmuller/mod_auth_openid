AC_INIT([mod_auth_openid], [0.0], [bmuller@butterfat.net])
# AC_CONFIG_SRCDIR([include/opkele/opkele-config.h])
# AC_CONFIG_HEADERS([config.h include/opkele/acconfig.h])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([no-dist])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG_CPLUSPLUS
AC_CANONICAL_HOST
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_HEADER_STDC

dnl provide flag --enable-debug
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


# find curl-config binary
AC_ARG_WITH(curl_config, AC_HELP_STRING([[--with-curl-config=FILE]], [Path to curl-config program]),
			[ curl_config="$withval" ],
			[ AC_PATH_PROG(curl_config, curl-config, [no], [$PATH:/usr/sbin/:/usr/local/apache2/bin]) ])
if test "$curl_config" = "no"; then
   AC_MSG_ERROR(Could not find the curl-config program.  You can specify a location with the --with-curl-config=FILE option.)
fi
$curl_config --prefix &> /dev/null
if test "$?" != "0"; then
  AC_MSG_ERROR($curl_config is not a valid curl-config program)
fi

AC_CHECK_LIB(pcre++, pcre_exec,[
  PCREPP_LIBS=-lpcre++
  AC_SUBST([PCREPP_LIBS])
  ],[
   AC_MSG_ERROR([no mimetic library found. get one from http://codesink.org/mimetic_mime_library.html])
  ]
)

AC_CHECK_LIB(curl, main, [
  LIBCURL=-lcurl
  AC_SUBST([LIBCURL])
  ], [
   AC_MSG_ERROR([no curl library found. get one from http://curl.haxx.se/])
  ]
)

AX_PATH_BDB([3], [],[
    AC_MSG_ERROR([no BDB library found. get one from http://www.oracle.com/database/berkeley-db/db/index.html])
  ])
AC_SUBST([BDB_LIBS])

APACHE_CFLAGS="-I`${APXS} -q INCLUDEDIR` -I`${apr_config} --includedir`"
AC_SUBST(APACHE_CFLAGS)

CURL_CFLAGS="-I`${curl_config} --prefix`/include"
AC_SUBST(APACHE_CFLAGS)

PKG_CHECK_MODULES([OPKELE],[libopkele],,[
 AC_MSG_ERROR([no libopkele library found. get one from http://kin.klever.net/libopkele/])
])
AC_SUBST(OPKELE_CFLAGS)
AC_SUBST(OPKELE_LIBS)

AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT

echo
echo " ***"
echo " *** You are now ready to build mod_auth_openid:"
echo " *** Enter the following commands:"
echo " ***"
echo " *** $> make"
echo " *** $> su root"
echo " *** $> make install"
echo " ***"
echo " *** Report bugs at https://www.butterfat.net/tracker/butterfat/index"
echo " *** Thanks for using GNU software."
echo " ***"
echo
