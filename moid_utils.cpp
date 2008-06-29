/*
Copyright (C) 2007-2008 Butterfat, LLC (http://butterfat.net)

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

#include "mod_auth_openid.h"

namespace modauthopenid {
  using namespace std;

  void debug(string s) {
#ifdef DEBUG
    print_to_error_log(s);
#endif
  };

  void print_to_error_log(string s) {
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
  };

  // get a descriptive string for an error; a short string is used as a GET param
  // value in the style of OpenID get params - short, no space, ...
  string error_to_string(error_result_t e, bool use_short_string) {
    string short_string, long_string;
    switch(e) {
    case no_idp_found:
      short_string = "no_idp_found";
      long_string = "There was either no identity provider found for the identity given"
	" or there was trouble connecting to it.";
      break;
    case invalid_id:
      short_string = "invalid_id";
      long_string = "The identity given is not a valid identity.";
      break;
    case idp_not_trusted:
      short_string = "idp_not_trusted";
      long_string = "The identity provider for the identity given is not trusted.";
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
  };

  string str_replace(string needle, string replacement, string haystack) {
    vector<string> v = explode(haystack, needle);
    string r = "";
    for(vector<string>::size_type i=0; i < v.size()-1; i++)
      r += v[i] + replacement;
    r += v[v.size()-1];
    return r;
  };

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
  };

  bool regex_match(string subject, string pattern) {
    const char * error;
    int erroffset;
    pcre * re = pcre_compile(pattern.c_str(), 0, &error, &erroffset, NULL);
    if (re == NULL) {
      print_to_error_log("regex compilation failed for regex \"" + pattern + "\": " + error);
      return false;
    }
    return (pcre_exec(re, NULL, subject.c_str(), subject.size(), 0, 0, NULL, 0) >= 0);
  };

  void strip(string& s) {
    while(!s.empty() && s.substr(0,1) == " ") s.erase(0,1);
    while(!s.empty() && s.substr(s.size()-1, 1) == " ") s.erase(s.size()-1,1);
  };

  // make a random alpha-numeric string size characters long
  void make_rstring(int size, string& s) {
    s = "";
    const char *cs = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand((unsigned) time(0));
    for(int index=0; index<size; index++)
      s += cs[rand()%62];
  }

  void print_sqlite_table(sqlite3 *db, string tablename) {
    fprintf(stdout, "Printing table: %s.  ", tablename.c_str());
    string sql = "SELECT * FROM " + tablename;
    int rc, nr, nc, size;
    char **table;
    rc = sqlite3_get_table(db, sql.c_str(), &table, &nr, &nc, 0);
    fprintf(stdout, "There are %d rows.\n", nr);    
    size = (nr * nc) + nc;
    for(int i=0; i<size; i++) {
      fprintf(stdout, "%s\t", table[i]);
      if(((i+1) % nc) == 0) 
	fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
    sqlite3_free_table(table);
  };

  bool test_sqlite_return(sqlite3 *db, int result, const string& context) {
    if(result != SQLITE_OK){
      string msg = "SQLite Error - " + context + ": %s\n";
      fprintf(stderr, msg.c_str(), sqlite3_errmsg(db));
      return false;
    }
    return true;
  };
}
