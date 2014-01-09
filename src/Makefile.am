noinst_LTLIBRARIES = libmodauthopenid.la
noinst_PROGRAMS = modauthopenid_tables test_runner
noinst_DATA = mod_auth_openid.la

INCLUDES = ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${PCRE_CFLAGS} ${CURL_CFLAGS}
AM_LDFLAGS = ${OPKELE_LIBS} ${PCRE_LIBS} ${CURL_LIBS} ${APR_LDFLAGS} ${APU_LDFLAGS}

libmodauthopenid_la_SOURCES = mod_auth_openid.cpp MoidConsumer.cpp moid_utils.cpp http_helpers.cpp \
	SessionManager.cpp config.h http_helpers.h mod_auth_openid.h MoidConsumer.h moid_utils.h \
	SessionManager.h types.h Dbd.h Dbd.cpp

modauthopenid_tables_SOURCES = modauthopenid_tables.cpp cli_common.cpp
modauthopenid_tables_LDFLAGS = -lmodauthopenid
modauthopenid_tables_DEPENDENCIES = libmodauthopenid.la

test_runner_SOURCES = test_runner.cpp cli_common.cpp \
    TestPreparedStatements.cpp TestSessionManager.cpp TestMoidConsumer.cpp
test_runner_LDFLAGS = -lmodauthopenid -lcppunit
test_runner_DEPENDENCIES = libmodauthopenid.la

AM_CXXFLAGS = -Wall

if NITPICK
AM_CXXFLAGS += -Wextra -Wundef -Wshadow -Wunsafe-loop-optimizations -Wconversion -Wmissing-format-attribute
AM_CXXFLAGS += -Wredundant-decls -ansi -Wmissing-noreturn
endif

if DEBUG
AM_CXXFLAGS += -DDEBUG
endif

if COVERAGE
AM_CXXFLAGS += -ftest-coverage -fprofile-arcs
endif

install-exec-local:
	${APXS} -i -a -n 'authopenid' mod_auth_openid.la

mod_auth_openid.la: libmodauthopenid.la
	${APXS} -c -o $@ $< ${APACHE_CFLAGS} ${OPKELE_CFLAGS} ${OPKELE_LIBS} \
			    ${PCRE_LIBS} ${CURL_LIBS}
