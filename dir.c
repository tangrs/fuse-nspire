/*
    FUSE filesystem for TI-Nspire calculators
    Copyright (C) 2013  Daniel Tang

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>

#include "nspire.h"

int nsp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	int ret, i;
	struct nspire_dir_info *list;

	device_lock(current_ctx);

	ret = nspire_dirlist(current_ctx->handle, path, &list);
	if (ret) {
		ret = std_libnspire_err(ret);
		goto end;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	for (i=0; i<list->num; i++) {
		filler(buf, list->items[i].name, NULL, 0);
	}

	ret = 0;

	nspire_dirlist_free(list);
end:
	device_unlock(current_ctx);
	return ret;
}

int nsp_mkdir(const char* path, mode_t mode) {
	int ret;

	device_lock(current_ctx);

	ret = nspire_dir_create(current_ctx->handle, path);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}

int nsp_rmdir(const char* path) {
	int ret;

	device_lock(current_ctx);

	ret = nspire_dir_delete(current_ctx->handle, path);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}

int nsp_rename(const char* src, const char* dst) {
	int ret;

	device_lock(current_ctx);

	ret = nspire_file_rename(current_ctx->handle, src, dst);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}

int nsp_delete(const char* path) {
	int ret;

	device_lock(current_ctx);

	ret = nspire_file_delete(current_ctx->handle, path);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}
