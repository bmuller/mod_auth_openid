#include <cppunit/extensions/HelperMacros.h>

#include "mod_auth_openid.h"

using namespace std;
using namespace modauthopenid;
using namespace CppUnit;

/**
 * DBD connection shared across all tests.
 */
extern const ap_dbd_t* g_dbd;
