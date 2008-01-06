ACLOCAL_AMFLAGS=-I ${top_srcdir}/acinclude.d/

noinst_LTLIBRARIES = libmodauthopenid.la
noinst_PROGRAMS = db_info
noinst_DATA = mod_auth_openid.la

INCLUDES = ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${BDB_CPPFLAGS} ${SQLITE3_CFLAGS} ${PCRE_CFLAGS} ${CURL_CFLAGS}
AM_LDFLAGS = ${OPKELE_LIBS} ${BDB_LIBS} ${SQLITE3_LDFLAGS} ${PCRE_LIBS} ${CURL_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp 
if SQLITE
libmodauthopenid_la_SOURCES += storage/SessionManagerSQLite.cpp storage/NonceManagerSQLite.cpp storage/MoidConsumerSQLite.cpp
else
libmodauthopenid_la_SOURCES += storage/SessionManagerBDB.cpp storage/NonceManagerBDB.cpp storage/MoidConsumerBDB.cpp
endif
db_info_SOURCES = db_info.cpp
db_info_LDFLAGS = -lmodauthopenid
db_info_DEPENDENCIES = libmodauthopenid.la

install-exec-local:
	${APXS} -i -a -n 'authopenid' mod_auth_openid.la

mod_auth_openid.la: libmodauthopenid.la
	${APXS} -c -o $@ $< ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${OPKELE_LIBS} ${BDB_LIBS} \
			    ${SQLITE3_CFLAGS} ${PCRE_LIBS} ${CURL_LIBS}