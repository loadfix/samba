/* 
   Unix SMB/CIFS implementation.

   rootDSE ldb module

   Copyright (C) Andrew Tridgell 2005
   Copyright (C) Simo Sorce 2005-2008
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "includes.h"
#include "ldb_private.h"
#include "system/time.h"
#include "dsdb/samdb/samdb.h"
#include "version.h"

struct private_data {
	int num_controls;
	char **controls;
	int num_partitions;
	struct ldb_dn **partitions;
};

/*
  return 1 if a specific attribute has been requested
*/
static int do_attribute(const char * const *attrs, const char *name)
{
	return attrs == NULL ||
		ldb_attr_in_list(attrs, name) ||
		ldb_attr_in_list(attrs, "*");
}

static int do_attribute_explicit(const char * const *attrs, const char *name)
{
	return attrs != NULL && ldb_attr_in_list(attrs, name);
}


/*
  add dynamically generated attributes to rootDSE result
*/
static int rootdse_add_dynamic(struct ldb_module *module, struct ldb_message *msg, const char * const *attrs)
{
	struct ldb_context *ldb;
	struct private_data *priv = talloc_get_type(ldb_module_get_private(module), struct private_data);
	char **server_sasl;
	const struct dsdb_schema *schema;

	ldb = ldb_module_get_ctx(module);
	schema = dsdb_get_schema(ldb);

	msg->dn = ldb_dn_new(msg, ldb, NULL);

	/* don't return the distinduishedName, cn and name attributes */
	ldb_msg_remove_attr(msg, "distinguishedName");
	ldb_msg_remove_attr(msg, "cn");
	ldb_msg_remove_attr(msg, "name");

	if (do_attribute(attrs, "currentTime")) {
		if (ldb_msg_add_steal_string(msg, "currentTime", 
					     ldb_timestring(msg, time(NULL))) != 0) {
			goto failed;
		}
	}

	if (do_attribute(attrs, "supportedControl")) {
 		int i;
		for (i = 0; i < priv->num_controls; i++) {
			char *control = talloc_strdup(msg, priv->controls[i]);
			if (!control) {
				goto failed;
			}
			if (ldb_msg_add_steal_string(msg, "supportedControl",
						     control) != 0) {
				goto failed;
 			}
 		}
 	}

	if (do_attribute(attrs, "namingContexts")) {
		int i;
		for (i = 0; i < priv->num_partitions; i++) {
			struct ldb_dn *dn = priv->partitions[i];
			if (ldb_msg_add_steal_string(msg, "namingContexts",
						     ldb_dn_alloc_linearized(msg, dn)) != 0) {
				goto failed;
 			}
 		}
	}

	server_sasl = talloc_get_type(ldb_get_opaque(ldb, "supportedSASLMechanims"), 
				       char *);
	if (server_sasl && do_attribute(attrs, "supportedSASLMechanisms")) {
		int i;
		for (i = 0; server_sasl && server_sasl[i]; i++) {
			char *sasl_name = talloc_strdup(msg, server_sasl[i]);
			if (!sasl_name) {
				goto failed;
			}
			if (ldb_msg_add_steal_string(msg, "supportedSASLMechanisms",
						     sasl_name) != 0) {
				goto failed;
			}
		}
	}

	if (do_attribute(attrs, "highestCommittedUSN")) {
		uint64_t seq_num;
		int ret = ldb_sequence_number(ldb, LDB_SEQ_HIGHEST_SEQ, &seq_num);
		if (ret == LDB_SUCCESS) {
			if (ldb_msg_add_fmt(msg, "highestCommittedUSN", 
					    "%llu", (unsigned long long)seq_num) != 0) {
				goto failed;
			}
		}
	}

	if (schema && do_attribute_explicit(attrs, "dsSchemaAttrCount")) {
		struct dsdb_attribute *cur;
		uint32_t n = 0;

		for (cur = schema->attributes; cur; cur = cur->next) {
			n++;
		}

		if (ldb_msg_add_fmt(msg, "dsSchemaAttrCount", 
				    "%u", n) != 0) {
			goto failed;
		}
	}

	if (schema && do_attribute_explicit(attrs, "dsSchemaClassCount")) {
		struct dsdb_class *cur;
		uint32_t n = 0;

		for (cur = schema->classes; cur; cur = cur->next) {
			n++;
		}

		if (ldb_msg_add_fmt(msg, "dsSchemaClassCount", 
				    "%u", n) != 0) {
			goto failed;
		}
	}

	if (schema && do_attribute_explicit(attrs, "dsSchemaPrefixCount")) {
		if (ldb_msg_add_fmt(msg, "dsSchemaPrefixCount", 
				    "%u", schema->num_prefixes) != 0) {
			goto failed;
		}
	}

	if (do_attribute_explicit(attrs, "validFSMOs")) {
		const struct dsdb_naming_fsmo *naming_fsmo;
		const struct dsdb_pdc_fsmo *pdc_fsmo;
		const char *dn_str;

		if (schema && schema->fsmo.we_are_master) {
			dn_str = ldb_dn_get_linearized(samdb_schema_dn(ldb));
			if (dn_str && dn_str[0]) {
				if (ldb_msg_add_fmt(msg, "validFSMOs", "%s", dn_str) != 0) {
					goto failed;
				}
			}
		}

		naming_fsmo = talloc_get_type(ldb_get_opaque(ldb, "dsdb_naming_fsmo"),
					      struct dsdb_naming_fsmo);
		if (naming_fsmo && naming_fsmo->we_are_master) {
			dn_str = ldb_dn_get_linearized(samdb_partitions_dn(ldb, msg));
			if (dn_str && dn_str[0]) {
				if (ldb_msg_add_fmt(msg, "validFSMOs", "%s", dn_str) != 0) {
					goto failed;
				}
			}
		}

		pdc_fsmo = talloc_get_type(ldb_get_opaque(ldb, "dsdb_pdc_fsmo"),
					   struct dsdb_pdc_fsmo);
		if (pdc_fsmo && pdc_fsmo->we_are_master) {
			dn_str = ldb_dn_get_linearized(samdb_base_dn(ldb));
			if (dn_str && dn_str[0]) {
				if (ldb_msg_add_fmt(msg, "validFSMOs", "%s", dn_str) != 0) {
					goto failed;
				}
			}
		}
	}

	if (schema && do_attribute_explicit(attrs, "vendorVersion")) {
		if (ldb_msg_add_fmt(msg, "vendorVersion", 
				    "%s", SAMBA_VERSION_STRING) != 0) {
			goto failed;
		}
	}

	/* TODO: lots more dynamic attributes should be added here */

	return LDB_SUCCESS;

failed:
	return LDB_ERR_OPERATIONS_ERROR;
}

/*
  handle search requests
*/

struct rootdse_context {
	struct ldb_module *module;
	struct ldb_request *req;
};

static struct rootdse_context *rootdse_init_context(struct ldb_module *module,
						    struct ldb_request *req)
{
	struct ldb_context *ldb;
	struct rootdse_context *ac;

	ldb = ldb_module_get_ctx(module);

	ac = talloc_zero(req, struct rootdse_context);
	if (ac == NULL) {
		ldb_set_errstring(ldb, "Out of Memory");
		return NULL;
	}

	ac->module = module;
	ac->req = req;

	return ac;
}

static int rootdse_callback(struct ldb_request *req, struct ldb_reply *ares)
{
	struct rootdse_context *ac;
	int ret;

	ac = talloc_get_type(req->context, struct rootdse_context);

	if (!ares) {
		return ldb_module_done(ac->req, NULL, NULL,
					LDB_ERR_OPERATIONS_ERROR);
	}
	if (ares->error != LDB_SUCCESS) {
		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}

	switch (ares->type) {
	case LDB_REPLY_ENTRY:
		/*
		 * if the client explicit asks for the 'netlogon' attribute
		 * the reply_entry needs to be skipped
		 */
		if (ac->req->op.search.attrs &&
		    ldb_attr_in_list(ac->req->op.search.attrs, "netlogon")) {
			talloc_free(ares);
			return LDB_SUCCESS;
		}

		/* for each record returned post-process to add any dynamic
		   attributes that have been asked for */
		ret = rootdse_add_dynamic(ac->module, ares->message,
					  ac->req->op.search.attrs);
		if (ret != LDB_SUCCESS) {
			talloc_free(ares);
			return ldb_module_done(ac->req, NULL, NULL, ret);
		}

		return ldb_module_send_entry(ac->req, ares->message, ares->controls);

	case LDB_REPLY_REFERRAL:
		/* should we allow the backend to return referrals in this case
		 * ?? */
		break;

	case LDB_REPLY_DONE:
		return ldb_module_done(ac->req, ares->controls,
					ares->response, ares->error);
	}

	talloc_free(ares);
	return LDB_SUCCESS;
}

static int rootdse_search(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb;
	struct rootdse_context *ac;
	struct ldb_request *down_req;
	int ret;

	ldb = ldb_module_get_ctx(module);

	/* see if its for the rootDSE - only a base search on the "" DN qualifies */
	if (!(req->op.search.scope == LDB_SCOPE_BASE && ldb_dn_is_null(req->op.search.base))) {
		/* Otherwise, pass down to the rest of the stack */
		return ldb_next_request(module, req);
	}

	ac = rootdse_init_context(module, req);
	if (ac == NULL) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	/* in our db we store the rootDSE with a DN of @ROOTDSE */
	ret = ldb_build_search_req(&down_req, ldb, ac,
					ldb_dn_new(ac, ldb, "@ROOTDSE"),
					LDB_SCOPE_BASE,
					NULL,
					req->op.search.attrs,
					NULL,/* for now skip the controls from the client */
					ac, rootdse_callback,
					req);
	if (ret != LDB_SUCCESS) {
		return ret;
	}

	return ldb_next_request(module, down_req);
}

static int rootdse_register_control(struct ldb_module *module, struct ldb_request *req)
{
	struct private_data *priv = talloc_get_type(ldb_module_get_private(module), struct private_data);
	char **list;

	list = talloc_realloc(priv, priv->controls, char *, priv->num_controls + 1);
	if (!list) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	list[priv->num_controls] = talloc_strdup(list, req->op.reg_control.oid);
	if (!list[priv->num_controls]) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	priv->num_controls += 1;
	priv->controls = list;

	return ldb_module_done(req, NULL, NULL, LDB_SUCCESS);
}

static int rootdse_register_partition(struct ldb_module *module, struct ldb_request *req)
{
	struct private_data *priv = talloc_get_type(ldb_module_get_private(module), struct private_data);
	struct ldb_dn **list;

	list = talloc_realloc(priv, priv->partitions, struct ldb_dn *, priv->num_partitions + 1);
	if (!list) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	list[priv->num_partitions] = ldb_dn_copy(list, req->op.reg_partition.dn);
	if (!list[priv->num_partitions]) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	priv->num_partitions += 1;
	priv->partitions = list;

	return ldb_module_done(req, NULL, NULL, LDB_SUCCESS);
}


static int rootdse_request(struct ldb_module *module, struct ldb_request *req)
{
	switch (req->operation) {

	case LDB_REQ_REGISTER_CONTROL:
		return rootdse_register_control(module, req);
	case LDB_REQ_REGISTER_PARTITION:
		return rootdse_register_partition(module, req);

	default:
		break;
	}
	return ldb_next_request(module, req);
}

static int rootdse_init(struct ldb_module *module)
{
	struct ldb_context *ldb;
	struct private_data *data;

	ldb = ldb_module_get_ctx(module);

	data = talloc(module, struct private_data);
	if (data == NULL) {
		return -1;
	}

	data->num_controls = 0;
	data->controls = NULL;
	data->num_partitions = 0;
	data->partitions = NULL;
	ldb_module_set_private(module, data);

	ldb_set_default_dns(ldb);

	return ldb_next_init(module);
}

static int rootdse_modify(struct ldb_module *module, struct ldb_request *req)
{
	struct ldb_context *ldb;
	struct ldb_result *ext_res;
	int ret;
	struct ldb_dn *schema_dn;
	struct ldb_message_element *schemaUpdateNowAttr;
	
	/*
		If dn is not "" we should let it pass through
	*/
	if (!ldb_dn_is_null(req->op.mod.message->dn)) {
		return ldb_next_request(module, req);
	}

	ldb = ldb_module_get_ctx(module);

	/*
		dn is empty so check for schemaUpdateNow attribute
		"The type of modification and values specified in the LDAP modify operation do not matter." MSDN
	*/
	schemaUpdateNowAttr = ldb_msg_find_element(req->op.mod.message, "schemaUpdateNow");
	if (!schemaUpdateNowAttr) {
		return LDB_ERR_OPERATIONS_ERROR;
	}

	schema_dn = samdb_schema_dn(ldb);
	if (!schema_dn) {
		ldb_reset_err_string(ldb);
		ldb_debug(ldb, LDB_DEBUG_WARNING,
			  "rootdse_modify: no schema dn present: (skip ldb_extended call)\n");
		return ldb_next_request(module, req);
	}

	ret = ldb_extended(ldb, DSDB_EXTENDED_SCHEMA_UPDATE_NOW_OID, schema_dn, &ext_res);
	if (ret != LDB_SUCCESS) {
		return LDB_ERR_OPERATIONS_ERROR;
	}
	
	talloc_free(ext_res);
	return ret;
}

_PUBLIC_ const struct ldb_module_ops ldb_rootdse_module_ops = {
	.name			= "rootdse",
	.init_context   = rootdse_init,
	.search         = rootdse_search,
	.request		= rootdse_request,
	.modify         = rootdse_modify
};