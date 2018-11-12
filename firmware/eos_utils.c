#include <stdio.h>
#include "eos_utils.h"

void symbol_to_str(uint64_t symbol, char *symbol_str) 
{
	uint64_t value = symbol;
	int i = 0;
	while(value > 0) {
		symbol_str[i] = value & 0xff;
		value >>= 8;
		i ++;
	}
	*(symbol_str + i) = '\0';
}

uint8_t format_asset(EosTypeAsset *asset, char *out) 
{
    int decimals = asset->symbol & 0xFF;
    double value = asset->amount * 1.0;
    for (int i = 0; i < decimals; i ++)
        value /= 10;
    char sym[12];
    symbol_to_str(asset->symbol, sym);
    return sprintf(out, "%.4f %s", value, sym);    
}