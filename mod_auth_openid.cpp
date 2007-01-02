/*
	Copyright 2002 Kevin O'Donnell

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * Include the core server components.
 */
#include "httpd.h"
#include "http_config.h"
#include "apr_strings.h"

#include "http_protocol.h"
#include "http_main.h"
#include "util_script.h"
#include "ap_config.h"
#include "http_log.h"

#include "moid.h"

extern "C" module AP_MODULE_DECLARE_DATA authopenid_module;

typedef struct {
  char *listen_location;
  char *db_location;
  bool enabled;
} modauthopenid_config;

typedef const char *(*CMD_HAND_TYPE) ();

//static void *create_modauthopenid_config(apr_pool_t *p, server_rec *s) {
static void *create_modauthopenid_config(apr_pool_t *p, char *s) {
  modauthopenid_config *newcfg;
  newcfg = (modauthopenid_config *) apr_pcalloc(p, sizeof(modauthopenid_config));
  newcfg->listen_location = "/openid";
  newcfg->db_location = "/tmp/mod_auth_openid.db";
  newcfg->enabled = false;
  return (void *) newcfg;
}

static void *modauthopenid_config_merge(apr_pool_t *p, void *basev, void *overridesv) {
  return overridesv;
}

static const char *set_modauthopenid_listen_location(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  //s_cfg = (modauthopenid_config *) ap_get_module_config(parms->server->module_config, &authopenid_module);
  s_cfg->listen_location = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_db_location(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  //s_cfg = (modauthopenid_config *) ap_get_module_config(parms->server->module_config, &authopenid_module);
  s_cfg->db_location = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_enabled(cmd_parms *parms, void *mconfig, int flag) {
  modauthopenid_config *s_cfg = (modauthopenid_config *) mconfig;
  //s_cfg = (modauthopenid_config *) ap_get_module_config(parms->server->module_config, &authopenid_module);
  s_cfg->enabled = (bool) flag;
  return NULL;
}

  
static const command_rec mod_authopenid_cmds[] = {
  AP_INIT_TAKE1("AuthOpenIDListenLocation", (CMD_HAND_TYPE) set_modauthopenid_listen_location, 
		NULL, ACCESS_CONF, "AuthOpenIDListenLocation <string>."),
  AP_INIT_TAKE1("AuthOpenIDDBLocation", (CMD_HAND_TYPE) set_modauthopenid_db_location, NULL, ACCESS_CONF,
		"AuthOpenIDDBLocation <string>"),
  AP_INIT_FLAG("AuthOpenIDEnabled", (CMD_HAND_TYPE) set_modauthopenid_enabled, NULL, ACCESS_CONF,
	       "AuthOpenIDEnabled <On | Off>"),
  {NULL}
};

static int http_sendstring(request_rec *r, std::string s) {
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
  return HTTP_MOVED_TEMPORARILY;
}

static int mod_authopenid_method_handler (request_rec *r) {

  // header for cookie Set-Cookie: _session_id=5d3ef8ed0dd06a083bd4bf5ed27d6388; path=/

  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(r->per_dir_config, &authopenid_module);

  // if we're not enabled for this location/dir, decline doing anything
  if(!s_cfg->enabled) return DECLINED;

  opkele::params_t params;
  if(r->args != NULL) params = opkele::parse_query_string(std::string(r->args));


  // if user is posting id, or requesting input box to put in id
  if(strcmp(r->uri, s_cfg->listen_location) == 0) {
    if(params.has_param("openid_identity")) {
      opkele::MoidConsumer *consumer = new opkele::MoidConsumer(std::string(s_cfg->db_location));     
      opkele::mode_t mode = opkele::mode_checkid_setup;
      const std::string return_to(r->uri);
      const std::string trust_root("http://itchy.lambastard.com");
      std::string re_direct = consumer->checkid_(mode, params.get_param("openid_identity"), return_to, trust_root);
      return http_redirect(r, re_direct);
      delete consumer;
    } else { //display an input form
      return http_sendstring(r, "hello there!");
    }
  }

  // otherwise....
  // if this is a callback attempt to athenticate, test credentials
  if(params.has_param("openid.method")) {
    /*
    opkele::MoidConsumer *consumer = new opkele::MoidConsumer(std::string(s_cfg->db_location));
    //apr_table_t *env = r->subprocess_env; 
    try {
      //consumer->id_res(params);
      // apr_table_setn(env, "OPENID_IDENTITY", params['openid.identity']);
    } catch(exception &e) {
      // unauthorized
      //apr_table_setn(env, "OPENID_IDENTITY", 'failed');
    }
    delete consumer;
    */
  }
  return DECLINED;
}
 
static void mod_authopenid_register_hooks (apr_pool_t *p) {
  ap_hook_handler(mod_authopenid_method_handler, NULL, NULL, APR_HOOK_LAST);
}

//module AP_MODULE_DECLARE_DATA 
module AP_MODULE_DECLARE_DATA authopenid_module = {
	STANDARD20_MODULE_STUFF,
	create_modauthopenid_config,
	modauthopenid_config_merge,
	NULL,
	NULL,
	mod_authopenid_cmds,
	mod_authopenid_register_hooks,
};
