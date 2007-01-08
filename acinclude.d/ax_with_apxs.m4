# code taken from mod_python's (http://www.modpython.org/) configure.in

AC_DEFUN([AX_WITH_APXS],
[

# check for --with-apxs
AC_MSG_CHECKING(for --with-apxs)
AC_ARG_WITH(apxs, AC_HELP_STRING([--with-apxs=PATH], [Path to apxs]),
[
  if test -x "$withval"
  then
    AC_MSG_RESULT([$withval executable, good])
    APXS=$withval
  else
    echo
    AC_MSG_ERROR([$withval not found or not executable])
  fi
],
AC_MSG_RESULT(no))

# if no apxs found yet, check /usr/local/apache/sbin
# since it's the default Apache location
if test -z "$APXS"; then
  AC_MSG_CHECKING(for apxs in /usr/local/apache/sbin)
  if test -x /usr/local/apache/sbin/apxs; then
    APXS=/usr/local/apache/sbin/apxs
    AC_MSG_RESULT([found, we'll use this. Use --with-apxs to specify another.])
  else
    AC_MSG_RESULT(no)
  fi
fi

# second last resort
if test -z "$APXS"; then
  AC_MSG_CHECKING(for apxs in your PATH)
  AC_PATH_PROG(APXS, apxs)
  if test -n "$APXS"; then
    AC_MSG_RESULT([found $APXS, we'll use this. Use --with-apxs to specify another.])
  fi
fi

# last resort
# some linux distributions use apxs2 for apache2 installations,
# so check for apxs2 before we give up.
if test -z "$APXS"; then
  AC_MSG_CHECKING(for apxs2 in your PATH)
  AC_PATH_PROG(APXS, apxs2)
  if test -n "$APXS"; then
    AC_MSG_RESULT([found $APXS, we'll use this. Use --with-apxs to specify another.])
  fi
fi

if test -z "$APXS"; then
  AC_MSG_ERROR([The apxs binary was not found.  Use --with-apxs to specify its location.])
fi

  # check Apache version
  AC_MSG_CHECKING(Apache version)
  HTTPD="`${APXS} -q SBINDIR`/`${APXS} -q TARGET`"
  if test ! -x "$HTTPD"; then
    AC_MSG_ERROR($APXS says that your apache binary lives at $HTTPD but that file isn't executable.  Specify the correct apxs location with --with-apxs)
  fi
  ver=`$HTTPD -v | /usr/bin/awk '/version/ {print $3}' | /usr/bin/awk -F/ '{print $2}'`
  AC_MSG_RESULT($ver)

  # make sure version begins with 2
  if test -z "`$HTTPD -v | egrep 'Server version: Apache/2'`"; then
    AC_MSG_ERROR([mod_auth_openid only works with Apache 2. The one you have seems to be $ver.])
  fi

AC_SUBST(APXS)
])
