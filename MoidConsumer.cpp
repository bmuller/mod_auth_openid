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

#include "mod_auth_openid.h"

namespace modauthopenid {
  using namespace std;
  using namespace opkele;
 
  assoc_t MoidConsumer::store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in) {
    return mc.store_assoc(server, handle, secret, expires_in);
  };

  assoc_t MoidConsumer::retrieve_assoc(const string& server, const string& handle) {
    return mc.retrieve_assoc(server, handle);
  };

  void MoidConsumer::invalidate_assoc(const string& server,const string& handle) {
    mc.invalidate_assoc(server, handle);
  };

  assoc_t MoidConsumer::find_assoc(const string& server) {
    return mc.find_assoc(server);
  };

  string MoidConsumer::checkid_setup(const string& identity,const string& return_to,const string& trust_root,extension_t *ext) {
    return mc.checkid_setup(identity, return_to, trust_root, ext);
  }

  void MoidConsumer::id_res(const params_t& pin,const string& identity,extension_t *ext) {
    return mc.id_res(pin, identity, ext);
  }

  // This is a method to be used by a utility program, never the apache module
  int MoidConsumer::num_records() {
    return mc.num_records();
  };

  void MoidConsumer::close() {
    mc.close();
  };
}



