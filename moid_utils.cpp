#include "mod_auth_openid.h"

namespace modauthopenid {
  using namespace std;

  void debug(string s) {
#ifdef DEBUG
    string time_s = "";
    time_t rawtime = time(NULL);
    tm *tm_t = localtime(&rawtime);
    char rv[40];
    if(strftime(rv, sizeof(rv)-1, "%a %b %d %H:%M:%S %Y", tm_t)) 
      time_s = "[" + string(rv) + "] ";
    s = time_s + "[" + string(PACKAGE_NAME) + "] " + s + "\n";
    // escape %'s
    string cleaned_s = "";
    vector<string> parts = opkele::explode(s, "%");
    for(int i=0; i<parts.size()-1; i++)
      cleaned_s += parts[i] + "%%";
    cleaned_s += parts[parts.size()-1];
    // stderr is redirected by apache to apache's error log
    fprintf(stderr, cleaned_s.c_str());
    fflush(stderr);
#endif
  };

}

namespace opkele {
  using namespace std;

  params_t remove_openid_vars(params_t params) {
    map<string,string>::iterator iter;
    for(iter = params.begin(); iter != params.end(); iter++) {
      string param_key(iter->first);
      if(param_key.substr(0, 7) == "openid.") {
	params.erase(param_key);
	// stupid map iterator screws up if we just continue the iteration...
	// so recusion to the rescue - we'll delete them one at a time
	return remove_openid_vars(params);
      } 
    }
    return params;
  }

  bool is_valid_url(string url) {
    pcrepp::Pcre reg("^https?://([-\\w\\.]+)+(:\\d+)?(/([\\w/_\\.]*(\\?\\S+)?)?)?");
    return reg.search(url);
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

  string url_decode(const string& str) {
    char * t = curl_unescape(str.c_str(),str.length());
    if(!t)
      throw failed_conversion(OPKELE_CP_ "failed to curl_escape()");
    string rv(t);
    curl_free(t);
    return rv;
  }

  params_t parse_query_string(const string& str) {
    params_t p;
    if(str.size() == 0) return p;

    vector<string> pairs = explode(str, "&");
    for(unsigned int i=0; i < pairs.size(); i++) {
      //fprintf(stdout, "pair = \"%s\"\n", pairs[i].c_str());
      string::size_type loc = pairs[i].find( "=", 0 );
      if( loc != string::npos ) {
        string key = url_decode(pairs[i].substr(0, loc));
        string value = url_decode(pairs[i].substr(loc+1));
        p[key] = value;
        //fprintf(stderr, "\"%s\" = \"%s\"\n", key.c_str(), value.c_str()); fflush(stderr);
      }
    }
    return p;
  }
}
