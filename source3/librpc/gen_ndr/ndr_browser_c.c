/* client functions auto-generated by pidl */

#include "includes.h"
#include <tevent.h>
#include "lib/util/tevent_ntstatus.h"
#include "librpc/gen_ndr/ndr_browser.h"
#include "librpc/gen_ndr/ndr_browser_c.h"

/* browser - client functions generated by pidl */

struct dcerpc_BrowserrQueryOtherDomains_r_state {
	TALLOC_CTX *out_mem_ctx;
};

static void dcerpc_BrowserrQueryOtherDomains_r_done(struct tevent_req *subreq);

struct tevent_req *dcerpc_BrowserrQueryOtherDomains_r_send(TALLOC_CTX *mem_ctx,
	struct tevent_context *ev,
	struct dcerpc_binding_handle *h,
	struct BrowserrQueryOtherDomains *r)
{
	struct tevent_req *req;
	struct dcerpc_BrowserrQueryOtherDomains_r_state *state;
	struct tevent_req *subreq;

	req = tevent_req_create(mem_ctx, &state,
				struct dcerpc_BrowserrQueryOtherDomains_r_state);
	if (req == NULL) {
		return NULL;
	}

	state->out_mem_ctx = talloc_new(state);
	if (tevent_req_nomem(state->out_mem_ctx, req)) {
		return tevent_req_post(req, ev);
	}

	subreq = dcerpc_binding_handle_call_send(state, ev, h,
			NULL, &ndr_table_browser,
			NDR_BROWSERRQUERYOTHERDOMAINS, state->out_mem_ctx, r);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, dcerpc_BrowserrQueryOtherDomains_r_done, req);

	return req;
}

static void dcerpc_BrowserrQueryOtherDomains_r_done(struct tevent_req *subreq)
{
	struct tevent_req *req =
		tevent_req_callback_data(subreq,
		struct tevent_req);
	NTSTATUS status;

	status = dcerpc_binding_handle_call_recv(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}

	tevent_req_done(req);
}

NTSTATUS dcerpc_BrowserrQueryOtherDomains_r_recv(struct tevent_req *req, TALLOC_CTX *mem_ctx)
{
	struct dcerpc_BrowserrQueryOtherDomains_r_state *state =
		tevent_req_data(req,
		struct dcerpc_BrowserrQueryOtherDomains_r_state);
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	talloc_steal(mem_ctx, state->out_mem_ctx);

	tevent_req_received(req);
	return NT_STATUS_OK;
}

NTSTATUS dcerpc_BrowserrQueryOtherDomains_r(struct dcerpc_binding_handle *h, TALLOC_CTX *mem_ctx, struct BrowserrQueryOtherDomains *r)
{
	NTSTATUS status;

	status = dcerpc_binding_handle_call(h,
			NULL, &ndr_table_browser,
			NDR_BROWSERRQUERYOTHERDOMAINS, mem_ctx, r);

	return status;
}

struct dcerpc_BrowserrQueryOtherDomains_state {
	struct BrowserrQueryOtherDomains orig;
	struct BrowserrQueryOtherDomains tmp;
	TALLOC_CTX *out_mem_ctx;
};

static void dcerpc_BrowserrQueryOtherDomains_done(struct tevent_req *subreq);

struct tevent_req *dcerpc_BrowserrQueryOtherDomains_send(TALLOC_CTX *mem_ctx,
							 struct tevent_context *ev,
							 struct dcerpc_binding_handle *h,
							 const char *_server_unc /* [in] [unique,charset(UTF16)] */,
							 struct BrowserrSrvInfo *_info /* [in,out] [ref] */,
							 uint32_t *_total_entries /* [out] [ref] */)
{
	struct tevent_req *req;
	struct dcerpc_BrowserrQueryOtherDomains_state *state;
	struct tevent_req *subreq;

	req = tevent_req_create(mem_ctx, &state,
				struct dcerpc_BrowserrQueryOtherDomains_state);
	if (req == NULL) {
		return NULL;
	}
	state->out_mem_ctx = NULL;

	/* In parameters */
	state->orig.in.server_unc = _server_unc;
	state->orig.in.info = _info;

	/* Out parameters */
	state->orig.out.info = _info;
	state->orig.out.total_entries = _total_entries;

	/* Result */
	ZERO_STRUCT(state->orig.out.result);

	state->out_mem_ctx = talloc_named_const(state, 0,
			     "dcerpc_BrowserrQueryOtherDomains_out_memory");
	if (tevent_req_nomem(state->out_mem_ctx, req)) {
		return tevent_req_post(req, ev);
	}

	/* make a temporary copy, that we pass to the dispatch function */
	state->tmp = state->orig;

	subreq = dcerpc_BrowserrQueryOtherDomains_r_send(state, ev, h, &state->tmp);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, dcerpc_BrowserrQueryOtherDomains_done, req);
	return req;
}

static void dcerpc_BrowserrQueryOtherDomains_done(struct tevent_req *subreq)
{
	struct tevent_req *req = tevent_req_callback_data(
		subreq, struct tevent_req);
	struct dcerpc_BrowserrQueryOtherDomains_state *state = tevent_req_data(
		req, struct dcerpc_BrowserrQueryOtherDomains_state);
	NTSTATUS status;
	TALLOC_CTX *mem_ctx;

	if (state->out_mem_ctx) {
		mem_ctx = state->out_mem_ctx;
	} else {
		mem_ctx = state;
	}

	status = dcerpc_BrowserrQueryOtherDomains_r_recv(subreq, mem_ctx);
	TALLOC_FREE(subreq);
	if (!NT_STATUS_IS_OK(status)) {
		tevent_req_nterror(req, status);
		return;
	}

	/* Copy out parameters */
	*state->orig.out.info = *state->tmp.out.info;
	*state->orig.out.total_entries = *state->tmp.out.total_entries;

	/* Copy result */
	state->orig.out.result = state->tmp.out.result;

	/* Reset temporary structure */
	ZERO_STRUCT(state->tmp);

	tevent_req_done(req);
}

NTSTATUS dcerpc_BrowserrQueryOtherDomains_recv(struct tevent_req *req,
					       TALLOC_CTX *mem_ctx,
					       WERROR *result)
{
	struct dcerpc_BrowserrQueryOtherDomains_state *state = tevent_req_data(
		req, struct dcerpc_BrowserrQueryOtherDomains_state);
	NTSTATUS status;

	if (tevent_req_is_nterror(req, &status)) {
		tevent_req_received(req);
		return status;
	}

	/* Steal possible out parameters to the callers context */
	talloc_steal(mem_ctx, state->out_mem_ctx);

	/* Return result */
	*result = state->orig.out.result;

	tevent_req_received(req);
	return NT_STATUS_OK;
}

NTSTATUS dcerpc_BrowserrQueryOtherDomains(struct dcerpc_binding_handle *h,
					  TALLOC_CTX *mem_ctx,
					  const char *_server_unc /* [in] [unique,charset(UTF16)] */,
					  struct BrowserrSrvInfo *_info /* [in,out] [ref] */,
					  uint32_t *_total_entries /* [out] [ref] */,
					  WERROR *result)
{
	struct BrowserrQueryOtherDomains r;
	NTSTATUS status;

	/* In parameters */
	r.in.server_unc = _server_unc;
	r.in.info = _info;

	status = dcerpc_BrowserrQueryOtherDomains_r(h, mem_ctx, &r);
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}

	/* Return variables */
	*_info = *r.out.info;
	*_total_entries = *r.out.total_entries;

	/* Return result */
	*result = r.out.result;

	return NT_STATUS_OK;
}
