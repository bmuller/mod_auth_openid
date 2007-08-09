/*
Copyright (C) 2007 Butterfat, LLC (http://butterfat.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Created by bmuller <bmuller@butterfat.net>
*/

#include "mod_auth_openid.h"

namespace modauthopenid {
  using namespace std;

  SessionManagerSQLite::SessionManagerSQLite(const string& storage_location) {
    is_closed = false;
    int rc = sqlite3_open(storage_location, &db);
    char *errMsg;
    if(!test_result(rc, "problem opening database"))
      return;
    rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS sessionmanager (session_id INT, hostname VARCHAR(255), path VARCHAR(255), identity VARCHAR(255), expires_on INT)", NULL, 0, &errMsg);
    test_result(rc, "problem creating table if it didn't exist already");
  };

  void SessionManagerSQLite::get_session(const string& session_id, SESSION& session) {
    ween_expired();
    sqlite3_stmt *pSelect;
    string query = "SELECT * FROM sessionmanager WHERE session_id = " + session_id;
    rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      return rc;
    }
    rc = sqlite3_step(pSelect);
    while( rc==SQLITE_ROW ){
      fprintf(stdout, "%s;\n", sqlite3_column_text(pSelect, 0));
      fprintf(stdout, "%s;\n", sqlite3_column_text(pSelect, 1));
      rc = sqlite3_step(pSelect);
    }
    rc = sqlite3_finalize(pSelect);



    Dbt data;
    char id[255];
    strcpy(id, session_id.substr(0, 254).c_str());
    Dbt key(id, strlen(id) + 1);
    data.set_data(&session);
    data.set_ulen(sizeof(SESSION));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      strcpy(session.identity, "");
      debug("could not find session id " + session_id + " in db: session probably just expired");
    }
  };

  bool SessionManagerSQLite::test_result(int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error in Session Manager - " + context + ": %s\n"
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
    
    SESSION s;
    // session_id is char[33], path, identity and hostname are char[255]
    strcpy(s.session_id, session_id.substr(0, 32).c_str());
    strcpy(s.path, path.substr(0, 254).c_str());
    strcpy(s.identity, identity.substr(0, 254).c_str());
    strcpy(s.hostname, hostname.substr(0, 254).c_str());

    // sessions last for a day becore being automatically invalidated
    s.expires_on = rawtime + 86400; 
    
    char id[255];
    strcpy(id, session_id.substr(0, 32).c_str()); // safety first!  id is char[33]
    Dbt key(id, strlen(id) + 1);
    Dbt data(&s, sizeof(SESSION));
    db_.put(NULL, &key, &data, 0);
    debug("storing session " + session_id + " for path " + path + " and id " + identity);
  };

  void SessionManagerSQLite::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *errMsg;
    string query = "DELETE FROM sessionmanager WHERE " + string(itoa(time)) + " > expires_on";
    rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem weening expired sessions from table");
  };

  int SessionManagerSQLite::num_records() {
    ween_expired();
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    int count = 0;
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0)
        count++;
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while reading sessions db");
    } catch(std::exception &e) {
      db_.errx("Error while reading sessions db! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
    return count;
  };

  void SessionManagerSQLite::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
}
