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
 
  bool NonceManager::is_valid(const string& nonce, bool delete_on_find) {
    return nm.is_valid(nonce, delete_on_find);
  };
  
  void NonceManager::delete_nonce(const string& nonce) {
    nm.delete_nonce(nonce);
  };

  void NonceManager::get_identity(const string& nonce, string& identity) {
    nm.get_identity(nonce, identity);
  };
  
  // The reason we need to store the identity for the case of delgation - we need
  // to keep track of the original identity used by the user.  The identity stored
  // with this nonce will be used later if auth succeeds to create the session
  // for the original identity.
  void NonceManager::add(const string& nonce, const string& identity) {
    nm.add(nonce, identity);
  };

  int NonceManager::num_records() {
    return nm.num_records();
  };

  void NonceManager::close() {
    nm.close();
  };
  
}
