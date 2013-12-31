#include <stdio.h>
#include <stdlib.h>

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_hash.h"

#include "test.h"

/**
 * DBD connection shared across all tests.
 */
const ap_dbd_t* g_dbd;

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
 * Drop existing SQL tables belonging to this module.
 */
void drop_tables(const ap_dbd_t* dbd)
{
  Dbd fancy_dbd(dbd);
  fancy_dbd.query("DROP TABLE IF EXISTS sessionmanager");
  fancy_dbd.query("DROP TABLE IF EXISTS authentication_sessions");
  fancy_dbd.query("DROP TABLE IF EXISTS associations");
  fancy_dbd.query("DROP TABLE IF EXISTS response_nonces");
}

/**
 * Create SQL tables used by this module.
 */
void create_tables(const ap_dbd_t* dbd)
{
  // constructors create tables as side effect
  SessionManager s(dbd);
  MoidConsumer   c(dbd, "blah", "blah");
}

/**
 * Try to prepare the SQL statements used by this module.
 * @attention Adds successfully prepared statements to dbd->prepared hashtable.
 */
void prepare_statements(const ap_dbd_t* dbd)
{
  // collect statements
  apr_array_header_t * statements = apr_array_make(
    dbd->pool,
    18 /* size hint: expected number of statements */,
    sizeof(labeled_statement_t));
  MoidConsumer  ::append_statements(statements);
  SessionManager::append_statements(statements);

  // prepare and store statements
  for (int i = 0; i < statements->nelts; i++) {
    apr_dbd_prepared_t *prepared_statement = NULL;
    labeled_statement_t *statement = &APR_ARRAY_IDX(statements, i, labeled_statement_t);
    int rc = apr_dbd_prepare(dbd->driver, dbd->pool, dbd->handle,
                             statement->code, statement->label,
                             &prepared_statement);
    if (rc != DBD_SUCCESS) continue;
    apr_hash_set(dbd->prepared, statement->label, APR_HASH_KEY_STRING, prepared_statement);
  }
}

/**
 * Initialize APR.
 * @attention Side effects modify argc/argv.
 */
void init_apr(int* argc, const char* const** argv)
{
  apr_status_t rc; // return code for APR functions

  // start up APR and normalize args
  rc = apr_app_initialize(argc, argv, NULL);
  exit_on_apr_err(rc, "apr_app_initialize");

  // register APR final cleanup function
  atexit(apr_terminate);
}

/**
 * Create a DBD connection and associated memory pool.
 */
ap_dbd_t* init_dbd(const char* dbd_drivername, const char* dbd_params)
{
  apr_status_t rc; // return code for APR functions

  // create a memory pool (will be cleaned up by apr_terminate)
  apr_pool_t* pool;
  rc = apr_pool_create(&pool, NULL);
  exit_on_apr_err(rc, "apr_pool_create");

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

  // enable strict mode for the connection
  Dbd(dbd).enable_strict_mode();

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
  init_apr(&argc, &argv);

  // print usage message and quit if invoked incorrectly
  if (argc != 3) {
    fprintf(stderr, "usage: %s <DBD driver name> <DB params string as used by apr_dbd_open()>\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *dbd_drivername = argv[1];
  const char *dbd_params     = argv[2];

  ap_dbd_t* dbd = init_dbd(dbd_drivername, dbd_params);

  // set up DB tables
  drop_tables(dbd);
  create_tables(dbd);

  // prepare statements for the connection
  prepare_statements(dbd);

  // put DBD connection somewhere tests can find it
  g_dbd = dbd;

  // run tests
  TextUi::TestRunner runner;
  TestFactoryRegistry& registry = TestFactoryRegistry::getRegistry();
  runner.addTest(registry.makeTest());
  bool tests_passed = runner.run();

  cleanup_dbd(dbd);

  return tests_passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
