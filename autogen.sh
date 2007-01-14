#! /bin/sh
libtoolize -f -c && aclocal  -I ./acinclude.d && autoheader && automake -a --copy && autoconf && ./configure "$@"
