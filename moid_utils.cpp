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

  // get a descriptive string for an error; a short string is used as a GET param
  // value in the style of OpenID get params - short, no space, ...
  string error_to_string(error_result_t e, bool use_short_string) {
    string short_string, long_string;
    switch(e) {
    case no_idp_found:
      short_string = "no_idp_found";
      long_string = "There was either no identity provider found at the identity URL given"
	" or there was trouble connecting to it.";
      break;
    case invalid_id_url:
      short_string = "invalid_id_url";
      long_string = "The identity URL given is not a valid URL.";
      break;
    case idp_not_trusted:
      short_string = "idp_not_trusted";
      long_string = "The identity provider for the identity URL given is not trusted.";
      break;
    case invalid_nonce:
      short_string = "invalid_nonce";
      long_string = "Invalid nonce; error while authenticating.";
      break;
    case canceled:
      short_string = "canceled";
      long_string = "Identification process has been canceled.";
      break;
    default: // unspecified
      short_string = "unspecified";
      long_string = "There has been an error while attempting to authenticate.";
      break;
    }
    return (use_short_string) ? short_string : long_string;
  }

  // assuming the url given will begin with http(s):// - worst case, return blank string
  string get_queryless_url(string url) {
    if(url.size() < 8)
      return "";
    if(url.find("http://",0) != string::npos || url.find("https://",0) != string::npos) {
      string::size_type last = url.find('?', 8);
      if(last != string::npos)
	return url.substr(0, last);
      return url;
    }
    return "";
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
    // taken from http://www.osix.net/modules/article/?id=586
    string regex = "^(https?://)"
      "(([0-9]{1,3}\\.){3}[0-9]{1,3}" // IP- 199.194.52.184
      "|" // allows either IP or domain
      "([0-9a-z_!~*'()-]+\\.)*" // tertiary domain(s)- www.
      "([0-9a-z][0-9a-z-]{0,61})?[0-9a-z]\\." // second level domain
      "[a-z]{2,6})" // first level domain- .com or .museum
      "(:[0-9]{1,4})?" // port number- :80
      "((/?)|" // a slash isn't required if there is no file name
      "(/[0-9a-z_!~*'().;?:@&=+$,%#-]+)+/?)$";
    pcrepp::Pcre reg(regex);
    return reg.search(url);
  }

  // This isn't a true html_escape function, but rather escapes just enough to get by for
  // quoted values - <blah name="stuff to be escaped">
  string html_escape(string s) {
    s = str_replace("\"", "&quot;", s);
    s = str_replace("<", "&lt;", s);
    s = str_replace(">", "&gt;", s);
    return s;
  }

  string str_replace(string needle, string replacement, string haystack) {
    vector<string> v = explode(haystack, needle);
    string r = "";
    for(vector<string>::size_type i=0; i < v.size()-1; i++)
      r += v[i] + replacement;
    r += v[v.size()-1];
    return r;
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
