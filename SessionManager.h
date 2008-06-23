/*
Copyright (C) 2007 Butterfat, LLC (http://butterfat.net)

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

  class SessionManager {
  public:
    SessionManager(const string& storage_location);
    ~SessionManager() { close(); };
    void get_session(const string& session_id, session_t& session);
    void store_session(const string& session_id, const string& hostname, const string& path, const string& identity);
    void print_table();
    void close();
  private:
    sqlite3 *db;
    void ween_expired();
    bool is_closed;
    bool test_result(int result, const string& context);
  };
}


