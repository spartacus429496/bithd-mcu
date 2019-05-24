/*
 *	Definitions for the 'hyper binary/text transmission over
 *	uart' protocol.
 *	aka. hbto_uart
 *
 *	Authors:
 *
 */

#ifndef _HBTO_UART_H
#define _HBTO_UART_H

#ifndef DEBUG_LOG_EN
//#define DEBUG_LOG_EN 1
#endif

//#define MTU 20
//#define MAX_ENTRIES (1024/MTU+1)
//based on vector
#define HBTO_HDR_LEN 9
//#define MAX_UART_PAYLOAD_LEN 1024+9+2
#define MAX_UART_PAYLOAD_LEN 80+9+2
typedef struct s_buff_vector {
    unsigned char stream[MAX_UART_PAYLOAD_LEN];
    //unsigned short entry_num;

}s_buf_vector_t;
#define HAED1_MAGIC 0xa5
#define HAED2_MAGIC 0x5a
#define HAED3_MAGIC '#'
#define TAIL_MAGIC 0xAA
//stream buf
typedef struct __attribute__((__packed__)) hbto_uart_meta{

    struct __attribute__((__packed__)) hbto_uart_hdr{
        unsigned char head1;
        unsigned char head2;
        unsigned char head3;
        unsigned short payload_len;
        unsigned short id;
        unsigned short reserve;//command id
    }hdr;
    unsigned char *p_payload;
    unsigned char crc_chk_sum;
    unsigned char tail;
} hbto_uart_meta_t;

typedef enum hbto_uart_state {
    HBTO_IDLE=0,
    HBTO_START,
    HBTO_HAED_S,
    HBTO_LEN_S,
    HBTO_ID_S,
    HBTO_RESERVE_S,
    HBTO_SENDING,
    HBTO_PAYLOAD_RECEIVING,
    HBTO_CHK_SUM_S,
    HBTO_TAIL_S,//9
    HBTO_COMPLETE,
} hbto_uart_state_t;

//class
typedef struct hbto_uart_obj{
    hbto_uart_state_t state;
    hbto_uart_meta_t meta;
    //operations
    /*
     *init
     *destroy
     *send
     *recv
     */

}hbto_uart_obj_t;

extern hbto_uart_obj_t hbto_uart_txer;
extern hbto_uart_obj_t hbto_uart_rxer;
extern s_buf_vector_t sbuf_tx;

void hbto_uart_init(hbto_uart_obj_t *p_er);
void hbto_uart_destroy(hbto_uart_obj_t *p_er);


//API
/*
 * in: p_data
 * internal: s_buf
 * out:
 */
void hbto_uart_send(unsigned char *p_data, unsigned short len);

/* a receiever fsm
 * in: data: 1 Byte
 * internal: s_buf
 * out:
 */
//void hbto_uart_recv(unsigned char *p_data, unsigned short len);
unsigned short hbto_uart_recv(unsigned char *p_data, unsigned short len, \
        unsigned char *p_out);


#endif	/*_HBTO_UART_H */
