#include "moid.h"

namespace opkele {
  using namespace std;
  
  typedef struct bdb_association {
    char server[255];
    char handle[100];
    char secret[30];
    int expires_on; // exact moment it expires
  } BDB_ASSOC;

  MoidConsumer::MoidConsumer(const string& storage_location) : db_(NULL, 0) {
    u_int32_t oFlags = DB_CREATE; // Open flags;

    try {
      db_.open(NULL,                // Transaction pointer
	       storage_location.c_str(),          // Database file name
	       NULL,                // Optional logical database name
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
    string secret_b64;
    secret.to_base64(secret_b64);
    time_t rawtime;
    time (&rawtime);

    BDB_ASSOC bassoc;
    strcpy(bassoc.secret, secret_b64.c_str());
    strcpy(bassoc.server, server.c_str());
    strcpy(bassoc.handle, handle.c_str());
    bassoc.expires_on = rawtime + expires_in;

    string id = server+handle;
    char c_id[255];
    strcpy(c_id, id.c_str());
    Dbt key(c_id, strlen(c_id) + 1);
    Dbt data(&bassoc, sizeof(BDB_ASSOC));
    db_.put(NULL, &key, &data, 0);
    auto_ptr<association_t> a(new association(server, handle, "assoc type", secret, expires_in, false));
    return a;
  };
  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle) {
    Dbt data;
    BDB_ASSOC bassoc;
    string id = server+handle;
    char c_id[255];
    strcpy(c_id, id.c_str());
    Dbt key(c_id, strlen(c_id) + 1);
    data.set_data(&bassoc);
    data.set_ulen(sizeof(BDB_ASSOC));
    data.set_flags(DB_DBT_USERMEM);
    if(db_.get(NULL, &key, &data, 0) == DB_NOTFOUND) {
      fprintf(stderr, "Could not find server %s and handle %s in db.\n", server.c_str(), handle.c_str());
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
    string id = server+handle;
    char c_id[255];
    strcpy(c_id, id.c_str());
    Dbt key(c_id, strlen(c_id) + 1);
    db_.del(NULL, &key, 0);
  };
  assoc_t MoidConsumer::find_assoc(const string& server) { throw failed_lookup("blah"); };


  // due to poor design in libopkele - this stuff must go here
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
      //|| (r=curl_easy_setopt(c,CURLOPT_USERAGENT,PACKAGE_NAME"/"PACKAGE_VERSION))
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
