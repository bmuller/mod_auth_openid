#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "Dbd.h"
#include "MoidConsumer.h"
#include "SessionManager.h"
#include "cli_common.h"

using namespace modauthopenid;
using namespace CppUnit;

/**
 * APR memory pool shared across all tests.
 */
apr_pool_t* g_pool;

/**
 * DBD connection shared across all tests.
 */
Dbd* g_dbd;

int main(int argc, const char* const* argv)
{
  apr_pool_t* pool = init_apr(&argc, &argv);

  // print usage message and quit if invoked incorrectly
  if (argc != 3) {
    fprintf(stderr, "usage: %s <DBD driver name> <DB params string as used by apr_dbd_open()>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *dbd_drivername = argv[1];
  const char *dbd_params     = argv[2];

  // APR DBD connection
  ap_dbd_t* c_dbd = init_dbd(pool, dbd_drivername, dbd_params);
  // C++ connection wrapper
  Dbd dbd(c_dbd);

  // set up DB tables for our classes
  SessionManager s(dbd);
  s.drop_tables();
  s.create_tables();
  MoidConsumer c(dbd);
  c.drop_tables();
  c.create_tables();

  // prepare statements for the connection
  prepare_statements(c_dbd);

  // put memory pool and DBD connection where tests can find them,
  // since we don't control how test classes are constructed
  g_pool = pool;
  g_dbd = &dbd;

  // run tests
  TextUi::TestRunner runner;
  TestFactoryRegistry& registry = TestFactoryRegistry::getRegistry();
  runner.addTest(registry.makeTest());
  bool tests_passed = runner.run();

  cleanup_dbd(c_dbd);

  return tests_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
