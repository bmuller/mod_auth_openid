noinst_LTLIBRARIES = libmodauthopenid.la

INCLUDES = ${APACHE_CFLAGS} ${OPKELE_CFLAGS}
libmodauthopenid_la_LDFLAGS = ${OPKELE_LIBS} ${BDB_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp

ACLOCAL_AMFLAGS=-I ${top_srcdir}/acinclude.d/

noinst_PROGRAMS = db_info
db_info_SOURCES = db_info.cpp
db_info_LDFLAGS = -lmodauthopenid

install-exec-local:
	cp ${top_builddir}/.libs/libmodauthopenid.la ${top_builddir}/.libs/libmodauthopenid.lai
	${APXS} -i -a -n 'authopenid' libmodauthopenid.la
