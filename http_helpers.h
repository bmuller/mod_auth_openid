/*
Copyright (C) 2007-2009 Butterfat, LLC (http://butterfat.net)

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

namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  // Send the string s as to the client (type text/html)
  int http_sendstring(request_rec *r, string s);

  //send Location header to given location
  int http_redirect(request_rec *r, string location);

  // show login page with given message string
  int show_html_input(request_rec *r, string msg);

  // get session id from cookie, if it exists, and put in session_id string - return if no cookie
  // with given name
  void get_session_id(request_rec *r, string cookie_name, string& session_id);

  // get the base location of the url (everything up to the last '/')
  void base_dir(string path, string& s);

  // return a url without the query string
  string get_queryless_url(string url);

  // remove all openid.*, modauthopenid.* parameters (except sreg and ax params)
  void remove_openid_vars(params_t& params);

  // html escape a string (used for putting get params into a page as hidden inputs)
  string html_escape(string s);

  // create a params_t object from a query string
  params_t parse_query_string(const string& str);

  // url decode a string
  string url_decode(const string& str);

  // create the cookie string that will be sent out in a header
  void make_cookie_value(string& cookie_value, const string& name, const string& session_id, const string& path, int cookie_lifespan);
}


