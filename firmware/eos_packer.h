#include <stdint.h>

uint64_t string_to_name(char*, int len);

int name_to_string(uint64_t, char*);

void delete_tail_dot(char*, int, char*, int*);

int len_expcet_tail_dot(char*, int);

int read_variable_uint(uint8_t *src, int postion,uint64_t *value);
uint8_t symbol2str(uint64_t, char *);
uint32_t bytes_to_uint32(uint8_t *pbuff);
uint64_t byte_reverse_to_64(uint8_t *buff);
void uint32_reverse_to_bytes(uint32_t a,uint8_t *pbuff,uint8_t count);
uint8_t uint64_to_numstring(uint64_t in,char *out);
uint8_t uint64_to_numstring_point(uint64_t in, char *out,uint8_t point_pos);
uint8_t uint64_to_num(uint64_t in, char *out);