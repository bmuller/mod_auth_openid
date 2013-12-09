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

#include "mod_auth_openid.h"

namespace modauthopenid {
  using namespace std;

  SessionManager::SessionManager(const ap_dbd_t* _dbd) {
    dbd = _dbd;

    int rc;     // return code for APR DBD functions
    int n_rows; // rows affected by query: not used here, but can't be NULL

    const char* query = "CREATE TABLE IF NOT EXISTS session "
                        "(session_id VARCHAR(33), hostname VARCHAR(255), path VARCHAR(255), "
                        "identity VARCHAR(255), username VARCHAR(255), expires_on BIGINT)";
    rc = apr_dbd_query(dbd->driver, dbd->handle, &n_rows, query);
    test_result(rc, "problem creating session table if it didn't exist already");
  };

  bool SessionManager::get_session(const string& session_id, session_t& session, time_t now) {
    now = now ? now : time(NULL);
    delete_expired(now);

    // mark session as invalid until we successfully load one
    session.identity = "";

    apr_dbd_prepared_t* statement = get_prepared("SessionManager_get_session");
    apr_dbd_results_t* results = NULL;
    int rc = apr_dbd_pvbselect(dbd->driver, dbd->pool, dbd->handle,
                               &results, statement, DBD_LINEAR_ACCESS,
                               session_id.c_str());
    if (!test_result(rc, "problem fetching session with id " + session_id)) {
      return false;
    }

    apr_dbd_row_t *row = NULL;
    rc = apr_dbd_get_row(dbd->driver, dbd->pool,
                         results, &row, DBD_NEXT_ROW);
    if (rc == DBD_NO_MORE_ROWS) {
      debug("could not find session id " + session_id + " in db: session probably just expired");
      return false;
    }

    // success: load session fields from row
    rc = apr_dbd_datum_get(dbd->driver, row, 5, APR_DBD_TYPE_LONGLONG, &session.expires_on);
    if (!test_result(rc, "problem loading expires_on column for session with id " + session_id)) {
      return false;
    }
    session.session_id = string(apr_dbd_get_entry(dbd->driver, row, 0));
    session.hostname   = string(apr_dbd_get_entry(dbd->driver, row, 1));
    session.path       = string(apr_dbd_get_entry(dbd->driver, row, 2));
    session.identity   = string(apr_dbd_get_entry(dbd->driver, row, 3));
    session.username   = string(apr_dbd_get_entry(dbd->driver, row, 4));

    consume_results(dbd, results, &row);
    return true;
  };

  bool SessionManager::test_result(int rc, const string& context) {
    if (rc != DBD_SUCCESS) {
      fprintf(stderr, "DBD Error in SessionManager - %s: %s\n",
              context.c_str(), apr_dbd_error(dbd->driver, dbd->handle, rc));
      return false;
    }
    return true;
  };

  bool SessionManager::store_session(const string& session_id, const string& hostname,
                                     const string& path, const string& identity,
                                     const string& username, int lifespan,
                                     time_t now) {
    now = now ? now : time(NULL);
    delete_expired(now);

    // lifespan will be 0 if not specified by user in config - so lasts as long as browser is open.  In this case, make it last for up to a day.
    apr_int64_t expires_on = (lifespan == 0) ? (now + 86400) : (now + lifespan);

    apr_dbd_prepared_t *statement = get_prepared("SessionManager_store_session");
    int n_rows;
    int rc = apr_dbd_pvbquery(dbd->driver, dbd->pool, dbd->handle,
                              &n_rows, statement,
                              session_id.c_str(),
                              hostname.c_str(),
                              path.c_str(),
                              identity.c_str(),
                              username.c_str(),
                              &expires_on);
    return test_result(rc, "problem inserting session into db");
  };

  void SessionManager::delete_expired(time_t now) {
    apr_dbd_prepared_t* statement = get_prepared("SessionManager_delete_expired");
    int n_rows;
    int rc = apr_dbd_pvbquery(dbd->driver, dbd->pool, dbd->handle,
                              &n_rows, statement,
                              &now);
    test_result(rc, "problem weening expired sessions from table");
  };

  void SessionManager::print_table() {
    print_sql_table(dbd, "session");
  };

  apr_dbd_prepared_t* SessionManager::get_prepared(const char* label) {
    return (apr_dbd_prepared_t *)apr_hash_get(dbd->prepared, label, APR_HASH_KEY_STRING);
  }

  void SessionManager::append_statements(apr_array_header_t* statements)
  {
    labeled_statement_t* statement;

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "SessionManager_get_session";
    statement->code  = "SELECT session_id,hostname,path,identity,username,expires_on "
                       "FROM session WHERE session_id=%s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "SessionManager_store_session";
    statement->code  = "INSERT INTO session "
                       "(session_id,hostname,path,identity,username,expires_on) "
                       "VALUES(%s,%s,%s,%s,%s,%lld)";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "SessionManager_delete_expired";
    statement->code  = "DELETE FROM session WHERE %lld >= expires_on";
  };
}
