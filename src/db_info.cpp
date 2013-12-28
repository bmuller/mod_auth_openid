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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_hash.h"

#include "mod_auth_openid.h"

using namespace std;
using namespace modauthopenid;

/**
 * Print the contents of our session and OpenID consumer tables.
 */
void print_tables(const ap_dbd_t* dbd)
{
  apr_int64_t now = time(NULL);
  printf("Current time: %ld\n\n", now);

  SessionManager s(dbd);
  s.print_tables();

  MoidConsumer c(dbd, "blah", "balh");
  c.print_tables();
}

/**
 * Print detail for a DBD error.
 */
void print_dbd_err(const ap_dbd_t* dbd, int rc, const char * tag)
{
  if (rc != DBD_SUCCESS) {
    const char *err_buf = apr_dbd_error(dbd->driver, dbd->handle, rc);
    printf("%s failed: DBD error %d: %s\n", tag, rc, err_buf);
  }
}

/**
 * TODO: move this to storage classes
 */
void drop_tables(const ap_dbd_t* _dbd)
{
  Dbd dbd(_dbd);
  dbd.query("DROP TABLE IF EXISTS sessionmanager");
  dbd.query("DROP TABLE IF EXISTS authentication_sessions");
  dbd.query("DROP TABLE IF EXISTS associations");
  dbd.query("DROP TABLE IF EXISTS response_nonces");
}

/**
 * TODO: make this an external utility's problem
 */
void create_tables(const ap_dbd_t* dbd)
{
  SessionManager s(dbd);
  MoidConsumer c(dbd, "blah", "balh");
}

/**
 * Try to prepare the SQL statements we use.
 * @attention: Run this before other tests. Adds successfully prepared statements to dbd->prepared hashtable.
 */
bool test_prepare_statements(const ap_dbd_t* dbd)
{
  int num_statements = 18;
  int num_successes = 0;

  apr_array_header_t *statements = apr_array_make(dbd->pool, num_statements /* initial size */, sizeof(labeled_statement_t));
  MoidConsumer  ::append_statements(statements);
  SessionManager::append_statements(statements);

  for (int i = 0; i < statements->nelts; i++) {
    apr_dbd_prepared_t *prepared_statement = NULL;
    labeled_statement_t *statement = &APR_ARRAY_IDX(statements, i, labeled_statement_t);
    int rc = apr_dbd_prepare(dbd->driver, dbd->pool, dbd->handle,
                             statement->code, statement->label,
                             &prepared_statement);
    print_dbd_err(dbd, rc, statement->label);
    if (rc != DBD_SUCCESS) continue;
    apr_hash_set(dbd->prepared, statement->label, APR_HASH_KEY_STRING, prepared_statement);
    num_successes++;
  }

  printf("Successfully prepared statements:\n");
  for (apr_hash_index_t *hi = apr_hash_first(dbd->pool, dbd->prepared); hi; hi = apr_hash_next(hi)) {
    const char *key;
    apr_hash_this(hi, (const void **)&key, NULL, NULL);
    printf("\t%s\n", key);
  }

  if (num_successes != num_statements) {
    printf("Failed to prepare %d statements:\n", num_statements - num_successes);
    return false;
  }

  return true;
}

/**
 * Print an error message and quit if an APR call fails.
 */
void exit_on_err(apr_status_t rc, const char * tag)
{
  char err_buf[256];
  
  if (rc != APR_SUCCESS) {
    apr_strerror(rc, err_buf, sizeof(err_buf));
    printf("%s failed: APR status %s\n", tag, err_buf);
    exit(1);
  }
}

/**
 * Test the SessionManager class.
 * @return True iff all tests pass.
 */
bool test_sessionmanager(const ap_dbd_t* dbd)
{
  bool success;
  SessionManager s(dbd);

  printf("Initial state: no sessions\n");
  s.delete_expired(LONG_MAX);
  s.print_tables();

  time_t now = 1386810000; // no religious or practical significance

  // a fake session
  int lifespan = 50;
  session_t stored_session;
  make_rstring(32, stored_session.session_id);
  stored_session.hostname = "example.org";
  stored_session.path = "/";
  stored_session.identity = "https://example.org/accounts/id?id=ABig4324Blob_ofLetters90_8And43Numbers";
  stored_session.username = "lizard_king";
  stored_session.expires_on = now + lifespan;

  printf("Storing a fake session\n");
  success = s.store_session(stored_session.session_id, stored_session.hostname,
                            stored_session.path, stored_session.identity,
                            stored_session.username, lifespan,
                            now);
  s.print_tables();
  if (!success) {
    printf("Failed to store session\n");
    return false;
  }

  session_t loaded_session;
  printf("Loading the previously stored session\n");
  success = s.get_session(stored_session.session_id, loaded_session, now);
  s.print_tables();
  if (!success) {
    printf("Failed to load session\n");
    return false;
  }

  bool compare_failed = false;
  // verify that stored session and loaded session are identical
  if (stored_session.session_id != loaded_session.session_id) {
    printf("Field not equal between stored and loaded session: session_id\n");
    compare_failed = true;
  }
  if (stored_session.hostname   != loaded_session.hostname) {
    printf("Field not equal between stored and loaded session: hostname\n");
    compare_failed = true;
  }
  if (stored_session.path       != loaded_session.path) {
    printf("Field not equal between stored and loaded session: path\n");
    compare_failed = true;
  }
  if (stored_session.identity   != loaded_session.identity) {
    printf("Field not equal between stored and loaded session: identity\n");
    compare_failed = true;
  }
  if (stored_session.username   != loaded_session.username) {
    printf("Field not equal between stored and loaded session: username\n");
    compare_failed = true;
  }
  if (stored_session.expires_on != loaded_session.expires_on) {
    printf("Field not equal between stored and loaded session: expires_on\n");
    compare_failed = true;
  }
  if (compare_failed) {
    printf("Stored and loaded sessions are not equal\n");
    return false;
  }
  printf("Stored and loaded sessions are equal\n");
  
  success = s.get_session(stored_session.session_id, loaded_session, now + lifespan);
  if (success) {
    printf("Should not have been able to retrieve expired session\n");
    return false;
  }
  printf("Session expired correctly\n");
  
  return true;
}

void print_assoc(opkele::assoc_t assoc)
{
  string secret_base64;
  assoc->secret().to_base64(secret_base64);

  printf("server = %s\n"
         "handle = %s\n"
         "assoc_type = %s\n"
         "secret (base64) = %s\n"
         "expires_in = %d\n"
         "stateless = %d\n",
         assoc->server().c_str(),
         assoc->handle().c_str(),
         assoc->assoc_type().c_str(),
         secret_base64.c_str(),
         assoc->expires_in(),
         assoc->stateless());
}

bool test_moidconsumer_assoc(const ap_dbd_t* dbd)
{
  MoidConsumer c(dbd, "foo", "bar");

  printf("Initial state: no associations\n");
  c.delete_expired(LONG_MAX);
  c.print_tables();

  time_t now = 1386810000; // no religious or practical significance
  int lifespan = 50;

  // store a fake association

  string stored_server("https://example.org/accounts");
  string stored_handle("a.handle");
  string stored_assoc_type("HMAC-SHA256");
  opkele::secret_t stored_secret;
  stored_secret.from_base64("QUFBQUFBQUE=");
  int stored_expires_in = lifespan;

  // TODO: opkele::prequeue_RP interface doesn't have a way to check if storage succeeded
  printf("Storing a fake association\n");
  opkele::assoc_t stored_assoc = c.store_assoc(
    stored_server,
    stored_handle,
    stored_assoc_type,
    stored_secret,
    stored_expires_in,
    now);
  print_assoc(stored_assoc);

  // load the fake association and compare to the stored version
  printf("Loading the fake association by server + handle\n");
  opkele::assoc_t loaded_assoc = c.retrieve_assoc(stored_server, stored_handle, now);
  print_assoc(loaded_assoc);

  bool compare_failed = false;
  // verify that stored session and loaded association are identical
  if (stored_assoc->server()          != loaded_assoc->server()) {
    printf("Field not equal between stored and loaded association: server\n");
    compare_failed = true;
  }
  if (stored_assoc->handle()          != loaded_assoc->handle()) {
    printf("Field not equal between stored and loaded association: handle\n");
    compare_failed = true;
  }
  if (stored_assoc->assoc_type()      != loaded_assoc->assoc_type()) {
    printf("Field not equal between stored and loaded association: assoc_type\n");
    compare_failed = true;
  }
  if (stored_assoc->secret()          != loaded_assoc->secret()) {
    printf("Field not equal between stored and loaded association: secret\n");
    compare_failed = true;
  }
  if (stored_assoc->expires_in()      != loaded_assoc->expires_in()) {
    printf("Field not equal between stored and loaded association: expires_in\n");
    compare_failed = true;
  }
  if (compare_failed) {
    printf("Stored and loaded associations are not equal\n");
    return false;
  }
  printf("Stored and loaded associations are equal\n");

  return true;
}

/**
 * TODO: split tests off into a separate executable.
 */
void run_tests(ap_dbd_t* dbd)
{
  bool all_pass = true;
  bool pass;

  // Do not disable this. It prepares statements used by other tests.
  printf("test_prepare_statements: starting...\n\n");
  pass = test_prepare_statements(dbd);
  all_pass &= pass;
  printf("\ntest_prepare_statements: %s.\n\n", pass ? "passed" : "FAILED");

  printf("test_sessionmanager: starting...\n\n");
  pass = test_sessionmanager(dbd);
  all_pass &= pass;
  printf("\ntest_sessionmanager: %s.\n\n", pass ? "passed" : "FAILED");

  printf("test_moidconsumer_assoc: starting...\n\n");
  pass = test_moidconsumer_assoc(dbd);
  all_pass &= pass;
  printf("\ntest_moidconsumer_assoc: %s.\n\n", pass ? "passed" : "FAILED");
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

  drop_tables(&dbd);
  create_tables(&dbd);
  run_tests(&dbd);

  // close the SQL connection
  rc = apr_dbd_close(dbd.driver, dbd.handle);
  exit_on_err(rc, "apr_dbd_close");

  return 0;
}
