#include <stdint.h>
#include <string.h>

void reader_init(uint8_t *, int);

void reader_get(uint8_t *);

void reader_get_short(uint16_t *);

void reader_get_int(uint32_t *);

void reader_get_long(uint64_t *);

void reader_get_bytes(uint8_t *, size_t);

void reader_get_variable_uint(uint64_t *);

void reader_free(void);