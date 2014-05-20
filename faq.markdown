---
layout: base
title: mod_auth_openid
---

## Why?
 I'm anxious to see more adoption of the OpenID standard.  Generally, early adopters are required to modify their applications to use libs written in their development language if they want to support distributed auth.  I wanted to create an alternative (and easier) way for developers to add support.  Applications can use custom login pages, only allow identities from trusted providers, etc., but they don't have to handle the redirection/encryption/etc associated with OpenID.

## Can I use mod_auth_openid to protect a dav share/my svn installation? 
 No.  Due to the distributed nature of the [OpenID specification](http://openid.net/specs/openid-authentication-1_1.html) users must be able to be redirected to an Identity Provider website to authenticate themselves, and then be redirected back to the initial resource they were attempting to view.  Any software that requires Basic HTTP Authentication (webdav, the *svn* client) cannot support such a redirection.  **mod_auth_openid** is designed specifically and solely for protecting web based resources when being viewed from a standard web browser.

## Why are you using an SQLite database?  Why do you need a database?  None of the other Apache 2 auth modules need one. 
 Some information needs to persist between requests (as a part of the distributed authentication process).  I decided to use a database (rather than using flat files) to reduce the footprint of the module.  I chose SQLite because it supports database files that exceed a terabyte (not that your DB would ever get that large, it's just nice to know) and because multiple tables can exist in the same database file.  The file can be deleted at any time (it's automatically recreated) with the only cost being that whoever is currently logged in/attempting to log in will be logged out/must reauthenticate.  Each record in the DB has an expiration time and is deleted when that time is reached.  See [the page](dbinfo.html) on the **db_info** program if you want to monitor the size of the database.

## Are GET parameters retained as a user authenticates? 
 Yes.  If a user visits a protected location with GET query parameters, they will be preserved throughout the authentication process.  These should be kept to a minimum (in size), though, due to URI query string size constraints in browsers (OpenID uses the query string to pass parameters around as well, adding to the length).

## How can I log a user out? 
  Just delete (overwrite) the *open_id_session_id* cookie.

## Why does your C++ look like a first year CS student at a community college wrote it? 
 Because I avoid C++ like the plague, and as a natural result I don't know it very well.  Send me patches to make it beautiful.

## How do I determine the identity URL of the person currently logged in? 
 The identity URL of the user currently logged is stored in the **REMOTE_USER** CGI environment variable.

##  Is it possible to limit login to some users, like htaccess/htpasswd does? 
 Yes, by using an external auth program and the [AuthOpenIDUserProgram](authuserprogram.html) option.  Also, it is possible to limit authentication to certain identity providers (by using **AuthOpenIDDistrusted** and **AuthOpenIDTrusted**, see [the main page](index.html) for more info).  

## I hate the look of the default login page.  How can I use something different? 
 It's easy.  See the [custom login page howto](custompage.html).

## What is the air speed velocity of an unladen swallow? 
 Already answered [here](http://answers.google.com/answers/main?cmd=threadview&id=233578).

## What if I'm using Apache 1.3? 
 From the latest version of Apache 1.3's [release page](http://www.apache.org/dist/httpd/Announcement1.3.html):
  *We strongly recommend that users of all earlier versions, including 1.3 family release, upgrade to to the current 2.2 version as soon as possible.*

## What if I'm using Windows? 
 As of yet, no one has compiled the module for Windows.  I will not do so.  If someone does, please send in instructions describing the process you used so that others can do so as well.  Feel free to send a link to your binary as well.

## Does Version 0.1 imply anything or is this actually ready for production? 
 Version 1.0 will imply a full-featured production ready project.  Right now it is beta "use at your own risk" unstable.

## Will it run on BSD? 
 There are claims that it at least runs on FreeBSD 7.  If you get it running on any other flavors, let me know and I'll update this answer.
