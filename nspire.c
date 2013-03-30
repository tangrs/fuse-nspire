#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "nspire.h"

int std_libnspire_err(int ret) {
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

static struct fuse_operations nspire_fs = {
	.getattr	= nsp_getattr,
	.readdir	= nsp_readdir,
	.mkdir		= nsp_mkdir,
	.unlink		= nsp_delete,
	.rmdir		= nsp_rmdir,
	.rename		= nsp_rename,
	.statfs		= nsp_statfs,
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
