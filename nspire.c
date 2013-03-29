#include "nspire.h"

static int std_libnspire_err(int ret) {
	switch (ret) {
	case -NSPIRE_ERR_TIMEOUT:	return -ETIMEDOUT;
	case -NSPIRE_ERR_NOMEM:		return -ENOMEM;
	case -NSPIRE_ERR_INVALID:	return -EINVAL;
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

#define ERR_EXIT(errstr, errnum) do { \
		fprintf(stderr, "Error (%d): %s\n", (errnum), (errstr)); \
		errno = (errnum); \
		return NULL; \
	} while (0)

static void* nsp_init(struct fuse_conn_info *conn) {
	int ret;
	struct nsp_ctx *ctx = malloc(sizeof(*ctx));
	if (!ctx)
		ERR_EXIT("Out of memory\n", -ENOMEM);
	if ((ret = nspire_init(&ctx->handle)))
		ERR_EXIT(nspire_strerror(ret), std_libnspire_err(ret));
	ctx->lock = 0;

	return ctx;
}
#undef ERR_EXIT

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

	for (i=0; i<list->num; i++) {
		filler(buf, list->items[i].name, NULL, 0);
	}

	nspire_dirlist_free(list);
end:
	device_unlock(current_ctx);
	return ret;
}

static void nspire_destroy(void* calc) {
    free(calc);
}

static struct fuse_operations nspire_fs = {
	.init = nsp_init,
	//.destroy = nspire_destroy,
	//.getattr = nspire_getattr,
	.readdir = nsp_readdir,
	//.mkdir = nspire_mkdir,
	//.unlink = nspire_unlink,
	//.rmdir = nspire_rmdir,
	//.rename = nspire_rename,
	//.truncate = nspire_truncate,
	//.create = nspire_create,
	//.open = nspire_open,
	//.read = nspire_read,
	//.write = nspire_write,
	//.release = nspire_release,
	//.opendir = nspire_opendir,
	//.releasedir = nspire_releasedir,
	//.fsync = nspire_fsync
};


int main(int argc, char **argv)
{
	return fuse_main(argc, argv, &nspire_fs, NULL);
}

