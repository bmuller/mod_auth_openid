#include <opkele/consumer.h>
#include <opkele/association.h>
#include <stdio.h>
#include <string>
#include "moid.h"

namespace opkele {
  using namespace std;

void test() {
  //std::string s = opkele::consumer_t::canonicalize("yourmom.com");
  //puts(s.c_str());
  //opkele::association a;
  mode_t mode = mode_checkid_setup; // mode_associate; //mode_checkid_setup;
  MoidConsumer *consumer = new MoidConsumer("/home/bmuller/mod_auth_openid/opkele_test/test.bdb");
  const string id = "http://identity.musc.edu/users/mullerb";
  const string return_to = "http://itchy.lambastard.com/openid/final";
  const string trust_root = "http://itchy.lambastard.com";
  //string r = consumer->checkid_(mode, id, return_to, trust_root);
  //puts(r.c_str());

  string resp = "openid.assoc_handle=%7BHMAC-SHA1%7D%7B4598668e%7D%7BGFVArQ%3D%3D%7D&openid.identity=http%3A%2F%2Fidentity.musc.edu%2Fusers%2Fmullerb&openid.mode=id_res&openid.return_to=http%3A%2F%2Fitchy.lambastard.com%2Fopenid%2Ffinal&openid.sig=%2FOVZ%2FUT9I6BD0JBn5%2F5A50AE1F0%3D&openid.signed=mode%2Cidentity%2Creturn_to";
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
