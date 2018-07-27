#include <string.h>

#include "eos_writer.h"

static uint8_t* _buf;
static int _index;
static int _capacity;

void wirter_reset(uint8_t *pBuff) 
{
    memset(pBuff, 0, INIT_CAPACITY);
    _buf = (uint8_t *) pBuff;
    _index = 0;
    _capacity = INIT_CAPACITY;
}

void ensure_capacity(int capacity)
{
    if(_capacity -_index < capacity) 
    {
        size_t new_len = _capacity * 2 + capacity;
        char tmp[new_len];
        memset(tmp, 0, new_len);
        memcpy(tmp, _buf, _index);
        //free(_buf);
        _buf = (uint8_t *)tmp;
        _capacity = new_len;
    }
}

void wirter_put(uint8_t b) 
{
    ensure_capacity(1);
    _buf[_index++] = b;
}

void wirter_put_short(uint16_t s) 
{
    ensure_capacity(2);
    _buf[_index++] = (uint8_t)0xff & s;
    _buf[_index++] = (uint8_t)0xff & (s >> 8);
}

void wirter_put_int(uint32_t value) 
{
    ensure_capacity(4);
    _buf[_index++] = (uint8_t) (0xFF & (value));
    _buf[_index++] = (uint8_t) (0xFF & (value >> 8));
    _buf[_index++] = (uint8_t) (0xFF & (value >> 16));
    _buf[_index++] = (uint8_t) (0xFF & (value >> 24));
}

void wirter_put_long(uint64_t value) 
{
    ensure_capacity(8);
    _buf[_index++] = (uint8_t) (0xFFL & (value));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 8));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 16));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 24));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 32));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 40));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 48));
    _buf[_index++] = (uint8_t) (0xFFL & (value >> 56));
}

void wirter_put_bytes(uint8_t *bytes, int len)
{
    ensure_capacity(len);
    memcpy(_buf + _index, bytes, len);
    _index += len;
}

void wirter_to_bytes(uint8_t *bytes, int *len) 
{
    memcpy(bytes, _buf, _index);
    *len = _index;
}

void wirter_bytes_length(uint16_t *len)
{
    *len = _index;
}

void  wirter_put_variable_uint(uint64_t val)
{
    do {
        uint8_t b = (uint8_t)((val) & 0x7f);
        val >>= 7;
        b |= ( ((val > 0) ? 1 : 0 ) << 7 );
        wirter_put(b);
    } while( val != 0 );
}
