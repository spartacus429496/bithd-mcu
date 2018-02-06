#ifndef _uart_H_
#define _uart_H_

extern TimProg T_Connect;                                //communicate timeout timer

 extern void usart_setup(void);
 extern void USART2_IRQHandler(void);

#define TimeClose 0
#define TimeOpen 1

#define ONE_SECOND_INTERVAL 10//1S delay
 //通信协议结构定义

 #define uart_0xa5        1 //0xa5
 #define uart_0x5a        2 //0x5a
 #define uart_length_H    3 //length high 8 bits
 #define uart_length_L    4 //length low 8 bits
 #define uart_cmd         5 //CMD byte
 #define uart_nouseful    6 //No useful byte
 #define uart_databytes   7 //Data bytes

 #define uart_first_0x5a  8
 #define uart_secend_0xa5 9

 #define MAX_resend 3 //最大重发次数

 #define NOACK    0
 #define SuccsACK 1
 #define FailACK  2

#define Stm32_idleMode       0x00
#define Stm32_KeyMode      0x01
 #define Stm32_TimerMode  0x02
 #define Stm32_LowpowerMode   0x03
#define Stm32_UpdataMode 0x04
#define Stm32_BalanceMode 0x05
#define Stm32_BuleParingMode 0x06
#define Stm32_Broadcastname 0x07
 typedef struct
 {
 	unsigned short length;    //长度
 	unsigned char  cmd;       //命令
 	unsigned char  no_use;    //备用字节
 	unsigned char* data;      //数据存储地址
 	unsigned short crc16;     //校验字节
 }
 uart_communication_dmwz;

 extern unsigned char stm32workstatus;
 extern unsigned char cmd_uart[5];
 extern unsigned char uartnum;
 extern uart_communication_dmwz uart_communicate_buf;
 extern unsigned char uart_enable_flag;
 extern unsigned char Ack_recive_enable;
 extern unsigned char uart_send_flag;
 extern unsigned char SaveNewbalanceflag;

 unsigned short CRC16_Uart_send(void);

 void uart_send_Bty(unsigned char* buf,unsigned short len);
 void UartDataSendrecive(void);
 void CmdSendUart(unsigned char cmd_uart,unsigned char* apdubuf,unsigned short apdulength);
bool usart_get_interrupt_source(uint32_t usart, uint32_t flag);

#endif
