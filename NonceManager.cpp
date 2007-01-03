#include "moid.h"

namespace modauthopenid {

  using namespace std;

  NonceManager::NonceManager(const string& storage_location)  : db_(NULL, 0) {
    u_int32_t oFlags = DB_CREATE; // Open flags;
    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       "nonces",                // Optional logical database name
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
  
  bool NonceManager::is_valid(const string& nonce, bool delete_on_find) {
    Dbt data;
    char id[255];
    strcpy(id, nonce.c_str());
    Dbt key(id, strlen(id) + 1);

    time_t rawtime;
    data.set_data(&rawtime);
    data.set_ulen(sizeof(time_t));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      fprintf(stderr, "Could not find nonce %s in nonce db.\n", nonce.c_str());
      return false;
    }
    if(delete_on_find) 
      db_.del(NULL, &key, 0);
    return true;
  };
  

  void NonceManager::add(const string& nonce) {
    time_t rawtime;
    time (&rawtime);
    
    char id[255];
    strcpy(id, nonce.c_str());
    Dbt key(id, strlen(id) + 1);
    Dbt data(&rawtime, sizeof(time_t));
    db_.put(NULL, &key, &data, 0);
  };

  void NonceManager::close() {
    try {
      db_.close(0);
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database close failed");
    } catch(std::exception &e) {
      db_.errx("Error closing database: %s", e.what());
    }
  };
  
}
