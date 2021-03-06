# **This project is no longer actively maintained.  If you'd like to take it over, email the author.**

# Basic Installation

First, you'll need a few prerequisites.

 * the latest libopkele from http://kin.klever.net/libopkele (C++ implementation of important OpenID functions)
 * libsqlite from http://www.sqlite.org (SQLite C libs)

Next, run:

     ./configure

or 

     ./configure --help

to see additional options that can be specified.

Next, run:

     make
     su root
     make install

Make sure that the file /tmp/mod_auth_openid.db is owned by the user running Apache.
You can do this by (assuming www-data is the user running apache):

     su root
     touch /tmp/mod_auth_openid.db
     chown www-data /tmp/mod_auth_openid.db

Or you can specify an alternate location that the user running apache has write 
privieges on (see the docs for the AuthOpenIDDBLocation directive on the homepage).


# Usage
In either a Directory, Location, or File directive in httpd.conf, place the following directive:

     AuthType            OpenID
     Require             valid-user

There are also additional, optional directives.  See the homepage for a list and docs.

The user's identity URL will be available in the REMOTE_USER cgi environment variable after 
authentication.

## Usage with HTTP Basic
You can configure Apache's HTTP Basic mechanism ahead of mod_auth_openid, allowing a client
to present either a valid Basic auth credential first or an OpenID credential second.

     AuthType               Basic
     AuthBasicAuthoritative Off
     AuthUserFile           .htpasswd
    
     AuthType               OpenID
     Require                valid-user


See [the project page](http://findingscience.com/mod_auth_openid) for more information.
