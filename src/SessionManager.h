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

  // This class keeps track of cookie based sessions
  class SessionManager {
  public:
    // storage_location is db location
    SessionManager(const string& storage_location);
    ~SessionManager() { close(); };

    // get session with id session_id and set values in session
    // if session doesn't exist, don't do anything
    void get_session(const string& session_id, session_t& session);

    // store given session information in a new session entry
    // if lifespan is 0, let it expire in a day.  otherwise, expire in "lifespan" seconds
    // See issue 16 - http://trac.butterfat.net/public/mod_auth_openid/ticket/16
    void store_session(const string& session_id, const string& hostname, const string& path, const string& identity, int lifespan);

    // print session table to stdout
    void print_table();
    
    // close database
    void close();
  private:
    sqlite3 *db;
    
    // delete all expired sessions
    void ween_expired();

    // db status
    bool is_closed;

    // test sqlite query result - print any errors to stderr
    bool test_result(int result, const string& context);
  };
}


