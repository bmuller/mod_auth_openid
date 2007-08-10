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



