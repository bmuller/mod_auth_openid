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

  NonceManager::NonceManager(const string& storage_location)  : db_(NULL, 0) {
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
  
  bool NonceManager::is_valid(const string& nonce, bool delete_on_find) {
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
  

  void NonceManager::add(const string& nonce) {
    ween_expired();
    time_t rawtime;
    time (&rawtime);
    NONCE n;
    n.expires_on = rawtime + 3600; // allow nonce to exist for one hour
    
    char id[255];
    strcpy(id, nonce.substr(0, 254).c_str());
    Dbt key(id, strlen(id) + 1);
    Dbt data(&n, sizeof(NONCE));
    db_.put(NULL, &key, &data, 0);
    debug("added nonce to nonces table: " + nonce);
  };

  void NonceManager::ween_expired() {
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

  int NonceManager::num_records() {
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

  void NonceManager::close() {
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
