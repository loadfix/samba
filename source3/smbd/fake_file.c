/* 
   Unix SMB/CIFS implementation.
   FAKE FILE suppport, for faking up special files windows want access to
   Copyright (C) Stefan (metze) Metzmacher	2003
   
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

struct fake_file_type {
	const char *name;
	enum FAKE_FILE_TYPE type;
	void *(*init_pd)(TALLOC_CTX *mem_ctx);
};

static const struct fake_file_type fake_files[] = {
#ifdef WITH_QUOTAS
	{FAKE_FILE_NAME_QUOTA_UNIX, FAKE_FILE_TYPE_QUOTA, init_quota_handle},
#endif /* WITH_QUOTAS */
	{NULL, FAKE_FILE_TYPE_NONE, NULL}
};

/****************************************************************************
 Create a fake file handle
****************************************************************************/

static struct fake_file_handle *init_fake_file_handle(enum FAKE_FILE_TYPE type)
{
	struct fake_file_handle *fh = NULL;
	int i;

	for (i=0; fake_files[i].name!=NULL; i++) {
		if (fake_files[i].type==type) {
			break;
		}
	}

	if (fake_files[i].name == NULL) {
		return NULL;
	}

	DEBUG(5,("init_fake_file_handle: for [%s]\n",fake_files[i].name));

	fh = talloc(NULL, struct fake_file_handle);
	if (fh == NULL) {
		DEBUG(0,("TALLOC_ZERO() failed.\n"));
		return NULL;
	}

	fh->type = type;

	if (fake_files[i].init_pd) {
		fh->private_data = fake_files[i].init_pd(fh);
	}
	return fh;
}

/****************************************************************************
 Does this name match a fake filename ?
****************************************************************************/

enum FAKE_FILE_TYPE is_fake_file(const char *fname)
{
#ifdef HAVE_SYS_QUOTAS
	int i;
#endif

	if (!fname) {
		return FAKE_FILE_TYPE_NONE;
	}

#ifdef HAVE_SYS_QUOTAS
	for (i=0;fake_files[i].name!=NULL;i++) {
		if (strncmp(fname,fake_files[i].name,strlen(fake_files[i].name))==0) {
			DEBUG(5,("is_fake_file: [%s] is a fake file\n",fname));
			return fake_files[i].type;
		}
	}
#endif

	return FAKE_FILE_TYPE_NONE;
}


/****************************************************************************
 Open a fake quota file with a share mode.
****************************************************************************/

NTSTATUS open_fake_file(struct smb_request *req, connection_struct *conn,
				uint16_t current_vuid,
				enum FAKE_FILE_TYPE fake_file_type,
				const char *fname,
				uint32 access_mask,
				files_struct **result)
{
	files_struct *fsp = NULL;
	NTSTATUS status;

	/* access check */
	if (conn->server_info->utok.uid != 0) {
		DEBUG(3, ("open_fake_file_shared: access_denied to "
			  "service[%s] file[%s] user[%s]\n",
			  lp_servicename(SNUM(conn)), fname,
			  conn->server_info->unix_name));
		return NT_STATUS_ACCESS_DENIED;

	}

	status = file_new(req, conn, &fsp);
	if(!NT_STATUS_IS_OK(status)) {
		return status;
	}

	DEBUG(5,("open_fake_file_shared: fname = %s, FID = %d, access_mask = 0x%x\n",
		fname, fsp->fnum, (unsigned int)access_mask));

	fsp->conn = conn;
	fsp->fh->fd = -1;
	fsp->vuid = current_vuid;
	fsp->fh->pos = -1;
	fsp->can_lock = False; /* Should this be true ? - No, JRA */
	fsp->access_mask = access_mask;
	string_set(&fsp->fsp_name,fname);
	
	fsp->fake_file_handle = init_fake_file_handle(fake_file_type);
	
	if (fsp->fake_file_handle==NULL) {
		file_free(req, fsp);
		return NT_STATUS_NO_MEMORY;
	}

	*result = fsp;
	return NT_STATUS_OK;
}

NTSTATUS close_fake_file(struct smb_request *req, files_struct *fsp)
{
	file_free(req, fsp);
	return NT_STATUS_OK;
}