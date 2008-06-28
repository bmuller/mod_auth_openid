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


namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  // Class to handle tasks of consumer - authentication session is based on a nonce id that is thrown in with
  // the params list
  class MoidConsumer : public prequeue_RP {
  public:
    // storage location is db location, _asnonceid is the association session nonce, and serverurl is 
    // the return to value (url initially requested by user)
    MoidConsumer(const string& storage_location, const string& _asnonceid, const string& _serverurl);
    virtual ~MoidConsumer() { close(); };

    // store a new assocation
    assoc_t store_assoc(const string& server,const string& handle,const string& type,const secret_t& secret,int expires_in);

    // retrieve an association
    assoc_t retrieve_assoc(const string& server,const string& handle);

    // invalidate assocation - deletes from db
    void invalidate_assoc(const string& server,const string& handle);

    // find any association for the given server OP
    assoc_t find_assoc(const string& server);

    // This is called with the openid.response_nonce - if isn't already in db, is stored for same amount of time
    // as the association, at which point a replay attack is impossible since assocation key is deleted
    void check_nonce(const string& OP,const string& nonce);

    // delete authentication session with the constructor param nonce if it exists
    void begin_queueing();

    // store the given endpoint - there actually is no queue - we just keep track of the first one
    // given - all subsequent calls are ignored
    void queue_endpoint(const openid_endpoint_t& ep);

    // get the endpoint set in queue_endpoint
    const openid_endpoint_t& get_endpoint() const;

    // same as begin_queuing
    void next_endpoint();

    // set the normalized id for this authentication session
    void set_normalized_id(const string& nid);

    // get id set in set_normalized_id
    const string get_normalized_id() const;

    // get the url that was given as a constructor parameter
    const string get_this_url() const;
    
    // check to see if a session exists with the nonce session id given in the constructor
    bool session_exists();

    // print all tables to stdout
    void print_tables();

    // close db
    void close();

    // delete session with given session nonce id in constructor param list
    void kill_session();
  private:
    sqlite3 *db;

    // delete all expired sessions
    void ween_expired();

    // test result from sqlite query - print error to stderr if there is one
    bool test_result(int result, const string& context);

    // strings for the nonce based authentication session and the server's url (the originally 
    // requested url)
    string asnonceid, serverurl; 

    // booleans for the database state and whether any endpoint has been set yet
    bool is_closed, endpoint_set;
    
    // The normalized id the user has attempted to use
    mutable string normalized_id;

    // the endpoint for the user's identity
    mutable openid_endpoint_t endpoint;
  };
}


