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

#ifndef _NSPIRE
#define _NSPIRE

#include <fuse.h>
#include <pthread.h>
#include <nspire.h>

#define current_ctx ((struct nsp_ctx*)(fuse_get_context()->private_data))
#define device_lock(h) atomic_lock(&((h)->lock))
#define device_unlock(h) atomic_unlock(&((h)->lock))


/* Generic lock interface */
typedef pthread_mutex_t lock_t;

static inline void atomic_init(lock_t *lock) {
	pthread_mutex_init(lock, NULL);
}

static inline int atomic_trylock(lock_t *lock) {
	return !pthread_mutex_trylock(lock);
}

static inline void atomic_lock(lock_t *lock) {
	pthread_mutex_lock(lock);
}

static inline void atomic_unlock(lock_t *lock) {
	pthread_mutex_unlock(lock);
}

struct nsp_ctx {
	nspire_handle_t *handle;
	lock_t lock;

	int allow_bigfile;
	int thresh_bigfile;
};

int std_libnspire_err(int ret);

int nsp_statfs(const char* path, struct statvfs *info);
int nsp_getattr(const char *path, struct stat *s);
int nsp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi);
int nsp_mkdir(const char* path, mode_t mode);
int nsp_rmdir(const char* path);
int nsp_rename(const char* src, const char* dst);
int nsp_delete(const char* path);
int nsp_truncate(const char *path, off_t size);
int nsp_open(const char *path, struct fuse_file_info *fi);
int nsp_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int nsp_write(const char *path, const char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi);
int nsp_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi);
int nsp_fsync(const char* path, int isdatasync, struct fuse_file_info* fi);
int nsp_release(const char* path, struct fuse_file_info* fi);
int nsp_access(const char* path, int);

#endif
