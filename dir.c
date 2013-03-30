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
