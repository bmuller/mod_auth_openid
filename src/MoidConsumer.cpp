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
 
  MoidConsumer::MoidConsumer(Dbd& _dbd, const string& _asnonceid,
                             const string& _serverurl) :
                             dbd(_dbd), asnonceid(_asnonceid), serverurl(_serverurl),
                             endpoint_set(false), normalized_id("") {}
 
  MoidConsumer::MoidConsumer(Dbd& _dbd) :
                             dbd(_dbd), asnonceid(""), serverurl(""),
                             endpoint_set(false), normalized_id("") {}

  bool MoidConsumer::create_tables()
  {
    bool success = true;

    const char* ddl_authentication_sessions = 
      "CREATE TABLE IF NOT EXISTS authentication_sessions "
      "(nonce "BIG_VARCHAR", uri "BIG_VARCHAR", claimed_id "BIG_VARCHAR", local_id "BIG_VARCHAR", "
      "normalized_id "BIG_VARCHAR", expires_on BIGINT)";
    success &= dbd.query(ddl_authentication_sessions);

    const char* ddl_assocations = 
      "CREATE TABLE IF NOT EXISTS associations "
      "(server "BIG_VARCHAR", handle "BIG_VARCHAR", assoc_type "BIG_VARCHAR", "
      "secret "BIG_VARCHAR", expires_on BIGINT)";
    success &= dbd.query(ddl_assocations);

    const char* ddl_response_nonces = 
      "CREATE TABLE IF NOT EXISTS response_nonces "
      "(server "BIG_VARCHAR", response_nonce "BIG_VARCHAR", expires_on BIGINT)";
    success &= dbd.query(ddl_response_nonces);

    return success;
  }

  bool MoidConsumer::drop_tables()
  {
    bool success = true;

    success &= dbd.query("DROP TABLE IF EXISTS authentication_sessions");
    success &= dbd.query("DROP TABLE IF EXISTS associations");
    success &= dbd.query("DROP TABLE IF EXISTS response_nonces");

    return success;
  }

  bool MoidConsumer::clear_tables()
  {
    bool success = true;

    success &= dbd.query("DELETE FROM authentication_sessions");
    success &= dbd.query("DELETE FROM associations");
    success &= dbd.query("DELETE FROM response_nonces");

    return success;
  }

  assoc_t MoidConsumer::store_assoc(const string& server, const string& handle,
                                    const string& assoc_type, const secret_t& secret,
                                    int expires_in)
  {
    return store_assoc(server, handle, assoc_type, secret, expires_in, time(NULL));
  }

  assoc_t MoidConsumer::store_assoc(const string& server, const string& handle,
                                    const string& assoc_type, const secret_t& secret,
                                    int expires_in, time_t now)
  {
    MOID_DEBUG("Storing association for \"" + server + "\" and handle \"" + handle + "\" in db");

    delete_expired(now);

    apr_int64_t expires_on = now + expires_in;

    string secret_base64;
    secret.to_base64(secret_base64);

    const void* args[] = {
      server.c_str(),
      handle.c_str(),
      secret_base64.c_str(),
      &expires_on,
      assoc_type.c_str(),
    };
    bool success = dbd.pbquery("MoidConsumer_store_assoc", args);
    if (!success) {
      MOID_DEBUG("problem storing association in associations table");
    }
    return assoc_t(new association(server, handle, assoc_type, secret, expires_on, false));
  }

  assoc_t MoidConsumer::load_assoc(apr_dbd_results_t* results, apr_dbd_row_t** row)
  {
    string server, handle, secret_base64, assoc_type;
    apr_int64_t expires_on;
    secret_t secret;

    dbd.getcol_string(*row, 0, server);
    dbd.getcol_string(*row, 1, handle);
    dbd.getcol_string(*row, 2, secret_base64);
    dbd.getcol_int64 (*row, 3, expires_on);
    dbd.getcol_string(*row, 4, assoc_type);

    dbd.close(results, row);

    secret.from_base64(secret_base64);

    return assoc_t(new association(server, handle, assoc_type, secret, expires_on, false));
  }

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle)
  {
    return retrieve_assoc(server, handle, time(NULL));
  }

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle, time_t now)
  {
    MOID_DEBUG("looking up association: server = " + server + " handle = " + handle);

    delete_expired(now);

    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    const void* args[] = {
      server.c_str(),
      handle.c_str(),
    };
    bool success = dbd.pbselect1("MoidConsumer_retrieve_assoc", &results, &row, args);
    if (!success) {
      MOID_DEBUG("could not find server \"" + server + "\" and handle \"" + handle + "\" in db.");
      throw failed_lookup(OPKELE_CP_ "Could not find association.");
    }

    return load_assoc(results, &row);
  }

  void MoidConsumer::invalidate_assoc(const string& server, const string& handle)
  {
    MOID_DEBUG("invalidating association: server = " + server + " handle = " + handle);
    const void* args[] = {
      server.c_str(),
      handle.c_str(),
    };
    bool success = dbd.pbquery("MoidConsumer_invalidate_assoc", args);
    if (!success) {
      MOID_DEBUG("problem invalidating assocation for server \"" +
            server + "\" and handle \"" + handle + "\"");
    }
  }

  assoc_t MoidConsumer::find_assoc(const string& server)
  {
    return find_assoc(server, time(NULL));
  }

  assoc_t MoidConsumer::find_assoc(const string& server, time_t now)
  {
    MOID_DEBUG("looking up association: server = " + server);

    delete_expired(now);

    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    const void* args[] = {
      server.c_str(),
    };
    bool success = dbd.pbselect1("MoidConsumer_find_assoc", &results, &row, args);
    if (!success) {
      MOID_DEBUG("could not find handle for server \"" + server + "\" in db.");
      throw failed_lookup(OPKELE_CP_ "Could not find association.");
    }

    MOID_DEBUG("found a handle for server \"" + server + "\" in db.");
    return load_assoc(results, &row);
  }

  void MoidConsumer::delete_expired(time_t now) {
    bool success;
    const void* args[] = {
      &now,
    };

    success = dbd.pbquery("MoidConsumer_delete_expired_associations", args);
    if (!success) {
      MOID_DEBUG("problem deleting expired associations from table");
    }

    success = dbd.pbquery("MoidConsumer_delete_expired_authentication_sessions", args);
    if (!success) {
      MOID_DEBUG("problem deleting expired authentication sessions from table");
    }

    success = dbd.pbquery("MoidConsumer_delete_expired_response_nonces", args);
    if (!success) {
      MOID_DEBUG("problem deleting expired response nonces from table");
    }
  }

  void MoidConsumer::check_nonce(const string& server, const string& nonce)
  {
    check_nonce(server, nonce, time(NULL));
  }

  void MoidConsumer::check_nonce(const string& server, const string& nonce, time_t now)
  {
    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    bool success;

    // Look for previous uses of this nonce.
    const void* find_args[] = {
      server.c_str(),
      nonce.c_str(),
    };
    success = dbd.pbselect1("MoidConsumer_check_nonce_find", &results, &row, find_args);
    if (success) {
      // result set contains at least one row
      dbd.close(results, &row);
      MOID_DEBUG("found preexisting nonce - could be a replay attack");
      throw opkele::id_res_bad_nonce(OPKELE_CP_ "old nonce used again - possible replay attack");
    }

    // Nonce not previously used, insert it into nonces table.
    // Expiration time will be based on association.
    apr_int64_t expires_on = find_assoc(server)->expires_in() + now;
    const void* insert_args[] = {
      server.c_str(),
      nonce.c_str(),
      &expires_on,
    };
    success = dbd.pbquery("MoidConsumer_check_nonce_insert", insert_args);
    if (!success) {
      MOID_DEBUG("problem adding new nonce to response_nonces table");
    }
  }

  bool MoidConsumer::session_exists() {
    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    bool success;
    const void* args[] = {
      asnonceid.c_str(),
    };
    success = dbd.pbselect1("MoidConsumer_session_exists", &results, &row, args);
    if (!success) {
      MOID_DEBUG("could not find authentication session \"" + asnonceid + "\" in db.");
      return false;
    }
    dbd.close(results, &row);
    return true;
  };

  void MoidConsumer::begin_queueing()
  {
    endpoint_set = false;
    kill_session();
  }

  void MoidConsumer::queue_endpoint(const openid_endpoint_t& ep)
  {
    queue_endpoint(ep, time(NULL));
  }

  void MoidConsumer::queue_endpoint(const openid_endpoint_t& ep, time_t now)
  {
    if (endpoint_set) {
      return;
    }

    MOID_DEBUG("Queueing endpoint " + ep.claimed_id + " : " + ep.local_id + " @ " + ep.uri);
    // allow nonce to exist for up to one hour without being returned
    apr_int64_t expires_on = now + 3600;
    const void* args[] = {
      asnonceid.c_str(),
      ep.uri.c_str(),
      ep.claimed_id.c_str(),
      ep.local_id.c_str(),
      &expires_on,
    };
    bool success = dbd.pbquery("MoidConsumer_queue_endpoint", args);
    if (!success) {
      MOID_DEBUG("problem queuing endpoint");
    }
  }

  const openid_endpoint_t& MoidConsumer::get_endpoint() const
  {
    MOID_DEBUG("Fetching endpoint");

    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    const void* args[] = {
      asnonceid.c_str(),
    };
    bool success = dbd.pbselect1("MoidConsumer_get_endpoint", &results, &row, args);
    if (!success) {
      MOID_DEBUG("could not find an endpoint for authentication session \""
                 + asnonceid + "\" in db.");
      throw opkele::exception(OPKELE_CP_ "No more endpoints queued");
    }

    dbd.getcol_string(row, 0, endpoint.uri);
    dbd.getcol_string(row, 1, endpoint.claimed_id);
    dbd.getcol_string(row, 2, endpoint.local_id);

    dbd.close(results, &row);

    return endpoint;
  }

  void MoidConsumer::next_endpoint()
  {
    MOID_DEBUG("Clearing all session information - we're only storing one endpoint, "
               "can't get next one, cause we didn't store it.");
    endpoint_set = false;
    kill_session();
  }

  void MoidConsumer::kill_session()
  {
    const void* args[] = {
      asnonceid.c_str(),
    };
    bool success = dbd.pbquery("MoidConsumer_kill_session", args);
    if (!success) {
      MOID_DEBUG("problem killing session");
    }
  }

  void MoidConsumer::set_normalized_id(const string& nid)
  {
    MOID_DEBUG("Set normalized id to: " + nid);
    normalized_id = nid;

    const void* args[] = {
      normalized_id.c_str(),
      asnonceid.c_str(),
    };
    bool success = dbd.pbquery("MoidConsumer_set_normalized_id", args);
    if (!success) {
      MOID_DEBUG("problem setting normalized id");
    }
  }

  const string MoidConsumer::get_normalized_id() const
  {
    if (normalized_id != "") {
      MOID_DEBUG("getting normalized id - " + normalized_id);
      return normalized_id;
    }

    // not set, fetch it from DB
    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    const void* args[] = {
      asnonceid.c_str(),
    };
    bool success = dbd.pbselect1("MoidConsumer_get_normalized_id", &results, &row, args);
    if (!success) {
      MOID_DEBUG("could not find a normalized_id for authentication session \""
                 + asnonceid + "\" in db.");
      throw opkele::exception(OPKELE_CP_ "cannot get normalized id");
    }

    dbd.getcol_string(row, 0, normalized_id);

    dbd.close(results, &row);

    MOID_DEBUG("getting normalized id - " + normalized_id);
    return normalized_id;
  };

  const string MoidConsumer::get_this_url() const {
    return serverurl;
  };

  void MoidConsumer::print_tables() {
    dbd.print_table("authentication_sessions");
    dbd.print_table("response_nonces");
    dbd.print_table("associations");
  }

  void MoidConsumer::append_statements(apr_array_header_t *statements)
  {
    labeled_statement_t* statement;

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_store_assoc";
    statement->code  = "INSERT INTO associations "
                       "(server, handle, secret, expires_on, assoc_type) "
                       "VALUES (%s, %s, %s, %lld, %s)";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_retrieve_assoc";
    statement->code  = "SELECT server, handle, secret, expires_on, assoc_type "
                       "FROM associations WHERE server = %s AND handle = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_invalidate_assoc";
    statement->code  = "DELETE FROM associations WHERE server = %s AND handle = %s";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_find_assoc";
    statement->code  = "SELECT server, handle, secret, expires_on, assoc_type "
                       "FROM associations WHERE server = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_delete_expired_associations";
    statement->code  = "DELETE FROM associations WHERE %lld > expires_on";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_delete_expired_authentication_sessions";
    statement->code  = "DELETE FROM authentication_sessions WHERE %lld > expires_on";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "MoidConsumer_delete_expired_response_nonces";
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
  }
}
