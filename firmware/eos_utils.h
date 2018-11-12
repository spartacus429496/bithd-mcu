#if !defined(_EOS_UTILS_H_)
#define _EOS_UTILS_H_

#include "eos_model.h"

uint8_t format_asset(EosTypeAsset *, char *);

void symbol_to_str(uint64_t, char *);

#endif // _EOS_UTILS_H_