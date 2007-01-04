lib_LTLIBRARIES = libmodauthopenid.la

INCLUDES = -I@apache_dir@/include -I/usr/include/curl
libmodauthopenid_la_LDFLAGS = ${LIBCURL} ${PCREPP_LIBS} ${MIMETIC_LIBS} ${BDB_LIBS} ${OPKELE_LIBS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp NonceManager.cpp SessionManager.cpp

APXS=@apache_dir@/bin/apxs
CXXFLAGS=`@apache_dir@/bin/apr-1-config --cflags --cppflags` 
LIBS=$LIBS `@apache_dir@/bin/apr-1-config --libs`
LDFLAGS=$LDFLAGS -Wl,--rpath -Wl,/usr/local/lib

install: all
	$(APXS) -i -a -n 'authopenid' libmodauthopenid.la