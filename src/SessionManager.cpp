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
    is_closed = false;
    dbd = _dbd;

    int rc;     // return code for APR DBD functions
    int n_rows; // rows affected by query: not used here, but can't be NULL

    string query = "CREATE TABLE IF NOT EXISTS sessionmanager "
      "(session_id VARCHAR(33), hostname VARCHAR(255), path VARCHAR(255), identity VARCHAR(255), username VARCHAR(255), expires_on INT)";
    rc = apr_dbd_query(dbd->driver, dbd->handle, &n_rows, query.c_str());
    test_result(rc, "problem creating sessionmanager table if it didn't exist already");
  };

  void SessionManager::get_session(const string& session_id, session_t& session) {
    ween_expired();
    const char *query = "SELECT session_id,hostname,path,identity,username,expires_on FROM sessionmanager WHERE session_id=%Q LIMIT 1";
    char *sql = sqlite3_mprintf(query, session_id.c_str());
    int nr, nc;
    char **table;
    int rc = sqlite3_get_table(db, sql, &table, &nr, &nc, 0);
    sqlite3_free(sql);
    test_result(rc, "problem fetching session with id " + session_id);
    if(nr==0) {
      session.identity = "";
      debug("could not find session id " + session_id + " in db: session probably just expired");
    } else {
      session.session_id = string(table[6]);
      session.hostname = string(table[7]);
      session.path = string(table[8]);
      session.identity = string(table[9]);
      session.username = string(table[10]);
      session.expires_on = strtol(table[11], 0, 0);
    }
    sqlite3_free_table(table);
  };

  bool SessionManager::test_result(int result, const string& context) {
    if (result != DBD_SUCCESS){
      fprintf(stderr, "DBD Error in SessionManager - %s: %s\n",
        context.c_str(), apr_dbd_error(dbd->driver, dbd->handle, result));
      return false;
    }
    return true;
  };

  void SessionManager::store_session(const string& session_id, const string& hostname, const string& path, const string& identity, const string& username, int lifespan) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);

    // lifespan will be 0 if not specified by user in config - so lasts as long as browser is open.  In this case, make it last for up to a day.
    int expires_on = (lifespan == 0) ? (rawtime + 86400) : (rawtime + lifespan);

    const char* url = "INSERT INTO sessionmanager (session_id,hostname,path,identity,username,expires_on) VALUES(%Q,%Q,%Q,%Q,%Q,%d)";
    char *query = sqlite3_mprintf(url, session_id.c_str(), hostname.c_str(), path.c_str(), identity.c_str(), username.c_str(), expires_on);
    debug(query);
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem inserting session into db");
  };

  void SessionManager::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *query = sqlite3_mprintf("DELETE FROM sessionmanager WHERE %d > expires_on", rawtime);
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem weening expired sessions from table");
  };

  // This is a method to be used by a utility program, never the apache module                 
  void SessionManager::print_table() {
    ween_expired();
    print_sql_table(dbd, "sessionmanager");
  };

  void SessionManager::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
}
