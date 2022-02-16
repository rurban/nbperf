#include <stdint.h>
#include "wyhash.h"

static inline void
mi_wyhash(const void *key, uint64_t keylen, unsigned seed, uint32_t *hashes)
{
    union {
	uint64_t u64;
	uint32_t u32[2];
    } tmp;
    tmp.u64 = wyhash(key, keylen, seed, _wyp);
    hashes[0] = tmp.u32[0];
    hashes[1] = tmp.u32[1];
}
