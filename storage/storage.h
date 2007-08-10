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

#ifdef SQLITE
#include <sqlite3.h>
#else
#include <db_cxx.h>
#endif

namespace modauthopenid {
  using namespace opkele;
  using namespace std;

  typedef struct session {
    char session_id[33];
    char hostname[255]; // name of server (this is in case there are virtual hosts on this server)
    char path[255];
    char identity[255];
    int expires_on; // exact moment it expires
  } SESSION;

  typedef struct nonce {
    int expires_on; // exact moment it expires
    char identity[255]; // identity nonce is good for
  } NONCE;

  typedef struct bdb_association {
    char server[255];
    char handle[100];
    char secret[30];
    int expires_on; // exact moment it expires
  } BDB_ASSOC;

#ifdef SQLITE  
  class SessionManagerSQLite {
  public:
    SessionManagerSQLite(const string& storage_location);
    ~SessionManagerSQLite() { close(); };
    void get_session(const string& session_id, SESSION& session);
    void store_session(const string& session_id, const string& hostname, const string& path, const string& identity);
    int num_records();
    void close();
  private:
    sqlite3 *db;
    void ween_expired();
    bool is_closed;
    bool test_result(int result, const string& context);
  };

  class NonceManagerSQLite {
  public:
    NonceManagerSQLite(const string& storage_location);
    ~NonceManagerSQLite() { close(); };
    bool is_valid(const string& nonce, bool delete_on_find);
    void add(const string& nonce, const string& identity);
    void delete_nonce(const string& nonce);
    void get_identity(const string& nonce, string& identity);
    int num_records();
    void close();
  private:
    sqlite3 *db;
    void ween_expired();
    bool test_result(int result, const string& context);
    bool is_closed;
  };

  class MoidConsumerSQLite : public opkele::consumer_t {
  public:
    MoidConsumerSQLite(const string& storage_location);
    virtual ~MoidConsumerSQLite() { close(); };
    assoc_t store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in);
    assoc_t retrieve_assoc(const string& server,const string& handle);
    void invalidate_assoc(const string& server,const string& handle);
    assoc_t find_assoc(const string& server);
    void print_db();
    int num_records();
    void close();
  private:
    sqlite3 *db;
    void ween_expired();
    bool test_result(int result, const string& context);
    bool is_closed;
  };

#else

  class SessionManagerBDB {
  public:
    SessionManagerBDB(const string& storage_location);
    ~SessionManagerBDB() { close(); };
    void get_session(const string& session_id, SESSION& session);
    void store_session(const string& session_id, const string& hostname, const string& path, const string& identity);
    int num_records();
    void close();
  private:
    Db db_;
    void ween_expired();
    bool is_closed;
  };  

  class NonceManagerBDB {
  public:
    NonceManagerBDB(const string& storage_location);
    ~NonceManagerBDB() { close(); };
    bool is_valid(const string& nonce, bool delete_on_find);
    void add(const string& nonce, const string& identity);
    void delete_nonce(const string& nonce);
    void get_identity(const string& nonce, string& identity);
    int num_records();
    void close();
  private:
    Db db_;
    void ween_expired();
    bool is_closed;
  };

  class MoidConsumerBDB : public opkele::consumer_t {
  public:
    MoidConsumerBDB(const string& storage_location);
    virtual ~MoidConsumerBDB() { close(); };
    assoc_t store_assoc(const string& server,const string& handle,const secret_t& secret,int expires_in);
    assoc_t retrieve_assoc(const string& server,const string& handle);
    void invalidate_assoc(const string& server,const string& handle);
    assoc_t find_assoc(const string& server);
    void print_db();
    int num_records();
    void close();
  private:
    Db db_;
    void ween_expired();
    bool is_closed;
  };
#endif
}

