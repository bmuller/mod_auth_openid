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

  SessionManagerBDB::SessionManagerBDB(const string& storage_location)  : db_(NULL, 0) {
    is_closed = false;
    u_int32_t oFlags = DB_CREATE; // Open flags;
    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       "sessions",                // Optional logical database name
	       DB_BTREE,            // Database access method
	       oFlags,              // Open flags
	       0);                  // File mode (using defaults)
      db_.set_errpfx("mod_auth_openid BDB error: ");
      db_.set_error_stream(&cerr); //this is apache's log
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database open failed %s", storage_location.c_str());
    } catch(std::exception &e) {
      db_.errx("Error opening database: %s", e.what());
    }
  };

  void SessionManagerBDB::get_session(const string& session_id, SESSION& session) {
    ween_expired();
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

  void SessionManagerBDB::store_session(const string& session_id, const string& hostname, const string& path, const string& identity) {
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

  void SessionManagerBDB::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) {
        SESSION *n = (SESSION *) data.get_data();
        if(rawtime > n->expires_on) 
          db_.del(NULL, &key, 0);
      }
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while weening sessions db");
    } catch(std::exception &e) {
      db_.errx("Error while weening sessions db! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
  };

  int SessionManagerBDB::num_records() {
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

  void SessionManagerBDB::close() {
    if(is_closed)
      return;
    is_closed = true;
    try {
      db_.close(0);
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database close failed");
    } catch(std::exception &e) {
      db_.errx("Error closing database: %s", e.what());
    }
  };
}
