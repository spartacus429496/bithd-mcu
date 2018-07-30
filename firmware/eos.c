#include "eos.h"
#include "fsm.h"
#include "layout2.h"
#include "messages.h"
#include "transaction.h"
#include "ecdsa.h"
#include "protect.h"
#include "crypto.h"
#include "secp256k1.h"
#include "sha2.h"
#include "address.h"
#include "util.h"
#include "gettext.h"
#include "eos_packer.h"
#include "eos_reader.h"
#include "eos_writer.h"
#include  "uart.h"

static bool eos_signing = false;
SHA256_CTX sha256_ctx;
static EOSTxSignature msg_tx_request;
static uint8_t privkey[32];
static uint32_t data_left;
static uint8_t totalnum_producer;
static uint8_t totalnum_proxy;
static uint8_t list_num=0;

static inline void hash_data(const uint8_t *buf, size_t size)
{
	sha256_Update(&sha256_ctx, buf, size);
}

static void layoutEosconfirmTxTRANSMFER(uint8_t *data,uint64_t size)
{
	uint64_t to;
	char tobuf[20];
	uint8_t len_to;
	uint64_t amount;
    char amountbuf[32];
	uint8_t len_amount=0;
	uint8_t point_pos;
	uint8_t len_1=0,len_2=0;
    char _to1[20] = "Send: ";
	char _to2[19];
	char _to3[19];
	memset(_to2,0,sizeof(_to2));
	memset(_to3,0,sizeof(_to3));

    if (size) {
		to = byte_reverse_to_64((uint8_t*)(data+8));
		len_to = name_to_string(to,tobuf);
		memcpy(_to1+6,tobuf,len_to);

		point_pos = *(data+24);
		amount = byte_reverse_to_64((uint8_t*)(data+16));	
		len_amount = uint64_to_numstring_point(amount, amountbuf,point_pos);
		if(len_amount >18)
		{
			len_1 = 18;
			len_2 = len_amount - len_1;
			memcpy(_to2, amountbuf, len_1);
			memcpy(_to3,&amountbuf[len_1],len_2);
			memcpy(_to3+len_2,(uint8_t*)(data+25),7);
		}
		else
		{
			memcpy(_to2, amountbuf, len_amount);
			memcpy(_to2+len_amount,(uint8_t*)(data+25),7);
		}	
	} else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		_("Transfer action"),
		_to1,
		_("Amount: "),
		_to2,
		_to3,
		NULL,
		NULL
	);
}

static void layoutEosconfirmTxDELEGATE(uint8_t *data,uint64_t size)
{
	uint8_t flag=0;
	uint64_t from=0;
	char frombuf[20];
	int from_len=0;
	uint64_t net_amount;
	char net_amountbuf[20];
	size_t net_amtlen=0;
	uint64_t cpu_amount;
	char cpu_amountbuf[20];
	size_t cpu_amtlen=0;

	char _to1[20] = "Acct: ";
	char _to2[20] ;
	memset(_to2,0,sizeof(_to2));
	char _to3[20] = "Get ";
	char _to4[20];
	memset(_to4,0,sizeof(_to4));
	char _to5[20] = "Get ";
	
	memset(net_amountbuf,0,sizeof(net_amountbuf));
	
    if (size) {
		from = byte_reverse_to_64((uint8_t*)data);
    	from_len = name_to_string(from,frombuf);
		memcpy(_to1 + 6, frombuf, from_len);

		net_amount = byte_reverse_to_64((uint8_t*)(data+16));
		if(net_amount != 0)
		{
			net_amtlen = uint64_to_numstring_point(net_amount, net_amountbuf,4);
			memcpy(_to2, net_amountbuf, net_amtlen);
			memcpy(_to2+net_amtlen,data+25,*(data+24)-1);//symbol
			memcpy(_to3+4,"Net Resources",13);
			flag = 1;
		}
		
		cpu_amount = byte_reverse_to_64((uint8_t*)(data+32));
		if(cpu_amount != 0)
		{
			cpu_amtlen = uint64_to_numstring_point(cpu_amount, cpu_amountbuf,4);
			if(flag == 1)
			{
				memcpy(_to4, cpu_amountbuf, cpu_amtlen);	
				memcpy(_to4+cpu_amtlen,(uint8_t*)(data+41),*(data+40)-1);//symbol
				memcpy(_to5+4,"Cpu Resources",13);
			}
			else
			{
				memcpy(_to2, cpu_amountbuf, cpu_amtlen);	
				memcpy(_to2+cpu_amtlen,(uint8_t*)(data+41),*(data+40)-1);//symbol
				memcpy(_to3+4,"Cpu Resources",13);
				strlcpy(_to4, "", sizeof(_to4));
				strlcpy(_to5, "", sizeof(_to5));
			}			
		}
		else
		{
			strlcpy(_to4, "", sizeof(_to4));
			strlcpy(_to5, "", sizeof(_to5));
		}
		
		if((from_len>20)||(net_amtlen>20)||(cpu_amtlen>20))
		{
			strlcpy(_to1, _("Data Error"), sizeof(_to1));
			strlcpy(_to2, "", sizeof(_to2));
			strlcpy(_to3, "", sizeof(_to3));
		}
	} else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
		strlcpy(_to3, "", sizeof(_to3));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		NULL,
		_to1,
		_to2,
		_to3,
		_to4,
		_to5,
		NULL
	);
}

static void layoutEosconfirmTxUNDELEGATE(uint8_t *data,uint64_t size)
{
	uint8_t flag=0;
    int from_len=0;
	uint8_t to_len;
	uint64_t from=0;
	char frombuf[64];
	uint64_t net_amount;
	char net_amountbuf[20];
	size_t net_amtlen=0;
	uint64_t cpu_amount;
	char cpu_amountbuf[20];
	size_t cpu_amtlen=0;

    to_len = size;

	char _to1[20] = "Acct: ";
	char _to2[20];
	memset(_to2,0,sizeof(_to2));
	char _to3[20] = "Release ";
	char _to4[20];
	memset(_to4,0,sizeof(_to4));
	char _to5[20] = "Release ";

    if (to_len) {
		from = byte_reverse_to_64((uint8_t*)data);
    	from_len = name_to_string(from,frombuf);
		memcpy(_to1 + 6, frombuf, from_len);

		net_amount = byte_reverse_to_64((uint8_t*)(data+16));
		if(net_amount != 0)
		{
			net_amtlen = uint64_to_numstring_point(net_amount, net_amountbuf,4);
			memcpy(_to2, net_amountbuf, net_amtlen);
			memcpy(_to2+net_amtlen,data+25,*(data+24)-1);//symbol
			memcpy(_to3+4,"Net Resources",13);
			flag = 1;
		}
		cpu_amount = byte_reverse_to_64((uint8_t*)(data+32));
		if(cpu_amount != 0)
		{
			cpu_amtlen = uint64_to_numstring_point(cpu_amount, cpu_amountbuf,4);
			if(flag == 1)
			{
				memcpy(_to4, cpu_amountbuf, cpu_amtlen);	
				memcpy(_to4+cpu_amtlen,(uint8_t*)(data+41),*(data+40)-1);//symbol
				memcpy(_to5+8,"Cpu Resources",13);
			}
			else
			{
				memcpy(_to2, cpu_amountbuf, cpu_amtlen);	
				memcpy(_to2+cpu_amtlen,(uint8_t*)(data+41),*(data+40)-1);//symbol
				memcpy(_to3+8,"Cpu Resources",13);
				strlcpy(_to4, "", sizeof(_to4));
				strlcpy(_to5, "", sizeof(_to5));
			}			
		}
		else
		{
			strlcpy(_to4, "", sizeof(_to4));
			strlcpy(_to5, "", sizeof(_to5));
		}
		
		if((from_len>20)||(net_amtlen>20)||(cpu_amtlen>20))
		{
			strlcpy(_to1, _("Data Error"), sizeof(_to1));
			strlcpy(_to2, "", sizeof(_to2));
			strlcpy(_to3, "", sizeof(_to3));
			strlcpy(_to4, "", sizeof(_to2));
			strlcpy(_to5, "", sizeof(_to3));
		}
	} else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
		strlcpy(_to3, "", sizeof(_to3));
		strlcpy(_to4, "", sizeof(_to2));
		strlcpy(_to5, "", sizeof(_to3));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		_("Undelegate Action"),
		_to1,
		_to2,
		_to3,
		_to4,
		_to5,
		NULL
	);
}

static void layoutEosconfirmTxBUY_RAM(uint8_t *data,uint64_t size)
{
    int from_len=0;
	uint64_t from=0;
	char frombuf[64];
	uint64_t amount;
	char amountbuf[64];
	size_t amtlen=0;
	char _to2[20];
	char _to4[20];

	char _to1[20] = "Account: ";
	memset(_to2,0,sizeof(_to2));
	char _to3[20] = "Amount: ";
	memset(_to4,0,sizeof(_to4));

    if (size) {
		from = byte_reverse_to_64((uint8_t*)data);
    	from_len = name_to_string(from,frombuf);
		memcpy(_to2, frombuf, from_len);

		amount = byte_reverse_to_64((uint8_t*)(data+16));
		amtlen = uint64_to_numstring_point(amount, amountbuf,4);
		memcpy(_to4, amountbuf, amtlen);
		memcpy(_to4+amtlen,"(EOS)",5);
	} else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
		strlcpy(_to3, "", sizeof(_to3));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		_("Buy ram ?"),
		_to1,
		_to2,
		_to3,
		_to4,
		NULL,
		NULL
	);
}

static void layoutEosconfirmTxSELL_RAM(uint8_t *data,uint64_t size)
{
	int from_len=0;
	uint64_t from=0;
	char frombuf[64];
	uint64_t amount;
	char amountbuf[64];
	size_t amtlen=0;
	
	char _to1[20] = "Account: ";
	char _to2[20];
	memset(_to2,0,sizeof(_to2));
	char _to3[20] = "Sell: ";
	char _to4[20];
	memset(_to4,0,sizeof(_to4));

    if (size) {
		from = byte_reverse_to_64((uint8_t*)data);
    	from_len = name_to_string(from,frombuf);
		memcpy(_to2, frombuf, from_len);

		amount = byte_reverse_to_64((uint8_t*)(data+8));
		amtlen = uint64_to_num(amount, amountbuf);
		memcpy(_to4, amountbuf, amtlen);
		memcpy(_to4+amtlen,"(Bytes)",7);
	} else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
	}

	layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_("Confirm"),
		_("Sell Ram ?"),
		_to1,
		_to2,
		_to3,
		_to4,
		NULL,
		NULL
	);
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


static uint8_t layoutEosconfirmTxVOTE(uint8_t *data,uint64_t size)
{
	uint8_t remain_num=0;
	uint64_t voter;
    char voterbuf[32];
	uint8_t len_vote;
	uint64_t producer;
    char producerbuf[32];
	uint8_t len_producer;

	char _to1[20] = "Voter:";
	char _to2[20] = "Proxy:";
	char _to3[20] = "Producer:";
	char _to4[20];
	char _usract[20] = "Confirm";
	memset(_to4,0,sizeof(_to4));

	if (size) 
	{
		voter = byte_reverse_to_64((uint8_t*)data);
    	len_vote = name_to_string(voter,voterbuf);
		memcpy(_to1 + 6, voterbuf, len_vote);

		totalnum_proxy = *(data+8);
		if(totalnum_proxy != 0)
		{
			totalnum_producer = *(data+16+totalnum_proxy*8+1);
		}
		else
		{
			totalnum_producer = *(data+16);
		}

		producer = byte_reverse_to_64((uint8_t*)(data+17));
    	len_producer = name_to_string(producer,producerbuf);
		
		memcpy(_to2,"Producer: ",10);
		memset(_to3,0,sizeof(_to3));
		memcpy(_to3,"1-",2);
		list_num ++;
		memcpy(_to3+2,producerbuf,len_producer);
		totalnum_producer -= 1;
		if((totalnum_producer)>0)
		{
			producer = byte_reverse_to_64((uint8_t*)(data+25));
			len_producer = name_to_string(producer,producerbuf);
			memcpy(_to4,"2-",2);
			list_num++;
			memcpy(_to4+2,producerbuf,len_producer);
			remain_num = totalnum_producer;
			totalnum_producer -= 1;
			if((totalnum_producer)>0)
			{
				memcpy(_usract,"Next Page",9);
			}				
		}
		else
		{
			remain_num = 0;
		}
		
	}else {
		strlcpy(_to1, _("to new contract?"), sizeof(_to1));
		strlcpy(_to2, "", sizeof(_to2));
		strlcpy(_to3, "", sizeof(_to3));
	}

    layoutDialogSwipe(&bmp_icon_question,
		_("Cancel"),
		_usract,
		_("Vote Action"),
		_to1,
		_to2,
		_to3,
		_to4,
		NULL,
		NULL
	);
	return remain_num;
}

static void layoutEosProducerVote(uint8_t *data)
{
	uint64_t producer;
    char producerbuf[32];
	uint8_t len_producer;
	char listbuff[3];
	uint8_t i=0;

	char _to1[20] = "Producer:";
	char _to2[20];
	char _to3[20];
	char _to4[20];
	memset(_to2,0,sizeof(_to2));
	memset(_to3,0,sizeof(_to3));
	memset(_to4,0,sizeof(_to4));
	char _usract[20] = "Confirm";

	while((totalnum_producer >0)&&(i<3))
	{
		producer = byte_reverse_to_64((uint8_t*)(data+i*8));
		len_producer = name_to_string(producer,producerbuf);
		
		list_num++;
		switch(i)
		{
			case 0:
				if(list_num>9)
				{
					listbuff[0] = 0x30+(list_num/10);
					listbuff[1] = 0x30+(list_num%10);					
					listbuff[2] = 0x2D;
					memcpy(_to2,listbuff,3);
					memcpy(_to2+3,producerbuf,len_producer);
				}
				else
				{					
					listbuff[0] = 0x30+list_num;
					listbuff[1] = 0x2D;
					memcpy(_to2,listbuff,2);
					memcpy(_to2+2,producerbuf,len_producer);
				}				
				break;
			case 1:
				if(list_num>9)
				{
					listbuff[0] = 0x30+(list_num/10);
					listbuff[1] = 0x30+(list_num%10);					
					listbuff[2] = 0x2D;
					memcpy(_to3,listbuff,3);
					memcpy(_to3+3,producerbuf,len_producer);
				}
				else
				{
					listbuff[0] = 0x30+list_num;
					listbuff[1] = 0x2D;
					memcpy(_to3,listbuff,2);
					memcpy(_to3+2,producerbuf,len_producer);
				}					
				break;
			case 2:
				if(list_num>9)
				{
					listbuff[0] = 0x30+(list_num/10);
					listbuff[1] = 0x30+(list_num%10);					
					listbuff[2] = 0x2D;
					memcpy(_to4,listbuff,3);
					memcpy(_to4+3,producerbuf,len_producer);
				}
				else
				{
					listbuff[0] = 0x30+list_num;
					listbuff[1] = 0x2D;
					memcpy(_to4,listbuff,2);
					memcpy(_to4+2,producerbuf,len_producer);
				}					
				break;
		}
		
		totalnum_producer -= 1;
		i++;
		memset(listbuff,0,sizeof(listbuff));
	}

	if(totalnum_producer >= 1)
	{
		memcpy(_usract,"Next Page",9);
	}

	layoutDialogSwipe(&bmp_icon_question,
	_("Cancel"),
	_usract,
	_("Vote Action"),
	_to1,
	_to2,
	_to3,
	_to4,
	NULL,
	NULL
	);
}

void eos_signing_init(EOSSignTx *msg, const HDNode *node)
{
	uint64_t action_name=0;
	uint8_t ret_vote=0;
	uint8_t msgbuff[32];
	uint8_t var_buf[256];
	uint16_t var_lenth;
	uint64_t len_sct;
	int num_bytes;

	eos_signing = true;
	wirter_reset(var_buf);
	sha256_Init(&sha256_ctx);

	num_bytes = read_variable_uint(&msg->actions.bytes[34],0,&len_sct);

	action_name = byte_reverse_to_64(&(msg->actions.bytes[9]));   //9-16:acount name

	switch(action_name)
	{
		case EOS_ACTION_DELEGATE:
			layoutEosconfirmTxDELEGATE(&msg->actions.bytes[34+num_bytes],len_sct);
			break;
		case EOS_ACTION_UNDELEGATE:
			layoutEosconfirmTxUNDELEGATE(&msg->actions.bytes[34+num_bytes],len_sct);
			break;
		case EOS_ACTION_VOTE:
			ret_vote = layoutEosconfirmTxVOTE(&msg->actions.bytes[34+num_bytes],len_sct);
			break;
		case EOS_ACTION_TRANSMFER:
			layoutEosconfirmTxTRANSMFER(&msg->actions.bytes[34+num_bytes],len_sct);
			break;
		case EOS_ACTION_BUY_RAM:
			layoutEosconfirmTxBUY_RAM(&msg->actions.bytes[34+num_bytes],len_sct);
			break;
		case EOS_ACTION_SELL_RAM:
			layoutEosconfirmTxSELL_RAM(&msg->actions.bytes[34+num_bytes],len_sct);
			break;
		default:
			eos_signing_abort();
        	return;
	}

    if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) {
    fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
    eos_signing_abort();
    return;
	}

	if(ret_vote != 0)
	{
		if(totalnum_producer > 30-2)
		{
			fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
			eos_signing_abort();
			return;
		}
		while(totalnum_producer >0)
		{
			if(totalnum_proxy>0)
			{
				layoutEosProducerVote(&msg->actions.bytes[34+num_bytes+list_num*8+17+totalnum_proxy*8+1]);
			}
			else
			{
				layoutEosProducerVote(&msg->actions.bytes[34+num_bytes+list_num*8+17]);
			}
			
			if (!protectButton(ButtonRequestType_ButtonRequest_SignTx, false)) 
			{
				fsm_sendFailure(FailureType_Failure_ActionCancelled, NULL);
				eos_signing_abort();
				return;
			}	
		}	
	}
	//step1
	layoutProgress(_("Signing"), 0);
	hash_data(msg->chain_id.bytes,msg->chain_id.size);//hash
	uint32_reverse_to_bytes(msg->expiration,msgbuff,4);
	uint32_reverse_to_bytes(msg->ref_block_num,&msgbuff[4],2);
	uint32_reverse_to_bytes(msg->ref_block_prefix,&msgbuff[6],4);
	hash_data(msgbuff,10);//hash
	wirter_put_variable_uint(msg->max_net_usage_words);
	wirter_put_variable_uint(msg->max_cpu_usage_ms);
	wirter_put_variable_uint(msg->delay_sec);
	wirter_bytes_length(&var_lenth);
	if(msg->free_actions.size == 0)
	{
		var_buf[var_lenth] = 0;
	}
	hash_data(var_buf,var_lenth+1);//hash
	hash_data(msg->actions.bytes,msg->actions.size);//hash
	memset(var_buf,0,32+1);
	hash_data(var_buf,32+1);

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