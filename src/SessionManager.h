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
   * This class keeps track of cookie-based sessions.
   */
  class SessionManager {
  public:
    /**
     * Constructor also creates any missing tables.
     * @param _dbd A DBD connection usually provided by mod_dbd.
     */
    SessionManager(const ap_dbd_t* _dbd);

    /**
     * Get session with id session_id and set values in session.
     * If session doesn't exist in DB, session.identity will be set to an empty string.
     * Also deletes expired sessions.
     * @param now Optional time parameter for tests. If omitted, current time will be used.
     * @return true iff session was loaded successfully.
     */
    bool get_session(const string& session_id, session_t& session, time_t now = 0);

    /**
     * Store given session information in a new session row in the DB.
     * If lifespan is 0, let it expire in a day. Otherwise, expire in lifespan seconds.
     * Also deletes expired sessions.
     * @see http://trac.butterfat.net/public/mod_auth_openid/ticket/16
     * @param now Optional time parameter for tests. If omitted, current time will be used.
     * @return true iff session was stored successfully.
     */
    bool store_session(const string& session_id, const string& hostname, const string& path,
                       const string& identity, const string& username, int lifespan,
                       time_t now = 0);

    /**
     * Print session table to stdout.
     * This will only be called from command-line utilities, not the Apache module itself.
     */
    void print_table();

    /**
     * Append names and SQL for prepared statements used by this class to the provided array.
     * @param statements APR array of labeled_statement_t.
     */
    static void append_statements(apr_array_header_t* statements);

    /**
     * Delete all expired sessions.
     */
    void delete_expired(time_t now);
  private:
    const ap_dbd_t* dbd;

    /**
     * Get a prepared statement by name.
     */
    apr_dbd_prepared_t* get_prepared(const char* name);

    /**
     * Test DBD result code, and print error message to stderr for errors.
     * @param rc Result code from a DBD function.
     * @param context Extra info for error message.
     * @return true iff the operation was a success.
     */
    bool test_result(int rc, const string& context);
  };
}
