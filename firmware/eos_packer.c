#include <string.h>
#include "eos_packer.h"

char CHAR_MAP[] = ".12345abcdefghijklmnopqrstuvwxyz";

uint64_t char_to_symbol(char c)
{
	if (c >= 'a' && c <='z') {
		return (uint64_t)(c -'a' + 6);
	}
	if (c >= '1' && c <= '5') {
		return (uint64_t)(c - '1' + 1);
	}
	return 0;
}

uint64_t string_to_name(char* name_str, int len)
{
	if ( len == 0 ) return 0;
	uint64_t value = 0;

	for (int i = 0; i < MAX_NAME_IDX; ++i)
	{
		uint64_t c = 0;
		if (i < len && i <= MAX_NAME_IDX) c = char_to_symbol(name_str[i]);
		if( i < MAX_NAME_IDX) {
            c &= 0x1f;
            c <<= 64 - 5 * ( i + 1 );
        } else {
            c &= 0x0f;
        }
        value |= c;
	}
	return value;
}

int name_to_string(uint64_t name, char* name_str)
{
	char str_tmp[MAX_NAME_IDX + 1];
	uint64_t name_tmp = name;
	for (int i = 0; i <= MAX_NAME_IDX; i++)
	{
		str_tmp[MAX_NAME_IDX-i] = CHAR_MAP[(int)(name_tmp & (i == 0 ? 0x0f : 0x1f))];
		name_tmp >>= (i == 0 ? 4 : 5);
	}
	int name_len;
	delete_tail_dot(str_tmp, MAX_NAME_IDX + 1, name_str, &name_len);
	return name_len;
}

int len_expcet_tail_dot(char* src, int len) {
	int i = len -1;
	for (; i >= 0; i --)
	{
		if ( src[i] == '.') continue;
		else break;
	}
	return i + 1;
}

void delete_tail_dot(char* src, int len, char* des, int* des_len)
{
	*des_len = len_expcet_tail_dot(src, len);
	memcpy(des, src, *des_len);
	des[*des_len] = '\0';
}

uint64_t read_variable_uint(uint8_t * src, int postion) 
{
	uint64_t value = 0;
	uint8_t b, by = 0;
	int i = postion;
	do {
		b = src[i++];
		value |= (b & 0x7f) << by;
		by += 7;
	} while ( (b & 0x80) != 0);
	return value;
}

void symbol2str(uint64_t symbol, char *symbol_str) 
{
	char str_tmp[8];
	int symbolLen = 0;
	for ( int i = 1; i < 8; i++) {
		char oneChar = (char)( (symbol >> (8*i)) & 0xFF );
		if ( oneChar != 0 ) {
			str_tmp[i] = oneChar;
			symbolLen++;
		}
		else {
			break;
		}
	}
	str_tmp[symbolLen + 1] = '\0';
	memcmp(symbol_str, str_tmp + 1, symbolLen);
}