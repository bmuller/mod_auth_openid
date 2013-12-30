/*
Copyright (C) 2007-2010 Butterfat, LLC (http://butterfat.net)

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
#include "http_request.h"
#include "apr_strings.h"
#include "apr_tables.h"
#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"
#include "mod_ssl.h"
#include "apr.h"
#include "apr_general.h"
#include "apr_time.h"
#include "apr_dbd.h"
#include "mod_dbd.h"
/* Needed for Apache 2.4 */
#include "mod_auth.h"

/* other general lib includes */
#include <curl/curl.h>
#include <pcre.h>

#include <ctime>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

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

/* Attribute Exchange */
#define AX_NAMESPACE "http://openid.net/srv/ax/1.0"
#define DEFAULT_AX_NAMESPACE_ALIAS "ax"

/* DBD constants */
#define DBD_SUCCESS           0
#define DBD_LINEAR_ACCESS     0
#define DBD_NEXT_ROW         -1
#define DBD_NO_MORE_ROWS     -1

/**
 * A VARCHAR big enough for most URLs.
 */
#define BIG_VARCHAR "VARCHAR(4000)"

/**
 * Debug logging with location info.
 * __FUNCTION__ is nonstandard but supported by GCC and MSVC.
 * @see http://gcc.gnu.org/onlinedocs/gcc/Function-Names.html
 * @see http://msdn.microsoft.com/en-us/library/b0084kay.aspx
 */
#ifdef DEBUG
#define MOID_DEBUG(msg) modauthopenid::debug((__FILE__), (__LINE__), (__FUNCTION__), (msg))
#else
#define MOID_DEBUG(msg)
#endif

/* mod_auth_openid includes */
#include "config.h"
#include "types.h"
#include "http_helpers.h"
#include "moid_utils.h"
#include "Dbd.h"
#include "SessionManager.h"
#include "MoidConsumer.h"
