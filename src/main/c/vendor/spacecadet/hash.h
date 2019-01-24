#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef size_t hashcode;

typedef hashcode (*hash_function)(const void *key);

hashcode identity_hash(const void *key);
hashcode identity_xor_hash(const void *key);

hashcode shift_add_xor_string_hash(const void *key);
hashcode shift_add_xor_string_buffer_hash(const uint8_t *key, size_t length);

hashcode sdbm_string_hash(const void *key);
hashcode sdbm_string_buffer_hash(const uint8_t *key, size_t length);

hashcode fnv1_string_hash(const void *key);
hashcode fnv1_string_buffer_hash(const uint8_t *key, size_t length);

hashcode fnv1a_string_hash(const void *key);
hashcode fnv1a_string_buffer_hash(const uint8_t *key, size_t length);

hashcode djb_string_hash(const void *key);
hashcode djb_string_buffer_hash(const uint8_t *key, size_t length);

typedef bool (*compare_function)(const void *key1, const void *key2);

bool string_comparitor(const void *key1, const void *key2);
