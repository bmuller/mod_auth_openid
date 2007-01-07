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

namespace opkele {
  using namespace std;
  using namespace modauthopenid;
 
  MoidConsumer::MoidConsumer(const string& storage_location) : db_(NULL, 0) {
    u_int32_t oFlags = DB_CREATE; // Open flags;
    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       "associations",                // Optional logical database name
	       DB_BTREE,            // Database access method
	       oFlags,              // Open flags
	       0);                  // File mode (using defaults)
      db_.set_errpfx("mod_auth_openid BDB error: ");
      db_.set_error_stream(&cerr); //this is apache's log
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database open failed %s", storage_location.c_str());
    } catch(exception &e) {
      db_.errx("Error opening database: %s", e.what());
    }
  };

  void MoidConsumer::close() {
    try {
      db_.close(0);
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Database close failed");
    } catch(std::exception &e) {
      db_.errx("Error closing database: %s", e.what());
    }
  };

  assoc_t MoidConsumer::store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in) {
    ween_expired();
    string secret_b64;
    secret.to_base64(secret_b64);
    time_t rawtime;
    time (&rawtime);

    BDB_ASSOC bassoc;
    // server is char[255], handle is char[100], and secret is char[30]
    strcpy(bassoc.secret, secret_b64.substr(0, 29).c_str());
    strcpy(bassoc.server, server.substr(0, 254).c_str());
    strcpy(bassoc.handle, handle.substr(0, 99).c_str());
    bassoc.expires_on = rawtime + expires_in;

    // We want to store with unique key on server and handle, but want to be able to search by server.
    // If the server+" "+handle string is greater than 254 chars it will be truncated.  This should
    // be ok....
    string id = server + " " + handle;
    char c_id[255]; 
    strcpy(c_id, id.substr(0, 254).c_str()); // safety first!

    Dbt key(c_id, strlen(c_id) + 1);
    Dbt data(&bassoc, sizeof(BDB_ASSOC));
    db_.put(NULL, &key, &data, 0);

    debug("Storing server \"" + server + "\" and handle \"" + handle + "\" in db");

    auto_ptr<association_t> a(new association(server, handle, "assoc type", secret, expires_in, false));
    return a;
  };

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle) {
    ween_expired();
    debug("looking up association: server = "+server+" handle = "+handle);
    Dbt data;
    BDB_ASSOC bassoc;
    string id = server + " " + handle;
    char c_id[255];
    strcpy(c_id, id.substr(0, 254).c_str());

    Dbt key(c_id, strlen(c_id) + 1);
    data.set_data(&bassoc);
    data.set_ulen(sizeof(BDB_ASSOC));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      debug("could not find server \"" + server + "\" and handle \"" + handle + "\" in db.");
      throw failed_lookup("Could not find association.");
    }

    time_t rawtime;
    time (&rawtime);
    int expires_in = bassoc.expires_on - rawtime;

    secret_t secret;
    secret.from_base64(bassoc.secret);

    auto_ptr<association_t> a(new association(bassoc.server, bassoc.handle, "assoc type", secret, expires_in, false));
    return a;    
  };
  void MoidConsumer::invalidate_assoc(const string& server,const string& handle) {
    debug("invalidating association: server = " + server + " handle = " + handle);
    string id = server + " " + handle;
    char c_id[255];
    strcpy(c_id, id.substr(0, 254).c_str());
    Dbt key(c_id, strlen(c_id) + 1);
    db_.del(NULL, &key, 0);
  };

  assoc_t MoidConsumer::find_assoc(const string& server) {
    ween_expired();
    debug("looking for any association with server = "+server);
    time_t rawtime;
    time (&rawtime);
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) {
        char * key_v = (char *) key.get_data();
        BDB_ASSOC * data_v = (BDB_ASSOC *) data.get_data();
	string key_s(key_v);
	vector<string> parts = explode(key_s, " ");
	// If server url we were given matches the current record, and it still has
	// at least five minutes until it expires (to give the user time to be redirected -> there -> back)
        if(parts.size()==2 && parts[0] == server && rawtime < (data_v->expires_on + 18000)) {
	  debug("....found one");
	  int expires_in = data_v->expires_on - rawtime;
	  secret_t secret;
	  secret.from_base64(data_v->secret);
	  auto_ptr<association_t> a(new association(data_v->server, data_v->handle, "assoc type", secret, expires_in, false));
	  return a;
        }
      }
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while looking for an assocation!");
    } catch(std::exception &e) {
      db_.errx("Error while looking for an association! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();

    throw failed_lookup("Could not find a valid handle."); 
  };

  void MoidConsumer::ween_expired() {
    time_t rawtime;
    time (&rawtime);
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) {
        char * key_v = (char *) key.get_data();
        BDB_ASSOC * data_v = (BDB_ASSOC *) data.get_data();
	if(rawtime > data_v->expires_on) {
	  db_.del(NULL, &key, 0);
	}
      }
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while weening associations table!");
    } catch(std::exception &e) {
      db_.errx("Error while weening associations table! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
  };

  // This is a method to be used by a utility program, never the apache module
  int MoidConsumer::num_records() {
    ween_expired();
    Dbt key, data;
    Dbc *cursorp;
    db_.cursor(NULL, &cursorp, 0);
    int count = 0;
    try {
      while (cursorp->get(&key, &data, DB_NEXT) == 0) 
	count++;
    } catch(DbException &e) {
      db_.err(e.get_errno(), "Error while counting associations records!");
    } catch(std::exception &e) {
      db_.errx("Error while counting associations records! %s", e.what());
    }
    if (cursorp != NULL)
      cursorp->close();
    return count;
  };

  // word for word from libopkele (none of these were included in the header files)
  class curl_t {
  public:
    CURL *_c;

    curl_t() : _c(0) { }
    curl_t(CURL *c) : _c(c) { }
    ~curl_t() throw() { if(_c) curl_easy_cleanup(_c); }

    curl_t& operator=(CURL *c) { if(_c) curl_easy_cleanup(_c); _c=c; return *this; }

    operator const CURL*(void) const { return _c; }
    operator CURL*(void) { return _c; }
  };
  static CURLcode curl_misc_sets(CURL* c) {
    CURLcode r;
    (r=curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1))
      || (r=curl_easy_setopt(c,CURLOPT_MAXREDIRS,5))
      || (r=curl_easy_setopt(c,CURLOPT_DNS_CACHE_TIMEOUT,120))
      || (r=curl_easy_setopt(c,CURLOPT_DNS_USE_GLOBAL_CACHE,1))
      || (r=curl_easy_setopt(c,CURLOPT_USERAGENT,PACKAGE_NAME"/"PACKAGE_VERSION))
      || (r=curl_easy_setopt(c,CURLOPT_TIMEOUT,20))
      ;
    return r;
  }
  static size_t _curl_tostring(void *ptr,size_t size,size_t nmemb,void *stream) {
    string *str = (string*)stream;
    size_t bytes = size*nmemb;
    size_t get = min(16384-str->length(),bytes);
    str->append((const char*)ptr,get);
    return get;
  }

}



