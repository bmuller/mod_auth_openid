#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_hash.h"

#include "mod_auth_openid.h"

using namespace std;
using namespace modauthopenid;
using namespace CppUnit;

/**
 * DBD connection shared across all tests.
 */
ap_dbd_t* g_dbd;

class StuffTest : public TestFixture
{
protected:
  const ap_dbd_t* dbd;

public:
  void setUp()
  {
    dbd = g_dbd;
  }

  void tearDown()
  {

  }

  void testStuff()
  {
    CPPUNIT_ASSERT(1 + 1 == 2);
  }

  CPPUNIT_TEST_SUITE(StuffTest);
  CPPUNIT_TEST(testStuff);
  CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(StuffTest);


/**
 * Print an error message and quit if an APR call fails.
 */
void exit_on_err(apr_status_t rc, const char* tag)
{
  char err_buf[256];
  if (rc != APR_SUCCESS) {
    apr_strerror(rc, err_buf, sizeof(err_buf));
    printf("%s failed: APR status %s\n", tag, err_buf);
    exit(1);
  }
}

int main(int argc, const char * const * argv)
{
  apr_status_t rc; // return code for APR functions

  // start up APR and normalize args
  rc = apr_app_initialize(&argc, &argv, NULL);
  exit_on_err(rc, "apr_app_initialize");

  // register APR final cleanup function
  atexit(apr_terminate);

  // print usage message and quit if invoked incorrectly
  if (argc != 3) {
    printf("usage: %s <DBD driver name> <DB params string as used by apr_dbd_open()>\n", argv[0]);
    return -1;
  }

  const char *dbd_drivername = argv[1];
  const char *dbd_params     = argv[2];

  // convenience struct containing memory pool, DB driver, DB connection, prepared statements
  ap_dbd_t dbd;

  // create a memory pool (will be cleaned up by apr_terminate)
  rc = apr_pool_create(&dbd.pool, NULL);
  exit_on_err(rc, "apr_pool_create");

  // create a hashtable for prepared statements
  dbd.prepared = apr_hash_make(dbd.pool);

  // start up DBD, the APR SQL abstraction layer
  rc = apr_dbd_init(dbd.pool);
  exit_on_err(rc, "apr_dbd_init");

  // get the DBD driver
  rc = apr_dbd_get_driver(dbd.pool, dbd_drivername, &dbd.driver);
  exit_on_err(rc, "apr_dbd_get_driver");

  // open a SQL connection
  const char *connect_err;
  rc = apr_dbd_open_ex(dbd.driver, dbd.pool, dbd_params, &dbd.handle, &connect_err);
  if (rc != APR_SUCCESS) {
    printf("apr_dbd_open_ex error: %s\n", connect_err);
  }
  exit_on_err(rc, "apr_dbd_open_ex");

  // enable strict mode for the connection
  Dbd(&dbd).enable_strict_mode();

  // put DBD connection somewhere tests can find it
  g_dbd = &dbd;

  // run tests
  TextUi::TestRunner runner;
  TestFactoryRegistry& registry = TestFactoryRegistry::getRegistry();
  runner.addTest(registry.makeTest());
  bool tests_passed = runner.run();

  // close the SQL connection
  rc = apr_dbd_close(dbd.driver, dbd.handle);
  exit_on_err(rc, "apr_dbd_close");

  return !tests_passed;
}
