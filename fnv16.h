#include <stdint.h>
#include <stddef.h>

static inline void fnv16_2(const void *key, size_t len, uint16_t seed0, uint16_t seed1,
                           uint16_t *hashes)
{
  uint8_t *data = (uint8_t *)key;
  const uint8_t *const end = &data[len];
  uint16_t h;

  h = seed0 ^ 0x811c;
  while (data < end) {
    h ^= *data++;
    h *= 0x1003;
  }
  hashes[0] = h;

  h = seed1 ^ 0x9dc5;
  data = (uint8_t *)key;
  while (data < end) {
    h ^= *data++;
    h *= 0x1193;
  }
  hashes[1] = h;
}
