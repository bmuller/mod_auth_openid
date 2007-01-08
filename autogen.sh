#! /bin/sh
aclocal  -I ./acinclude.d && autoheader && automake -a && autoconf && ./configure "$@"