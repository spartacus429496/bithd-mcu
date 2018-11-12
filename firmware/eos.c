#include "eos.h"
#include "fsm.h"
#include "layout2.h"
#include "messages.h"
#include "ecdsa.h"
#include "protect.h"
#include "crypto.h"
#include "secp256k1.h"
#include "gettext.h"
#include "eos_reader.h"
#include "eos_writer.h"
#include "eos_model.h"
#include "eos_action_reader.h"
#include "eos_action_data_reader.h"
#include "eos_utils.h"

static bool eos_signing = false;
SHA256_CTX sha256_ctx;
static EOSTxSignature msg_tx_request;
static uint8_t privkey[32];
static uint32_t data_left;

static inline void hash_bytes(const uint8_t *buf, const size_t size)
{
	sha256_Update(&sha256_ctx, buf, size);
}

static void hash_uint(const uint64_t value, const size_t size) 
{
	uint8_t buff[8];
	for (size_t i = 0; i < size; i++)
	{
		buff[i] = (uint8_t)(value >> i * 8 & 0xFF);
	}
	hash_bytes(buff, size);
}

static void hash_vint(const uint64_t value) 
{
	uint64_t val = value;
	int index = 0;
	uint8_t buff[8];
	do {
        uint8_t b = (uint8_t)((val) & 0x7f);
        val >>= 7;
        b |= ( ((val > 0) ? 1 : 0 ) << 7 );
        buff[index++] = b;
    } while( val != 0 );
	hash_bytes(buff, index);	
}

static void hash_zero(const size_t size) 
{
	uint8_t _zero[size];
	memset(_zero, 0, size);
	hash_bytes(_zero, size);
}

void eos_signing_abort(void)
{
    if (eos_signing) {
		memset(privkey, 0, sizeof(privkey));
		layoutHome();
		eos_signing = false;
	}
}

static int eos_is_canonic(uint8_t v, uint8_t signature[64])
{
	(void) signature;
	return (v & 2) == 0;
}

static void send_signature(void)
{
	uint8_t hash[32], sig[64];
	uint8_t v;
	
	sha256_Final(&sha256_ctx,hash);

	if (ecdsa_sign_digest_DER(&secp256k1, privkey, hash, sig, &v, eos_is_canonic) != 0) {
		fsm_sendFailure(FailureType_Failure_ProcessError, _("Signing failed"));
		eos_signing_abort();
		return;
	}

	layoutProgress(_("Signing"), 1000);

	memset(privkey, 0, sizeof(privkey));

	/* Send back the result */
	msg_tx_request.has_signature_v = true;
	
	msg_tx_request.signature_v = v;
	
	msg_tx_request.has_signature_r = true;
	msg_tx_request.signature_r.size = 32;
	memcpy(msg_tx_request.signature_r.bytes, sig, 32);

	msg_tx_request.has_signature_s = true;
	msg_tx_request.signature_s.size = 32;
	memcpy(msg_tx_request.signature_s.bytes, sig+32, 32);

	msg_write(MessageType_MessageType_EOSTxSignature, &msg_tx_request);

	eos_signing_abort();
}

bool confirm_eosio_vote_producer(EosReaderCTX *ctx)
{
	EosioVoteProducer vote_producer;
	if (!reader_get_vote_producer(ctx, &vote_producer)) {
		return false;
	}

	char _voting_desc[] = "Confirm voting";
	char _producers[] = "producers:";
	char _producer1[20]; 
	char _producer2[20];
	char _producer3[20];

	char _next[] = _("Next");
	char _confirm[] = _("Confirm");
	char _cancel[] = _("Cancel");

	uint8_t page_count = 0;

	while (page_count < vote_producer.producer_size) {

		memset(_producer1, 0, 20);
		memset(_producer2, 0, 20);
		memset(_producer3, 0, 20);	

		uint8_t left_count = vote_producer.producer_size - page_count;
		uint8_t page_size = left_count > 3? 3: left_count;
		switch (page_size) 
		{
			case 3: 
				format_producer(vote_producer.producers[page_count + 2], page_count + 3, _producer3);
			case 2: 
				format_producer(vote_producer.producers[page_count + 1], page_count + 2, _producer2);
			case 1: 
				format_producer(vote_producer.producers[page_count + 0], page_count + 1, _producer1);
				break;
		}

		page_count += 3;

		layoutDialogSwipe(
			&bmp_icon_question,
			_cancel,
			page_count >= vote_producer.producer_size? _confirm: _next,
			NULL,
			_voting_desc,
			_producers,
			_producer1,
			_producer2,
			_producer3,
			NULL
		);

		if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
			return false;
		}
	}

	char _relly_vote[] = "Relly vote for these";
	char _relly_producers[] = "producers?";
	char _voter[] = "voter:";
	char voter[13];
	int size = name_to_str(vote_producer.voter, voter);
	voter[size] = '\0';

	layoutDialogSwipe(
		&bmp_icon_question,
		_cancel,
		_confirm,
		NULL,
		_relly_vote,
		_relly_producers,
		_voter,
		voter,
		NULL,
		NULL
	);

	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false);
}

bool confirm_eosio_token_transfer(EosReaderCTX *ctx) 
{
	EosioTokenTransfer transfer;
	if (!reader_get_transfer(ctx, &transfer)) {
		return false;
	}

	char _sending_desc[] = "Confirm sending";
	char _send_value[] = "_____________________";
	char _to_desc[] = "to:";
	char _to[] = "______________________";

	int _size = name_to_str(transfer.to, _to);
	_to[_size] = '\0';

	char quantity[40];
	uint8_t qlen = format_asset(&transfer.quantity, quantity);
	memcpy(_send_value, quantity, qlen);
	_send_value[qlen] = '\0';

	layoutDialogSwipe(
		&bmp_icon_question,
		_("Cancel"),
		_("Confrim"),
		NULL,
		_sending_desc,
		_send_value,
		_to_desc,
		_to,
		NULL,
		NULL
	);
	if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
		return false;
	}

	char _really_send[] = "Really send";
	char _from_desc[] = "pay account:";
	char _from[] = "______________________";
	_size = name_to_str(transfer.from, _from);
	_from[_size] = '\0';

	layoutDialogSwipe(
		&bmp_icon_question,
		_("Cancel"),
		_("Confrim"),
		NULL,
		_really_send,
		_send_value,
		_from_desc,
		_from,
		NULL,
		NULL
	);
	return protectButton(ButtonRequestType_ButtonRequest_SignTx, false); 
}

void eos_signing_init(EOSSignTx *msg, const HDNode *node)
{
	EosReaderCTX reader_ctx;
	action_reader_init(&reader_ctx, msg->actions.bytes, msg->actions.size + 1);
	uint64_t action_count = action_reader_count(&reader_ctx);
	for (uint8_t i=0; i < action_count; i ++) {
		EosAction action;
		action_reader_next(&reader_ctx, &action);
		if (action.account == EOSIO) {
			switch (action.name) 
			{
				case ACTION_NEW_ACCOUNT:
				break;
				case ACTION_BUY_RAM:
				break;
				case ACTION_SELL_RAM: 
				break;
				case ACTION_SELL_RAM_BYTES: 
				break;
				case ACTION_DELEGATE: 
				break; 
				case ACTION_UNDELEGATE: 
				break;
				case ACTION_VOTE_PRODUCER:
					if (!confirm_eosio_vote_producer(&reader_ctx)) {
						fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
						eos_signing_abort();
						return;		
					}
					break;
				default:
				break;
			}
		} else if (action.account == EOSIO_TOKEN) {
			switch (action.name) 
			{
				case ACTION_TRANSMFER: 
					if (!confirm_eosio_token_transfer(&reader_ctx)) {
						fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
						eos_signing_abort();
						return;	
					}
					break;
				default: 
				eos_signing_abort();
				break;
			}
		} else if (action.account == EOSIO_MSIG) {
			switch (action.name)
			{
				case ACTION_PROPOSE: 
				break;
				case ACTION_UNAPPROVE: 
				break;
				case ACTION_APPROVE: 
				break;
				case ACTION_CANCEL: 
				break;
				case ACTION_EXEC: 
				break;
				default:
				break;
			}
			eos_signing_abort();
		} else {
			// other actions.
			eos_signing_abort();
		}
	}

	eos_signing = true;
	sha256_Init(&sha256_ctx);

	//step1
	layoutProgress(_("Signing"), 0);
	hash_bytes(msg->chain_id.bytes, msg->chain_id.size); // chain_id.
	hash_uint(msg->expiration, 4); // expiration
	hash_uint(msg->ref_block_num, 2); // ref_block_num
	hash_uint(msg->ref_block_prefix, 4); // ref_block_prefix.
	hash_vint(msg->max_net_usage_words); // max_net_usage_words
	hash_vint(msg->max_cpu_usage_ms); // max_cpu_usage_ms
	hash_vint(msg->delay_sec);		// dalay_sec
	hash_bytes(msg->free_actions.bytes, msg->free_actions.size);
	hash_bytes(msg->actions.bytes,msg->actions.size);
	hash_zero(33);

	//step2
	layoutProgress(_("Signing"), 100);
    memcpy(privkey, node->private_key, 32);

    send_signature();
}

static void send_request_chunk(void)
{

}
void eos_signing_txack(EOSTxAck *tx)
{
    tx->has_data = false;

    if (!eos_signing) {
		fsm_sendFailure(FailureType_Failure_UnexpectedMessage, _("Not in Eos signing mode"));
		layoutHome();
		return;
	}

	if (data_left > 0) {
		send_request_chunk();
	} else {
		send_signature();
	}
}