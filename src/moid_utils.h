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

namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  /**
   * Get the appropriate error string for the given OpenID error.
   * @return value in the style of OpenID get params - short, no space, ...
   */
  string error_to_string(error_result_t e, bool use_short_string);

  /**
   * Explode string s into parts, split based on occurrences of e.
   */
  vector<string> explode(string s, string e);

  /**
   * Replace needle with replacement in haystack.
   */
  string str_replace(string needle, string replacement, string haystack);

  /**
   * Write a debug message to the error log.
   * Should be using ap_log_error, but that would mean passing a server_rec* or request_rec* around.
   * I'm just assuming that if you're going to be debugging it shouldn't really matter, since
   * Apache redirects stderr to the error log anyway.
   */
  void debug(string file, int line, string function, string s);

  /**
   * @return true iff pattern found in subject.
   */
  bool regex_match(string subject, pcre * re);

  /**
   * Create a regular expression from the given pattern.
   */
  pcre * make_regex(string pattern);

  /**
   * Strip any spaces before or after actual string in s.
   */
  void strip(string& s);

  /**
   * Make a random string.
   * @param size Size of string.
   */
  void make_rstring(int size, string& s);

  /** 
   * Exec a program located at exec_location with a single parameter of username.
   * program should return a exec_result_t value.
   * @attention: if program hangs, so does Apache.
   */
  exec_result_t exec_auth(string exec_location, string username);

  /**
   * Convert a exec_result_t value to a meaningful error message.
   */
  string exec_error_to_string(exec_result_t e, string exec_location, string id);

  /** 
   * Generate a random integer.
   */
  int true_random();
}
