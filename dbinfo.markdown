---
layout: base
title: mod_auth_openid
---
This is information regarding the **db_info** program.

After configuring, making, installing, and using the module, a file will be created for the database (/tmp/mod_auth_openid.db by default).  Return to the mod_auth_openid directory and type:
{% highlight bash %}
make db_info
{% endhighlight %}
This will create an executable in the current directory named '''db_info'''.  Use the program like:
{% highlight bash %}
./db_info /path/to/db/file.db
{% endhighlight %}
It will print out the number of sessions, associations, and nonces in the database.

You can also use the *--enable-debug* configuration option to get debugging information in the apache 2 error log.