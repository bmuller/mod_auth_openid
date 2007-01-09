#! /bin/sh
libtoolize -f && aclocal  -I ./acinclude.d && autoheader && automake -a && autoconf && ./configure "$@"