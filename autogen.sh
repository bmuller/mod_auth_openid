#! /bin/sh
libtoolize -f -c && aclocal -I ./acinclude.d && autoheader && automake -ac && autoconf && ./configure "$@"
