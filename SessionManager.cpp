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

  SessionManager::SessionManager(const string& storage_location) {
    is_closed = false;
    int rc = sqlite3_open(storage_location.c_str(), &db);
    if(!test_result(rc, "problem opening database"))
      return;
    string query = "CREATE TABLE IF NOT EXISTS sessionmanager "
      "(session_id VARCHAR(33), hostname VARCHAR(255), path VARCHAR(255), identity VARCHAR(255), expires_on INT)";
    rc = sqlite3_exec(db, query.c_str(), 0, 0, 0);
    test_result(rc, "problem creating table if it didn't exist already");
  };

  void SessionManager::get_session(const string& session_id, session_t& session) {
    ween_expired();
    const char *query = "SELECT session_id,hostname,path,identity,expires_on FROM sessionmanager WHERE session_id=%Q LIMIT 1";
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
      session.session_id = string(table[5]);
      session.hostname = string(table[6]);
      session.path = string(table[7]);
      session.identity = string(table[8]);
      session.expires_on = strtol(table[9], 0, 0);
    }
    sqlite3_free_table(table);
  };

  bool SessionManager::test_result(int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error in Session Manager - " + context + ": %s\n";
      fprintf(stderr, msg.c_str(), sqlite3_errmsg(db));
      sqlite3_close(db);
      is_closed = true;
      return false;
    }
    return true;
  };

  void SessionManager::store_session(const string& session_id, const string& hostname, const string& path, const string& identity) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);
    int expires_on = rawtime + 86400;
    const char* url = "INSERT INTO sessionmanager (session_id,hostname,path,identity,expires_on) VALUES(%Q,%Q,%Q,%Q,%d)";
    char *query = sqlite3_mprintf(url, session_id.c_str(), hostname.c_str(), path.c_str(), identity.c_str(), expires_on);
    debug(query);
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem inserting session into db");    
  };

  void SessionManager::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *errMsg;
    string s_time;
    int_to_string(rawtime, s_time);
    string query = "DELETE FROM sessionmanager WHERE " + s_time + " > expires_on";
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem weening expired sessions from table");
  };

  int SessionManager::num_records() {
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

  void SessionManager::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
}
