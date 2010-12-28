---
layout: base
title: mod_auth_openid
---
# Introduction : The Apache OpenID Module 
**mod_auth_openid** is an authentication module for the [Apache 2] webserver.  It handles the functions of an [OpenID](http://openid.net) consumer as specified in the [OpenID 2.0 specification](http://openid.net/specs/openid-authentication-2_0.html).  See the [FAQ][faq] for more information.  Download the current release from the [the releases page](releases.html).

You can, now, specify an external program for authorization.  That is, after a user has authenticated themselves their identity can be passed to an external program that then returns a value that either authorize them or not to see the resource being protected.  See [AuthUserProgram](authuserprogram.html) for more information.

# Example 
Most people want to see an example first: 
 * [http://butterfat.net/mod_auth_openid/othersecret](http://butterfat.net/mod_auth_openid/othersecret) (uses default login page)
 * [http://butterfat.net/mod_auth_openid/secret](http://butterfat.net/mod_auth_openid/secret) (uses [custom login page][custompage])

# Installation 
*These docs assume that you have [Apache 2] installed and running already.*
## Prerequisites 
 * libopkele (>= 2.0): a C++ implementation of important OpenID functions - [http://kin.klever.net/libopkele/](http://kin.klever.net/libopkele/)
 * libsqlite: SQLite C libs - [http://www.sqlite.org](http://www.sqlite.org)

## Get The Source 
You can download the current stable release from [the releases page](releases.html) or use git to get a development release:
{% highlight bash %}
git clone git://github.com/bmuller/mod_auth_openid.git
{% endhighlight %}

*Note that if you download a development release you will need current versions of the autotools installed, and you must run ./autogen.sh first before following these instructions.*

## Compile
Enter the mod_auth_openid directory and type:
{% highlight bash %}
./configure
{% endhighlight %}

You can use the following to see additional configuration options:
{% highlight bash %}
./configure --help
{% endhighlight %}

Then:
{% highlight bash %}
make
su root
make install
{% endhighlight %}

Verify that the module has been enabled in your ''httpd.conf'':
{% highlight apache %}
# note that the path to your module might be different
LoadModule authopenid_module /usr/lib/apache2/modules/mod_auth_openid.so
{% endhighlight %}

Depending on where you specify your AuthOpenIDDBLocation (see below), you may need to touch the db file as the user that's running Apache (or chown the directory it's being stored in).  For instance:
{% highlight bash %}
# /tmp/mod_auth_openid.db is the default location for the DB
su root
touch /tmp/mod_auth_openid.db
chown www-data /tmp/mod_auth_openid.db
{% endhighlight %}

## Usage 
Place the following directive in either a Directory, Location, or File directive in your httpd.conf (or in an .htaccess file if you have *AllowOverride AuthConfig*):
{% highlight apache %}
AuthType			  OpenID
require valid-user
{% endhighlight %}
The *valid-user* constraint can be replaced with *require user http://myopenid.com/myusername/* if you want to restrict access to a specific user.

The following are optional:
{% highlight apache %}
AuthOpenIDDBLocation              /some/location/my_file.db
AuthOpenIDTrusted                 ^http://myopenid.com/server$ ^http://someprovider.com/idp$
AuthOpenIDDistrusted              ^http://hackerdomain ^http://openid.microsoft.com$ 
AuthOpenIDUseCookie               Off
AuthOpenIDTrustRoot               http://example.com
AuthOpenIDCookieName              example_cookie_name
AuthOpenIDLoginPage               /login.html
AuthOpenIDCookieLifespan          3600 # one hour
AuthOpenIDServerName              http://example.com
AuthOpenIDUserProgram             /path/to/authorization/program
AuthOpenIDCookiePath              /path/to/protect
{% endhighlight %}

 * **AuthOpenIDDBLocation**: Specifies the place the BDB file should be stored.  *Default:* /tmp/mod_auth_openid.db.
 * **AuthOpenIDTrusted**: If specified, only users using providers that match one of the (Perl compatible) regular expressions listed will be allowed to authenticate.  *Default:* Trust all providers.
 * **AuthOpenIDDistrusted**: If specified, only users using providers that do not match one of the (Perl compatible) regular expressions listed will be allowed to authenticate.  You can use this in combination with AuthOpenIDTrusted; in that case, only a domain that is listed as trusted and not listed as distrusted can be used.  *Default:* No providers are distrusted.
 * **AuthOpenIDUseCookie**: If "Off", then a session cookie will not be set on the client upon successful authentication.  The page will load once; if reloaded or if the user visits it again it will ask the user to reauthenticate.  *Default:* On
 * **AuthOpenIDTrustRoot**: User's are asked to approve this value by their identity provider after redirection.  Most providers will error out unless this value matches the URL they are being redirected from, or some subset of that URL.  For instance, if a user is trying to access **http://example.com/protected/index.html** then either **http://example.com** or **http://example.com/protected/** would work but **http://example.com/protected/area/** would not.  *Default:* The URL the user is trying to access (without filenames / query parameters at the end).  
 * **AuthOpenIDCookieName**: The name of the session cookie set by mod_auth_openid.  *Default:* open_id_session_id
 * **AuthOpenIDLoginPage**: The URL location of a customized login page.  This could be a location on a different server or domain.  *Default:* use the mod_auth_openid login page that exists in the module.  See the [custom login page howto][custompage] for more information. 
 * **AuthOpenIDCookieLifespan**: The number of seconds that the session cookie should live after being set.  *Default:* If the cookie lifespan is not set than it will expire at the end of the browser session (when the browser is closed).
 * **AuthOpenIDServerName**: If mod_auth_openid is being used behind a proxy, this option can be used to specify a hostname that will be used to create redirection URLs.
 * **AuthOpenIDUserProgram**: A user specified program for authorization functions.  Please please oh please read [the documentation](authuserprogram.html) before using this.
 * **AuthOpenIDCookiePath**: Explicitly set the path of the auth cookie (for instance, if you want to explicitly grant access to a location other than the one the user is trying to access).
Next, restart apache:
{% highlight bash %}
/path/to/apache2/bin/apachectl stop
/path/to/apache2/bin/apachectl start
{% endhighlight %}

After a user authenticates themselves, the user's identity will be available in the *REMOTE_USER* cgi environment variable.  A cookie named *open_id_session_id* is saved to maintain each user's session.

# Upgrading 
If you're upgrading, make sure you delete the old database file before upgrading and after stopping apache (the db file is in */tmp/mod_auth_openid.db* by default).

# Attribute Exchange 
See the [AttributeExchange](attex.html).

# Questions/Problems/Complaints 
First, read the [FAQ][faq].  If it's a bug, report it by creating a new ticket (link at top).  If it's a complaint or question, email [the mailing list](https://lists.butterfat.net/mailman/listinfo/mod-auth-openid).

[custompage]: custompage.html
[faq]: faq.html
[apache 2]: http://httpd.apache.org