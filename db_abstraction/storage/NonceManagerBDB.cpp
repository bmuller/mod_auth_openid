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

  NonceManagerBDB::NonceManagerBDB(const string& storage_location)  : db_(NULL, 0) {
    is_closed = false;
    u_int32_t oFlags = DB_CREATE; // Open flags;
    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       "nonces",                // Optional logical database name
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
  
  bool NonceManagerBDB::is_valid(const string& nonce, bool delete_on_find) {
    ween_expired();
    Dbt data;
    NONCE n;
    char id[255];
    strcpy(id, nonce.substr(0, 254).c_str());
    Dbt key(id, strlen(id) + 1);

    data.set_data(&n);
    data.set_ulen(sizeof(NONCE));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      debug("failed auth attempt: could not find nonce \"" + nonce + "\" in nonce db.");
      return false;
    }
    if(delete_on_find) {
      db_.del(NULL, &key, 0);
      debug("deleting nonce " + nonce + " from nonces table in db");
    }
    return true;
  };
  
  void NonceManagerBDB::delete_nonce(const string& nonce) {
    ween_expired();
    is_valid(nonce, true);
  };

  void NonceManagerBDB::get_identity(const string& nonce, string& identity) {
    ween_expired();
    Dbt data;
    NONCE n;
    char id[255];
    strcpy(id, nonce.substr(0, 254).c_str());
    Dbt key(id, strlen(id) + 1);

    data.set_data(&n);
    data.set_ulen(sizeof(NONCE));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      debug("failed to get identity: could not find nonce \"" + nonce + "\" in nonce db.");
    } else {
      identity = string(n.identity);
    }
  };
  
  // The reason we need to store the identity for the case of delgation - we need
  // to keep track of the original identity used by the user.  The identity stored
  // with this nonce will be used later if auth succeeds to create the session
  // for the original identity.
  void NonceManagerBDB::add(const string& nonce, const string& identity) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);
    NONCE n;
    n.expires_on = rawtime + 3600; // allow nonce to exist for one hour
    strcpy(n.identity, identity.substr(0, 254).c_str());
    
    char id[255];
    strcpy(id, nonce.substr(0, 254).c_str());
    Dbt key(id, strlen(id) + 1);
    Dbt data(&n, sizeof(NONCE));
    db_.put(NULL, &key, &data, 0);
    debug("added nonce to nonces table: " + nonce + " for identity: " + identity);
  };

  void NonceManagerBDB::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) {
	NONCE *n = (NONCE *) data.get_data();
        if(rawtime > n->expires_on) {
          db_.del(NULL, &key, 0);
        }
      }
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while weening nonce table!");
    } catch(std::exception &e) {
      db_.errx("Error while weening nonce table! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
  };

  int NonceManagerBDB::num_records() {
    ween_expired();
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    int count = 0;
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0)
        count++;
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while counting nonce table records!");
    } catch(std::exception &e) {
      db_.errx("Error while counting nonce table records! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
    return count;
  };

  void NonceManagerBDB::close() {
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
