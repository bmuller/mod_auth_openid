# Basic Installation

First, you'll need the latest libopkele from http://kin.klever.net/libopkele (C++ implementation of important OpenID functions).

You'll also need [mod_dbd](http://httpd.apache.org/docs/current/mod/mod_dbd.html) installed and activated to provide an interface between mod_auth_openid and a SQL database that will be used for session storage. Most Apache installs will come with mod_dbd.

If you plan to use MySQL, you may also need to install an additional DBD driver specific to MySQL. For example, Ubuntu packages theirs as (http://packages.ubuntu.com/precise/libaprutil1-dbd-mysql). Mac OS X does not provide one, but you can build it yourself with [a patch to Homebrew's apr-util formula](https://github.com/Homebrew/homebrew-dupes/pull/257).

Next, run:

     ./configure

or 

     ./configure --help

to see additional options that can be specified.

Next, run:

     make
     su root
     make install


# Usage

In a server or vhost section in your Apache config, configure mod_dbd to provide a session backend:

     # MySQL
     DBDriver mysql
     DBDParams "dbname=openid user=mod_auth_openid pass=abracadabra"

     # SQLite
     DBDriver sqlite3
     DBDParams /tmp/mod_auth_openid.db

If using SQLite, make sure that the DB file is owned by the user running Apache. Assuming www-data is the user running Apache and you want to store your session DB at /tmp/mod_auth_openid.db, you can do this by:

     su root
     touch /tmp/mod_auth_openid.db
     chown www-data /tmp/mod_auth_openid.db

In either a Directory, Location, or File directive in httpd.conf, place the following directive:

     AuthType            OpenID
     Require             valid-user

There are also additional, optional directives.  See [the project page](http://findingscience.com/mod_auth_openid) for a list of directives and additional docs.

The user's identity URL will be available in the REMOTE_USER cgi environment variable after 
authentication.

# API documentation

A Doxyfile is provided. To create HTML documentation, install [Doxygen](http://www.doxygen.org/) and run it with

     doxygen

Output will be in [doc/html/](doc/html/).

# Tests

The db_info utility that's built along with the Apache module contains a limited test suite. You can run it by providing the same SQL DB driver name and driver params that you'd provide to mod_dbd (see above). It will wipe out the contents of the database, so don't run it against production DBs.

     # MySQL
     src/db_info mysql "dbname=openid user=mod_auth_openid pass=abracadabra"

     # SQLite
     src/db_info sqlite3 /tmp/mod_auth_openid.db

# Code coverage

To see what code the tests actually cover, you'll need to rebuild with compiler flags for coverage instrumentation, and install [lcov](http://ltp.sourceforge.net/coverage/lcov.php) to generate HTML reports.

     ./configure --enable-coverage
     make
     src/db_info <DBDriver> <DBDParams>
     lcov --capture --directory src --output-file coverage.info
     genhtml coverage.info --output-directory coverage

Coverage reports will be in [coverage/](coverage/).
