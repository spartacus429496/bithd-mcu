#include "eos_reader.h"

static uint8_t *_buf;
static int _index;
static int _length;

int check_available(int num) 
{
    return _length - _index > num;
}

void reader_init(uint8_t * buf, int len) 
{
    _buf = buf;
    _index = 0;
    _length = len;
}

void reader_get(uint8_t * b) 
{
    if(check_available(1)) {
        *b = _buf[_index++];
    }
}


void reader_get_short(uint16_t * s)
{
    if(check_available(2)){
        *s = (((_buf[_index++] & 0xFF)) | ((_buf[_index++] & 0xFF) << 8)) & 0xFFFF;
    }
}

void reader_get_int(uint32_t *i)
{
    if(check_available(4)){
        *i = ((_buf[_index++] & 0xFF)) | ((_buf[_index++] & 0xFF) << 8) | ((_buf[_index++] & 0xFF) << 16)
            | ((_buf[_index++] & 0xFF) << 24);
    }
}

void reader_get_long(uint64_t *l)
{
    if(check_available(8)){
        *l = ((_buf[_index++] & 0xFFL)) | ((uint64_t)(_buf[_index++] & 0xFFL) << 8) | ((uint64_t)(_buf[_index++] & 0xFFL) << 16)
            | ((uint64_t)(_buf[_index++] & 0xFFL) << 24) | ((uint64_t)(_buf[_index++] & 0xFFL) << 32) | ((uint64_t)(_buf[_index++] & 0xFFL) << 40)
            | ((uint64_t)(_buf[_index++] & 0xFFL) << 48) | ((uint64_t)(_buf[_index++] & 0xFFL) << 56);
    }
}

void reader_get_bytes(uint8_t *bytes, size_t len) 
{
    if(check_available(len)) {
        memcpy(bytes, _buf + _index, len);
        _index += len;
    }
}

void reader_get_variable_uint(uint64_t *val)
{
    uint64_t v = 0;
    uint8_t b=0, by = 0;
    do {
        reader_get(&b);
        v |= ( b & 0x7F) << by;
        by +=7;
    }
    while ( (b & 0x80) != 0 );
    *val = v;
}

void reader_free(void) 
{
    // free(_buf);
    _length = 0;
    _index = 0;
}