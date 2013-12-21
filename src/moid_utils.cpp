/*
Copyright (C) 2007-2010 Butterfat, LLC (http://butterfat.net)

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

  void debug(string file, int line, string function, string s) {
    char line_buf[21]; // big enough for any int + NUL
    sprintf(line_buf, "%d", line);
    string time_s = "";
    time_t rawtime = time(NULL);
    tm *tm_t = localtime(&rawtime);
    char rv[40];
    if(strftime(rv, sizeof(rv)-1, "%a %b %d %H:%M:%S %Y", tm_t)) 
      time_s = "[" + string(rv) + "] ";
    s = time_s + "[" + string(PACKAGE_NAME) + "] "
      + file + ":" + line_buf + ":" + function + ": "
      + s + "\n";
    // escape %'s
    string cleaned_s = "";
    vector<string> parts = explode(s, "%");
    for(unsigned int i=0; i<parts.size()-1; i++)
      cleaned_s += parts[i] + "%%";
    cleaned_s += parts[parts.size()-1];
    // stderr is redirected by apache to apache's error log
    fputs(cleaned_s.c_str(), stderr);
    fflush(stderr);
  }

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
    case unauthorized:
      short_string = "unauthorized";
      long_string = "User is not authorized to access this location.";
      break;
    case ax_bad_response:
      short_string = "ax_bad_response";
      long_string = "Error while reading user profile data.";
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
    if(v.size()) {
      for(vector<string>::size_type i=0; i < v.size()-1; i++)
        r += v[i] + replacement;
      r += v[v.size()-1];
    }
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

  pcre * make_regex(string pattern) {
    const char * error;
    int erroffset;
    return pcre_compile(pattern.c_str(), 0, &error, &erroffset, NULL);
  }

  bool regex_match(string subject, pcre * re) {
    return (pcre_exec(re, NULL, subject.c_str(), subject.size(), 0, 0, NULL, 0) >= 0);
  }

  void strip(string& s) {
    while(!s.empty() && s.substr(0,1) == " ") s.erase(0,1);
    while(!s.empty() && s.substr(s.size()-1, 1) == " ") s.erase(s.size()-1,1);
  }

  // make a random alpha-numeric string size characters long
  void make_rstring(int size, string& s) {
    s = "";
    const char *cs = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for(int index=0; index<size; index++)
      s += cs[true_random()%62];
  }

  void print_sql_table(const ap_dbd_t* dbd, string tablename) {
    printf("Printing table: %s.\n", tablename.c_str());
    string sql = "SELECT * FROM " + tablename;
    int rc, nr, nc;
    apr_dbd_results_t *results = NULL;
    rc = apr_dbd_select(dbd->driver, dbd->pool, dbd->handle, &results, sql.c_str(), DBD_LINEAR_ACCESS);
    if (rc != DBD_SUCCESS) {
      printf("Couldn't fetch contents. DBD error %d: %s\n\n",
             rc, apr_dbd_error(dbd->driver, dbd->handle, rc));
      return;
    }

    nc = apr_dbd_num_cols(dbd->driver, results);
    for (int col = 0; col < nc; col++) {
      const char *name = apr_dbd_get_name(dbd->driver, results, col);
      printf("%s\t", name);
    }
    printf("\n");

    apr_dbd_row_t *row = NULL;
    nr = 0; // apr_dbd_num_tuples() row count function doesn't work with "async"/linear row access mode
    while (apr_dbd_get_row(dbd->driver, dbd->pool, results, &row, DBD_NEXT_ROW) != DBD_NO_MORE_ROWS) {
      nr++;
      for (int col = 0; col < nc; col++) {
        const char *value = apr_dbd_get_entry(dbd->driver, row, col);
        printf("%s\t", value);
      }
      printf("\n");
    }
    
    printf("There are %d rows.\n\n", nr);
  }
  
  void consume_results(const ap_dbd_t* dbd, apr_dbd_results_t* results, apr_dbd_row_t** row) {
    while (apr_dbd_get_row(dbd->driver, dbd->pool, results, row, DBD_NEXT_ROW) != DBD_NO_MORE_ROWS) {
      // do nothing
    }
  }

  string exec_error_to_string(exec_result_t e, string exec_location, string id) {
    string error;
    switch(e) {
    case fork_failed:
      error = "Could not fork to exec program: " + exec_location + "when attempting to auth " + id;
      break;
    case child_no_return:
      error = "Problem waiting for child " + exec_location + " to return when authenticating " + id;
      break;
    case id_refused:
      error = id + " not authenticated by " + exec_location;
      break;
    default: // unspecified
      error = "Error while attempting to authenticate " + id + " using the program " + exec_location;
      break;
    }
    return error;
  }

  exec_result_t exec_auth(string exec_location, string username) {
    if(exec_location.size() > 255)
      exec_location.resize(255);
    if(username.size() > 255)
      username.resize(255);

    char *const argv[] = { (char *) exec_location.c_str(), (char *) username.c_str(), NULL };
    exec_result_t result = id_refused;
    int rvalue = 0;
    
    pid_t pid = fork();
    switch(pid) {
    case -1:
      // Fork failed
      result = fork_failed;
      break;
    case 0:
      // congrats, you're a kid
      execv(exec_location.c_str(), argv);
      // if we make it here, exec failed, exit from kid with rvalue 1
      exit(1);
    default:
      // you're an adult parent, act responsibly
      if(waitpid(pid, &rvalue, 0) == -1) 
        result = child_no_return;
      else
        result = (rvalue == 0) ? id_accepted : id_refused;
      break;
    }
    return result;
  }

  int true_random() {
#if APR_HAS_RANDOM
    unsigned char buf[2];
    if (apr_generate_random_bytes(buf, 2) == APR_SUCCESS)
      return (buf[0] << 8) | buf[1];
#endif
    apr_uint64_t time_now = apr_time_now();
    srand((unsigned int)(((time_now >> 32) ^ time_now) & 0xffffffff));
    return rand() & 0x0FFFF;
  }
}
