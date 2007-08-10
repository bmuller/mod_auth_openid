/*
Copyright (C) 2007 Butterfat, LLC (http://butterfat.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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
