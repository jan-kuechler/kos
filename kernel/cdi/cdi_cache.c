#include <cdi/cache.c>
#include "cdi_impl.h"

struct cdi_cache* cdi_cache_create(size_t block_size, size_t blkpriv_len,
    cdi_cache_read_block_t* read_block,
    cdi_cache_write_block_t* write_block,
    void* prv_data)
{
	UNIMPLEMENTED
}

void cdi_cache_destroy(struct cdi_cache* cache)
{
	UNIMPLEMENTED
}

int cdi_cache_sync(struct cdi_cache* cache)
{
	UNIMPLEMENTED
}

struct cdi_cache_block* cdi_cache_block_get(struct cdi_cache* cache,
    uint64_t blocknum, int noread)
{
	UNIMPLEMENTED
}

void cdi_cache_block_release(struct cdi_cache* cache,
    struct cdi_cache_block* block)
{
	UNIMPLEMENTED
}

void cdi_cache_block_dirty(struct cdi_cache* cache,
    struct cdi_cache_block* block)
{
	UNIMPLEMENTED
}
