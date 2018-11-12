#ifndef _EOS_TRANSACTION_READER_H_
#define _EOS_TRANSACTION_READER_H_
#include "eos_reader.h"
#include "eos_model.h"

void transcation_reader_init(EosReaderCTX *, uint8_t *, int);

int transaction_reader_get(EosReaderCTX *, EosTransaction *);
#endif