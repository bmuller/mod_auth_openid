#include <cppunit/extensions/HelperMacros.h>

#include "mod_auth_openid.h"

using namespace std;
using namespace modauthopenid;
using namespace CppUnit;

/**
 * APR memory pool shared across all tests.
 */
extern apr_pool_t* g_pool;

/**
 * DBD connection shared across all tests.
 */
extern Dbd* g_dbd;
