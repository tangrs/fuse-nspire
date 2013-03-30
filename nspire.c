#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>

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
	.truncate	= nsp_truncate,
	.create		= nsp_create,
	.open		= nsp_open,
	.read		= nsp_read,
	.write		= nsp_write,
	.release	= nsp_release,
	.fsync		= nsp_fsync,
	//.access		= nsp_access,
};


#define OPTION(t, p, v) { (t), offsetof(struct nsp_ctx, p), (v) }

static struct fuse_opt nsp_opts[] = {
	OPTION("allow_bigfile", allow_bigfile, 1),
	OPTION("thresh_bigfile=%i", thresh_bigfile, 0),
};

#undef OPTION

#define ERR_EXIT(errstr, errnum) do { \
		fprintf(stderr, "Error (%d): %s\n", (errnum), (errstr)); \
		exit(errnum); \
	} while (0)

int main(int argc, char **argv) {
	int ret;
	struct nsp_ctx *ctx = malloc(sizeof(*ctx));
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (!ctx)
		ERR_EXIT("Out of memory\n", -ENOMEM);

	if ((ret = nspire_init(&ctx->handle)))
		ERR_EXIT(nspire_strerror(ret), std_libnspire_err(ret));

	atomic_init(&ctx->lock);
	ctx->allow_bigfile = 0;
	ctx->thresh_bigfile = 512 * 1024;

	fuse_opt_parse(&args, ctx, nsp_opts, NULL);

	if (ctx->allow_bigfile) {
		fprintf(stderr,
			"Warning: Allowing files larger than %d bytes to be "
			"opened. Large files may hang filesystem operations.\n",
			ctx->thresh_bigfile);
	}

	ret = fuse_main(args.argc, args.argv, &nspire_fs, ctx);

	nspire_free(ctx->handle);
	free(ctx);

	return ret;
}
#undef ERR_EXIT
