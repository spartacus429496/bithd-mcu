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

int len_expcet_tail_dot(char *src, int len)
{
	int i;
	for (i = 0; i <= len; i++)
	{
		if (src[i] == '.')
			continue;
		else
			break;
	}
	if(i >= len)
		return i;
	len = len - i;
	memmove(src, &src[i], len);
	for (i = 0; i <= len; i++)
	{
		if (src[i] != '.')
			continue;
		else
			break;
	}
	return i;
}

void delete_tail_dot(char *src, int len, char *des, int *des_len)
{
	*des_len = len_expcet_tail_dot(src, len);
	memcpy(des, src, *des_len);
	des[*des_len] = 0;
}

int read_variable_uint(uint8_t *src, int postion,uint64_t *value)
{
	*value = 0;
	uint8_t b, by = 0;
	int i = postion;
	do
	{
		b = src[i++];
		*value |= (b & 0x7f) << by;
		by += 7;
	} while ((b & 0x80) != 0);
	return i;
}

uint8_t symbol2str(uint64_t symbol, char *symbol_str)
{
	char str_tmp[8];
	uint8_t symbolLen = 0;
	for (uint8_t i = 1; i < 8; i++)
	{
		char oneChar = (char)((symbol >> (8 * i)) & 0xFF);
		if (oneChar != 0)
		{
			str_tmp[i] = oneChar;
			symbolLen++;
		}
		else
		{
			break;
		}
	}
	str_tmp[symbolLen + 1] = 0;
	memcpy(symbol_str, &str_tmp[1], symbolLen);
	return symbolLen;
}

uint64_t byte_reverse_to_64(uint8_t *buff)
{
	uint64_t tmp = 0;

	tmp |= (((uint64_t)buff[0]) << 0) & 0x00000000000000FF;
	tmp |= (((uint64_t)buff[1]) << 8) & 0x000000000000FF00;
	tmp |= (((uint64_t)buff[2]) << 16) & 0x0000000000FF0000;
	tmp |= (((uint64_t)buff[3]) << 24) & 0x00000000FF000000;
	tmp |= (((uint64_t)buff[4]) << 32) & 0x000000FF00000000;
	tmp |= (((uint64_t)buff[5]) << 40) & 0x0000FF0000000000;
	tmp |= (((uint64_t)buff[6]) << 48) & 0x00FF000000000000;
	tmp |= (((uint64_t)buff[7]) << 56) & 0xFF00000000000000;
	return tmp;
}

void uint32_reverse_to_bytes(uint32_t a, uint8_t *pbuff, uint8_t count)
{
	for (uint8_t i = 0; i < count; i++)
	{
		pbuff[i] = (uint8_t)(a >> i * 8);
	}
}

uint32_t bytes_to_uint32(uint8_t *pbuff)
{
	uint32_t tmp = 0;

	tmp = (pbuff[3] << 24) + (pbuff[2] << 16) + (pbuff[1] << 8) + pbuff[0];
	return tmp;
}

uint8_t uint64_to_numstring(uint64_t in, char *out)
{
	char tembuff[11];
	uint8_t len, i;

	for (i = 0; i < 11; i++)
	{
		out[i] = (in % 10) + 0x30;
		in /= 10;
	}
	for (i = 10; i > 0; i--)
	{
		if (out[i] != 0x30)
		{
			break;
		}
	}
	len = i+1;
	if(len>4)
	{
		len = i+1;
	}
	else
	{
		if(len==4)
		{
			len = i+2;
		}
		else if(len == 3)
		{
			len = i+3;
		}
		else if(len == 2)
		{
			len = i+4;
		}
		else if(len == 1)
		{
			len = i+5;
		}		
	}
	memcpy(tembuff, &out[4], len - 4);
	*(out + 4) = '.';
	memcpy(&out[5], tembuff, len - 4);
	len += 1;
	for (i = 0; i < len; i++)
	{
		tembuff[i] = out[len - i - 1];
	}
	memcpy(out, tembuff, len);

	return len;
}

uint8_t uint64_to_numstring_point(uint64_t in, char *out,uint8_t point_pos)
{
	char tembuff[32];
	uint8_t len,last_len=0, i,count=0,count_temp=0;

	for (i = 0; i < 20; i++)
	{
		tembuff[i] = (in % 10) + 0x30;
		in /= 10;
	}
	for (i = 19; i > 0; i--)
	{
		if (tembuff[i] != 0x30)
		{
			break;
		}
	}
	len = i+1;

	memcpy(out,tembuff,point_pos);
	count_temp +=point_pos;
	count += point_pos;
	*(out+count) = '.';
	count++;

	last_len = len - point_pos;
	if(len <= point_pos)
	{
		*(out+count) = '0';
		count ++;
	}
	else
	{
		for(i=0;i<last_len/3;i++)
		{
			memcpy(&out[count],&tembuff[count_temp],3);		
			count +=3;
			if(len-count_temp<=3)
			{
				break;
			}
			*(out+count) = ',';
			count += 1;
			count_temp += 3;
		}
	}
	
	len -= count_temp;
	if(len<3)
	{
		memcpy(&out[count],&tembuff[count_temp],len);		
		count +=len;
	}

	for (i = 0; i < count; i++)
	{
		tembuff[i] = out[count -i-1];
	}
	memcpy(out, tembuff, count);

	return count;
}

uint8_t uint64_to_num(uint64_t in, char *out)
{
	char tembuff[32];
	uint8_t len, i,count=0,count_temp=0;

	for (i = 0; i < 20; i++)
	{
		tembuff[i] = (in % 10) + 0x30;
		in /= 10;
	}
	for (i = 19; i > 0; i--)
	{
		if (tembuff[i] != 0x30)
		{
			break;
		}
	}
	len = i+1;

	for(i=0;i<len/3;i++)
	{
		memcpy(&out[count],&tembuff[count_temp],3);		
		count +=3;
		if(len-count_temp<=3)
		{
			break;
		}
		*(out+count) = ',';
		count += 1;
		count_temp += 3;
	}
	
	len -= count_temp;
	if(len<3)
	{
		memcpy(&out[count],&tembuff[count_temp],len);		
		count +=len;
	}

	for (i = 0; i < count; i++)
	{
		tembuff[i] = out[count -i-1];
	}
	memcpy(out, tembuff, count);

	return count;
}