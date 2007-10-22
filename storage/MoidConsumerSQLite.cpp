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
  using namespace opkele;
 
  MoidConsumerSQLite::MoidConsumerSQLite(const string& storage_location) {
    is_closed = false;
    int rc = sqlite3_open(storage_location.c_str(), &db);
    char *errMsg;
    if(!test_result(rc, "problem opening database"))
      return;
    string query = "CREATE TABLE IF NOT EXISTS associations "
      "(id VARCHAR(255), server VARCHAR(255), handle VARCHAR(100), secret VARCHAR(30), expires_on INT)";
    rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem creating associations table if it didn't exist already");
  };

  assoc_t MoidConsumerSQLite::store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in) {
    debug("Storing server \"" + server + "\" and handle \"" + handle + "\" in db");
    ween_expired();
    char *errMsg;
    string secret_b64;
    secret.to_base64(secret_b64);
    time_t rawtime;
    time (&rawtime);
    string id = server + " " + handle;
    string s_expires_on;
    int_to_string((rawtime + expires_in), s_expires_on);
    string query = "INSERT INTO associations (id, server, handle, secret, expires_on) VALUES("
      "\"" + id.substr(0, 254) + "\", "
      "\"" + server + "\", "
      "\"" + handle + "\", "
      "\"" + secret_b64 + "\", " + s_expires_on + ")"; 
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem storing association in associations table");
    int expires_on = rawtime + expires_in;
    return assoc_t(new association(server, handle, "assoc type", secret, expires_on, false));
  };

  assoc_t MoidConsumerSQLite::retrieve_assoc(const string& server, const string& handle) {
    ween_expired();
    debug("looking up association: server = " + server + " handle = " + handle);
    sqlite3_stmt *pSelect;
    string id = server + " " + handle;
    string query = "SELECT * FROM associations WHERE id = \"" + id + "\"";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      throw failed_lookup(OPKELE_CP_ "Could not find association due to db error.");
    }
    rc = sqlite3_step(pSelect);
    if(rc != SQLITE_ROW){
      debug("could not find server \"" + server + "\" and handle \"" + handle + "\" in db.");
      rc = sqlite3_finalize(pSelect);
      throw failed_lookup(OPKELE_CP_ "Could not find association.");
    }

    char c_key[255];
    char c_server[255];
    char c_handle[100];
    char c_secret[30];
    snprintf(c_key, 255, "%s", sqlite3_column_text(pSelect, 0));
    snprintf(c_server, 255, "%s", sqlite3_column_text(pSelect, 1));
    snprintf(c_handle, 100, "%s", sqlite3_column_text(pSelect, 2));
    snprintf(c_secret, 30, "%s", sqlite3_column_text(pSelect, 3));
    int expires_on = sqlite3_column_int(pSelect, 4);
    time_t rawtime;
    time (&rawtime);
    secret_t secret;
    secret.from_base64(c_secret);
    rc = sqlite3_finalize(pSelect);
    return assoc_t(new association(c_server, c_handle, "assoc type", secret, expires_on, false));
  };

  void MoidConsumerSQLite::invalidate_assoc(const string& server,const string& handle) {
    debug("invalidating association: server = " + server + " handle = " + handle);
    char *errMsg;
    string id = server + " " + handle;
    string query = "DELETE FROM associations WHERE id = \"" + id + "\"";
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem invalidating assocation for server \"" + server + "\" and handle \"" + handle + "\"");
  };

  assoc_t MoidConsumerSQLite::find_assoc(const string& server) {
    ween_expired();
    debug("looking for any association with server = " + server);
    time_t rawtime;
    time (&rawtime);
    sqlite3_stmt *pSelect;
    string query = "SELECT * FROM associations";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      throw failed_lookup(OPKELE_CP_ "Could not find a valid handle due to db error."); 
    }
    char c_key[255];
    char c_server[255];
    char c_handle[100];
    char c_secret[30];
    int expires_on;
    vector<string> parts;
    rc = sqlite3_step(pSelect);
    while(rc == SQLITE_ROW){
      snprintf(c_key, 255, "%s", sqlite3_column_text(pSelect, 0));
      snprintf(c_server, 255, "%s", sqlite3_column_text(pSelect, 1));
      snprintf(c_handle, 100, "%s", sqlite3_column_text(pSelect, 2));
      snprintf(c_secret, 30, "%s", sqlite3_column_text(pSelect, 3));
      expires_on = sqlite3_column_int(pSelect, 4);
      parts = explode(string(c_key), " ");
      // If server url we were given matches the current record, and it still has
      // at least five minutes until it expires (to give the user time to be redirected -> there -> back)
      if(parts.size()==2 && parts[0] == server && rawtime < (expires_on + 18000)) {
	debug("....found one");
	rc = sqlite3_finalize(pSelect);
	secret_t secret;
	secret.from_base64(c_secret);
	auto_ptr<association_t> a(new association(c_server, c_handle, "assoc type", secret, expires_on, false));
	return a;
      }    
      rc = sqlite3_step(pSelect);
    } 
    rc = sqlite3_finalize(pSelect);
    throw failed_lookup(OPKELE_CP_ "Could not find a valid handle."); 
  };

  bool MoidConsumerSQLite::test_result(int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error in MoidConsumer - " + context + ": %s\n";
      fprintf(stderr, msg.c_str(), sqlite3_errmsg(db));
      sqlite3_close(db);
      is_closed = true;
      return false;
    }
    return true;
  };

  void MoidConsumerSQLite::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *errMsg;
    string s_time;
    int_to_string(rawtime, s_time);
    string query = "DELETE FROM associations WHERE " + s_time + " > expires_on";
    int rc = sqlite3_exec(db, query.c_str(), NULL, 0, &errMsg);
    test_result(rc, "problem weening expired sessions from table");
  };

  // This is a method to be used by a utility program, never the apache module
  int MoidConsumerSQLite::num_records() {
    ween_expired();
    int number = 0;
    sqlite3_stmt *pSelect;
    string query = "SELECT COUNT(*) AS count FROM associations";
    int rc = sqlite3_prepare(db, query.c_str(), -1, &pSelect, 0);
    if( rc!=SQLITE_OK || !pSelect ){
      debug("error preparing sql query: " + query);
      return number;
    }
    rc = sqlite3_step(pSelect);
    if(rc == SQLITE_ROW){
      number = sqlite3_column_int(pSelect, 0);
    } else {
      debug("Problem fetching num records from associations table");
    }
    rc = sqlite3_finalize(pSelect);
    return number;
  };

  void MoidConsumerSQLite::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
}



