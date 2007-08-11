/*
Copyright (C) 2007 Butterfat, LLC (http://butterfat.net)

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

  SessionManagerSQLite::SessionManagerSQLite(const string& storage_location) {
    is_closed = false;
    int rc = sqlite3_open(storage_location.c_str(), &db);
    char *errMsg;
    if(!test_result(rc, "problem opening database"))
      return;
    string query = "CREATE TABLE IF NOT EXISTS sessionmanager "
      "(session_id VARCHAR(33), hostname VARCHAR(255), path VARCHAR(255), identity VARCHAR(255), expires_on INT)";
    rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem creating table if it didn't exist already");
  };

  void SessionManagerSQLite::get_session(const string& session_id, SESSION& session) {
    ween_expired();
    sqlite3_stmt *pSelect;
    string query = "SELECT * FROM sessionmanager WHERE session_id = \"" + session_id + "\"";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      return;
    }
    rc = sqlite3_step(pSelect);
    if(rc == SQLITE_ROW){
      snprintf(session.identity, 33, "%s", sqlite3_column_text(pSelect, 0));
      snprintf(session.hostname, 255, "%s", sqlite3_column_text(pSelect, 1));
      snprintf(session.path, 255, "%s", sqlite3_column_text(pSelect, 2));
      snprintf(session.identity, 255, "%s", sqlite3_column_text(pSelect, 3));
      session.expires_on = sqlite3_column_int(pSelect, 4);
    } else {
      strcpy(session.identity, "");
      debug("could not find session id " + session_id + " in db: session probably just expired");
    }
    rc = sqlite3_finalize(pSelect);
    debug("your momma: " + string(session.identity));
  };

  bool SessionManagerSQLite::test_result(int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error in Session Manager - " + context + ": %s\n";
      fprintf(stderr, msg.c_str(), sqlite3_errmsg(db));
      sqlite3_close(db);
      is_closed = true;
      return false;
    }
    return true;
  };

  void SessionManagerSQLite::store_session(const string& session_id, const string& hostname, const string& path, const string& identity) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);
    string s_expires_on;
    char *errMsg;
    int_to_string((rawtime + 86400), s_expires_on);
    string query = "INSERT INTO sessionmanager (session_id, hostname, path, identity, expires_on) VALUES("
      "\"" + session_id + "\", "
      "\"" + hostname + "\", "
      "\"" + path + "\", "
      "\"" + identity + "\", " + s_expires_on + ")";
    debug("storing session " + session_id + " for path " + path + " and id " + identity);
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem inserting session into db");    
  };

  void SessionManagerSQLite::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *errMsg;
    string s_time;
    int_to_string(rawtime, s_time);
    string query = "DELETE FROM sessionmanager WHERE " + s_time + " > expires_on";
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem weening expired sessions from table");
  };

  int SessionManagerSQLite::num_records() {
    ween_expired();
    int number = 0;
    sqlite3_stmt *pSelect;
    string query = "SELECT COUNT(*) AS count FROM sessionmanager";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      return number;
    }
    rc = sqlite3_step(pSelect);
    if(rc == SQLITE_ROW){
       number = sqlite3_column_int(pSelect, 0);
    } else {
      debug("Problem fetching num records from sessionmanager table");
    }
    rc = sqlite3_finalize(pSelect);    
    return number;
  };

  void SessionManagerSQLite::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
}
