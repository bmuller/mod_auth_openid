noinst_LTLIBRARIES = libmodauthopenid.la
noinst_PROGRAMS = db_info
noinst_DATA = mod_auth_openid.la

EXTRA_DIST = UPGRADE

ACLOCAL_AMFLAGS = -I acinclude.d

INCLUDES = ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${SQLITE3_CFLAGS} ${PCRE_CFLAGS} ${CURL_CFLAGS}
AM_LDFLAGS = ${OPKELE_LIBS} ${SQLITE3_LDFLAGS} ${PCRE_LIBS} ${CURL_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp http_helpers.cpp \
	SessionManager.cpp config.h  http_helpers.h  mod_auth_openid.h  MoidConsumer.h  moid_utils.h \
	SessionManager.h  types.h

db_info_SOURCES = db_info.cpp
db_info_LDFLAGS = -lmodauthopenid
db_info_DEPENDENCIES = libmodauthopenid.la

install-exec-local:
	${APXS} -i -a -n 'authopenid' mod_auth_openid.la

mod_auth_openid.la: libmodauthopenid.la
	${APXS} -c -o $@ $< ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${OPKELE_LIBS} \
			    ${SQLITE3_CFLAGS} ${PCRE_LIBS} ${CURL_LIBS}