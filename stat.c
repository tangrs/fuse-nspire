#include "nspire.h"

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int nsp_statfs(const char* path, struct statvfs *info) {
	struct nspire_devinfo devinfo;
	int ret;

	device_lock(current_ctx);

	ret = nspire_device_info(current_ctx->handle, &devinfo);
	if (ret) {
		ret = std_libnspire_err(ret);
		goto end;
	}

	memset(info, 0, sizeof(*info));

	/* Going by the rounded figure of a 1 byte file */
	info->f_bsize = info->f_frsize = 1 * 1024;
	info->f_blocks = devinfo.storage.total / info->f_bsize;
	info->f_bfree = info->f_bavail = devinfo.storage.free / info->f_bsize;
	info->f_namemax = sizeof(((struct nspire_dir_item*)0)->name) - 1;

	ret = 0;
end:
	device_unlock(current_ctx);
	return ret;
}

int nsp_getattr(const char *path, struct stat *s) {
	int ret;
	struct nspire_dir_item i;

	device_lock(current_ctx);

	ret = nspire_attr(current_ctx->handle, path, &i);
	if (ret) {
		ret = std_libnspire_err(ret);
		goto end;
	}

	memset(s, 0, sizeof(*s));
	s->st_size = i.size;
	s->st_nlink = 1;
	s->st_mode = (i.type == NSPIRE_DIR ? S_IFDIR : S_IFREG) | 0755;
	s->st_mtime = s->st_atime = s->st_ctime = i.date;

	s->st_uid = getuid();
	s->st_gid = getgid();

	ret = 0;
end:
	device_unlock(current_ctx);
	return ret;
}

int nsp_access(const char *path, int mask) {
	int ret;
	struct nspire_dir_item i;

	device_lock(current_ctx);

	ret = nspire_attr(current_ctx->handle, path, &i);
	if (ret) {
		ret = std_libnspire_err(ret);
		goto end;
	}

	if (!current_ctx->allow_bigfile &&
			i.size > current_ctx->thresh_bigfile) {
		ret = -EFBIG;
		goto end;
	}

	ret = 0;
end:
	device_unlock(current_ctx);
	return ret;
}
