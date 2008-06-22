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
#include <pcre.h>
#include <algorithm>


#include <opkele/exception.h>
#include <opkele/types.h>
#include <opkele/util.h>
#include <opkele/uris.h>
#include <opkele/discovery.h>
#include <opkele/association.h>
#include <opkele/sreg.h>
#include <opkele/prequeue_rp.h>

/*
#include <opkele/consumer.h>
#include <opkele/association.h>
#include <opkele/exception.h>
#include <opkele/prequeue_rp.h>
#include <opkele/sreg.h>
#include <opkele/util.h>
#include <opkele/types.h>
*/

#include <time.h>
#include <string>
#include <vector>
#include <sqlite3.h>

/* overwrite package vars set by apache */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include "config.h"

namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  enum error_result_t { no_idp_found, invalid_id_url, idp_not_trusted, invalid_nonce, canceled, unspecified };

  typedef struct session {
    char session_id[33];
    char hostname[255]; // name of server (this is in case there are virtual hosts on this server)
    char path[255];
    char identity[255];
    int expires_on; // exact moment it expires
  } SESSION;

  typedef struct nonce {
    int expires_on; // exact moment it expires
    char identity[255]; // identity nonce is good for
  } NONCE;

  class SessionManager {
  public:
    SessionManager(const string& storage_location);
    ~SessionManager() { close(); };
    void get_session(const string& session_id, SESSION& session);
    void store_session(const string& session_id, const string& hostname, const string& path, const string& identity);
    int num_records();
    void close();
  private:
    sqlite3 *db;
    void ween_expired();
    bool is_closed;
    bool test_result(int result, const string& context);
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
    sqlite3 *db;
    void ween_expired();
    bool test_result(int result, const string& context);
    bool is_closed;
  };

  class MoidConsumer : public prequeue_RP {
  public:
    MoidConsumer(const string& storage_location, const string& _asnonceid, const string& _serverurl);
    virtual ~MoidConsumer() { close(); };
    assoc_t store_assoc(const string& server,const string& handle,const string& type,const secret_t& secret,int expires_in);
    assoc_t retrieve_assoc(const string& server,const string& handle);
    void invalidate_assoc(const string& server,const string& handle);
    assoc_t find_assoc(const string& server);
    void check_nonce(const string& OP,const string& nonce);
    void begin_queueing();
    void queue_endpoint(const openid_endpoint_t& ep);
    const openid_endpoint_t& get_endpoint() const;
    void next_endpoint();
    void set_normalized_id(const string& nid);
    const string get_normalized_id() const;
    const string get_this_url() const;
    void print_db();
    int num_records();
    void close();
  private:
    sqlite3 *db;
    void ween_expired();
    bool test_result(int result, const string& context);
    bool is_closed, endpoint_set;
    string asnonceid, normalized_id, serverurl; 
    mutable openid_endpoint_t endpoint;
  };


  // in moid_utils.cpp
  string error_to_string(error_result_t e, bool use_short_string);
  string get_queryless_url(string url);
  params_t parse_query_string(const string& str);
  vector<string> explode(string s, string e);
  string str_replace(string needle, string replacement, string haystack);
  string html_escape(string s);
  string url_decode(const string& str);
  params_t remove_openid_vars(params_t params);
  string get_base_url(string url);
  void make_cookie_value(string& cookie_value, const string& name, const string& session_id, const string& path, int cookie_lifespan);
  // Should be using ap_log_error, but that would mean passing a server_rec* or request_rec* around..... 
  // gag....  I'm just assuming that if you're going to be debugging it shouldn't really matter, since
  // apache redirects stderr to the error log anyway.
  void debug(string s);
  void int_to_string(int i, string& s);
  bool regex_match(string subject, string pattern);
  void print_to_error_log(string s);
  void strip(string& s);
  void make_rstring(int size, string& s);

  // in http_helpers.cpp
  int http_sendstring(request_rec *r, string s);
  int http_redirect(request_rec *r, string location);
  int show_html_input(request_rec *r, string msg);
  void get_session_id(request_rec *r, string cookie_name, string& session_id);
}


