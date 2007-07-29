#! /bin/sh
libtoolize -f && aclocal -I ./acinclude.d -I /usr/share/clocal && autoheader && automake -a && autoconf && ./configure "$@"
