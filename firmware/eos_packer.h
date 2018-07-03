#include <stdint.h>

#define MAX_NAME_IDX 			12

#define EOS_ACTION_VOTE 		15938989903989314928
#define EOS_ACTION_TRANSMFER 	14829575313431724032
#define EOS_ACTION_DELEGATE 	5378043540636893184
#define EOS_ACTION_UNDELEGATE 	15335505127214321600
#define EOS_ACTION_BUY_RAM		4520896354024685568
#define EOS_ACTION_SELL_RAM		14025084004210835456

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