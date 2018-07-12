#include <stdint.h>

#define MAX_NAME_IDX 			12

#define EOS_ACTION_VOTE 		0xdd32aade89d21800
#define EOS_ACTION_TRANSMFER 	0xcdcd3c2d57000000
#define EOS_ACTION_DELEGATE 	0x4aa2a61b2a3f0000
#define EOS_ACTION_UNDELEGATE 	0xd4d2a8a986ca9000
#define EOS_ACTION_BUY_RAM		0x3ebd734800000000
#define EOS_ACTION_SELL_RAM		0xc2a31b9800000000
  
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

uint64_t read_variable_uint(uint8_t*, int);