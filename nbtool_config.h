#include <stdint.h>
#include <stddef.h>
#define __RCSID(x)
#define __dead volatile

void mi_vector_hash(const void *key, size_t keylen, uint32_t seed, uint32_t *hashes);
