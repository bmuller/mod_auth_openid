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

  NonceManagerSQLite::NonceManagerSQLite(const string& storage_location) {
    is_closed = false;
    int rc = sqlite3_open(storage_location.c_str(), &db);
    char *errMsg;
    if(!test_result(rc, "problem opening database"))
      return;
    string query = "CREATE TABLE IF NOT EXISTS noncemanager (identity VARCHAR(255), nonce VARCHAR(255), expires_on INT)";
    rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem creating table if it didn't exist already");
  };

  bool NonceManagerSQLite::test_result(int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error in Session Manager - " + context + ": %s\n";
      fprintf(stderr, msg.c_str(), sqlite3_errmsg(db));
      sqlite3_close(db);
      is_closed = true;
      return false;
    }
    return true;
  };
  
  bool NonceManagerSQLite::is_valid(const string& nonce, bool delete_on_find) {
    ween_expired();
    bool found;
    char *errMsg;
    sqlite3_stmt *pSelect;
    string query = "SELECT * FROM noncemanager WHERE nonce = \"" + nonce + "\"";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      return false;
    }
    rc = sqlite3_step(pSelect);
    found = (rc == SQLITE_ROW);
    rc = sqlite3_finalize(pSelect);
    if(!found) {
      debug("failed auth attempt: could not find nonce \"" + nonce + "\" in nonce db.");
      return false;
    }
    if(delete_on_find) {
      debug("deleting nonce " + nonce + " from nonces table in db");     
      query = "DELETE FROM noncemanager WHERE nonce = \"" + nonce + "\"";
      rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
      test_result(rc, "problem deleting nonce " + nonce + " from noncemanager");      
    }
    return true;
  };
  
  void NonceManagerSQLite::delete_nonce(const string& nonce) {
    ween_expired();
    is_valid(nonce, true);
  };

  void NonceManagerSQLite::get_identity(const string& nonce, string& identity) {
    ween_expired();
    sqlite3_stmt *pSelect;
    char c_identity[255];
    string query = "SELECT identity FROM noncemanager WHERE nonce = \"" + nonce + "\"";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      return;
    }
    rc = sqlite3_step(pSelect);
    if(rc == SQLITE_ROW){
      snprintf(c_identity, 255, "%s", sqlite3_column_text(pSelect, 0));
      identity = string(c_identity);
    } else {
      identity = "";
      debug("failed to get identity: could not find nonce \"" + nonce + "\" in nonce db.");
    }
    rc = sqlite3_finalize(pSelect);
  };
  
  // The reason we need to store the identity for the case of delgation - we need
  // to keep track of the original identity used by the user.  The identity stored
  // with this nonce will be used later if auth succeeds to create the session
  // for the original identity.
  void NonceManagerSQLite::add(const string& nonce, const string& identity) {
    ween_expired();
    char *errMsg;
    debug("adding nonce to nonces table: " + nonce + " for identity: " + identity);
    time_t rawtime;
    time (&rawtime);
    string s_expires_on;
    int_to_string((rawtime + 3600), s_expires_on); // allow nonce to exist for one hour
    string query = "INSERT INTO noncemanager (nonce, identity, expires_on) VALUES("
      "\"" + nonce + "\", "
      "\"" + identity + "\", " + s_expires_on + ")";
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem inserting nonce " + nonce + " for identity " + identity);
  };

  void NonceManagerSQLite::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *errMsg;
    string s_time;
    int_to_string(rawtime, s_time);
    string query = "DELETE FROM noncemanager WHERE " + s_time + " > expires_on";
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem weening expired nonces from table");
  };

  int NonceManagerSQLite::num_records() {
    ween_expired();
    int number = 0;
    sqlite3_stmt *pSelect;
    string query = "SELECT COUNT(*) AS count FROM noncemanager";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      return number;
    }
    rc = sqlite3_step(pSelect);
    if(rc == SQLITE_ROW){
      number = sqlite3_column_int(pSelect, 0);
    } else {
      debug("Problem fetching num records from noncemanager table");
    }
    rc = sqlite3_finalize(pSelect);
    return number;
  };

  void NonceManagerSQLite::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
  
}
