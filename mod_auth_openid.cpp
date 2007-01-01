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

static void *create_modauthopenid_config(apr_pool_t *p, server_rec *s) {
  modauthopenid_config *newcfg;
  newcfg = (modauthopenid_config *) apr_pcalloc(p, sizeof(modauthopenid_config));
  newcfg->listen_location = "/openid";
  newcfg->db_location = "/tmp/mod_auth_openid.db";
  newcfg->enabled = false;
  return (void *) newcfg;
}

static const char *set_modauthopenid_listen_location(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(parms->server->module_config, &authopenid_module);
  s_cfg->listen_location = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_db_location(cmd_parms *parms, void *mconfig, const char *arg) {
  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(parms->server->module_config, &authopenid_module);
  s_cfg->db_location = (char *) arg;
  return NULL;
}

static const char *set_modauthopenid_enabled(cmd_parms *parms, void *mconfig, int flag) {
  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(parms->server->module_config, &authopenid_module);
  s_cfg->enabled = (bool) flag;
  return NULL;
}

  
static const command_rec mod_authopenid_cmds[] = {
  AP_INIT_TAKE1("AuthOpenIDListenLocation", (CMD_HAND_TYPE) set_modauthopenid_listen_location, 
		NULL, RSRC_CONF, "AuthOpenIDListenLocation <string>."),
  AP_INIT_TAKE1("AuthOpenIDDBLocation", (CMD_HAND_TYPE) set_modauthopenid_db_location, NULL, RSRC_CONF,
		"AuthOpenIDDBLocation <string>"),
  AP_INIT_FLAG("AuthOpenIDEnabled", (CMD_HAND_TYPE) set_modauthopenid_enabled, NULL, RSRC_CONF,
	       "AuthOpenIDEnabled <On | Off>"),
  {NULL}
};

static int mod_authopenid_method_handler (request_rec *r) {
  //opkele::MoidConsumer *consumer = new opkele::MoidConsumer("/tmp/thisisyourmom.db");
  //delete consumer;

  modauthopenid_config *s_cfg;
  s_cfg = (modauthopenid_config *) ap_get_module_config(r->server->module_config, &authopenid_module);
  std::string e = "no";
  if(s_cfg->enabled) e="yes";



  fprintf(stderr, r->uri);

  fflush(stderr);

  apr_table_t *env = r->subprocess_env; 
  apr_table_setn(env, "OPENID_IDENTITY", "on");
  if(apr_strnatcmp(r->uri, "/hello")!=0) return DECLINED;

  conn_rec *c = r->connection;
  apr_bucket *b;
  apr_bucket_brigade *bb = apr_brigade_create(r->pool, c->bucket_alloc);
  b = apr_bucket_transient_create("hello", 5, c->bucket_alloc);
  APR_BRIGADE_INSERT_TAIL(bb, b);
  b = apr_bucket_eos_create(c->bucket_alloc);
  APR_BRIGADE_INSERT_TAIL(bb, b);
  
  if (ap_pass_brigade(r->output_filters, bb) != APR_SUCCESS)
    return HTTP_INTERNAL_SERVER_ERROR;
  //return DECLINED;
  return OK;
}

static void mod_authopenid_register_hooks (apr_pool_t *p) {
  ap_hook_handler(mod_authopenid_method_handler, NULL, NULL, APR_HOOK_LAST);
}

//module AP_MODULE_DECLARE_DATA 
module AP_MODULE_DECLARE_DATA authopenid_module = {
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	create_modauthopenid_config,
	NULL,
	mod_authopenid_cmds,
	mod_authopenid_register_hooks,
};
