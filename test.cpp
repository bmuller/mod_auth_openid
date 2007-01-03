#include <opkele/consumer.h>
#include <opkele/association.h>
#include <stdio.h>
#include <string>
#include "moid.h"

namespace opkele {
  using namespace std;
  using namespace modauthopenid;

void test() {
  string db_location = "/home/bmuller/mod_auth_openid/mod_auth_openid/mod_auth_openid.db";
  //SessionManager *s = new SessionManager(db_location);
  //s->store_session("one", "path", "identity", "server");
  //SESSION sess;
  //s->get_session("one", sess);
  //puts(sess.path);
  //delete s;
  //return;
  //std::string s = opkele::consumer_t::canonicalize("yourmom.com");
  //puts(s.c_str());
  //opkele::association a;

  NonceManager *nm = new NonceManager(db_location);
  nm->add("a nonce");
  if(nm->is_valid("a nonce"))
    puts("valid!");
  delete nm;
  return;
  MoidConsumer *consumer = new MoidConsumer(db_location);
  const string id = "http://identity.musc.edu/users/mullerb";
  const string return_to = "http://itchy.lambastard.com/openid/final";
  const string trust_root = "http://itchy.lambastard.com";
  string r;
  try {
    r = consumer->checkid_setup(id, return_to, trust_root);
  } catch (exception &e) {
    cout << "Error!";
    cout << e.what();
  }
  cout << "Successful....";
  printf("Output: \"%s\"\n", r.c_str());
  return;
  string resp = "openid.assoc_handle=%7BHMAC-SHA1%7D%7B459a977a%7D%7BBC2rwg%3D%3D%7D&openid.identity=http%3A%2F%2Fidentity.musc.edu%2Fusers%2Fmullerb&openid.mode=id_res&openid.return_to=http%3A%2F%2Fitchy.lambastard.com%2Fopenid%2Ffinal&openid.sig=h6xu9vukxPxg%2Byj16Audyz2pLPo%3D&openid.signed=mode%2Cidentity%2Creturn_to";
  params_t p = parse_query_string(resp);
  try {
    consumer->id_res(p, id);
    cout << "You should come home and meet my sister.\n";
  } catch(exception &e) {
    cout << "UNAUTHORIZED!!!!!\n";
    cout << e.what();
  }


  //string r = consumer->checkid_(mode, id, return_to, trust_root);
  //puts(r.c_str());
  //opkele::assoc_t a = consumer->associate("http://identity.musc.edu/index.php");
  //puts(a->server().c_str());
  //puts(a->handle().c_str());
  //int m = consumer->yourmom();
  //if(m==69) puts("yes, 69");

  delete consumer;
}}

int main() { opkele::test(); return 0; }
