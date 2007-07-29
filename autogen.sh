#! /bin/sh
libtoolize -f && aclocal -I ./acinclude.d -I /usr/share/aclocal && autoheader && automake -a && autoconf && ./configure "$@"
