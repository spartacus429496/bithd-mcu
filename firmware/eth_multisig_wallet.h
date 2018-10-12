#ifndef ETH_MULTISIG_WALLET
#define ETH_MULTISIG_WALLET
#define ETH_MULTISIG_CONTRACT_LENGTH 5845
extern const unsigned char multisig_wallet_contract[ETH_MULTISIG_CONTRACT_LENGTH];
extern const unsigned char params_start[63];
extern const unsigned char length_start[31];
extern const unsigned char address_start[12];
extern const unsigned char method_submit_tx[4];
extern const unsigned char submit_end[64];
extern const unsigned char method_confirm[4];
extern const unsigned char uint32_start[28];
#endif