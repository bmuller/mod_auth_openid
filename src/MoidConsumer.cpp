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
  using namespace opkele;
 
  MoidConsumer::MoidConsumer(const ap_dbd_t* _dbd, const string& _asnonceid, const string& _serverurl) :
                             dbd(_dbd), asnonceid(_asnonceid), serverurl(_serverurl),
                             is_closed(false), endpoint_set(false), normalized_id("")
  {
    const char* ddl_authentication_sessions = 
      "CREATE TABLE IF NOT EXISTS authentication_sessions "
      "(nonce VARCHAR(255), uri VARCHAR(255), claimed_id VARCHAR(255), local_id VARCHAR(255), "
      "normalized_id VARCHAR(255), expires_on INT)";
    dbd.query(ddl_authentication_sessions);

    const char* ddl_assocations = 
      "CREATE TABLE IF NOT EXISTS associations "
      "(server VARCHAR(255), handle VARCHAR(100), encryption_type VARCHAR(50), secret VARCHAR(30), "
      "expires_on INT)";
    dbd.query(ddl_assocations);

    const char* ddl_response_nonces = 
      "CREATE TABLE IF NOT EXISTS response_nonces "
      "(server VARCHAR(255), response_nonce VARCHAR(100), expires_on INT)";
    dbd.query(ddl_response_nonces);
  }

  assoc_t MoidConsumer::store_assoc(const string& server, const string& handle, const string& type,
                                    const secret_t& secret, int expires_in)
  {
    return store_assoc(server, handle, type, secret, expires_in, 0);
  }

  assoc_t MoidConsumer::store_assoc(const string& server, const string& handle, const string& type,
                                    const secret_t& secret, int expires_in, time_t now)
  {
    debug("Storing association for \"" + server + "\" and handle \"" + handle + "\" in db");

    now = now ? now : time(NULL);
    delete_expired(now);

    apr_int64_t expires_on = now + expires_in;

    const void* args[] = {
      server.c_str(), 
      handle.c_str(), 
      util::encode_base64(&(secret.front()), secret.size()).c_str(),
      &expires_on,
      type.c_str(),
    };
    bool success = dbd.pbquery("", args);
    if (!success) {
      debug("problem storing association in associations table");
      // TODO: return sentinel value for invalid assoc_t?
    }
    return assoc_t(new association(server, handle, type, secret, expires_on, false));
  }

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle)
  {
    return retrieve_assoc(server, handle, 0);
  }

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle, time_t now)
  {
    debug("looking up association: server = " + server + " handle = " + handle);

    now = now ? now : time(NULL);
    delete_expired(now);

    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    const void* args[] = {
      server.c_str(),
      handle.c_str(),
    };
    bool success = dbd.pbselect1("MoidConsumer_retrieve_assoc", &results, &row, args);
    if (!success) {
      debug("could not find server \"" + server + "\" and handle \"" + handle + "\" in db.");
      throw failed_lookup(OPKELE_CP_ "Could not find association.");
    }
    string server_from_db, handle_from_db, secret_base64, encryption_type;
    apr_int64_t expires_on;
    dbd.getcol_string(row, 0, server_from_db);
    dbd.getcol_string(row, 1, handle_from_db);
    dbd.getcol_string(row, 2, secret_base64);
    dbd.getcol_int64 (row, 3, expires_on);
    dbd.getcol_string(row, 4, encryption_type);

    dbd.close(results, &row);

    secret_t secret;
    util::decode_base64(secret_base64, secret);
    return assoc_t(new association(server_from_db, handle_from_db, encryption_type, secret,
                                   expires_on, false));
  }

  void MoidConsumer::invalidate_assoc(const string& server,const string& handle) {
    debug("invalidating association: server = " + server + " handle = " + handle);
    // MoidConsumer_invalidate_assoc
    char *query = sqlite3_mprintf("DELETE FROM associations WHERE server=%Q AND handle=%Q", server.c_str(), handle.c_str());
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem invalidating assocation for server \"" + server + "\" and handle \"" + handle + "\"");
  };

  assoc_t MoidConsumer::find_assoc(const string& server) {
    delete_expired();
    debug("looking up association: server = " + server);

    const char *query = "SELECT server,handle,secret,expires_on,encryption_type FROM associations WHERE server=%Q LIMIT 1";
    char *sql = sqlite3_mprintf(query, server.c_str());
    int nr, nc;
    char **table;
    int rc = sqlite3_get_table(db, sql, &table, &nr, &nc, 0);
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
    assoc_t result = assoc_t(new association(table[5], table[6], table[9], secret, strtol(table[8], 0, 0), false));
    sqlite3_free_table(table);
    return result;
  };

  bool MoidConsumer::test_result(int result, const string& context) {
    fprintf(stderr, "TODO: remove test_result\n");
    return false;
  };

  void MoidConsumer::delete_expired(time_t now) {
    time_t rawtime;
    time (&rawtime);
    // MoidConsumer_delete_expired_associations
    char *query = sqlite3_mprintf("DELETE FROM associations WHERE %d > expires_on", rawtime);
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem deleting expired associations from table");

    // MoidConsumer_delete_expired_authentication_sessions
    query = sqlite3_mprintf("DELETE FROM authentication_sessions WHERE %d > expires_on", rawtime);
    rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem deleting expired authentication sessions from table");

    // MoidConsumer_delete_expired_response_nonces
    query = sqlite3_mprintf("DELETE FROM response_nonces WHERE %d > expires_on", rawtime);
    rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem deleting expired response nonces from table");
  };


  void MoidConsumer::check_nonce(const string& server, const string& nonce) {
    char **table;
    int nr, nc;
    // MoidConsumer_check_nonce_find
    char *query = sqlite3_mprintf("SELECT response_nonce FROM response_nonces WHERE server=%Q AND response_nonce=%Q", server.c_str(), nonce.c_str());
    int rc = sqlite3_get_table(db, query, &table, &nr, &nc, 0);
    sqlite3_free(query);
    if(nr != 0) {
      debug("found preexisting nonce - could be a replay attack");
      sqlite3_free_table(table);
      throw opkele::id_res_bad_nonce(OPKELE_CP_ "old nonce used again - possible replay attack");
    }
    sqlite3_free_table(table);

    // so, old nonce not found, insert it into nonces table.  Expiration time will be based on association
    int expires_on = find_assoc(server)->expires_in() + time(0);
    // MoidConsumer_check_nonce_insert
    const char *sql = "INSERT INTO response_nonces (server,response_nonce,expires_on) VALUES(%Q,%Q,%d)";
    query = sqlite3_mprintf(sql, server.c_str(), nonce.c_str(), expires_on);
    rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem adding new nonce to response_nonces table");
  };

  bool MoidConsumer::session_exists() {
    // MoidConsumer_session_exists
    char *query = sqlite3_mprintf("SELECT nonce FROM authentication_sessions WHERE nonce=%Q LIMIT 1", asnonceid.c_str());
    int nr, nc;
    char **table;
    int rc = sqlite3_get_table(db, query, &table, &nr, &nc, 0);
    sqlite3_free(query);
    test_result(rc, "problem fetching authentication session by nonce");
    bool exists = true;
    if(nr==0) {
      debug("could not find authentication session \"" + asnonceid + "\" in db.");
      exists = false;
    } 
    sqlite3_free_table(table);
    return exists;
  };

  void MoidConsumer::begin_queueing() {
    endpoint_set = false;
    // MoidConsumer_begin_queueing
    char *query = sqlite3_mprintf("DELETE FROM authentication_sessions WHERE nonce=%Q", asnonceid.c_str());
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem resetting authentication session");
  };

  void MoidConsumer::queue_endpoint(const openid_endpoint_t& ep) {
    if(!endpoint_set) {
      debug("Queueing endpoint " + ep.claimed_id + " : " + ep.local_id + " @ " + ep.uri);
      time_t rawtime;
      time (&rawtime);
      int expires_on = rawtime + 3600;  // allow nonce to exist for up to one hour without being returned
      // MoidConsumer_queue_endpoint
      const char *query = "INSERT INTO authentication_sessions (nonce,uri,claimed_id,local_id,expires_on) VALUES(%Q,%Q,%Q,%Q,%d)";
      char *sql = sqlite3_mprintf(query, asnonceid.c_str(), ep.uri.c_str(), ep.claimed_id.c_str(), ep.local_id.c_str(), expires_on);
      debug(string(sql));
      int rc = sqlite3_exec(db, sql, 0, 0, 0);
      sqlite3_free(sql);
      test_result(rc, "problem queuing endpoint");
      endpoint_set = true;
    }
  }

  const openid_endpoint_t& MoidConsumer::get_endpoint() const {
    debug("Fetching endpoint");
    // MoidConsumer_get_endpoint
    char *query = sqlite3_mprintf("SELECT uri,claimed_id,local_id FROM authentication_sessions WHERE nonce=%Q LIMIT 1", asnonceid.c_str());
    int nr, nc;
    char **table;
    /*int rc = */sqlite3_get_table(db, query, &table, &nr, &nc, 0);
    sqlite3_free(query);
    //test_sqlite_return(db, rc, "problem fetching authentication session");
    if(nr==0) {
      debug("could not find an endpoint for authentication session \"" + asnonceid + "\" in db.");
      sqlite3_free_table(table);
      throw opkele::exception(OPKELE_CP_ "No more endpoints queued");
    } 
    // resulting row has table indexes:                                                   
    // uri   claimed_id   local_id
    // 3     4            5
    endpoint.uri = string(table[3]);
    endpoint.claimed_id = string(table[4]);
    endpoint.local_id = string(table[5]);
    sqlite3_free_table(table);
    return endpoint;
  };

  void MoidConsumer::next_endpoint() {
    debug("Clearing all session information - we're only storing one endpoint, can't get next one, cause we didn't store it.");
    kill_session();
    endpoint_set = false;
  };

  void MoidConsumer::kill_session() {
    // MoidConsumer_kill_session
    char *query = sqlite3_mprintf("DELETE FROM authentication_sessions WHERE nonce=%Q", asnonceid.c_str());
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem killing session");
  };

  void MoidConsumer::set_normalized_id(const string& nid) {
    debug("Set normalized id to: " + nid);
    normalized_id = nid;
    // MoidConsumer_set_normalized_id
    char *query = sqlite3_mprintf("UPDATE authentication_sessions SET normalized_id=%Q WHERE nonce=%Q", normalized_id.c_str(), asnonceid.c_str());
    debug(string(query));
    int rc = sqlite3_exec(db, query, 0, 0, 0);
    sqlite3_free(query);
    test_result(rc, "problem setting normalized id");
  };

  const string MoidConsumer::get_normalized_id() const {
    if(normalized_id != "") {
      debug("getting normalized id - " + normalized_id);
      return normalized_id;
    }
    char *query = sqlite3_mprintf("SELECT normalized_id FROM authentication_sessions WHERE nonce=%Q LIMIT 1", asnonceid.c_str());
    int nr, nc;
    char **table;
    /*int rc = */sqlite3_get_table(db, query, &table, &nr, &nc, 0);
    sqlite3_free(query);
    //test_sqlite_return(db, rc, "problem fetching authentication session");
    if(nr==0) {
      debug("could not find an normalized_id for authentication session \"" + asnonceid + "\" in db.");
      sqlite3_free_table(table);
      throw opkele::exception(OPKELE_CP_ "cannot get normalized id");
    } 
    normalized_id = string(table[1]);
    sqlite3_free_table(table);  
    debug("getting normalized id - " + normalized_id);
    return normalized_id;
  };

  const string MoidConsumer::get_this_url() const {
    return serverurl;
  };

  // This is a method to be used by a utility program, never the apache module
  void MoidConsumer::print_tables() {
    //print_sql_table(dbd, "authentication_sessions");
    //print_sql_table(dbd, "response_nonces");
    //print_sql_table(dbd, "associations");
  };

  void MoidConsumer::append_statements(apr_array_header_t *statements)
  {
    labeled_statement_t* statement;

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_store_assoc";
    statement->code  = "INSERT INTO associations "
                       "(server, handle, secret, expires_on, encryption_type) "
                       "VALUES (%s, %s, %s, %lld, %s)";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_retrieve_assoc";
    statement->code  = "SELECT server, handle, secret, expires_on, encryption_type "
                       "FROM associations WHERE server = %s AND handle = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_invalidate_assoc";
    statement->code  = "DELETE FROM associations WHERE server = %s AND handle = %s";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_find_assoc";
    statement->code  = "SELECT server, handle, secret, expires_on, encryption_type "
                       "FROM associations WHERE server = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_delete_expired_associations";
    statement->code  = "DELETE FROM associations WHERE %lld > expires_on";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_delete_expired_authentication_sessions";
    statement->code  = "DELETE FROM authentication_sessions WHERE %lld > expires_on";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_delete_expired_authentication_sessions";
    statement->code  = "DELETE FROM response_nonces WHERE %lld > expires_on";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_check_nonce_find";
    statement->code  = "SELECT response_nonce FROM response_nonces "
                       "WHERE server = %s AND response_nonce = %s";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_check_nonce_insert";
    statement->code  = "INSERT INTO response_nonces "
                       "(server, response_nonce, expires_on) "
                       "VALUES (%s, %s, %lld)";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_session_exists";
    statement->code  = "SELECT nonce FROM authentication_sessions "
                       "WHERE nonce = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_begin_queueing";
    statement->code  = "DELETE FROM authentication_sessions WHERE nonce = %s";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_queue_endpoint";
    statement->code  = "INSERT INTO authentication_sessions "
                       "(nonce, uri, claimed_id, local_id, expires_on) "
                       "VALUES (%s, %s, %s, %s, %d)";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_get_endpoint";
    statement->code  = "SELECT uri, claimed_id, local_id "
                       "FROM authentication_sessions WHERE nonce = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_kill_session";
    statement->code  = "DELETE FROM authentication_sessions WHERE nonce = %s";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_set_normalized_id";
    statement->code  = "UPDATE authentication_sessions "
                       "SET normalized_id = %s WHERE nonce = %s";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_get_normalized_id";
    statement->code  = "SELECT normalized_id "
                       "FROM authentication_sessions WHERE nonce = %s LIMIT 1";
  };
}
