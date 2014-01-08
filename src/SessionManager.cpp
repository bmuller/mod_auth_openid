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

  SessionManager::SessionManager(Dbd& _dbd) : dbd(_dbd) {}

  bool SessionManager::create_tables()
  {
    const char* ddl_sessionmanager =
      "CREATE TABLE IF NOT EXISTS sessionmanager "
      "(session_id "BIG_VARCHAR", hostname "BIG_VARCHAR", path "BIG_VARCHAR", "
      "identity "BIG_VARCHAR", username "BIG_VARCHAR", expires_on BIGINT)";
    return dbd.query(ddl_sessionmanager);
  }

  bool SessionManager::drop_tables()
  {
    return dbd.query("DROP TABLE IF EXISTS sessionmanager");
  }

  bool SessionManager::clear_tables()
  {
    return dbd.query("DELETE FROM sessionmanager");
  }

  bool SessionManager::get_session(const string& session_id, session_t& session)
  {
    return get_session(session_id, session, time(NULL));
  }

  bool SessionManager::get_session(const string& session_id, session_t& session, time_t now)
  {
    delete_expired(now);

    // mark session as invalid until we successfully load one
    session.identity = "";

    // try to find the session
    apr_dbd_results_t* results;
    apr_dbd_row_t* row;
    const void* args[] = {
      session_id.c_str(),
    };
    bool success = dbd.pbselect1("SessionManager_get_session", &results, &row, args);
    if (!success) {
      MOID_DEBUG("could not find session id " + session_id + " in db: session probably just expired");
      return false;
    }

    // load session fields from row
    dbd.getcol_string(row, 0, session.session_id);
    dbd.getcol_string(row, 1, session.hostname);
    dbd.getcol_string(row, 2, session.path);
    dbd.getcol_string(row, 3, session.identity);
    dbd.getcol_string(row, 4, session.username);
    dbd.getcol_int64 (row, 5, session.expires_on);

    dbd.close(results, &row);

    return true;
  }

  bool SessionManager::store_session(const string& session_id, const string& hostname,
                                     const string& path, const string& identity,
                                     const string& username, int lifespan)
  {
    return store_session(session_id, hostname, path, identity, username, lifespan, time(NULL));
  }

  bool SessionManager::store_session(const string& session_id, const string& hostname,
                                     const string& path, const string& identity,
                                     const string& username, int lifespan,
                                     time_t now)
  {
    delete_expired(now);

    // lifespan will be 0 if not specified by user in config - so lasts as long as browser is open.  In this case, make it last for up to a day.
    apr_int64_t expires_on = (lifespan == 0) ? (now + 86400) : (now + lifespan);

    const void* args[] = {
      session_id.c_str(),
      hostname.c_str(),
      path.c_str(),
      identity.c_str(),
      username.c_str(),
      &expires_on,
    };
    bool success = dbd.pbquery("SessionManager_store_session", args);
    if (!success) {
      MOID_DEBUG("problem inserting session into db");
      return false;
    }
    return true;
  }

  void SessionManager::delete_expired(time_t now) {
    const void* args[] = {
      &now,
    };
    bool success = dbd.pbquery("SessionManager_delete_expired", args);
    if (!success) {
      MOID_DEBUG("problem deleting expired sessions from table");
    }
  }

  void SessionManager::print_tables() {
    dbd.print_table("sessionmanager");
  }

  void SessionManager::append_statements(apr_array_header_t* statements)
  {
    labeled_statement_t* statement;

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "SessionManager_get_session";
    statement->code  = "SELECT session_id, hostname, path, identity, username, expires_on "
                       "FROM sessionmanager WHERE session_id = %s LIMIT 1";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "SessionManager_store_session";
    statement->code  = "INSERT INTO sessionmanager "
                       "(session_id, hostname, path, identity, username, expires_on) "
                       "VALUES (%s, %s, %s, %s, %s, %lld)";

    statement = (labeled_statement_t *)apr_array_push(statements);
    statement->label = "SessionManager_delete_expired";
    statement->code  = "DELETE FROM sessionmanager WHERE %lld >= expires_on";
  }
}
