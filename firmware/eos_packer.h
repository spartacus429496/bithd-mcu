#include <stdint.h>

#define MAX_NAME_IDX 			13
#define ACCOUNT_EOSIO  			0x5530ea0000000000
#define EOS_ACTION_VOTE 		0xdd32aade89d21570
#define EOS_ACTION_TRANSMFER 	0xcdcd3c2d57000000
#define EOS_ACTION_DELEGATE 	0x4aa2a61b2a3f0000
#define EOS_ACTION_UNDELEGATE 	0xd4d2a8a986ca8fc0
#define EOS_ACTION_BUY_RAM		0x3ebd734800000000
#define EOS_ACTION_SELL_RAM		0xc2a31b9a40000000
  
typedef uint64_t EosAccountName;
typedef uint64_t EosTypeName;

typedef struct eos_type_asset
{
	uint64_t amount;
	uint64_t symbol;
} EosTypeAsset;

typedef struct eos_vote {
	EosAccountName 	voter;
	EosAccountName 	proxy;
	EosAccountName*	producers;
} EosVote;

typedef struct eos_transmfer {
	EosAccountName 	from;
	EosAccountName 	to;
	EosTypeAsset	quantity;
	uint8_t*		memo;
} EosTransmfer;

typedef struct eos_delegate {
 	EosAccountName	from;
 	EosAccountName 	receiver;
	EosTypeAsset 	net_quantity;
	EosTypeAsset 	cpu_quantity;
	uint8_t			unused_data;
} EosDelegate, EosUndelegate;

typedef struct eos_buy_ram {
	EosAccountName 	from;
	EosAccountName	receiver;
	EosTypeAsset	quantity;
} EosBuyRam;

typedef struct eos_sell_ram {
	EosAccountName 	from;
	uint64_t 		bytes;		
} EosSellRam;

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