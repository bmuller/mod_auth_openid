##
##  Makefile -- Build procedure for fast3lpoad Apache module
##
##  This is a C++ module so things have to be handled a little differently.

#   the used tools
APXS=/home/bmuller/mod_auth_openid/apache2/bin/apxs
APACHECTL=/home/bmuller/mod_auth_openid/apache2/bin/apachectl

# Get all of apxs's internal values.
APXS_CC=`$(APXS) -q CC`   
APXS_TARGET=`$(APXS) -q TARGET`   
APXS_CFLAGS=`$(APXS) -q CFLAGS`   
APXS_SBINDIR=`$(APXS) -q SBINDIR`   
APXS_CFLAGS_SHLIB=`$(APXS) -q CFLAGS_SHLIB`   
APXS_INCLUDEDIR=`$(APXS) -q INCLUDEDIR`   
APXS_LD_SHLIB=`$(APXS) -q LD_SHLIB`
APXS_LIBEXECDIR=`$(APXS) -q LIBEXECDIR`
APXS_LDFLAGS_SHLIB=`$(APXS) -q LDFLAGS_SHLIB`
APXS_SYSCONFDIR=`$(APXS) -q SYSCONFDIR`
APXS_LIBS_SHLIB=`$(APXS) -q LIBS_SHLIB`

#   the default target
all: libmodauthopenid.so

# compile the shared object file. use g++ instead of letting apxs call
# ld so we end up with the right c++ stuff. We do this in two steps,
# compile and link.

# compile
mod_auth_openid.o: mod_auth_openid.cpp
	g++ -c -fPIC -I$(APXS_INCLUDEDIR) -I. -I- $(APXS_CFLAGS) $(APXS_CFLAGS_SHLIB) -o $@ $< -I/usr/include/curl

# link
libmodauthopenid.so: MoidConsumer.o moid_utils.o mod_auth_openid.o 
	g++ -fPIC -shared -o $@ MoidConsumer.o moid_utils.o mod_auth_openid.o $(APXS_LIBS_SHLIB) -Wall -Wl,--rpath -Wl,/usr/local/lib -lopkele -lcurl -lpcre++ -lmimetic -ldb_cxx

MoidConsumer.o:
	g++ MoidConsumer.cpp -o MoidConsumer.o -c
moid_utils.o:
	g++ moid_utils.cpp -o moid_utils.o -c


# install the shared object file into Apache 
install: all
	$(APXS) -i -n 'authopenid' libmodauthopenid.so

# display the apxs variables
check_apxs_vars:
	@echo APXS_CC $(APXS_CC);\
	echo APXS_TARGET $(APXS_TARGET);\
	echo APXS_CFLAGS $(APXS_CFLAGS);\
	echo APXS_SBINDIR $(APXS_SBINDIR);\
	echo APXS_CFLAGS_SHLIB $(APXS_CFLAGS_SHLIB);\
	echo APXS_INCLUDEDIR $(APXS_INCLUDEDIR);\
	echo APXS_LD_SHLIB $(APXS_LD_SHLIB);\
	echo APXS_LIBEXECDIR $(APXS_LIBEXECDIR);\
	echo APXS_LDFLAGS_SHLIB $(APXS_LDFLAGS_SHLIB);\
	echo APXS_SYSCONFDIR $(APXS_SYSCONFDIR);\
	echo APXS_LIBS_SHLIB $(APXS_LIBS_SHLIB)

#   cleanup
clean:
	-rm -f *.so *.o *~

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop

