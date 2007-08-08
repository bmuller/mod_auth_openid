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

  void SessionManager::get_session(const string& session_id, SESSION& session) {
    sm.get_session(session_id, session);
  };

  void SessionManager::store_session(const string& session_id, const string& hostname, const string& path, const string& identity) {
    sm.store_session(session_id, hostname, path, identity);
  };

  int SessionManager::num_records() {
    return sm.num_records();
  };

  void SessionManager::close() {
    sm.close();
  };
}
