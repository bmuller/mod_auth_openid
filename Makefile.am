lib_LTLIBRARIES = libmodauthopenid.la

INCLUDES = -I`@APXS@ -q INCLUDEDIR` -I`@curl_config@ --prefix`/include -I`@apr_config@ --includedir`
libmodauthopenid_la_LDFLAGS = ${LIBCURL} ${PCREPP_LIBS} ${MIMETIC_LIBS} ${BDB_LIBS} ${OPKELE_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp

AM_CXXFLAGS=`@apr_config@ --cflags --cppflags` 
LIBS=`@apr_config@ --libs`
LDFLAGS= -Wl,--rpath -Wl,/usr/local/lib

db_info: ${libmodauthopenid_la_OBJECTS}
	${CXX} db_info.cpp NonceManager.o MoidConsumer.o moid_utils.o SessionManager.o -o $@ ${LDFLAGS} \
	${libmodauthopenid_la_LDFLAGS} ${INCLUDES}
