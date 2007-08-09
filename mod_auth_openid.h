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


/* Apache includes. */
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "apr_strings.h"
#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"

/* other includes */
#include <curl/curl.h>
#include <pcre++.h>
#include <algorithm>
#include <opkele/consumer.h>
#include <opkele/association.h>
#include <opkele/exception.h>
#include <db_cxx.h>
#include <time.h>
#include <string>
#include <vector>

/* overwrite package vars set by apache */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include "config.h"
#include "storage/storage.h"

namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  enum error_result_t { no_idp_found, invalid_id_url, idp_not_trusted, invalid_nonce, canceled, unspecified };

  typedef struct bdb_association {
    char server[255];
    char handle[100];
    char secret[30];
    int expires_on; // exact moment it expires
  } BDB_ASSOC;
  
  typedef struct nonce {
    int expires_on; // exact moment it expires
    char identity[255]; // identity nonce is good for
  } NONCE;

  class MoidConsumer : public opkele::consumer_t {
  public:
    MoidConsumer(const string& storage_location);
    virtual ~MoidConsumer() { close(); };
    assoc_t store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in);
    assoc_t retrieve_assoc(const string& server,const string& handle);
    void invalidate_assoc(const string& server,const string& handle);
    assoc_t find_assoc(const string& server);
    void print_db();
    int num_records();
    void close();    
  private:
    Db db_;
    void ween_expired();
    bool is_closed;
  };

  class SessionManager {
  public:
    SessionManager(const string& storage_location) : sm(storage_location) {};
    void get_session(const string& session_id, SESSION& session);
    void store_session(const string& session_id, const string& hostname, const string& path, const string& identity);
    int num_records();
    void close();
  private:
#ifdef SQLITE
    SessionManagerSQLite sm;
#else
    SessionManagerBDB sm;
#endif
  };

  class NonceManager {
  public:
    NonceManager(const string& storage_location);
    ~NonceManager() { close(); };
    bool is_valid(const string& nonce, bool delete_on_find = false);
    void add(const string& nonce, const string& identity);
    void delete_nonce(const string& nonce);
    void get_identity(const string& nonce, string& identity);
    int num_records();
    void close();
  private:
    Db db_;
    void ween_expired();
    bool is_closed;
  };

  // in moid_utils.cpp
  string error_to_string(error_result_t e, bool use_short_string);
  string get_queryless_url(string url);
  params_t parse_query_string(const string& str);
  vector<string> explode(string s, string e);
  string str_replace(string needle, string replacement, string haystack);
  string html_escape(string s);
  bool is_valid_url(string url);
  string url_decode(const string& str);
  params_t remove_openid_vars(params_t params);
  string get_base_url(string url);
  void make_cookie_value(string& cookie_value, const string& name, const string& session_id, const string& path, int cookie_lifespan);
  // Should be using ap_log_error, but that would mean passing a server_rec* or request_rec* around..... 
  // gag....  I'm just assuming that if you're going to be debugging it shouldn't really matter, since
  // apache redirects stderr to the error log anyway.
  void debug(string s);
  void int_to_string(int i, string& s);
}

