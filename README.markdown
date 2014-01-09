# Installation

First, you'll need the latest [libopkele](http://kin.klever.net/libopkele) (a C++ implementation of important OpenID functions).

You'll also need [mod_dbd](http://httpd.apache.org/docs/current/mod/mod_dbd.html) installed and activated to provide an interface between `mod_auth_openid` and a SQL database that will be used for session storage. Most Apache installs will come with `mod_dbd`.

If you plan to use MySQL, you may also need to install an additional DBD driver specific to MySQL. For example, Ubuntu packages theirs as (http://packages.ubuntu.com/precise/libaprutil1-dbd-mysql). Mac OS X does not provide one, but you can build it yourself with [a patch to Homebrew's apr-util formula](https://github.com/Homebrew/homebrew-dupes/pull/257).

Next, run:

     ./configure

or 

     ./configure --help

to see additional options that can be specified.

Next, to actually build, install, and activate the module, run:

     make
     sudo make install

     # Ubuntu (your system may have a different way to activate Apache modules)
     sudo a2enmod dbd
     sudo a2enmod authopenid

Create `mod_auth_openid`'s tables in the session backend by running the `modauthopenid_tables` tool with the DBD driver name and params. If using MySQL, make sure that your user has table creation privileges, and note that the password currently has to be provided in plaintext:
     
     # MySQL
     modauthopenid_tables mysql "dbname=openid user=mod_auth_openid pass=abracadabra" create

If using SQLite, make sure that the DB file is owned by the user running Apache. Assuming `www-data` is the user running Apache and you want to store your session DB at `/tmp/mod_auth_openid.db`, you can do this by running the table tool as `www-data`:

     # SQLite
     sudo -u www-data modauthopenid_tables sqlite3 /tmp/mod_auth_openid.db create

# Usage

In a server or vhost section in your Apache config, configure `mod_dbd` to provide a session backend:

     # MySQL
     DBDriver mysql
     DBDParams "dbname=openid user=mod_auth_openid pass=abracadabra"

     # SQLite
     DBDriver sqlite3
     DBDParams /tmp/mod_auth_openid.db

In either a `Directory`, `Location`, or `File` directive in `httpd.conf`, place the following directive:

     AuthType            OpenID
     Require             valid-user

There are also additional, optional directives.  See [the project page](http://findingscience.com/mod_auth_openid) for a list of directives and additional docs.

The user's identity URL will be available in the `REMOTE_USER` CGI environment variable after authentication.

# API documentation

A Doxyfile is provided. To create HTML documentation, install [Doxygen](http://www.doxygen.org/) and run it with

     doxygen

Output will be in [doc/html/](doc/html/).

# Tests

The `test_runner` utility that's built along with the Apache module contains a [CppUnit](http://cppunit.sourceforge.net/)-based test suite. You can run it by providing the same SQL DB driver name and driver params that you'd provide to `mod_dbd` or `modauthopenid_tables` (see above). It will wipe out the contents of the database, so *don't run it against production DBs*.

     # MySQL
     src/test_runner mysql "dbname=openid user=mod_auth_openid pass=abracadabra"

     # SQLite
     src/test_runner sqlite3 /tmp/mod_auth_openid.db

# Code coverage

To see what code the tests actually cover, you'll need to rebuild with compiler flags for coverage instrumentation, and install [lcov](http://ltp.sourceforge.net/coverage/lcov.php) to generate HTML reports.

     ./configure --enable-coverage
     make
     src/test_runner <DBDriver> <DBDParams> # see above
     lcov --capture --directory src --output-file coverage.info
     genhtml coverage.info --output-directory coverage

Coverage reports will be in [coverage/](coverage/).
