lib_LTLIBRARIES = libmodauthopenid.la

INCLUDES = -I@apache_dir@/include -I/usr/include/curl
libmodauthopenid_la_LDFLAGS = ${LIBCURL} ${PCREPP_LIBS} ${MIMETIC_LIBS} ${BDB_LIBS} ${OPKELE_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp

APXS=@apache_dir@/bin/apxs
AM_CXXFLAGS=`@apache_dir@/bin/apr-1-config --cflags --cppflags` 
LIBS=`@apache_dir@/bin/apr-1-config --libs`
LDFLAGS= -Wl,--rpath -Wl,/usr/local/lib

install: all
	$(APXS) -i -a -n 'authopenid' libmodauthopenid.la
db_info: ${libmodauthopenid_la_OBJECTS}
	${CXX} db_info.cpp NonceManager.o MoidConsumer.o moid_utils.o SessionManager.o -o $@ ${LDFLAGS} \
	${libmodauthopenid_la_LDFLAGS} ${INCLUDES}
