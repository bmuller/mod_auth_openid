#include <opkele/consumer.h>
#include <opkele/util.h>
#include <opkele/association.h>
#include <stdio.h>
#include <string>
#include "mod_auth_openid.h"
#include <opkele/data.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

namespace opkele {
  using namespace std;
  using namespace modauthopenid;

  string strip(string s) {
    if(s.find("assoc_handle") != string::npos) {
      for(int i=0; i<s.length(); i++) {
	int q = s.at(i);
	cout << "char: " << q << "\n";
      }
      return "assoc_handle";
    }
    while(!s.empty() && (s.substr(0,1) == " " || s.substr(0,1) == "\n")) s.erase(0,1);
    while(!s.empty() && (s.substr(s.size()-1, 1)==" " || s.substr(s.size()-1, 1) == "\n"))
      s.erase(s.size()-1,1);
    cout << "returning \"" << s << "\"\n";
    fflush(stdout);
    return s;
  }





  class curl_t {
  public:
    CURL *_c;

    curl_t() : _c(0) { }
    curl_t(CURL *c) : _c(c) { }
    ~curl_t() throw() { if(_c) curl_easy_cleanup(_c); }

    curl_t& operator=(CURL *c) { if(_c) curl_easy_cleanup(_c); _c=c; return *this; }

    operator const CURL*(void) const { return _c; }
    operator CURL*(void) { return _c; }
  };

  static CURLcode curl_misc_sets(CURL* c) {
    CURLcode r;
    (r=curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1))
      || (r=curl_easy_setopt(c,CURLOPT_MAXREDIRS,5))
      || (r=curl_easy_setopt(c,CURLOPT_DNS_CACHE_TIMEOUT,120))
      || (r=curl_easy_setopt(c,CURLOPT_DNS_USE_GLOBAL_CACHE,1))
      || (r=curl_easy_setopt(c,CURLOPT_USERAGENT,PACKAGE_NAME"/"PACKAGE_VERSION))
      || (r=curl_easy_setopt(c,CURLOPT_TIMEOUT,20))
      ;
    return r;
  }

  static size_t _curl_tostring(void *ptr,size_t size,size_t nmemb,void *stream) {
    string *str = (string*)stream;
    size_t bytes = size*nmemb;
    size_t get = min(16384-str->length(),bytes);
    str->append((const char*)ptr,get);
    return get;
  }

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
  /*
  NonceManager *nm = new NonceManager(db_location);
  nm->add("a nonce");
  if(nm->is_valid("a nonce"))
    puts("valid!");
  delete nm;
  return;
  */

  MoidConsumer *consumer = new MoidConsumer(db_location);
  const string id = "http://identity.musc.edu/users/mullerb";
  const string return_to = "http://itchy.lambastard.com/openid/final";
  const string trust_root = "http://itchy.lambastard.com";
  string r;
  assoc_t assoc;

  try{
    string server = "http://openid.claimid.com/server";
  util::dh_t dh = DH_new();
  if(!dh)
    throw exception_openssl(OPKELE_CP_ "failed to DH_new()");
  dh->p = util::dec_to_bignum(data::_default_p);
  dh->g = util::dec_to_bignum(data::_default_g);
  if(!DH_generate_key(dh))
    throw exception_openssl(OPKELE_CP_ "failed to DH_generate_key()");
  string request = 
        "openid.mode=associate"
        "&openid.assoc_type=HMAC-SHA1"
        "&openid.session_type=DH-SHA1"
    "&openid.dh_consumer_public=";
  request += util::url_encode(util::bignum_to_base64(dh->pub_key));
  curl_t curl = curl_easy_init();
  if(!curl)
    throw exception_curl(OPKELE_CP_ "failed to curl_easy_init()");
  string response;


  CURLcode r;
  (r=curl_misc_sets(curl))
    || (r=curl_easy_setopt(curl,CURLOPT_URL,server.c_str()))
    || (r=curl_easy_setopt(curl,CURLOPT_POST,1))
    || (r=curl_easy_setopt(curl,CURLOPT_POSTFIELDS,request.data()))
    || (r=curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,request.length()))
    || (r=curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,_curl_tostring))
    || (r=curl_easy_setopt(curl,CURLOPT_WRITEDATA,&response))
    ;
  if(r)
    throw exception_curl(OPKELE_CP_ "failed to curl_easy_setopt()",r);
  if(r=curl_easy_perform(curl))
    throw exception_curl(OPKELE_CP_ "failed to curl_easy_perform()",r);
  params_t p; p.parse_keyvalues(response);

  cout << "\n\n" << response << "\n\n";



  map<string,string>::iterator iter;
  params_t pt;
  for(iter = p.begin(); iter != p.end(); iter++) 
    pt[strip(iter->first)] = strip(iter->second);
  p = pt;
  for(iter = p.begin(); iter != p.end(); iter++) {
    cout << "\"" << iter->first << "\" = \"" << iter->second << "\"\n";
  }


  if(!p.has_param("assoc_handle")) { cout << "no assoc_handle"; }

  if(p.has_param("assoc_type") && p.get_param("assoc_type")!="HMAC-SHA1")
    throw bad_input(OPKELE_CP_ "unsupported assoc_type");
  string st;
  if(p.has_param("session_type")) st = p.get_param("session_type");
  if((!st.empty()) && st!="DH-SHA1")
    throw bad_input(OPKELE_CP_ "unsupported session_type");
  secret_t secret;
  if(st.empty()) {
    secret.from_base64(p.get_param("mac_key"));
  }else{
    util::bignum_t s_pub = util::base64_to_bignum(p.get_param("dh_server_public"));
    vector<unsigned char> ck(DH_size(dh));
    int cklen = DH_compute_key(&(ck.front()),s_pub,dh);
    if(cklen<0)
      throw exception_openssl(OPKELE_CP_ "failed to DH_compute_key()");
    ck.resize(cklen);
    // OpenID algorithm requires extra zero in case of set bit here
    if(ck[0]&0x80) ck.insert(ck.begin(),1,0);
    unsigned char key_sha1[SHA_DIGEST_LENGTH];
    SHA1(&(ck.front()),ck.size(),key_sha1);
    secret.enxor_from_base64(key_sha1,p.get_param("enc_mac_key"));
  }
  int expires_in = 0;
  if(p.has_param("expires_in")) {
    expires_in = util::string_to_long(p.get_param("expires_in"));
  }else if(p.has_param("issued") && p.has_param("expiry")) {
    expires_in = util::w3c_to_time(p.get_param("expiry"))-util::w3c_to_time(p.get_param("issued"));
  }else
    throw bad_input(OPKELE_CP_ "no expiration information");

  } catch (exception &e) {
    cout << "Error!";
    cout << e.what();
    return;
  }









  return;
  try {
    //consumer->checkid_setup(id, return_to, trust_root);
    r = consumer->associate("http://openid.claimid.com/server")->handle();
  } catch (exception &e) {
    cout << "Error!";
    cout << e.what();
    return;
  }
  cout << "Successful....";
  cout << r << "\n";
  delete consumer;
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
