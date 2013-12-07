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

#include <iostream>
#include <time.h>
#include <stdlib.h>

#include "apr_errno.h"
#include "apr_pools.h"

#include "mod_auth_openid.h"

using namespace std;
using namespace modauthopenid;

/**
 * Print the contents of our session and OpenID consumer tables.
 */
void print_databases(const ap_dbd_t* dbd) {
  cout << "Current time: " << time(0) << endl;

  SessionManager s(dbd);
  s.print_table();
  s.close();

  MoidConsumer c(dbd, "blah", "balh");
  c.print_tables();
  c.close();
};

/**
 * Print an error message and quit if an APR call fails.
 */
void exit_on_err(apr_status_t rc, const char * tag) {
  char err_buf[256];
  
  if (rc != APR_SUCCESS) {
    apr_strerror(rc, err_buf, sizeof(err_buf));
    cout << tag << " failed: " << err_buf << endl;
    exit(1);
  }
}

int main(int argc, const char * const * argv) {
  apr_status_t rc; // return code for APR functions

  // start up APR and normalize args
  rc = apr_app_initialize(&argc, &argv, NULL);
  exit_on_err(rc, "apr_app_initialize");

  // register APR final cleanup function
  atexit(apr_terminate);

  // print usage message and quit if invoked incorrectly
  if (argc != 3) {
    cout << "usage: " << argv[0] << " <DBD driver name> <DB params string as used by apr_dbd_open()>\n";
    return -1;
  }

  const char *dbd_drivername = argv[1];
  const char *dbd_params     = argv[2];

  // convenience struct containing memory pool, DB driver, DB connection
  ap_dbd_t dbd;

  // create a memory pool (will be cleaned up by apr_terminate)
  rc = apr_pool_create(&dbd.pool, NULL);
  exit_on_err(rc, "apr_pool_create");

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
    cout << "apr_dbd_open_ex error: " << connect_err << endl;
  }
  exit_on_err(rc, "apr_dbd_open_ex");

  print_databases(&dbd);

  // close the SQL connection
  rc = apr_dbd_close(dbd.driver, dbd.handle);
  exit_on_err(rc, "apr_dbd_close");

  return 0;
}
