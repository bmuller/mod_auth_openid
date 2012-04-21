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

  enum error_result_t { no_idp_found, invalid_id, idp_not_trusted, invalid_nonce, canceled, unspecified, unauthorized, ax_bad_response };
  enum exec_result_t { id_accepted, fork_failed, child_no_return, id_refused };

  typedef struct session {
    string session_id;
    string hostname; // name of server (this is in case there are virtual hosts on this server)
    string path;
    string identity;
    string username; // optional - set by AuthOpenIDAXUsername
    int expires_on; // exact moment it expires
  } session_t;

  // Wrapper for basic_openid_message - just so it works with openid namespace
  class modauthopenid_message_t : public params_t {
  public:
    modauthopenid_message_t(params_t& _bom) { bom = _bom; };
    bool has_field(const string& n) const { return bom.has_param("openid."+n); };
    const string& get_field(const string& n) const { return bom.get_param("openid."+n); };
    bool has_ns(const string& uri) const { return bom.has_ns(uri); };
    string get_ns(const string& uri) const { return bom.get_ns(uri); };
    fields_iterator fields_begin() const { return bom.fields_begin(); };
    fields_iterator fields_end() const { return bom.fields_end(); };
    void reset_fields() { bom.reset_fields(); };
    void set_field(const string& n,const string& v) { bom.set_field(n, v); };
    void reset_field(const string& n) { bom.reset_field(n); };

  private:
    params_t bom;
  };

}


