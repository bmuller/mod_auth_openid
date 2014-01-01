#include <stdio.h>
#include <stdlib.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_hash.h"

#include "test.h"

/**
 * APR memory pool shared across all tests.
 */
apr_pool_t* g_pool;

/**
 * DBD connection shared across all tests.
 */
Dbd* g_dbd;

/**
 * Print an error message and quit if an APR call fails.
 */
void exit_on_apr_err(apr_status_t rc, const char* tag)
{
  char err_buf[256];
  if (rc != APR_SUCCESS) {
    apr_strerror(rc, err_buf, sizeof(err_buf));
    fprintf(stderr, "%s failed: APR status %s\n", tag, err_buf);
    exit(EXIT_FAILURE);
  }
}

/**
 * Try to prepare the SQL statements used by this module.
 * @attention Adds successfully prepared statements to c_dbd->prepared hashtable.
 */
void prepare_statements(const ap_dbd_t* c_dbd)
{
  // collect statements
  apr_array_header_t * statements = apr_array_make(
    c_dbd->pool,
    18 /* size hint: expected number of statements */,
    sizeof(labeled_statement_t));
  MoidConsumer  ::append_statements(statements);
  SessionManager::append_statements(statements);

  // prepare and store statements
  for (int i = 0; i < statements->nelts; i++) {
    apr_dbd_prepared_t *prepared_statement = NULL;
    labeled_statement_t *statement = &APR_ARRAY_IDX(statements, i, labeled_statement_t);
    int rc = apr_dbd_prepare(c_dbd->driver, c_dbd->pool, c_dbd->handle,
                             statement->code, statement->label,
                             &prepared_statement);
    if (rc != DBD_SUCCESS) continue;
    apr_hash_set(c_dbd->prepared, statement->label, APR_HASH_KEY_STRING, prepared_statement);
  }
}

/**
 * Initialize APR and create a memory pool.
 * @attention Side effects modify argc/argv.
 */
apr_pool_t* init_apr(int* argc, const char* const** argv)
{
  apr_status_t rc; // return code for APR functions

  // start up APR and normalize args
  rc = apr_app_initialize(argc, argv, NULL);
  exit_on_apr_err(rc, "apr_app_initialize");

  // register APR final cleanup function
  atexit(apr_terminate);

  // create a memory pool (will be cleaned up by apr_terminate)
  apr_pool_t* pool;
  rc = apr_pool_create(&pool, NULL);
  exit_on_apr_err(rc, "apr_pool_create");

  return pool;
}

/**
 * Create a DBD connection from a pool and DB connection params.
 */
ap_dbd_t* init_dbd(apr_pool_t* pool, const char* dbd_drivername, const char* dbd_params)
{
  apr_status_t rc; // return code for APR functions

  // convenience struct containing memory pool, DB driver,
  // DB connection, prepared statements
  ap_dbd_t* dbd = (ap_dbd_t*)apr_palloc(pool, sizeof(ap_dbd_t));
  dbd->pool = pool;

  // create a hashtable for prepared statements
  dbd->prepared = apr_hash_make(dbd->pool);

  // start up DBD, the APR SQL abstraction layer
  rc = apr_dbd_init(dbd->pool);
  exit_on_apr_err(rc, "apr_dbd_init");

  // get the DBD driver
  rc = apr_dbd_get_driver(dbd->pool, dbd_drivername, &dbd->driver);
  exit_on_apr_err(rc, "apr_dbd_get_driver");

  // open a SQL connection
  const char *connect_err;
  rc = apr_dbd_open_ex(
      dbd->driver, dbd->pool, dbd_params, &dbd->handle, &connect_err);
  if (rc != APR_SUCCESS) {
    fprintf(stderr, "apr_dbd_open_ex error: %s\n", connect_err);
  }
  exit_on_apr_err(rc, "apr_dbd_open_ex");

  return dbd;
}

/**
 * Close connection before we exit.
 */
void cleanup_dbd(const ap_dbd_t* dbd)
{
  apr_status_t rc; // return code for APR functions

  // close the SQL connection
  rc = apr_dbd_close(dbd->driver, dbd->handle);
  exit_on_apr_err(rc, "apr_dbd_close");
}

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
  MoidConsumer c(dbd, /* other params not needed */ "", "");
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
