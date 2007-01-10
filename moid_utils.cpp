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
    vector<string> parts = explode(s, "%");
    for(unsigned int i=0; i<parts.size()-1; i++)
      cleaned_s += parts[i] + "%%";
    cleaned_s += parts[parts.size()-1];
    // stderr is redirected by apache to apache's error log
    fprintf(stderr, cleaned_s.c_str());
    fflush(stderr);
#endif
  };

  // taken from libopkele consumer_t code
  string canonicalize(const string& url) {
    string rv = url;
    // strip leading and trailing spaces
    string::size_type i = rv.find_first_not_of(" \t\r\n");
    if(i==string::npos)
      throw bad_input(OPKELE_CP_ "empty URL");
    if(i)
      rv.erase(0,i);
    i = rv.find_last_not_of(" \t\r\n");
    if(i==string::npos) 
      return url;
    if(i<(rv.length()-1))
      rv.erase(i+1);
    // add missing http://
    i = rv.find("://");
    if(i==string::npos) { // primitive. but do we need more?
      rv.insert(0,"http://");
      i = sizeof("http://")-1;
    }else{
      i += sizeof("://")-1;
    }
    string::size_type qm = rv.find('?',i);
    string::size_type sl = rv.find('/',i);
    if(qm!=string::npos) {
      if(sl==string::npos || sl>qm)
        rv.insert(qm,1,'/');
    }else{
      if(sl==string::npos)
        rv += '/';
    }
    return rv;
  }

  // assuming the url given will begin with http(s):// - worst case, return blank string
  string get_base_url(string url) {
    if(url.size() < 8)
      return "";
    if(url.find("http://",0) != string::npos || url.find("https://",0) != string::npos) {
      string::size_type last = url.find('/', 8);
      string::size_type last_q = url.find('?', 8);
      if(last==string::npos || (last_q<last && last_q!=string::npos))
	last = last_q;
      if(last != string::npos)
	return url.substr(0, last);
      return url;
    }
    return "";
  }

  params_t remove_openid_vars(params_t params) {
    map<string,string>::iterator iter;
    for(iter = params.begin(); iter != params.end(); iter++) {
      string param_key(iter->first);
      if(param_key.substr(0, 7) == "openid.") {
	params.erase(param_key);
	// stupid map iterator screws up if we just continue the iteration...
	// so recursion to the rescue - we'll delete them one at a time
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
      throw failed_conversion(OPKELE_CP_ "failed to curl_unescape()");
    string rv(t);
    curl_free(t);
    return rv;
  }

  params_t parse_query_string(const string& str) {
    params_t p;
    if(str.size() == 0) return p;

    vector<string> pairs = explode(str, "&");
    for(unsigned int i=0; i < pairs.size(); i++) {
      string::size_type loc = pairs[i].find( "=", 0 );
      // if loc found and loc isn't last char in string
      if( loc != string::npos && loc != str.size()-1) {
        string key = url_decode(pairs[i].substr(0, loc));
        string value = url_decode(pairs[i].substr(loc+1));
        p[key] = value;
      }
    }
    return p;
  }
}
