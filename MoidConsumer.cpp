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
 
  MoidConsumer::MoidConsumer(const string& storage_location, const string& _asnonceid, const string& _serverurl) :
                             asnonceid(_asnonceid), is_closed(false), serverurl(_serverurl) {
    int rc = sqlite3_open(storage_location.c_str(), &db);
    if(!test_result(rc, "problem opening database"))
      return;

    string query = "CREATE TABLE IF NOT EXISTS authentication_sessions "
      "(nonce VARCHAR(255), server VARCHAR(255), handle VARCHAR(100), secret VARCHAR(30), expires_on INT, identity VARCHAR(255))";
    rc = sqlite3_exec(db, query.c_str(), 0, 0, 0);
    test_result(rc, "problem creating sessions table if it didn't exist already");

    query = "CREATE TABLE IF NOT EXISTS associations "
      "(server VARCHAR(255), handle VARCHAR(100), encryption_type VARCHAR(50), secret VARCHAR(30), expires_on INT)";
    rc = sqlite3_exec(db, query.c_str(), 0, 0, 0);
    test_result(rc, "problem creating associations table if it didn't exist already");
  };


  assoc_t MoidConsumer::store_assoc(const string& server,const string& handle,const string& type,const secret_t& secret,int expires_in) {
    debug("Storing association for \"" + server + "\" and handle \"" + handle + "\" in db");
    ween_expired();

    time_t rawtime;
    time (&rawtime);
    int expires_on = rawtime + expires_in;

    char *query = "INSERT INTO assocations (server, handle, secret, expires_on, encryption_type) VALUES(%Q,%Q,%Q,%d,%Q)";
    char *sql = sqlite3_mprintf(query, 
				server.c_str(), 
				handle.c_str(), 
				util::encode_base64(&(secret.front()),secret.size()).c_str(),
				expires_on,
				type.c_str());
    int rc = sqlite3_exec(db, sql, 0, 0, 0);
    sqlite3_free(sql);
    test_result(rc, "problem storing association in associations table");

    return assoc_t(new association(server, handle, type, secret, expires_on, false));
  };

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle) {
    ween_expired();
    debug("looking up association: server = " + server + " handle = " + handle);

    char *query = "SELECT server,handle,secret,expires_on,encryption_type FROM associations WHERE server=%Q AND handle=%Q LIMIT 1";
    char *sql = sqlite3_mprintf(query, server.c_str(), handle.c_str());
    int nr, nc;
    char **table;
    int rc = sqlite3_get_table(db, sql, table, nr, nc, 0);
    sqlite3_free(sql);
    test_result(rc, "problem fetching association");
    if(nr ==0) {
      debug("could not find server \"" + server + "\" and handle \"" + handle + "\" in db.");
      sqlite3_free_table(table);
      throw failed_lookup(OPKELE_CP_ "Could not find association.");
    }
    // resulting row has table indexes: 
    // server  handle  secret  expires_on  encryption_type
    // 5       6       7       8           9
    secret_t secret; 
    util::decode_base64(table[7], secret);
    assoc_t result = assoc_t(new association(table[5], table[6], table[9], secret, strtol(table[8])));
    sqlite3_free_table(table);
    return result;
  };

  void MoidConsumer::invalidate_assoc(const string& server,const string& handle) {
    debug("invalidating association: server = " + server + " handle = " + handle);
    char *query = sqlite3_mprintf("DELETE FROM associations WHERE server=%Q AND handle=%Q", server.c_str(), handle.c_str());
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem invalidating assocation for server \"" + server + "\" and handle \"" + handle + "\"");
  };

  assoc_t MoidConsumer::find_assoc(const string& server) {
    ween_expired();
    debug("looking up association: server = " + server + " handle = " + handle);

    char *query = "SELECT server,handle,secret,expires_on,encryption_type FROM associations WHERE server=%Q LIMIT 1";
    char *sql = sqlite3_mprintf(query, server.c_str(), handle.c_str());
    int nr, nc;
    char **table;
    int rc = sqlite3_get_table(db, sql, table, nr, nc, 0);
    sqlite3_free(sql);
    test_result(rc, "problem fetching association");
    if(nr==0) {
      debug("could not find handle for server \"" + server + "\" in db.");
      sqlite3_free_table(table);
      throw failed_lookup(OPKELE_CP_ "Could not find association.");
    } else {
      debug("found a handle for server \"" + server + "\" in db.");
    }
    // resulting row has table indexes: 
    // server  handle  secret  expires_on  encryption_type
    // 5       6       7       8           9
    secret_t secret; 
    util::decode_base64(table[7], secret);
    assoc_t result = assoc_t(new association(table[5], table[6], table[9], secret, strtol(table[8])));
    sqlite3_free_table(table);
    return result;
  };

  bool MoidConsumer::test_result(int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error in MoidConsumer - " + context + ": %s\n";
      fprintf(stderr, msg.c_str(), sqlite3_errmsg(db));
      sqlite3_close(db);
      is_closed = true;
      return false;
    }
    return true;
  };

  void MoidConsumer::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    char *query = sqlite3_mprintf("DELETE FROM associations WHERE %d > expires_on", rawtime);
    int rc = sqlite3_exec(db, query.c_str(), 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem weening expired sessions from table");
  };

  void check_nonce(const string& server, const string& nonce) {
  };

  void begin_queueing() {
    endpoint_set = false;
  };

  void queue_endpoint(const openid_endpoint_t& ep) {
    debug("Queueing endpoint " + ep.claimed_id + " : " + ep.local_id + " @ " + ep.uri);
    endpoint = ep;
    endpoint_set = true;
    /*
    sqlite3_mem_t<char*> S = sqlite3_mprintf(
                    "INSERT INTO endpoints_queue"
                    " (as_id,eq_ctime,eq_ordinal,eq_uri,eq_claimed_id,eq_local_id)"
                    " VALUES (%ld,strftime('%%s','now'),%d,%Q,%Q,%Q)",
                    as_id,ordinal++,
                    ep.uri.c_str(),ep.claimed_id.c_str(),ep.local_id.c_str());
    db.exec(S);
    */
  }

  const openid_endpoint_t& get_endpoint() const {
    if(!endpoint_set)
      throw opkele::exception(OPKELE_CP_ "No more endpoints queued");
    return endpoint;
  };

  void next_endpoint() {
  };

  void set_normalized_id(const string& nid) {
    idurl = nid;
  };

  const string get_normalized_id() const {
    return idurl;
  };

  const string get_this_url() const {
    return serverurl;
  };

  // This is a method to be used by a utility program, never the apache module
  int MoidConsumer::num_records() {
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

  void MoidConsumer::close() {
    if(is_closed)
      return;
    is_closed = true;
    test_result(sqlite3_close(db), "problem closing database");
  };
}



