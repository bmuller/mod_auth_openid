ACLOCAL_AMFLAGS=-I ${top_srcdir}/acinclude.d/

noinst_LTLIBRARIES = libmodauthopenid.la
noinst_PROGRAMS = db_info
noinst_DATA = mod_auth_openid.la

INCLUDES = ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${BDB_CPPFLAGS}
AM_LDFLAGS = ${OPKELE_LIBS} ${BDB_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp

db_info_SOURCES = db_info.cpp
db_info_LDFLAGS = -lmodauthopenid
db_info_DEPENDENCIES = libmodauthopenid.la

install-exec-local:
	${APXS} -i -a -n 'authopenid' mod_auth_openid.la

mod_auth_openid.la: libmodauthopenid.la
	${APXS} -c -o $@ $< ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${OPKELE_LIBS} ${DBD_LIBS}