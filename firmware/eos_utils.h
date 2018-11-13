#if !defined(_EOS_UTILS_H_)
#define _EOS_UTILS_H_

#include "eos_model.h"

uint8_t format_asset(EosTypeAsset *, char *);

uint8_t format_producer(uint64_t, int, char *);

void symbol_to_str(uint64_t, char *);

int name_to_str(uint64_t, char*);

uint8_t format_eos_pubkey(uint8_t *, uint8_t, int, char *);

#endif // _EOS_UTILS_H_