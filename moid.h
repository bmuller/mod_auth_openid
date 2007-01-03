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

namespace opkele {

  using namespace std;

  class MoidConsumer : public opkele::consumer_t {
  public:
    MoidConsumer(const string& storage_location);
    ~MoidConsumer() { close(); };
    assoc_t store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in);
    assoc_t retrieve_assoc(const string& server,const string& handle);
    void invalidate_assoc(const string& server,const string& handle);
    assoc_t find_assoc(const string& server);
    void print_db();
  private:
    Db db_;
    void close();    
    void ween_expired();
  };

  // in moid_utils.cpp
  params_t parse_query_string(const string& str);
  vector<string> explode(string s, string e);
  bool is_valid_url(string url);
}

namespace modauthopenid {

  using namespace std;

  typedef struct nonce {
    int expires_on; // exact moment it expires
  } NONCE;

  typedef struct session {
    char session_id[33];
    char path[255];
    char identity[255];
    //char identity_server[255];
    int expires_on; // exact moment it expires
  } SESSION;
  
  class SessionManager {
  public:
    SessionManager(const string& storage_location);
    ~SessionManager() { close(); };
    void get_session(const string& session_id, SESSION& session);
    void store_session(const string& session_id, const string& path, const string& identity);
  private:
   Db db_;
   void close();
   void ween_expired();
 };

 class NonceManager {
 public:
   NonceManager(const string& storage_location);
   ~NonceManager() { close(); };
   bool is_valid(const string& nonce, bool delete_on_find = true);
   void add(const string& nonce);
 private:
   Db db_;
   void close();
   void ween_expired();
 };


}
