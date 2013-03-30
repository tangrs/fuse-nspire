#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nspire.h"

struct file_cache {
	size_t len;
	lock_t lock;
	int needs_sync;
	unsigned char data[];
};

int nsp_truncate(const char *path, off_t size) {
	int ret;
	void *buffer = NULL;
	struct nspire_dir_item i;

	device_lock(current_ctx);

	if (!size) {
		/* Truncate to zero */
		ret = nspire_file_touch(current_ctx->handle, path);
		ret = std_libnspire_err(ret);
		goto end;
	}

	/* Truncate to something useful */
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

	buffer = malloc(size);
	if (!buffer) {
		ret = -ENOMEM;
		goto end;
	}
	memset(buffer, 0, size);

	/* Going to be super slow for large files */
	ret = nspire_file_read(current_ctx->handle, path, buffer, size, NULL);
	if (ret) {
		ret = std_libnspire_err(ret);
		goto end;
	}

	ret = nspire_file_write(current_ctx->handle, path, buffer, size);
	if (ret) {
		ret = std_libnspire_err(ret);
		goto end;
	}

	ret = 0;
end:
	free(buffer);
	device_unlock(current_ctx);
	return ret;
}

int nsp_open(const char *path, struct fuse_file_info *fi) {
	int ret;
	struct file_cache *file;
	struct nspire_dir_item i;

	device_lock(current_ctx);

	/* Get size of file */
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

	/* Allocate buffer */
	file = malloc(sizeof(*file) + i.size);
	if (!file) {
		ret = -ENOMEM;
		goto end;
	}
	atomic_init(&file->lock);
	file->len = i.size;
	file->needs_sync = 0;

	/* Populate with cache data of file contents */
	if (file->len) {
		/* Can be very slow with large files */
		ret = nspire_file_read(current_ctx->handle, path,
				file->data, file->len, &file->len);
		if (ret) {
			ret = std_libnspire_err(ret);
			goto error_free;
		}
	}

	/* Assign to file handle */
	fi->fh = (typeof(fi->fh))file;
	/* Make sure we can cast to an integer and back without loss of info */
	assert((void*)(fi->fh) == (void*)file);

	ret = 0;
	goto end;

error_free:
	free(file);
end:
	device_unlock(current_ctx);
	return ret;
}


int nsp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	int ret;
	struct nspire_dir_item i;

	device_lock(current_ctx);

	ret = nspire_attr(current_ctx->handle, path, &i);
	if (ret) {
		if (ret == -NSPIRE_ERR_NONEXIST) {
			ret = nspire_file_touch(current_ctx->handle, path);
			if (ret) {
				ret = std_libnspire_err(ret);
				goto error;
			}
		} else {
			ret = std_libnspire_err(ret);
			goto error;
		}
	}

	device_unlock(current_ctx);
	return nsp_open(path, fi);

error:
	device_unlock(current_ctx);
	return ret;
}

int nsp_write(const char *path, const char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	int ret;
	struct file_cache *f = (struct file_cache *)(fi->fh);
	if (!f)
		return -EINVAL;

	atomic_lock(&f->lock);

	if (offset + size > f->len) {
		struct file_cache *new;
		new = realloc(f, sizeof(*new) + offset + size);
		if (!new) {
			ret = -ENOMEM;
			goto end;
		}
		f = new;
		fi->fh = (typeof(fi->fh))new;
		f->len = offset + size;
	}

	memcpy(f->data + offset, buf, size);

	f->needs_sync = 1;

	ret = size;
end:
	atomic_unlock(&f->lock);
	return ret;
}

int nsp_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	struct file_cache *f = (struct file_cache *)(fi->fh);
	if (!f)
		return -EINVAL;

	atomic_lock(&f->lock);

	memset(buf, 0, size);

	if (offset + size > f->len) {
		size = f->len - offset;
	}

	memcpy(buf, f->data + offset, size);

	atomic_unlock(&f->lock);
	return size;
}

int nsp_fsync(const char* path, int isdatasync, struct fuse_file_info* fi) {
	int ret;
	struct file_cache *f = (struct file_cache *)(fi->fh);
	if (!f)
		return -EINVAL;

	if (!f->needs_sync) return 0;

	device_lock(current_ctx);
	atomic_lock(&f->lock);

	ret = nspire_file_write(current_ctx->handle, path, f->data, f->len);
	if (!ret)
		f->needs_sync = 0;

	ret = std_libnspire_err(ret);

	atomic_unlock(&f->lock);
	device_unlock(current_ctx);
	return ret;
}

int nsp_release(const char* path, struct fuse_file_info* fi) {
	int ret;
	struct file_cache *f = (struct file_cache *)(fi->fh);

	ret = nsp_fsync(path, 0, fi);
	free(f);
	fi->fh = 0;

	return ret;
}
