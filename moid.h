#include <curl/curl.h>
#include <pcre++.h>
#include <algorithm>
#include <opkele/consumer.h>
#include <opkele/association.h>
#include <opkele/exception.h>
#include <db_cxx.h>
#include <time.h>
#include <string>

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
  };

  // in moid_utils.cpp
  params_t parse_query_string(const string& str);
  vector<string> explode(string s, string e);

}

namespace modauthopenid {

using namespace std;

typedef struct session {
  char session_id[32];
  char path[255];
  char identity[255];
  char identity_server[255];
  int expires_on; // exact moment it expires
} SESSION;

class SessionManager {
 public:
  SessionManager(const string& storage_location);
  ~SessionManager() { close(); };
  void get_session(const string& session_id, SESSION& session);
  void store_session(const string& session_id, const string& path, const string& identity, const string& identity_server);
 private:
  Db db_;
  void close();
};

}
