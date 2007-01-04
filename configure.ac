AC_INIT([mod_auth_openid.so], [0.0], [bmuller@butterfat.net])
# AC_CONFIG_SRCDIR([include/opkele/opkele-config.h])
# AC_CONFIG_HEADERS([config.h include/opkele/acconfig.h])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_LANG_CPLUSPLUS
AC_CANONICAL_HOST
AC_PROG_INSTALL
AM_PROG_LIBTOOL

AC_CHECK_HEADERS(string vector opkele/exception.h openssl/ssl.h)

AC_HEADER_STDC

AC_PROG_RANLIB

dnl provide flag --enable-debug
AC_ARG_ENABLE(debug,
[  --enable-debug          enable debugging output],
[if test "$enableval" = "yes"; then
    DEBUG="yes"
    CPPFLAGS="$CPPFLAGS -DDEBUG"
fi]
)

# Define a macro that is used to parse a --with-apache parameter
# The macro is named "APACHE_DIR"
AC_DEFUN([APACHE_DIR],[
  AC_ARG_WITH(apache, [  --with-apache[=DIR]       Apache server directory],,[with_apache="no"])
  AC_MSG_CHECKING(for Apache directory)
  if test "$with_apache" = "no"; then
    AC_MSG_ERROR( You need to specify the apache directory using --with-apache)
  else
    # make sure that a well known include file exists
    if test -e $with_apache/include/httpd.h; then
      apache_dir=$with_apache
      AC_MSG_RESULT(APACHE found!)
    else
      AC_MSG_ERROR( $with_apache not found.  Check the value you specified with --with-apache)
    fi
  fi
])

# Now call the APACHE_DIR macro that was just specified
APACHE_DIR

# Save the location of apache into the "apache_dir" variable.
# The AC_SUBST macro causes the variable to be saved in config.status
AC_SUBST(apache_dir)

AC_LANG_PUSH([C++])
 AC_CHECK_LIB([mimetic],[main],[
   MIMETIC_LIBS=-lmimetic
   AC_SUBST([MIMETIC_LIBS])
  ],[
   AC_MSG_ERROR([no mimetic library found. get one from http://codesink.org/mimetic_mime_library.html])
  ]
 )
AC_LANG_POP([C++])


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

AC_CHECK_LIB(db_cxx, main, [
  BDB_LIBS=-ldb_cxx
  AC_SUBST([BDB_LIBS])
  ],[
   AC_MSG_ERROR([no BDB library found. get one from http://www.oracle.com/database/berkeley-db/db/index.html])
  ]
)

OPKELE_LIBS=-lopkele;
AC_SUBST([OPKELE_LIBS]) 

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
echo " *** $> $apache_dir/bin/apxs -i -a -n 'authopenid' libmodauthopenid.la"
echo " ***"
echo " *** Thanks for using GNU software."
echo " ***"
echo
