#include "eos_transaction_reader.h"
#include "eos_action_reader.h"

void transcation_reader_init(EosReaderCTX *_ctx, uint8_t *buf, int len)
{
    reader_init(_ctx, buf, len);
}

int transaction_reader_get(EosReaderCTX *_ctx, EosTransaction *trx)
{
    if (!reader_get_int(_ctx, &trx->expiration)) {
        return FAILED;
    }
    if (!reader_get_short(_ctx, &trx->ref_block_num)) {
        return FAILED;
    }
    if (!reader_get_int(_ctx, &trx->ref_block_prefix)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &trx->max_net_usage_words)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &trx->max_cpu_usage_ms)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &trx->delay_sec)) {
        return FAILED;
    }
    if (!reader_get_variable_uint(_ctx, &trx->contract_free_actions_size)) {
        return FAILED;
    }
    if (trx->contract_free_actions_size > 1) {
        return FAILED;
    }
    for (uint8_t i = 0; i < trx->contract_free_actions_size; i++) {
        if (!action_reader_next(_ctx, trx->contract_free_actions + i)) {
            return FAILED;
        }
    }
    if (!reader_get_variable_uint(_ctx, &trx->actions_size)) {
       return FAILED;
    }
    if (trx->actions_size > 4) {
        return FAILED;
    }
    for (uint8_t i = 0; i < trx->actions_size; i++) {
        if (!action_reader_next(_ctx, trx->actions + i)) {
            return FAILED;
        }
    }
    if (!reader_get_variable_uint(_ctx, &trx->transaction_extensions_size)) {
        return FAILED;
    } 
    return SUCCESS;
}