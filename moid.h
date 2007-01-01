#include <curl/curl.h>
#include <pcre++.h>
#include <algorithm>
#include <opkele/consumer.h>
#include <opkele/association.h>
#include <opkele/exception.h>
#include <db_cxx.h>
#include <time.h>

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
  private:
    Db db_;
    void close();    
  };

  // in moid_utils.cpp
  params_t parse_query_string(const string& str);
  vector<string> explode(string s, string e);

}
