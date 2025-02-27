#include <libwasm.h>

#define FNV1A_OFFSET_BASIS 0xCBF29CE484222325UL
#define FNV1A_PRIME        0x00000100000001B3UL

// FNV1-A hash implementation
uint64_t hash(const char* s) {
    if (!s)
        return NULL_STRING_HASH;

    uint64_t ret = FNV1A_OFFSET_BASIS;
    while (*s) {
        ret ^= *s;
        ret *= FNV1A_PRIME;
        s++;
    }

    return ret;
}