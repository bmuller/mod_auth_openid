#! /bin/sh
aclocal && autoheader && automake -a && autoconf && ./configure "$@"