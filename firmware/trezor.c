/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "trezor.h"
#include "oled.h"
#include "bitmaps.h"
#include "util.h"
#include "usb.h"
#include "setup.h"
#include "storage.h"
#include "layout.h"
#include "layout2.h"
#include "rng.h"
#include "timer.h"
#include "buttons.h"
#include "gettext.h"
#include "fastflash.h"


#include <libopencm3/stm32/usart.h>
#include <timerbitpie.h>
#include "../uart.h"
#include <libopencm3/stm32/gpio.h>
#include "./bithdapp.h"
#include "hbto_uart.h"
#include "messages.h"



int main(void)
{

	setup();
	__stack_chk_guard = random32(); // this supports compiler provided unpredictable stack protection checks
	oledInit();
#if 0
	usart_setup();                  //uart init 115200
#else
    usart_proj_polaris_setup();
#endif


#if FASTFLASH
	uint16_t state = gpio_port_read(BTN_PORT);
	if ((state & BTN_PIN_NO) == 0) {
		run_bootloader();
	}
#endif

	timer_init();

#if DEBUG_LINK
	oledSetDebugLink(1);
	storage_reset();                // wipe storage if debug link
	storage_reset_uuid();
	storage_commit();
	storage_clearPinArea();         // reset PIN failures if debug link
#endif


	storage_init();

#if 1//jack_debug

		oledDrawStringCenter(0, "BitJewel");
		oledRefresh();
         //           oledClear();
          //          oledRefresh();
#endif
    //unsigned char ack_succs[3]={0x5a,0xa5,0x00};
    for (;;) {
#if 0
        //for (uint16_t i=0;i<10000;i++) {
        for (;;) {
            //uart_send_Bty(ack_succs,3);//串口发送 成功ACK
            uart_send_Bty(hello_str, sizeof(hello_str));
            delay(100000);

        }
#endif
        //while(1);
        static unsigned char flag = 0;
        static unsigned char recved_cnt= 0;
        //for (;;){
#if 0
            if (flag == 0) {
                do {
                    delay(100000);
                    buttonUpdate();
                } while (!button.YesUp && !button.NoUp);

                if (button.NoUp) {
                    //show_halt(); // no button was pressed -> halt
                    oledClear();
                    oledRefresh();
                    uart_send_Bty(hello_str, sizeof(hello_str));
                    oledDrawStringCenter(4, "sending hello!!");
                    oledRefresh();
                    flag = 1;
                }else if (button.YesUp) {
                    //show_halt(); // no button was pressed -> halt
                    oledClear();
                    oledRefresh();
                    //uart_send_Bty(hello_str, sizeof(hello_str));
                    oledDrawStringCenter(1, (const char *)hello_str);
                    oledRefresh();
                }
            }
#endif




#if 0

            if (!button.YesDown) {
                //uart_send_Bty(hello_str, sizeof(hello_str));
                oledDrawStringCenter(1, (const char *)hello_str);
                oledRefresh();
            }

            if (button.NoDown) {
		        oledClear();
                oledRefresh();
                //uart_send_Bty(hello_str, sizeof(hello_str));
                oledDrawStringCenter(2, "cleared ");
                oledRefresh();
            }
#endif


#if 0
            //if (uart_recv_flag == 1) {
            hbto_uart_state_t *p_state = &hbto_uart_rxer.state;
            if (*p_state == HBTO_COMPLETE) {
                //oledClear();
                //oledRefresh();
                if (recved_cnt ==0) {
                    //oledDrawStringCenter(1, "first received:");
                    oledDrawString(0, 0, "first received:");
                    recved_cnt ++;
                    oledRefresh();
                    oledDrawString(0, 10, (const char *)uart_recv_buf_data);
                    oledRefresh();
                    uart_recv_flag = 0;
                    uart_recv_reset();
                    delay(100000);
                    //uart_send_Bty(hello_str, sizeof(hello_str));


                } else if (recved_cnt ==1) {
                    oledDrawString(0, 20, "2ed received:");
                    recved_cnt ++;
                    oledRefresh();
                    oledDrawString(0, 30, (const char *)uart_recv_buf_data);
                    oledRefresh();
                    uart_recv_flag = 0;
                    uart_recv_reset();
                    delay(100000);
                    uart_send_Bty(hello_str, sizeof(hello_str));

                }else if (recved_cnt ==2) {
                    oledDrawString(0, 37, "3ed :");
                    recved_cnt ++;
                    oledRefresh();
                    oledDrawString(0, 45, (const char *)(uart_recv_buf_data+5));
                    oledRefresh();
                    uart_recv_flag = 0;
                    uart_recv_reset();
                    delay(100000);
                    //uart_send_Bty(hello_str, sizeof(hello_str));

                } else {
		            oledClear();
                    oledRefresh();
                    oledDrawString(0, 30, (const char *)uart_recv_buf_data);
                    oledRefresh();

                    uart_recv_flag = 0;
                    uart_recv_reset();
                    delay(100000);

                }


                 }
                    delay(100000);
        }
#endif
	    usbPoll();
	    //bithdapp();
	}

	return 0;
}


void UartDataRecvCallback(void)
{
    static unsigned char index = 0;
    int len = 0;
#define MAX_LEN 20
    unsigned char screen_line_str[MAX_LEN+1] = {0};
    hbto_uart_state_t *p_state = &hbto_uart_rxer.state;
    if (*p_state == HBTO_COMPLETE) {
        len = uart_recv_buf_len;
        index ++;
        index>=6?index=1:(void)index;
        oledClear();
        oledRefresh();
        //oledDrawStringCenter(1, "first received:");
        oledDrawString(0, 0, "Received data:");
        oledRefresh();
        index = 0;
        do {
            memset(screen_line_str,0,MAX_LEN +1);
            memcpy(screen_line_str, (const char *)uart_recv_buf_data+index*MAX_LEN, MAX_LEN);
            //oledDrawString(0, 10*index, (const char *)uart_recv_buf_data+index*15);
            oledDrawString(0, 10*(index+1), screen_line_str);
            len-=MAX_LEN;
            index++;
        } while(len>=MAX_LEN);

            memset(screen_line_str,0,MAX_LEN +1);
            //memcpy(screen_line_str, (const char *)uart_recv_buf_data+index*15, len);
            memcpy(screen_line_str, (const char *)uart_recv_buf_data+index*MAX_LEN, MAX_LEN);
            oledDrawString(0, 10*(index+1), screen_line_str);
            //len-=15;


        oledRefresh();
#if 0
        delay(100000);
#endif
#if 1//real
        msg_read(uart_recv_buf_data, 64);//refer to main_rx_callback
#else
        //uart_recv_reset();
        //delay(100000);
        //uart_send_Bty(hello_str, sizeof(hello_str));


        //unsigned char array_uart_out[64] = "upate key message\r\n";
        unsigned char array_uart_out[65] = "1234567890123456789012345678901234567890123456789012345678901234";
        memcpy(array_uart_out,uart_recv_buf_data,uart_recv_buf_len);
        unsigned char *p_in = array_uart_out;

        uart_recv_reset();
        unsigned short len = strlen(array_uart_out);
        //hbto_uart_send(p_in, len);
        hbto_uart_send(p_in, 64);
#endif
#if 0
        delay(100000);
#endif
        uart_recv_reset();

    } else {
        //oledDrawString(0, 30, "not complete");
        //oledRefresh();

    }
}

