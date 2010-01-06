/*
Copyright (C) 2007-2009 Butterfat, LLC (http://butterfat.net)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Created by bmuller <bmuller@butterfat.net>
*/


/* Apache includes. */
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "apr_strings.h"
#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"
#include "mod_ssl.h"
#include "apr.h"
#include "apr_general.h"
#include "apr_time.h"

/* other general lib includes */
#include <curl/curl.h>
#include <pcre.h>
#include <sqlite3.h>

#include <ctime>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>
#include <string>
#include <vector>

/* opkele includes */
#include <opkele/exception.h>
#include <opkele/types.h>
#include <opkele/util.h>
#include <opkele/association.h>
#include <opkele/prequeue_rp.h>

/* overwrite package vars set by apache */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef PACKAGE_URL

/* Header enctype for POSTed form data */
#define DEFAULT_POST_ENCTYPE "application/x-www-form-urlencoded"

/* mod_auth_openid includes */
#include "config.h"
#include "types.h"
#include "http_helpers.h"
#include "moid_utils.h"
#include "SessionManager.h"
#include "MoidConsumer.h"
