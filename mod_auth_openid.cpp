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

extern "C" module AP_MODULE_DECLARE_DATA authopenid_module;

typedef struct {
  char *db_location;
  bool enabled;
  bool use_cookie;
  apr_array_header_t *trusted;
} modauthopenid_config;

typedef const char *(*CMD_HAND_TYPE) ();

static std::string url_encode(const std::string& str) {
  char * t = curl_escape(str.c_str(),str.length());
  if(!t)
    return std::string("");
  std::string rv(t);
  curl_free(t);
  return rv;
}

// get the base directory of the url
static void base_dir(std::string path, std::string& s) {
  // guaranteed that path will at least be "/" - but just to be safe...
  if(path.size() == 0)
    return;
  std::string::size_type q = path.find('?', 0);
  int i;
  if(q != std::string::npos)
    i = path.find_last_of('/', q);
  else
    i = path.find_last_of('/');
  s = path.substr(0, i+1);
}

// make a random alpha-numeric string size characters long
static void make_rstring(int size, std::string& s) {
  s = "";
  char *cs = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  srand((unsigned) time(0));
  for(int index=0; index<size; index++)
    s += cs[rand()%62];
}

static void *create_modauthopenid_config(apr_pool_t *p, char *s) {
  modauthopenid_config *newcfg;
  newcfg = (modauthopenid_config *) apr_pcalloc(p, sizeof(modauthopenid_config));
  newcfg->db_location = "/tmp/mod_auth_openid.db";
  newcfg->enabled = false;
  newcfg->use_cookie = true;
  newcfg->trusted = apr_array_make(p, 5, sizeof(char *));
  return (void *) newcfg;
}

static const char *set_modauthopenid_db_location(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->db_location = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_enabled(cmd_parms *parms, void *mconfig, int flag) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->enabled = (bool) flag;
  return NULL;
}

static const char *set_modauthopenid_usecookie(cmd_parms *parms, void *mconfig, int flag) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->use_cookie = (bool) flag;
  return NULL;
}

static const char *add_modauthopenid_trusted(cmd_parms *cmd, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  *(const char **)apr_array_push(s_cfg->trusted) = arg;
  return NULL;
}

  
static const command_rec mod_authopenid_cmds[] = {
  AP_INIT_TAKE1("AuthOpenIDDBLocation", (CMD_HAND_TYPE) set_modauthopenid_db_location, NULL, ACCESS_CONF,
		"AuthOpenIDDBLocation <string>"),
  AP_INIT_FLAG("AuthOpenIDEnabled", (CMD_HAND_TYPE) set_modauthopenid_enabled, NULL, ACCESS_CONF,
	       "AuthOpenIDEnabled <On | Off>"),
  AP_INIT_FLAG("AuthOpenIDUseCookie", (CMD_HAND_TYPE) set_modauthopenid_usecookie, NULL, ACCESS_CONF,
	       "AuthOpenIDUseCookie <On | Off> - use session auth?"),
  AP_INIT_ITERATE("AuthOpenIDTrusted", (CMD_HAND_TYPE) add_modauthopenid_trusted, NULL, ACCESS_CONF,
		  "AuthOpenIDTrusted <a list of trusted identity providers>"),
  {NULL}
};

static int http_sendstring(request_rec *r, std::string s) {
  // no idea why the following line only sometimes worked.....
  //apr_table_setn(r->headers_out, "Content-Type", "text/html");
  ap_set_content_type(r, "text/html");
  const char *c_s = s.c_str();
  conn_rec *c = r->connection;
  apr_bucket *b;
  apr_bucket_brigade *bb = apr_brigade_create(r->pool, c->bucket_alloc);
  b = apr_bucket_transient_create(c_s, strlen(c_s), c->bucket_alloc);
  APR_BRIGADE_INSERT_TAIL(bb, b);
  b = apr_bucket_eos_create(c->bucket_alloc);
  APR_BRIGADE_INSERT_TAIL(bb, b);
  
  if (ap_pass_brigade(r->output_filters, bb) != APR_SUCCESS)
    return HTTP_INTERNAL_SERVER_ERROR;
  return OK;
}

static int http_redirect(request_rec *r, std::string location) {
  apr_table_setn(r->headers_out, "Location", location.c_str());
  apr_table_setn(r->headers_out, "Cache-Control", "no-cache");
  modauthopenid::debug("redirecting client to: " + location);
  return HTTP_MOVED_TEMPORARILY;
}

static void full_uri(request_rec *r, std::string& result) {
  std::string hostname(r->hostname);
  std::string protocol(r->protocol);
  std::string uri(r->uri);
  apr_port_t i_port = ap_get_server_port(r);
  std::string prefix = (i_port == 443) ? "https://" : "http://";
  char *port = apr_psprintf(r->pool, "%lu", (unsigned long) i_port);
  std::string s_port = (i_port == 80 || i_port == 443) ? "" : ":" + std::string(port);
  std::string args = (r->args == NULL) ? "" : "?" + std::string(r->args);
  result = prefix + hostname + s_port + uri + args;
}

static void strip(std::string& s) {
  while(!s.empty() && s.substr(0,1) == " ") s.erase(0,1);
  while(!s.empty() && s.substr(s.size()-1, 1) == " ") s.erase(s.size()-1,1);
}

static void get_session_id(request_rec *r, std::string& session_id) {
  const char * cookies_c = apr_table_get(r->headers_in, "Cookie");
  if(cookies_c == NULL) 
    return;
  std::string cookies(cookies_c);
  std::vector<std::string> pairs = opkele::explode(cookies, ";");
  for(std::string::size_type i = 0; i < pairs.size(); i++) {
    std::vector<std::string> pair = opkele::explode(pairs[i], "=");
    if(pair.size() == 2) {
      std::string key = pair[0];
      strip(key);
      std::string value = pair[1];
      strip(value);
      modauthopenid::debug("cookie sent by client: \""+key+"\"=\""+value+"\"");
      if(key == "open_id_session_id") {
	session_id = pair[1];
	return;
      }
    }
  }
}

static int show_input(request_rec *r, std::string msg) {
  std::string result = "<html><head><script type=\"text/javascript\">function s() { ";
  if(msg != "")
    result+="alert(\"" + msg + "\");";
  result += " var qstring = ''; var parts = window.location.search.substring(1, window.location.search.length).split('&');";
  result += " if(parts.length>1) for(var i=0; i<parts.length; i++) if(parts[i].split('=')[0].substr(0,7)!='openid.') qstring+='&'+parts[i];";
  result += " var p = prompt(\"Enter your identity url.\"); if(!p) { document.getElementById(\"msg\").innerHTML=";
  result += "\"Authentication required!\"; return;} document.getElementById(\"msg\").innerHTML=\"Loading...\";";
  result += " window.location='?openid.identity='+p+qstring; }</script><body onload=\"s();\">";
  result += " <h1><div id=\"msg\"></div></h1></body></html>";
  return http_sendstring(r, result);
}

static bool is_trusted_provider(modauthopenid_config *s_cfg, std::string url) {
  if(apr_is_empty_array(s_cfg->trusted))
    return true;
  char **trusted_sites = (char **) s_cfg->trusted->elts;
  std::string trusted_site;
  std::string base_url = opkele::get_base_url(url);
  for (int i = 0; i < s_cfg->trusted->nelts; i++) {
    trusted_site = std::string(trusted_sites[i]);
    if(base_url == trusted_site) {
      modauthopenid::debug(trusted_site + " is a trusted identity provider");
      return true;
    }
  }
  modauthopenid::debug(base_url + " is not a trusted identity provider");
  return false;
}

static int mod_authopenid_method_handler (request_rec *r) {
  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(r->per_dir_config, &authopenid_module);

  // if we're not enabled for this location/dir, decline doing anything
  if(!s_cfg->enabled) 
    return DECLINED;
  
  // make a record of our being called
  modauthopenid::debug("***" + std::string(PACKAGE_STRING) + " module has been called***");

  // test for valid session - if so, return DECLINED
  std::string session_id = "";
  get_session_id(r, session_id);
  if(session_id != "" && s_cfg->use_cookie) {
    modauthopenid::debug("found session_id in cookie: " + session_id);
    modauthopenid::SESSION session;
    modauthopenid::SessionManager *sm = new modauthopenid::SessionManager(std::string(s_cfg->db_location));
    sm->get_session(session_id, session);
    delete sm;

    // if session found 
    if(std::string(session.identity) != "") {
      std::string uri_path;
      base_dir(std::string(r->uri), uri_path);
      std::string valid_path(session.path);
      // if found session has a valid path
      if(valid_path == uri_path.substr(0, valid_path.size()) && apr_strnatcmp(session.hostname, r->hostname)==0) {
	apr_table_setn(r->subprocess_env, "REMOTE_USER", session.identity);
	return DECLINED;
      } else {
	modauthopenid::debug("session found for different path or hostname");
      }
    }
  }

  // parse the get params
  opkele::params_t params;
  if(r->args != NULL) params = opkele::parse_query_string(std::string(r->args));
  std::string identity = (params.has_param("openid.identity")) ? params.get_param("openid.identity") : "unknown";

  // if user is posting id (only openid.identity will contain a value)
  if(params.has_param("openid.identity") && !params.has_param("openid.assoc_handle")) {
    // remove all openid GET query params (openid.*) - we don't want that maintained through
    // the redirection process.  We do, however, want to keep all aother GET params.
    // also, add a nonce for security
    params = opkele::remove_openid_vars(params);
    modauthopenid::NonceManager *nm = new modauthopenid::NonceManager(std::string(s_cfg->db_location));
    std::string nonce;
    make_rstring(10, nonce);
    nm->add(nonce);
    delete nm;
    params["openid.nonce"] = nonce;
    //remove first char - ? to fit r->args standard
    std::string args = params.append_query("", "").substr(1); 
    apr_cpystrn(r->args, args.c_str(), 1024);
    if(!opkele::is_valid_url(identity))
      return show_input(r, "You must give a valid URL for your identity.");
    opkele::MoidConsumer *consumer = new opkele::MoidConsumer(std::string(s_cfg->db_location));     
    std::string return_to, trust_root, re_direct;
    full_uri(r, return_to);
    base_dir(return_to, trust_root);
    try {
      re_direct = consumer->checkid_setup(identity, return_to, trust_root);
    } catch (opkele::exception &e) {
      delete consumer;
      return show_input(r, "Could not open \\\""+identity+"\\\" or no identity found there.  Please check the URL.");
    }
    delete consumer;
    if(!is_trusted_provider(s_cfg , re_direct))
       return show_input(r, "The identity provider for \\\""+identity+"\\\" is not trusted by this site.");
    return http_redirect(r, re_direct);
  } else if(params.has_param("openid.assoc_handle")) { // user has been redirected, authenticate them and set cookie
    // make sure nonce is present
    if(!params.has_param("openid.nonce"))
      return show_input(r, "Error in authentication.  Nonce not found.");
    opkele::MoidConsumer *consumer = new opkele::MoidConsumer(std::string(s_cfg->db_location));
    try {
      consumer->id_res(params);
      delete consumer;

      // if no exception raised, check nonce
      modauthopenid::NonceManager *nm = new modauthopenid::NonceManager(std::string(s_cfg->db_location));
      if(!nm->is_valid(params.get_param("openid.nonce"))) {
	delete nm;
	return show_input(r, "Error in authentication.  Nonce invalid."); 
      }
      delete nm;

      if(s_cfg->use_cookie) {
	// now set auth cookie, if we're doing session based auth
	std::string session_id, hostname, path, cookie_value, redirect_location, args;
	make_rstring(32, session_id);
	base_dir(std::string(r->uri), path);
	cookie_value = "open_id_session_id=" + session_id + "; path=" + path;
	modauthopenid::debug("setting cookie: " + cookie_value);
	apr_table_setn(r->err_headers_out, "Set-Cookie", cookie_value.c_str());
	hostname = std::string(r->hostname);

	// save session values
	modauthopenid::SessionManager *sm = new modauthopenid::SessionManager(std::string(s_cfg->db_location));
	sm->store_session(session_id, hostname, path, identity);
	delete sm;

	params = opkele::remove_openid_vars(params);
	args = params.append_query("", "").substr(1);
	if(args.length() == 0)
	  r->args = NULL;
	else
	  apr_cpystrn(r->args, args.c_str(), 1024);
	full_uri(r, redirect_location);
	return http_redirect(r, redirect_location);
      }
      
      // if we're not setting cookie - don't redirect, just show page
      return DECLINED;

    } catch(opkele::exception &e) {
      std::string result = "Error in authentication: " + std::string(e.what());
      modauthopenid::debug(result);
      delete consumer;
      return show_input(r, result);
    }
  } else { //display an input form
    if(params.has_param("openid.mode") && params.get_param("openid.mode") == "cancel")
      return show_input(r, "Previous authentication attempt canceled.");
    return show_input(r, "");
  }
}

static void mod_authopenid_register_hooks (apr_pool_t *p) {
  ap_hook_handler(mod_authopenid_method_handler, NULL, NULL, APR_HOOK_FIRST);
}

//module AP_MODULE_DECLARE_DATA 
module AP_MODULE_DECLARE_DATA authopenid_module = {
	STANDARD20_MODULE_STUFF,
	create_modauthopenid_config,
	NULL, // config merge function - default is to override
	NULL,
	NULL,
	mod_authopenid_cmds,
	mod_authopenid_register_hooks,
};
