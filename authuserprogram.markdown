---
layout: base
title: mod_auth_openid
---
# Authorization User Program 
*This is currently only supported in SVN and versions > 0.3*

Due to high demand, the ability to specify a user program to determine authorization has been added using the **AuthOpenIDUserProgram** Apache configuration option.  Please, oh please, read the whole page before doing this.

First, create your authorization program.  It should return a exit value of **0** if the user is authorized and another value otherwise.  Here is a simple example:
{% highlight bash %}
#!sh
#!/bin/bash                                 
if [ "$1" == "bmuller.myopenid.com" ]
then
    exit 0
else
    exit 1
fi
{% endhighlight %}
This example will only authorize the identity {{{bmuller.myopenid.com}}}, all other users (even though they will be authenticated at this point) will not be able to see the resource.  Your program could, of course, be anything from compiled code to a php script - the file must be readable and executable by the Apache process and must return a non-zero value for unauthorized users.  **YOUR PROGRAM MUST NOT HANG**: The Apache process will hang until your program returns.  If your program relies on a network connection or intensive disk IO, that's fine, just make sure that you return in a timely manner if there is a problem.  To illustrate, the following:
{% highlight bash %}
#!sh
#!/bin/bash
sleep 10
exit 0
{% endhighlight %}
Makes the Apache process hang for 10 seconds before authorizing the user.  This would be bad.  Make sure your program returns in a timely manner.  

Then, add the
{% highlight apache %}
AuthOpenIDUserProgram  </full/path/to/executable/>
{% endhighlight %}
option to your Apache configuration file with the full path to your executable.

As always, use at your own risk.



