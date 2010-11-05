---
layout: base
title: mod_auth_openid
---
# OpenID Attribute Exchange Support
The simplest manner of retrieving attributes is to first set up a [CustomLoginPage](custompage.html) (do that first, then keep reading).  In this example, I'll be fetching an email address from Google when a user authenticates.  First, the custom login page:
{% highlight html %}
<html><body>
<h1>This is my super cool personalized login page</h1>
<form action="/supersecret" method="GET">
  <input type="text" name="openid_identifier" />
  <input type="hidden" name="openid.ns.ext1" value="http://openid.net/srv/ax/1.0" />
  <input type="hidden" name="openid.ext1.mode" value="fetch_request" />
  <input type="hidden" name="openid.ext1.type.email" value="http://axschema.org/contact/email" />
  <input type="hidden" name="openid.ext1.required" value="email" />
  <input type="submit" value="Log In" />
</form>
</body></html>
{% endhighlight %}
When the user puts in their Google identity (https://www.google.com/accounts/o8/id for everyone) they will see their email address being requested.  After authentication and redirection, the openid.ext1 parameters will be kept and the *openid.ext1.type.email* query parameter will contain the value of the user's email address.

# References 
 * [http://openid.net/specs/openid-attribute-exchange-1_0.html](http://openid.net/specs/openid-attribute-exchange-1_0.html)
 * [http://code.google.com/apis/accounts/docs/OpenID.html](http://code.google.com/apis/accounts/docs/OpenID.html)