#include <string.h>

#include "hash.h"

hashcode identity_hash(const void *key)
{
    return (hashcode)key;
}

hashcode identity_xor_hash(const void *key)
{
    hashcode result = (hashcode)key;
    result ^= result >> 20 ^ result >> 12;
    result ^= result >> 7 ^ result >> 4;
    return result;
}

#define LEFT_MAGNITUDE 5
#define RIGHT_MAGNATUDE 2

/**
 * M. V. Ramakrishna and J. Zobel. Performance in practice of string hashing functions. In DASFAA, pages 215–224, 1997.
 * http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.18.7520
 */
hashcode shift_add_xor_string_hash(const void *key)
{
    uint8_t *string = (uint8_t *)key;
    size_t length = strlen((const char *)string);
    return shift_add_xor_string_buffer_hash(string, length);
}

hashcode shift_add_xor_string_buffer_hash(const uint8_t *key, size_t length)
{
    hashcode result = 0ul;
    for(size_t i = 0; i < length; i++)
    {
        result ^= (result << LEFT_MAGNITUDE) + (result >> RIGHT_MAGNATUDE) + (unsigned long)key[i];
    }

    return result;
}

hashcode sdbm_string_hash(const void *key)
{
    uint8_t *string = (uint8_t *)key;
    size_t length = strlen((char *)string);
    return sdbm_string_buffer_hash(string, length);
}

hashcode sdbm_string_buffer_hash(const uint8_t *key, size_t length)
{
    hashcode result = 0ul;
    for(size_t i = 0; i < length; i++)
    {
        result = (unsigned long)key[i] + (result << 6) + (result << 16) - result;
    }

    return result;
}

static const size_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
static const size_t FNV_PRIME = 0x100000001b3;

hashcode fnv1_string_hash(const void *key)
{
    uint8_t *string = (uint8_t *)key;
    size_t length = strlen((char *)string);
    return fnv1_string_buffer_hash(string, length);
}

hashcode fnv1_string_buffer_hash(const uint8_t *key, size_t length)
{
    hashcode result = FNV_OFFSET_BASIS;
    for(size_t i = 0; i < length; i++)
    {
        result *= FNV_PRIME;
        result ^= (unsigned long)key[i];
    }

    return result;
}

hashcode fnv1a_string_hash(const void *key)
{
    uint8_t *string = (uint8_t *)key;
    size_t length = strlen((char *)string);
    return fnv1a_string_buffer_hash(string, length);
}

hashcode fnv1a_string_buffer_hash(const uint8_t *key, size_t length)
{
    hashcode result = FNV_OFFSET_BASIS;
    for(size_t i = 0; i < length; i++)
    {
        result ^= (unsigned long)key[i];
        result *= FNV_PRIME;
    }

    return result;
}

hashcode djb_string_hash(const void *key)
{
    uint8_t *string = (uint8_t *)key;
    size_t length = strlen((char *)string);
    return djb_string_buffer_hash(string, length);
}

hashcode djb_string_buffer_hash(const uint8_t *key, size_t length)
{
    hashcode result = 5381ul;
    for(size_t i = 0; i < length; i++)
    {
        result = ((result << 5) + result) ^ (unsigned long)key[i];
    }

    return result;
}

bool string_comparitor(const void *key1, const void *key2)
{
    return 0 == strcmp((char *)key1, (char *)key2);
}
