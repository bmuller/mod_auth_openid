#include "moid.h"

namespace opkele {
  using namespace std;

  string make_rstring(int size) {
    string s = "";
    char *cs = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand((unsigned) time(0));
    for(int index=0; index<size; index++)
      s+= cs[rand()%62];
    return s;
  }

  vector<string> explode(string s, string e) {
    vector<string> ret;
    int iPos = s.find(e, 0);
    int iPit = e.length();
    while(iPos>-1) {
      if(iPos!=0)
        ret.push_back(s.substr(0,iPos));
      s.erase(0,iPos+iPit);
      iPos = s.find(e, 0);
    }
    if(s!="")
      ret.push_back(s);
    return ret;
  }

  params_t parse_query_string(const string& str) {
    params_t p;
    if(str.size() == 0) return p;
    char * t = curl_unescape(str.c_str(), str.length());
    if(!t)
      throw failed_conversion(OPKELE_CP_ "failed to curl_unescape()");
    string rv(t);
    curl_free(t);

    vector<string> pairs = explode(rv, "&");
    for(unsigned int i=0; i < pairs.size(); i++) {
      //fprintf(stdout, "pair = \"%s\"\n", pairs[i].c_str());
      string::size_type loc = pairs[i].find( "=", 0 );
      if( loc != string::npos ) {
        string key = pairs[i].substr(0, loc);
        string value = pairs[i].substr(loc+1);
        p[key] = value;
        //fprintf(stderr, "\"%s\" = \"%s\"\n", key.c_str(), value.c_str());
      }
    }
    return p;
  }
}
