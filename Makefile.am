noinst_LTLIBRARIES = libmodauthopenid.la

INCLUDES = ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${CURL_CFLAGS}
libmodauthopenid_la_LDFLAGS = ${LIBCURL} ${PCREPP_LIBS} ${BDB_LIBS} ${OPKELE_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp

AM_CXXFLAGS=`@apr_config@ --cflags --cppflags` 
LIBS=`@apr_config@ --libs`
ACLOCAL_AMFLAGS=-I ${top_srcdir}/acinclude.d/

noinst_PROGRAMS = db_info
db_info_SOURCES = db_info.cpp
db_info_LDFLAGS = ${CURL_CFLAGS} ${OPKELE_LIBS} ${BDB_LIBS} -lmodauthopenid

install-exec-local:
	cp ${top_builddir}/.libs/libmodauthopenid.la ${top_builddir}/.libs/libmodauthopenid.lai
	${APXS} -i -a -n 'authopenid' libmodauthopenid.la
