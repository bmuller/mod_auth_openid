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

  int http_sendstring(request_rec *r, string s, int success_rvalue) {
    // no idea why the following line only sometimes worked.....
    //apr_table_setn(r->headers_out, "Content-Type", "text/html");
    ap_set_content_type(r, "text/html");
    const char *c_s = s.c_str();
    conn_rec *c = r->connection;
    apr_bucket *b;
    apr_bucket_brigade *bb = apr_brigade_create(r->pool, c->bucket_alloc);
    b = apr_bucket_transient_create(c_s, strlen(c_s), c->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, b);
    b = apr_bucket_eos_create(c->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, b);

    if (ap_pass_brigade(r->output_filters, bb) != APR_SUCCESS)
      return HTTP_INTERNAL_SERVER_ERROR;
    return success_rvalue;
  };

  int send_form_post(request_rec *r, string location) {
    string::size_type last = location.find('?', 0);
    string url = (last != string::npos) ? location.substr(0, last) : location;

    params_t params;
    if(url.size() < location.size())
      params = parse_query_string(location.substr(url.size()+1));

    string inputs = "";
    map<string,string>::iterator iter;
    for(iter = params.begin(); iter != params.end(); iter++) {
      string key(iter->first);
      inputs += "<input type=\"hidden\" name=\"" + key + "\" value=\"" + params[key] + "\" />";
    }

    string result = 
      "<html><head><title>redirection</title></head><body onload=\"document.getElementById('form').submit();\">"
      "This page will automatically redirect you to your identity provider.  "
      "If you are not immediately redirected, click the submit button below."
      "<form id=\"form\" action=\"" + url + "\" method=\"post\">" + inputs + "<input type=\"submit\" value=\"submit\">"      
      "</form></body></html>";
      
    // return HTTP_UNAUTHORIZED so that no further modules can produce output
    return http_sendstring(r, result, HTTP_UNAUTHORIZED);
  };

  int http_redirect(request_rec *r, string location) {
    // Because IE is retarded, we have to do a form post if the URL is too big (over 2048 characters)
    if(location.size() > 2000) {
      debug("Redirecting via POST to: " + location);
      return send_form_post(r, location);
    } else {
      debug("Redirecting via HTTP_MOVED_TEMPORARILY to: " + location);
      apr_table_set(r->headers_out, "Location", location.c_str());
      apr_table_setn(r->err_headers_out, "Cache-Control", "no-cache");
      return HTTP_MOVED_TEMPORARILY;
    }
  };

  int show_html_input(request_rec *r, string msg) {
    opkele::params_t params;
    if(r->args != NULL)
      params = parse_query_string(string(r->args));
    string identity = params.has_param("openid_identifier") ? params.get_param("openid_identifier") : "";
    remove_openid_vars(params);
    map<string,string>::iterator iter;
    string args = "";
    string key, value;
    for(iter = params.begin(); iter != params.end(); iter++) {
      key = html_escape(iter->first);
      value = html_escape(iter->second);
      args += "<input type=\"hidden\" name=\"" + key + "\" value = \"" + value + "\" />";
    }
    string result =
    "<html><head><title>Protected Location</title><style type=\"text/css\">"
    "#msg { border: 1px solid #ff0000; background: #ffaaaa; font-weight: bold; padding: 10px; }\n"
    "a { text-decoration: none; }\n"
    "a:hover { text-decoration: underline; }\n"
    "#desc { border: 1px solid #000; background: #ccc; padding: 10px; }\n"
    "#sig { text-align: center; font-style: italic; margin-top: 50px; color: #777; font-size: .7em; }\n"
    "#sig a { color: #222; }\n"
    ".loginbox { background: url(http://www.openid.net/login-bg.gif) no-repeat; background-color: #fff; " // logo location is in 1.1 spec, should stay same
    " background-position: 0 50%; color: #000; padding-left: 18px; }\n"
    "form { margin: 15px; }\n"
    "</style></head><body>"
    "<h1>Protected Location</h1>"
    "<p id=\"desc\">This site is protected and requires that you identify yourself with an "
    "<a href=\"http://openid.net\">OpenID</a> url.  To find out how it works, see "
    "<a href=\"http://openid.net/what/\">http://openid.net/what/</a>.  You can sign up for "
    "an identity on one of the sites listed <a href=\"http://openid.net/get/\">here</a>.</p>"
      + (msg.empty()?"":"<div id=\"msg\">"+msg+"</div>") +
    "<form action=\"\" method=\"get\">"
    "<b>Identity URL:</b> <input type=\"text\" name=\"openid_identifier\" value=\""+identity+"\" size=\"30\" class=\"loginbox\" />"
    "<input type=\"submit\" value=\"Log In\" />" + args +
    "</form>"
    "<div id=\"sig\">protected by <a href=\"" + PACKAGE_URL + "\">" + PACKAGE_STRING + "</a></div>"
      "<body></html>";

    // return HTTP_UNAUTHORIZED so that no further modules can produce output
    return http_sendstring(r, result, HTTP_UNAUTHORIZED);
  };

  void get_session_id(request_rec *r, string cookie_name, string& session_id) {
    const char * cookies_c = apr_table_get(r->headers_in, "Cookie");
    if(cookies_c == NULL)
      return;
    string cookies(cookies_c);
    vector<string> pairs = explode(cookies, ";");
    for(string::size_type i = 0; i < pairs.size(); i++) {
      vector<string> pair = explode(pairs[i], "=");
      if(pair.size() == 2) {
        string key = pair[0];
        strip(key);
        string value = pair[1];
        strip(value);
        debug("cookie sent by client: \""+key+"\"=\""+value+"\"");
        if(key == cookie_name) {
          session_id = pair[1];
          return;
        }
      }
    }
  };

  // get the base directory of the url
  void base_dir(string path, string& s) {
    // guaranteed that path will at least be "/" - but just to be safe... 
    if(path.size() == 0)
      return;
    string::size_type q = path.find('?', 0);
    int i;
    if(q != string::npos)
      i = path.find_last_of('/', q);
    else
      i = path.find_last_of('/');
    s = path.substr(0, i+1);
  };

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
  };

  void remove_openid_vars(params_t& params) {
    map<string,string>::iterator iter, iter_next;
    for(iter = params.begin(); iter != params.end(); ) {
      iter_next = iter;
      ++iter_next;
      string param_key(iter->first);
      // if starts with openid. or modauthopenid. (for the nonce) or openid_identifier (the login) remove it
      if((param_key.substr(0, 7) == "openid." || param_key.substr(0, 14) == "modauthopenid." || param_key == "openid_identifier")) {
        params.erase(iter); // invalidates iter, but its successor iter_next is still valid
      }
      iter = iter_next;
    }
  };

  void get_extension_params(params_t& extparams, params_t& params) {
    map<string,string>::iterator iter;
    extparams.reset_fields();
    for(iter = params.begin(); iter != params.end(); iter++) {
      string param_key(iter->first);
      vector<string> parts = explode(param_key, ".");
      // if there is more than one "." in the param name then we're 
      // dealing with an extension parameter
      if(parts.size() > 2)
	    extparams[param_key] = params[param_key];
    }
  };

  // for each key/value in params_one, set params_two[key] = value
  void merge_params(params_t& params_one, params_t& params_two) {
    map<string,string>::iterator iter;
    for(iter = params_one.begin(); iter != params_one.end(); iter++) {
      string param_key(iter->first);
      params_two[param_key] = params_one[param_key];
    }
  };

  // This isn't a true html_escape function, but rather escapes just enough to get by for
  // quoted values - <blah name="stuff to be escaped">  
  string html_escape(string s) {
    s = str_replace("&", "&amp;", s);
    s = str_replace("'", "&#39;", s);
    s = str_replace("\"", "&quot;", s);
    s = str_replace("<", "&lt;", s);
    s = str_replace(">", "&gt;", s);
    return s;
  };

  string url_decode(const string& str) {
    // if +'s aren't replaced with %20's then curl won't unescape to spaces properly
    string url = str_replace("+", "%20", str);

    CURL *curl = curl_easy_init();
    if(!curl)
      throw failed_conversion(OPKELE_CP_ "failed to curl_easy_init()");

    char * t = curl_easy_unescape(curl, url.c_str(), url.length(), NULL);
    if(!t)
      throw failed_conversion(OPKELE_CP_ "failed to curl_unescape()");

    string rv(t);
    curl_free(t);
    curl_easy_cleanup(curl);
    return rv;
  };

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
  };

  void make_cookie_value(string& cookie_value, const string& name, const string& session_id, const string& path, int cookie_lifespan, bool secure_cookie) {
    cookie_value = name + "=" + session_id + "; path=" + path + "; HttpOnly";
    if(cookie_lifespan != 0) {
      time_t t;
      t = time(NULL) + cookie_lifespan;
      struct tm *tmp;
      tmp = gmtime(&t);
      char expires[200];
      strftime(expires, sizeof(expires), "%a, %d-%b-%Y %H:%M:%S GMT", tmp);
      cookie_value += "; expires=" + string(expires);
    }
    if (secure_cookie) {
      cookie_value += "; Secure";
    }
  };

  // Get the post query string from a HTTP POST
  bool get_post_data(request_rec *r, string& qs) {
    // check to make sure the right content type was used
    const char *type = apr_table_get(r->headers_in, "Content-Type");
    if (strcasecmp(type, DEFAULT_POST_ENCTYPE) != 0)
      return false;

    apr_bucket_brigade *bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);
    apr_status_t ret;
    int seen_eos, child_stopped_reading;
    seen_eos = child_stopped_reading = 0; 
    char *query_string = NULL;

    do { 
      ret = ap_get_brigade(r->input_filters, bb, AP_MODE_READBYTES, APR_BLOCK_READ, 8192); 
      if(ret != APR_SUCCESS)
	return false;

      apr_bucket *bucket; 
      for(bucket=APR_BRIGADE_FIRST(bb); bucket!=APR_BRIGADE_SENTINEL(bb); bucket=APR_BUCKET_NEXT(bucket)) { 
        apr_size_t len; 
        const char *data; 
        if(APR_BUCKET_IS_EOS(bucket)) { 
          seen_eos = 1; 
          break; 
        }
        if(APR_BUCKET_IS_FLUSH(bucket)) 
          continue;
        if(child_stopped_reading)
          continue; 
    
        ret = apr_bucket_read(bucket, &data, &len, APR_BLOCK_READ); 
        if(ret != APR_SUCCESS) {
          child_stopped_reading = 1;
        } else {
          if (query_string == NULL) 
            query_string = apr_pstrndup(r->pool, data, len);
          else 
            query_string = apr_pstrcat(r->pool, query_string, apr_pstrndup(r->pool, data, len), NULL);
        }
      } 
      apr_brigade_cleanup(bb); 
    } while (!seen_eos); 

    qs = (query_string == NULL) ? "" : string(query_string);
    return true; 
  };

  // Get request parameters - whether POST or GET
  void get_request_params(request_rec *r, params_t& params) {
    string query;
    if(r->method_number == M_GET && r->args != NULL) {
      debug("Request GET params: " + string(r->args));
      params = parse_query_string(string(r->args));
    } else if(r->method_number == M_POST && get_post_data(r, query)) {
      debug("Request POST params: " + query);
      params = parse_query_string(query);
      if (r->args != NULL) {
        merge_params(parse_query_string(string(r->args)), params);
      }
    }
  };
  
}
