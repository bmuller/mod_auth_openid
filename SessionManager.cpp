#include "moid.h"

namespace modauthopenid {

  using namespace std;

  SessionManager::SessionManager(const string& storage_location)  : db_(NULL, 0) {
    u_int32_t oFlags = DB_CREATE; // Open flags;
    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       "sessions",                // Optional logical database name
	       DB_BTREE,            // Database access method
	       oFlags,              // Open flags
	       0);                  // File mode (using defaults)
      db_.set_errpfx("mod_openid bdb: ");
      db_.set_error_stream(&cerr); //this is apache's log
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database open failed %s", storage_location.c_str());
    } catch(exception &e) {
      db_.errx("Error opening database: %s", e.what());
    }
  };

  void SessionManager::get_session(const string& session_id, SESSION& session) {
    ween_expired();
    Dbt data;
    char id[255];
    strcpy(id, session_id.c_str());
    Dbt key(id, strlen(id) + 1);
    data.set_data(&session);
    data.set_ulen(sizeof(SESSION));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      strcpy(session.identity, "");
      //fprintf(stderr, "Could not find session %s and handle %s in db.\n", server.c_str(), handle.c_str());
    }
  };

  void SessionManager::store_session(const string& session_id, const string& path, const string& identity) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);
    
    SESSION s;
    strcpy(s.session_id, session_id.c_str());
    strcpy(s.path, path.c_str());
    strcpy(s.identity, identity.c_str());
    //strcpy(s.identity_server, identity_server.c_str());

    // sessions last for a day becore being automatically invalidated
    s.expires_on = rawtime + 86400; 
    
    char id[255];
    strcpy(id, session_id.c_str());
    Dbt key(id, strlen(id) + 1);
    Dbt data(&s, sizeof(SESSION));
    db_.put(NULL, &key, &data, 0);
  };

  void SessionManager::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) {
        SESSION *n = (SESSION *) data.get_data();
        if(rawtime > n->expires_on) {
          //fprintf(stderr, "Expires_on %i is greater than current time %i", data_v->expires_on, rawtime); fflush(stderr);
          db_.del(NULL, &key, 0);
        }
      }
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error!");
    } catch(std::exception &e) {
      db_.errx("Error! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
  };

  int SessionManager::num_records() {
    ween_expired();
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    int count = 0;
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0)
        count++;
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error!");
    } catch(std::exception &e) {
      db_.errx("Error! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
    return count;
  };

  void SessionManager::close() {
    try {
      db_.close(0);
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database close failed");
    } catch(std::exception &e) {
      db_.errx("Error closing database: %s", e.what());
    }
  };
}
