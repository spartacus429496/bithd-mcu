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

		oledDrawStringCenter(0, "hello BACKUP!");
		oledRefresh();
#endif
    //unsigned char ack_succs[3]={0x5a,0xa5,0x00};
    unsigned char hello_str[] = "hello, world!\n";
    for (;;) {
        for (uint16_t i=0;i<10;i++) {
            //uart_send_Bty(ack_succs,3);//串口发送 成功ACK
            uart_send_Bty(hello_str, sizeof(hello_str));

        }
        //while(1);
        for (;;){};
		usbPoll();
	  bithdapp();
	}

	return 0;
}
