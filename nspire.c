#include "nspire.h"

static int std_libnspire_err(int ret) {
	switch (ret) {
	case NSPIRE_ERR_SUCCESS:	return 0;
	case -NSPIRE_ERR_TIMEOUT:	return -ETIMEDOUT;
	case -NSPIRE_ERR_NOMEM:		return -ENOMEM;
	case -NSPIRE_ERR_INVALID:	return -EPERM;
	case -NSPIRE_ERR_LIBUSB:	return -EIO;
	case -NSPIRE_ERR_NODEVICE:	return -ENODEV;
	case -NSPIRE_ERR_INVALPKT:	return -EIO;
	case -NSPIRE_ERR_NACK:		return -EIO;
	case -NSPIRE_ERR_BUSY:		return -EBUSY;
	case -NSPIRE_ERR_EXISTS:	return -EEXIST;
	case -NSPIRE_ERR_NONEXIST:	return -ENOENT;
	case -NSPIRE_ERR_OSFAILED:	return -EPERM;
	default:			return -EINVAL;
	}
}

static int nsp_statfs(const char* path, struct statvfs *info) {
	struct nspire_devinfo devinfo;
	int ret;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

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

static int nsp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	int ret, i;
	struct nspire_dir_info *list;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

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

static int nsp_getattr(const char *path, struct stat *s) {
	int ret;
	struct nspire_dir_item i;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

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

static int nsp_rename(const char* src, const char* dst) {
	int ret;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

	ret = nspire_file_rename(current_ctx->handle, src, dst);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}

static int nsp_mkdir(const char* path, mode_t mode) {
	int ret;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

	ret = nspire_dir_create(current_ctx->handle, path);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}

static int nsp_rmdir(const char* path) {
	int ret;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

	ret = nspire_dir_delete(current_ctx->handle, path);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}

static int nsp_delete(const char* path) {
	int ret;

	if (!device_trylock(current_ctx))
		return -EAGAIN;

	ret = nspire_file_delete(current_ctx->handle, path);

	device_unlock(current_ctx);
	return std_libnspire_err(ret);
}


static struct fuse_operations nspire_fs = {
	.getattr = nsp_getattr,
	.readdir = nsp_readdir,
	.mkdir = nsp_mkdir,
	.unlink = nsp_delete,
	.rmdir = nsp_rmdir,
	.rename = nsp_rename,
	.statfs = nsp_statfs,
	//.truncate = nspire_truncate,
	//.create = nspire_create,
	//.open = nspire_open,
	//.read = nspire_read,
	//.write = nspire_write,
	//.release = nspire_release,
	//.opendir = nsp_opendir,
	//.releasedir = nspire_releasedir,
	//.fsync = nspire_fsync
};


#define ERR_EXIT(errstr, errnum) do { \
		fprintf(stderr, "Error (%d): %s\n", (errnum), (errstr)); \
		exit(errnum); \
	} while (0)

int main(int argc, char **argv)
{
	int ret;
	struct nsp_ctx *ctx = malloc(sizeof(*ctx));
	if (!ctx)
		ERR_EXIT("Out of memory\n", -ENOMEM);
	if ((ret = nspire_init(&ctx->handle)))
		ERR_EXIT(nspire_strerror(ret), std_libnspire_err(ret));
	ctx->lock = 0;

	ret = fuse_main(argc, argv, &nspire_fs, ctx);

	nspire_free(ctx->handle);
	free(ctx);

	return ret;
}
#undef ERR_EXIT
