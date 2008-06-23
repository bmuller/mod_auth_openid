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

  class MoidConsumer : public prequeue_RP {
  public:
    MoidConsumer(const string& storage_location, const string& _asnonceid, const string& _serverurl);
    virtual ~MoidConsumer() { close(); };
    assoc_t store_assoc(const string& server,const string& handle,const string& type,const secret_t& secret,int expires_in);
    assoc_t retrieve_assoc(const string& server,const string& handle);
    void invalidate_assoc(const string& server,const string& handle);
    assoc_t find_assoc(const string& server);
    void check_nonce(const string& OP,const string& nonce);
    void begin_queueing();
    void queue_endpoint(const openid_endpoint_t& ep);
    const openid_endpoint_t& get_endpoint() const;
    void next_endpoint();
    void set_normalized_id(const string& nid);
    const string get_normalized_id() const;
    const string get_this_url() const;
    bool session_exists();
    void print_db();
    int num_records();
    void close();
    void kill_session();
  private:
    sqlite3 *db;
    void ween_expired();
    bool test_result(int result, const string& context);
    bool is_closed, endpoint_set;
    string asnonceid, serverurl; 
    mutable string normalized_id;
    mutable openid_endpoint_t endpoint;
  };
}


