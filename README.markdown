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
     Require           valid-user

There are also additional, optional directives.  See the homepage for a list and docs.

The user's identity URL will be available in the REMOTE_USER cgi environment variable after 
authentication.

See [the project page](http://findingscience.com/mod_auth_openid) for more information.

# New features in this fork

I wanted to make mod_auth_openid more useful to sites using
[Google Apps OpenID](https://developers.google.com/accounts/docs/OpenID).

## AuthOpenIDSingleIdP
In general, OpenID users can directly enter the identity they want to claim, or they can enter the identity
of their provider, and then the provider can show some UI for choosing an identity to claim. If your site
only allows a single provider, as will be the case if you're using Google's OpenID for authenticating users to
internal sites, use the `AuthOpenIDSingleIdP <provider URL>` directive to preset it and mod_auth_openid will skip
the login form entirely. Your users won't need to remember Google's OpenID provider identity, and they'll have
less clicking to do.

    AuthOpenIDSingleIdP https://www.google.com/accounts/o8/id                       # use Google's OpenID

## AuthOpenIDAXRequire
When you claim an OpenID through Google's IdP, the returned OpenID looks like
`https://www.google.com/accounts/o8/id?id=ABig4324Blob_ofLetters90_8And43Numbers`, which doesn't tell you
which Google Apps domain they logged in from. If you want to find out who a user is for authorization purposes,
you need to use the [Attribute Exchange (AX) extension to OpenID](http://openid.net/specs/openid-attribute-exchange-1_0.html)
to ask the IdP for something you can check against. In the case of Google Apps, you can get their Apps username and
domain by requesting their email address. The `AuthOpenIDAXRequire <alias> <URI> <regex>` lets you require
[some attributes identified by URIs](http://openid.net/specs/openid-attribute-properties-list-1_0-01.html)
to be returned with an OpenID response, and then validate the returned attribute against a regex. If the attribute
isn't returned with the response, or if it doesn't match the regex, validation fails. You can have more than one
`AuthOpenIDAXRequire` directive; all must pass for successful authentication.

    AuthOpenIDAXRequire email http://axschema.org/contact/email @example\.com$      # users from example.com Apps domain only

## AuthOpenIDAXUsername
As noted above, OpenIDs are not required to be pretty or even readable. You can use `AuthOpenIDAXUsername <alias>`
to set Apache's `REMOTE_USER` variable to the AX attribute with that alias. This is what shows up in log files,
and it's also accessible to whatever application you're wrapping with OpenID authentication.

    AuthOpenIDAXUsername email                                                      # username is email address

## AuthOpenIDSecureCookie

`AuthOpenIDSecureCookie <On|Off>` controls whether the `Secure` attribute is set on your session cookies. If you're not using
HTTPS for protecting authentication information in transit (ideally for *everything*), you're doing it wrong, so I was tempted
to make this feature always on. However, sometimes I'm too lazy to set up SSL/TLS on local-only development boxes.

    AuthOpenIDSecureCookie On                                                       # always for production sites!

## HttpOnly attribute set on session cookies

[There is no reason that your session cookie should be accessible from JavaScript.](https://www.owasp.org/index.php/HttpOnly)
`HttpOnly` is now always set on your session cookies.

