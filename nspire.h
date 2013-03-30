#ifndef _NSPIRE
#define _NSPIRE

#include <fuse.h>
#include <nspire.h>

#define current_ctx ((struct nsp_ctx*)(fuse_get_context()->private_data))

struct nsp_ctx {
	nspire_handle_t *handle;
	unsigned long lock;

};

static inline int device_trylock(struct nsp_ctx *h) {
	return !(__sync_lock_test_and_set(&h->lock, 1));
}

static inline void device_unlock(struct nsp_ctx *h) {
	__sync_lock_test_and_set(&h->lock, 0);
}

int std_libnspire_err(int ret);

int nsp_statfs(const char* path, struct statvfs *info);
int nsp_getattr(const char *path, struct stat *s);
int nsp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi);
int nsp_mkdir(const char* path, mode_t mode);
int nsp_rmdir(const char* path);
int nsp_rename(const char* src, const char* dst);
int nsp_delete(const char* path);

#endif
