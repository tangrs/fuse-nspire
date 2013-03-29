#include <fuse.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <nspire.h>

struct nsp_ctx {
	nspire_handle_t *handle;
	unsigned long lock;

};

static int device_trylock(struct nsp_ctx *h) {
	return !(__sync_lock_test_and_set(&h->lock, 1));
}

static void device_unlock(struct nsp_ctx *h) {
	__sync_lock_test_and_set(&h->lock, 0);
}

#define current_ctx ((struct nsp_ctx*)(fuse_get_context()->private_data))
