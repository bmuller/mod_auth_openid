---
layout: base
title: mod_auth_openid
---
# Introduction 
You should have mod_auth_openid installed at this point and working for some location/directory/file.  It's assumed that the default web page that allows a user to identify herself looks terrible in your opinion.  So here's how you make your own.

*For information on passing along attribute exchange parameters, see [AttributeExchange](attex.html) too.*

# Example 
Let's say you want to protect the location "/supersecret" (that is, *http://example.com/supersecret*) on your web server, and you want to use your own custom page for users to enter their identity.
## Step 1: Determine where your page will live 
The page that you create **must** exist *outside* of your protected location/directory.  Other than that one rule, it can live anywhere, even on a different server.  Use whatever tools/framework/language you want to generate the page.  For the purposes of our example, let's say the page will live at "/login.html".

## Step 2: Add the location to your httpd.conf 
Use the **AuthOpenIDLoginPage** option to specify the location of your login page.  This is *not* the location on your file system, but rather a full URL (*http://.....*) or the location relative to your server's www root.
{% highlight apache %}
<Location /supersecret>
	AuthType			OpenID
	require valid-user
        AuthOpenIDLoginPage		/login.html
</Location>
{% endhighlight %}

## Step 3: Make your login page 
It really doesn't matter what's on your page, either, other than a form.  This form must use the "GET" method to send at least one parameter to your protected location.  The one parameter that you must have is named **openid_identifier**.  This should be the identity URL the user inputs.  You may have as many additional variables as you'd like; they will be preserved throughout the authentication process.  For instance:
{% highlight html %}
<!-- this is http://example.com/login.html -->
<html><body>
<h1>This is my super cool personalized login page</h1>
<form action="/supersecret" method="GET">
  <input type="text" name="openid_identifier" />
  <input type="submit" value="Log In" />
</form>
</body></html>
{% endhighlight %}
Also, there is a GET parameter **modauthopenid.referrer** that will be sent to the custom login page that contains the original location the user requested.
## Step 4: Handle errors on your login page 
In the case that a user cancels the identification process or that there is an authentication error, the user will be redirected back to the login page location with a special GET parameter named **modauthopenid.error** (as well as any other parameters you may have added in the first place).  This parameter will have one of six possible values:
 * **no_idp_found**:  This is returned when the there was no identity provider URL found on the identity page given by the user, or if the page could not be downloaded.  The user probably just mistyped her identity URL.
 * **invalid_id**: This is returned when the identity given is not syntactically valid (for either a URI or XRI).
 * **idp_not_trusted**: This is returned when the identity provider of the user is not trusted.  This will only occur if you have at least one of **AuthOpenIDTrusted** or **AuthOpenIDDistrusted** set.
 * **invalid_nonce**: This is a security error.  It generally means that someone is attempting a replay attack, though more innocuous reasons are possible (such as a user who doesn't have cookies enabled refreshing the page).
 * **canceled**: This is returned if a user cancels the authentication process.
 * **unspecified**: This error can occur for a number of reasons, such a bad signature of the query parameters returned from a user's identity provider.  Most likely, the user should simply be instructed to attempt again.

You should display a message on your page based upon this response.  If you do not have/know a server side language that can do this, you can use JavaScript to access the query parameters and notify the user of the reason they have been redirected back to the login page.

That's it.
