/*
Copyright (C) 2007-2010 Butterfat, LLC (http://butterfat.net)

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

#define APDEBUG(r, msg, ...) ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, msg, __VA_ARGS__);
#define APWARN(r, msg, ...) ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r, msg, __VA_ARGS__);
#define APERR(r, msg, ...) ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, msg, __VA_ARGS__);

extern "C" module AP_MODULE_DECLARE_DATA authopenid_module;

typedef struct {
  const char *db_location;
  char *trust_root;
  const char *cookie_name;
  char *login_page;
  bool use_cookie;
  apr_array_header_t *trusted;
  apr_array_header_t *distrusted;
  int cookie_lifespan;
  char *server_name;
  char *auth_program;
  char *cookie_path;
  bool use_auth_program;
} modauthopenid_config;

typedef const char *(*CMD_HAND_TYPE) ();

static void *create_modauthopenid_config(apr_pool_t *p, char *s) {
  modauthopenid_config *newcfg;
  newcfg = (modauthopenid_config *) apr_pcalloc(p, sizeof(modauthopenid_config));
  newcfg->db_location = "/tmp/mod_auth_openid.db";
  newcfg->use_cookie = true;
  newcfg->cookie_name = "open_id_session_id";
  newcfg->cookie_path = NULL; 
  newcfg->trusted = apr_array_make(p, 5, sizeof(char *));
  newcfg->distrusted = apr_array_make(p, 5, sizeof(char *));
  newcfg->trust_root = NULL;
  newcfg->cookie_lifespan = 0;
  newcfg->server_name = NULL;
  newcfg->auth_program = NULL;
  newcfg->use_auth_program = false;
  return (void *) newcfg;
}

static const char *set_modauthopenid_db_location(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->db_location = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_cookie_path(cmd_parms *parms, void *mconfig, const char *arg) { 
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig; 
  s_cfg->cookie_path = (char *) arg; 
  return NULL; 
} 

static const char *set_modauthopenid_trust_root(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->trust_root = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_login_page(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->login_page = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_cookie_name(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->cookie_name = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_usecookie(cmd_parms *parms, void *mconfig, int flag) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->use_cookie = (bool) flag;
  return NULL;
}

static const char *set_modauthopenid_cookie_lifespan(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->cookie_lifespan = atoi(arg);
  return NULL;
}

static const char *add_modauthopenid_trusted(cmd_parms *cmd, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  *(const char **)apr_array_push(s_cfg->trusted) = arg;
  return NULL;
}

static const char *add_modauthopenid_distrusted(cmd_parms *cmd, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  *(const char **)apr_array_push(s_cfg->distrusted) = arg;
  return NULL;
}

static const char *set_modauthopenid_server_name(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->server_name = (char *) arg;
  return NULL;
} 
 
static const char *set_modauthopenid_auth_program(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  s_cfg->auth_program = (char *) arg;
  s_cfg->use_auth_program = true;
  return NULL;
} 

static const command_rec mod_authopenid_cmds[] = {
  AP_INIT_TAKE1("AuthOpenIDCookieLifespan", (CMD_HAND_TYPE) set_modauthopenid_cookie_lifespan, NULL, OR_AUTHCFG,
		"AuthOpenIDCookieLifespan <number seconds>"),
  AP_INIT_TAKE1("AuthOpenIDDBLocation", (CMD_HAND_TYPE) set_modauthopenid_db_location, NULL, OR_AUTHCFG,
		"AuthOpenIDDBLocation <string>"),
  AP_INIT_TAKE1("AuthOpenIDLoginPage", (CMD_HAND_TYPE) set_modauthopenid_login_page, NULL, OR_AUTHCFG,
		"AuthOpenIDLoginPage <url string>"),
  AP_INIT_TAKE1("AuthOpenIDTrustRoot", (CMD_HAND_TYPE) set_modauthopenid_trust_root, NULL, OR_AUTHCFG,
		"AuthOpenIDTrustRoot <trust root to use>"),
  AP_INIT_TAKE1("AuthOpenIDCookieName", (CMD_HAND_TYPE) set_modauthopenid_cookie_name, NULL, OR_AUTHCFG,
		"AuthOpenIDCookieName <name of cookie to use>"),
  AP_INIT_TAKE1("AuthOpenIDCookiePath", (CMD_HAND_TYPE) set_modauthopenid_cookie_path, NULL, OR_AUTHCFG, 
		"AuthOpenIDCookiePath <path of cookie to use>"), 
  AP_INIT_FLAG("AuthOpenIDUseCookie", (CMD_HAND_TYPE) set_modauthopenid_usecookie, NULL, OR_AUTHCFG,
	       "AuthOpenIDUseCookie <On | Off> - use session auth?"),
  AP_INIT_ITERATE("AuthOpenIDTrusted", (CMD_HAND_TYPE) add_modauthopenid_trusted, NULL, OR_AUTHCFG,
		  "AuthOpenIDTrusted <a list of trusted identity providers>"),
  AP_INIT_ITERATE("AuthOpenIDDistrusted", (CMD_HAND_TYPE) add_modauthopenid_distrusted, NULL, OR_AUTHCFG,
		  "AuthOpenIDDistrusted <a blacklist list of identity providers>"),
  AP_INIT_TAKE1("AuthOpenIDServerName", (CMD_HAND_TYPE) set_modauthopenid_server_name, NULL, OR_AUTHCFG,
		"AuthOpenIDServerName <server name and port prefix>"),
  AP_INIT_TAKE1("AuthOpenIDUserProgram", (CMD_HAND_TYPE) set_modauthopenid_auth_program, NULL, OR_AUTHCFG,
		"AuthOpenIDUserProgram <full path to authentication program>"),
  {NULL}
};

// Get the full URI of the request_rec's request location 
// clean_params specifies whether or not all openid.* and modauthopenid.* params should be cleared
static void full_uri(request_rec *r, std::string& result, modauthopenid_config *s_cfg, bool clean_params=false) {
  std::string hostname(r->hostname);
  std::string uri(r->uri);
  apr_port_t i_port = ap_get_server_port(r);
  // Fetch the APR function for determining if we are looking at an https URL
  APR_OPTIONAL_FN_TYPE(ssl_is_https) *using_https = APR_RETRIEVE_OPTIONAL_FN(ssl_is_https);
  std::string prefix = (using_https != NULL && using_https(r->connection)) ? "https://" : "http://";
  char *port = apr_psprintf(r->pool, "%lu", (unsigned long) i_port);
  std::string s_port = (i_port == 80 || i_port == 443) ? "" : ":" + std::string(port);

  std::string args;
  if(clean_params) {
    opkele::params_t params;
    if(r->args != NULL) params = modauthopenid::parse_query_string(std::string(r->args));
    modauthopenid::remove_openid_vars(params);
    args = params.append_query("", "");
  } else {
    args = (r->args == NULL) ? "" : "?" + std::string(r->args);
  }

  if(s_cfg->server_name == NULL)
    result = prefix + hostname + s_port + uri + args;
  else
    result = std::string(s_cfg->server_name) + uri + args;
}

static int show_input(request_rec *r, modauthopenid_config *s_cfg, modauthopenid::error_result_t e) {
  if(s_cfg->login_page == NULL) {
    std::string msg = modauthopenid::error_to_string(e, false);
    if(e == modauthopenid::canceled)
      msg += "<br>Try to log out from thrid party website first";
    return modauthopenid::show_html_input(r, msg);
  }
  opkele::params_t params;
  if(r->args != NULL) 
    params = modauthopenid::parse_query_string(std::string(r->args));
  modauthopenid::remove_openid_vars(params);  

  std::string uri_location;
  full_uri(r, uri_location, s_cfg, true);
  params["modauthopenid.referrer"] = uri_location;

  params["modauthopenid.error"] = modauthopenid::error_to_string(e, true);
  return modauthopenid::http_redirect(r, params.append_query(s_cfg->login_page, ""));
}

static int show_input(request_rec *r, modauthopenid_config *s_cfg) {
  if(s_cfg->login_page == NULL) 
    return modauthopenid::show_html_input(r, "");
  opkele::params_t params;
  if(r->args != NULL) 
    params = modauthopenid::parse_query_string(std::string(r->args));
  modauthopenid::remove_openid_vars(params);
  std::string uri_location;
  full_uri(r, uri_location, s_cfg, true);
  params["modauthopenid.referrer"] = uri_location;
  return modauthopenid::http_redirect(r, params.append_query(s_cfg->login_page, ""));
}

static bool is_trusted_provider(modauthopenid_config *s_cfg, std::string url, request_rec *r) {
  if(apr_is_empty_array(s_cfg->trusted))
    return true;
  char **trusted_sites = (char **) s_cfg->trusted->elts;
  std::string base_url = modauthopenid::get_queryless_url(url);
  for (int i = 0; i < s_cfg->trusted->nelts; i++) {
    pcre * re = modauthopenid::make_regex(trusted_sites[i]);
    if(re == NULL) { 
      APERR(r, "regex compilation failed for regex: %s", trusted_sites[i]);
    } else if(modauthopenid::regex_match(base_url, re)) {
      modauthopenid::debug(base_url + " is a trusted identity provider");
      return true;
    }
  }
  APWARN(r, "%s is NOT a trusted identity provider", base_url.c_str());
  return false;
}

static bool is_distrusted_provider(modauthopenid_config *s_cfg, std::string url, request_rec *r) {
  if(apr_is_empty_array(s_cfg->distrusted))
    return false;
  char **distrusted_sites = (char **) s_cfg->distrusted->elts;
  std::string base_url = modauthopenid::get_queryless_url(url);
  for (int i = 0; i < s_cfg->distrusted->nelts; i++) {
    pcre * re = modauthopenid::make_regex(distrusted_sites[i]);
    if(re == NULL) {
      APERR(r, "regex compilation failed for regex: %s", distrusted_sites[i]);
    } else if(modauthopenid::regex_match(base_url, re)) {
      APWARN(r, "%s is a distrusted (on black list) identity provider", base_url.c_str());
      return true;
    }
  }
  APDEBUG(r, "%s is NOT a distrusted identity provider (not blacklisted)", base_url.c_str());
  return false;
};

static bool has_valid_session(request_rec *r, modauthopenid_config *s_cfg) {
  // test for valid session - if so, return DECLINED
  std::string session_id = "";
  modauthopenid::get_session_id(r, std::string(s_cfg->cookie_name), session_id);
  if(session_id != "" && s_cfg->use_cookie) {
    modauthopenid::debug("found session_id in cookie: " + session_id);
    modauthopenid::session_t session;
    modauthopenid::SessionManager sm(std::string(s_cfg->db_location));
    sm.get_session(session_id, session);
    sm.close();

    // if session found 
    if(std::string(session.identity) != "") {
      std::string uri_path;
      modauthopenid::base_dir(std::string(r->uri), uri_path);
      std::string valid_path(session.path);
      // if found session has a valid path
      if(valid_path == uri_path.substr(0, valid_path.size()) && apr_strnatcmp(session.hostname.c_str(), r->hostname)==0) {
	const char* idchar = std::string(session.identity).c_str();
	APDEBUG(r, "setting REMOTE_USER to \"%s\"", idchar);
	r->user = apr_pstrdup(r->pool, idchar);
	return true;
      } else {
	APDEBUG(r, "session found for different path or hostname (cooke was for %s)", session.hostname.c_str());
      }
    }
  }
  return false;
};


static int start_authentication_session(request_rec *r, modauthopenid_config *s_cfg, opkele::params_t& params, 
					std::string& return_to, std::string& trust_root) {
  // remove all openid GET query params (openid.*) - we don't want that maintained through
  // the redirection process.  We do, however, want to keep all aother GET params.
  // also, add a nonce for security 
  std::string identity = params.get_param("openid_identifier");
  // pull out the extension parameters before we get rid of openid.*
  opkele::params_t ext_params;
  modauthopenid::get_extension_params(ext_params, params);
  modauthopenid::remove_openid_vars(params);

  // add a nonce and reset what return_to is
  std::string nonce, re_direct;
  modauthopenid::make_rstring(10, nonce);
  modauthopenid::MoidConsumer consumer(std::string(s_cfg->db_location), nonce, return_to);    
  params["modauthopenid.nonce"] = nonce;
  full_uri(r, return_to, s_cfg, true);
  return_to = params.append_query(return_to, "");

  // get identity provider and redirect
  try {
    consumer.initiate(identity);
    opkele::openid_message_t cm; 
    re_direct = consumer.checkid_(cm, opkele::mode_checkid_setup, return_to, trust_root).append_query(consumer.get_endpoint().uri);
    re_direct = ext_params.append_query(re_direct, "");
  } catch (opkele::failed_xri_resolution &e) {
    consumer.close();
    return show_input(r, s_cfg, modauthopenid::invalid_id);
  } catch (opkele::failed_discovery &e) {
    consumer.close();
    return show_input(r, s_cfg, modauthopenid::invalid_id);
  } catch (opkele::bad_input &e) {
    consumer.close();
    return show_input(r, s_cfg, modauthopenid::invalid_id);
  } catch (opkele::exception &e) {
    consumer.close();
    APERR(r, "Error while fetching idP location: %s", e.what());
    return show_input(r, s_cfg, modauthopenid::no_idp_found);
  }
  consumer.close();
  if(!is_trusted_provider(s_cfg , re_direct, r) || is_distrusted_provider(s_cfg, re_direct, r))
    return show_input(r, s_cfg, modauthopenid::idp_not_trusted);
  return modauthopenid::http_redirect(r, re_direct);
};


static int set_session_cookie(request_rec *r, modauthopenid_config *s_cfg, opkele::params_t& params, std::string identity) {
  // now set auth cookie, if we're doing session based auth
  std::string session_id, hostname, path, cookie_value, redirect_location, args;
  if(s_cfg->cookie_path != NULL) 
    path = std::string(s_cfg->cookie_path); 
  else 
    modauthopenid::base_dir(std::string(r->uri), path); 
  modauthopenid::make_rstring(32, session_id);
  modauthopenid::make_cookie_value(cookie_value, std::string(s_cfg->cookie_name), session_id, path, s_cfg->cookie_lifespan); 
  APDEBUG(r, "Setting cookie after authentication of user %s", identity.c_str());
  apr_table_set(r->err_headers_out, "Set-Cookie", cookie_value.c_str());
  hostname = std::string(r->hostname);

  // save session values
  modauthopenid::SessionManager sm(std::string(s_cfg->db_location));
  sm.store_session(session_id, hostname, path, identity, s_cfg->cookie_lifespan);
  sm.close();

  opkele::params_t ext_params;
  modauthopenid::get_extension_params(ext_params, params);
  modauthopenid::remove_openid_vars(params);
  modauthopenid::merge_params(ext_params, params);
  args = params.append_query("", "").substr(1);
  if(args.length() == 0)
    r->args = NULL;
  else
    apr_cpystrn(r->args, args.c_str(), 1024);
  full_uri(r, redirect_location, s_cfg);
  return modauthopenid::http_redirect(r, redirect_location);
};

static int validate_authentication_session(request_rec *r, modauthopenid_config *s_cfg, opkele::params_t& params, std::string& return_to) {
  // make sure nonce is present
  if(!params.has_param("modauthopenid.nonce")) 
    return show_input(r, s_cfg, modauthopenid::invalid_nonce);

  modauthopenid::MoidConsumer consumer(std::string(s_cfg->db_location), params.get_param("modauthopenid.nonce"), return_to);
  try {
    consumer.id_res(modauthopenid::modauthopenid_message_t(params));
    
    // if no exception raised, check nonce
    if(!consumer.session_exists()) {
      consumer.close();
      return show_input(r, s_cfg, modauthopenid::invalid_nonce); 
    }

    // if we should be using a user specified auth program, run it to see if user is authorized
    if(s_cfg->use_auth_program) {
      std::string username = consumer.get_claimed_id();
      std::string progname = std::string(s_cfg->auth_program);
      modauthopenid::exec_result_t eresult = modauthopenid::exec_auth(progname, username);
      if(eresult != modauthopenid::id_accepted) {
	std::string error = modauthopenid::exec_error_to_string(eresult, progname, username);
	APERR(r, "Error in authentication: %s", error.c_str());	
	consumer.close();
	return show_input(r, s_cfg, modauthopenid::unauthorized);       
      } else {
	APDEBUG(r, "Authenticated %s using %s", username.c_str(), progname.c_str());	
      }
    }

    // Make sure that identity is set to the original one given by the user (in case of delegation
    // this will be different than openid_identifier GET param
    std::string identity = consumer.get_claimed_id();
    consumer.kill_session();
    consumer.close();

    if(s_cfg->use_cookie) 
      return set_session_cookie(r, s_cfg, params, identity);
      
    // if we're not setting cookie - don't redirect, just show page
    APERR(r, "Setting REMOTE_USER to %s", identity.c_str());
    r->user = apr_pstrdup(r->pool, identity.c_str());
    return DECLINED;
  } catch(opkele::exception &e) {
    APERR(r, "Error in authentication: %s", e.what());
    consumer.close();
    return show_input(r, s_cfg, modauthopenid::unspecified);
  }
};

static int mod_authopenid_method_handler(request_rec *r) {
  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(r->per_dir_config, &authopenid_module);

  // if we're not enabled for this location/dir, decline doing anything
  const char *current_auth = ap_auth_type(r);
  if (!current_auth || strcasecmp(current_auth, "openid"))
    return DECLINED;

  // make a record of our being called
  APDEBUG(r, "*** %s module has been called ***", PACKAGE_STRING);
  
  // if user has a valid session, they are authorized (OK)
  if(has_valid_session(r, s_cfg))
    return OK;

  // parse the get/post params
  opkele::params_t params;
  modauthopenid::get_request_params(r, params);

  // get our current url and trust root
  std::string return_to, trust_root;
  full_uri(r, return_to, s_cfg);
  if(s_cfg->trust_root == NULL)
    modauthopenid::base_dir(return_to, trust_root);
  else
    trust_root = std::string(s_cfg->trust_root);

  // if user is posting id (only openid_identifier will contain a value)
  if(params.has_param("openid_identifier") && !params.has_param("openid.assoc_handle")) {
    return start_authentication_session(r, s_cfg, params, return_to, trust_root);
  } else if(params.has_param("openid.assoc_handle")) { // user has been redirected, authenticate them and set cookie
    return validate_authentication_session(r, s_cfg, params, return_to);
  } else { //display an input form
    if(params.has_param("openid.mode") && params.get_param("openid.mode") == "cancel")
      return show_input(r, s_cfg, modauthopenid::canceled);
    return show_input(r, s_cfg);
  }
}

static int mod_authopenid_check_user_access(request_rec *r) {
  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(r->per_dir_config, &authopenid_module);
  char *user = r->user;
  int m = r->method_number;
  int required_user = 0;
  register int x;
  const char *t, *w;
  const apr_array_header_t *reqs_arr = ap_requires(r);
  require_line *reqs;

  if (!reqs_arr) 
    return DECLINED;
  
  reqs = (require_line *)reqs_arr->elts;
  for (x = 0; x < reqs_arr->nelts; x++) {
    if (!(reqs[x].method_mask & (AP_METHOD_BIT << m))) 
      continue;

    t = reqs[x].requirement;
    w = ap_getword_white(r->pool, &t);
    if (!strcasecmp(w, "valid-user"))
      return OK;

    if (!strcasecmp(w, "user")) {
      required_user = 1;
      while (t[0]) {
	w = ap_getword_conf(r->pool, &t);
	if (!strcmp(user, w))
	  return OK;
      }
    }
  }

  if (!required_user)
    return DECLINED;

  APERR(r, "Access to %s failed: user '%s' invalid", r->uri, user);
  ap_note_auth_failure(r);
  return HTTP_UNAUTHORIZED;
}

static void mod_authopenid_register_hooks (apr_pool_t *p) {
  ap_hook_check_user_id(mod_authopenid_method_handler, NULL, NULL, APR_HOOK_MIDDLE);
  ap_hook_auth_checker(mod_authopenid_check_user_access, NULL, NULL, APR_HOOK_MIDDLE);
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
