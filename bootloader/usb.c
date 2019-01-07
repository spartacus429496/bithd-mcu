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

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/stm32/flash.h>

#include <string.h>

#include "buttons.h"
#include "bootloader.h"
#include "oled.h"
#include "rng.h"
#include "usb.h"
#include "serialno.h"
#include "layout.h"
#include "util.h"
#include "signatures.h"
#include "sha2.h"
#include "ecdsa.h"
#include "secp256k1.h"

#include "../timerbitpie.h"
#include "uart.h"

extern void SuccessAck(void);
extern unsigned char needsuccessack_flag;

#define ENDPOINT_ADDRESS_IN         (0x81)
#define ENDPOINT_ADDRESS_OUT        (0x01)

static bool brand_new_firmware;
#if 0
static const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x534c,
	.idProduct = 0x0001,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};
#endif

static const uint8_t hid_report_descriptor[] = {
	0x06, 0x00, 0xff,  // USAGE_PAGE (Vendor Defined)
	0x09, 0x01,        // USAGE (1)
	0xa1, 0x01,        // COLLECTION (Application)
	0x09, 0x20,        // USAGE (Input Report Data)
	0x15, 0x00,        // LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,  // LOGICAL_MAXIMUM (255)
	0x75, 0x08,        // REPORT_SIZE (8)
	0x95, 0x40,        // REPORT_COUNT (64)
	0x81, 0x02,        // INPUT (Data,Var,Abs)
	0x09, 0x21,        // USAGE (Output Report Data)
	0x15, 0x00,        // LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,  // LOGICAL_MAXIMUM (255)
	0x75, 0x08,        // REPORT_SIZE (8)
	0x95, 0x40,        // REPORT_COUNT (64)
	0x91, 0x02,        // OUTPUT (Data,Var,Abs)
	0xc0               // END_COLLECTION
};

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0111,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

static const struct usb_endpoint_descriptor hid_endpoints[2] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = ENDPOINT_ADDRESS_IN,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = ENDPOINT_ADDRESS_OUT,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct usb_interface_descriptor hid_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_HID,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
	.endpoint = hid_endpoints,
	.extra = &hid_function,
	.extralen = sizeof(hid_function),
}};

#if 0
static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = hid_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,
	.interface = ifaces,
};
#endif

// static const char *usb_strings[] = {
// 	"SatoshiLabs",
// 	"TREZOR",
// 	"", // empty serial
// };

// static int hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len, usbd_control_complete_callback *complete)
// {
// 	(void)complete;
// 	(void)dev;

// 	if ((req->bmRequestType != 0x81) ||
// 	    (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
// 	    (req->wValue != 0x2200))
// 		return 0;

// 	/* Handle the HID report descriptor. */
// 	*buf = (uint8_t *)hid_report_descriptor;
// 	*len = sizeof(hid_report_descriptor);

// 	return 1;
// }

enum {
	STATE_READY,
	STATE_OPEN,
	STATE_FLASHSTART,
	STATE_FLASHING,
	STATE_CHECK,
	STATE_END,
};


extern unsigned char Data_64Bytes[UartDataMax];
static uint32_t flash_pos = 0, flash_len = 0;
static char flash_state = STATE_READY;
static uint16_t msg_id = 0xFFFF;
static uint32_t msg_size = 0;

static uint8_t meta_backup[FLASH_META_LEN];

static void send_msg_success(void)
{
	
  unsigned char data_64Bytes[]={'?','#','#',0x00,0x02,
							  0x00,0x00,0x00,0x00,
							  0x00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
							  0x00,0x00,0x00,0,0x00,0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
							  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
  memcpy(Data_64Bytes,data_64Bytes,sizeof(data_64Bytes));
  CmdSendUart(1,Data_64Bytes,64);
}


static void send_msg_failure(void)
{
	// send response: Failure message (id 3), payload len 2
		// code = 99 (Failure_FirmwareError)

	  unsigned char data_64Bytes[]={'?','#','#',
	                              0x00,0x03,
								  0x00,0x00,0x00,0x02,
								  0x08,0x63,
								  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
								  0x00,0x00,0x00,0,0x00,0,0x00,
								  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
								  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
								  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	                                                          };
	  memcpy(Data_64Bytes,data_64Bytes,sizeof(data_64Bytes));
		CmdSendUart(1,Data_64Bytes,64);

}

static void send_msg_features(void)
{
	// response: Features message (id 17), payload len 30
	//           - vendor = "bitcointrezor.com"
	//           - major_version = VERSION_MAJOR
	//           - minor_version = VERSION_MINOR
	//           - patch_version = VERSION_PATCH
	//           - bootloader_mode = True
	//           - firmware_present = True/False


	  unsigned char data_64Bytes[]={'?','#','#',
                              0x00,0x11,
							  0x00,0x00,0x00,0x1e,
							  0x0a,0x11,
							  'b','i','t','c','o','i','n','t','r','e','z','o','r','.','c','o','m',0x10,
							  0x01,0x18,0x03,0x20,0x03,0x28,0x01,
							  0x90,0x01,0x01,
							  0x00,0x00,0x00,0x00,0x00,0x00,
							  0x00,0x00,0x00,0x00,0x00,0x00,
							  0x00,0x00,0x00,0x00,0x00,0x00,
							  0x00,0x00,0x00,0x00,0x00,0x00,0x00
                                                          };

		
		if (brand_new_firmware) 
		{
			data_64Bytes[38]=0x00;
		}

  memcpy(Data_64Bytes,data_64Bytes,sizeof(data_64Bytes));
  CmdSendUart(1,Data_64Bytes,64);
}

static void send_msg_buttonrequest_firmwarecheck(void)
{
	// send response: ButtonRequest message (id 26), payload len 2
		// code = ButtonRequest_FirmwareCheck (9)
	  unsigned char data_64Bytes[]={'?','#','#',
	                                                          0x00,0x1a,
								  0x00,0x00,0x00,0x02,
								  0x08,0x09,
								  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
								  0x00,0x00,0x00,0,0x00,0,0x00,
								  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
								  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
								  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	                                                          };
	  memcpy(Data_64Bytes,data_64Bytes,sizeof(data_64Bytes));
		CmdSendUart(1,Data_64Bytes,64);
}

static void erase_metadata_sectors(void)
{
	flash_unlock();
	for (int i = FLASH_META_SECTOR_FIRST; i <= FLASH_META_SECTOR_LAST; i++) {
		flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
	}
	flash_lock();
}

static void backup_metadata(uint8_t *backup)
{
	memcpy(backup, (void *)FLASH_META_START, FLASH_META_LEN);
}

static void restore_metadata(const uint8_t *backup)
{
	flash_unlock();
	for (int i = 0; i < FLASH_META_LEN / 4; i++) {
		const uint32_t *w = (const uint32_t *)(backup + i * 4);
		flash_program_word(FLASH_META_START + i * 4, *w);
	}
	flash_lock();
}

void Datas_rx_callback(unsigned char *buf,unsigned short length)
{
  unsigned short j,i=0;

  j=length/64;
  for(i=0;i<j;i++)
    {
      _rx_callback(&buf[i*64]);
    }

}

void _rx_callback(unsigned char* buf)
{

	static uint8_t towrite[4] __attribute__((aligned(4)));
	static int wi;


	if (flash_state == STATE_END) {
		return;
	}

	if (flash_state == STATE_READY || flash_state == STATE_OPEN || flash_state == STATE_FLASHSTART || flash_state == STATE_CHECK) {
		if (buf[0] != '?' || buf[1] != '#' || buf[2] != '#') {	// invalid start - discard
			return;
		}
		// struct.unpack(">HL") => msg, size
		msg_id = (buf[3] << 8) + buf[4];
		msg_size = (buf[5] << 24) + (buf[6] << 16) + (buf[7] << 8) + buf[8];
	}

	if (flash_state == STATE_READY || flash_state == STATE_OPEN) {
		if (msg_id == 0x0000) {		// Initialize message (id 0)
			send_msg_features();
			flash_state = STATE_OPEN;
			return;
		}
		if (msg_id == 0x0001) {		// Ping message (id 1)
			send_msg_success();
			return;
		}
		if (msg_id == 0x0020) {		// SelfTest message (id 32)

			// USB TEST
			layoutProgress("TESTING USB ...", 0);
			bool status_usb = (buf[9] == 0x0a) && (buf[10] == 53) && (0 == memcmp(buf + 11, "\x00\xFF\x55\xAA\x66\x99\x33\xCC" "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!" "\x00\xFF\x55\xAA\x66\x99\x33\xCC", 53));

			// RNG TEST
			layoutProgress("TESTING RNG ...", 250);
			uint32_t cnt[256];
			memset(cnt, 0, sizeof(cnt));
			for (int i = 0; i < (256 * 2000); i++) {
				uint32_t r = random32();
				cnt[r & 0xFF]++;
				cnt[(r >> 8) & 0xFF]++;
				cnt[(r >> 16) & 0xFF]++;
				cnt[(r >> 24) & 0xFF]++;
			}
			bool status_rng = true;
			for (int i = 0; i < 256; i++) {
				status_rng = status_rng && (cnt[i] >= 7600) && (cnt[i] <= 8400);
			}

			// CPU TEST
			layoutProgress("TESTING CPU ...", 500);
			// privkey :   e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
			// pubkey  : 04a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd
			//             5b8dec5235a0fa8722476c7709c02559e3aa73aa03918ba2d492eea75abea235
			// digest  :   c84a4cc264100070c8be2acf4072efaadaedfef3d6209c0fe26387e6b1262bbf
			// sig:    :   f7869c679bbed1817052affd0264ccc6486795f6d06d0c187651b8f3863670c8
			//             2ccf89be32a53eb65ea7c007859783d46717986fead0833ec60c5729cdc4a9ee
			bool status_cpu = (0 == ecdsa_verify_digest(&secp256k1,
				(const uint8_t *)"\x04\xa3\x4b\x99\xf2\x2c\x79\x0c\x4e\x36\xb2\xb3\xc2\xc3\x5a\x36\xdb\x06\x22\x6e\x41\xc6\x92\xfc\x82\xb8\xb5\x6a\xc1\xc5\x40\xc5\xbd\x5b\x8d\xec\x52\x35\xa0\xfa\x87\x22\x47\x6c\x77\x09\xc0\x25\x59\xe3\xaa\x73\xaa\x03\x91\x8b\xa2\xd4\x92\xee\xa7\x5a\xbe\xa2\x35",
				(const uint8_t *)"\xf7\x86\x9c\x67\x9b\xbe\xd1\x81\x70\x52\xaf\xfd\x02\x64\xcc\xc6\x48\x67\x95\xf6\xd0\x6d\x0c\x18\x76\x51\xb8\xf3\x86\x36\x70\xc8\x2c\xcf\x89\xbe\x32\xa5\x3e\xb6\x5e\xa7\xc0\x07\x85\x97\x83\xd4\x67\x17\x98\x6f\xea\xd0\x83\x3e\xc6\x0c\x57\x29\xcd\xc4\xa9\xee",
				(const uint8_t *)"\xc8\x4a\x4c\xc2\x64\x10\x00\x70\xc8\xbe\x2a\xcf\x40\x72\xef\xaa\xda\xed\xfe\xf3\xd6\x20\x9c\x0f\xe2\x63\x87\xe6\xb1\x26\x2b\xbf"));

			// FLASH TEST
			layoutProgress("TESTING FLASH ...", 750);

			// backup metadata
			backup_metadata(meta_backup);

			// write test pattern
			erase_metadata_sectors();
			flash_unlock();
			for (int i = 0; i < FLASH_META_LEN / 4; i++) {
				flash_program_word(FLASH_META_START + i * 4, 0x3C695A0F);
			}
			flash_lock();

			// compute hash of written test pattern
			uint8_t hash[32];
			sha256_Raw((unsigned char *)FLASH_META_START, FLASH_META_LEN, hash);

			// restore metadata from backup
			erase_metadata_sectors();
			restore_metadata(meta_backup);
			memset(meta_backup, 0, sizeof(meta_backup));

			// compare against known hash computed via the following Python3 script:
			// hashlib.sha256(binascii.unhexlify('0F5A693C' * 8192)).hexdigest()
			bool status_flash = (0 == memcmp(hash, "\xa6\xc2\x25\xa4\x76\xa1\xde\x76\x09\xe0\xb0\x07\xf8\xe2\x5a\xec\x1d\x75\x8d\x5c\x36\xc8\x4a\x6b\x75\x4e\xd5\x3d\xe6\x99\x97\x64", 32));

			bool status_all = status_usb && status_rng && status_cpu && status_flash;

			if (status_all) {
				send_msg_success();
			} else {
				send_msg_failure();
			}
			layoutDialog(status_all ? &bmp_icon_info : &bmp_icon_error,
				NULL, NULL, NULL,
				status_usb   ? "Test USB ... OK"   : "Test USB ... Failed",
				status_rng   ? "Test RNG ... OK"   : "Test RNG ... Failed",
				status_cpu   ? "Test CPU ... OK"   : "Test CPU ... Failed",
				status_flash ? "Test FLASH ... OK" : "Test FLASH ... Failed",
				NULL,
				NULL
			);
			return;
		}
	}

	if (flash_state == STATE_OPEN) {
		if (msg_id == 0x0006) {		// FirmwareErase message (id 6)
			if (!brand_new_firmware) {
				layoutDialog(&bmp_icon_question, "Abort", "Continue", NULL, "Install new", "firmware?", NULL, "Never do this without", "your recovery card!", NULL);
				SuccessAck();
				do {
					delay(100000);
					buttonUpdate();
				} while (!button.YesUp && !button.NoUp);
			}
			delay(10000000);

			if (brand_new_firmware || button.YesUp) {
				// backup metadata
				backup_metadata(meta_backup);
				flash_unlock();
				// erase metadata area
				for (int i = FLASH_META_SECTOR_FIRST; i <= FLASH_META_SECTOR_LAST; i++) {
					layoutProgress("ERASING ... Please wait", 1000 * (i - FLASH_META_SECTOR_FIRST) / (FLASH_CODE_SECTOR_LAST - FLASH_META_SECTOR_FIRST));
					flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
				}
				for (int i = 0; i < (FLASH_META_LEN-FLASH_META_DESC_LEN)/ 4; i++) {
					const uint32_t *w = (const uint32_t *)(meta_backup + i * 4 + FLASH_META_DESC_LEN);
					flash_program_word(FLASH_META_START + i * 4+ FLASH_META_DESC_LEN, *w);
					}
				// erase code area
				for (int i = FLASH_CODE_SECTOR_FIRST; i <= FLASH_CODE_SECTOR_LAST; i++) {
					layoutProgress("ERASING ... Please wait", 1000 * (i - FLASH_META_SECTOR_FIRST) / (FLASH_CODE_SECTOR_LAST - FLASH_META_SECTOR_FIRST));
					flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
				}
				layoutProgress("INSTALLING ... Please wait", 0);
				flash_lock();
				send_msg_success();
				flash_state = STATE_FLASHSTART;
				return;
			}
			send_msg_failure();
			flash_state = STATE_END;
			layoutDialog(&bmp_icon_warning, NULL, NULL, NULL, "Firmware installation", "aborted.", NULL, "You may now", "Close your BITHD.", NULL);
			return;
		}
		return;
	}

	if (flash_state == STATE_FLASHSTART) {
		if (msg_id == 0x0007) {		// FirmwareUpload message (id 7)
			if (buf[9] != 0x0a) { // invalid contents
				send_msg_failure();
				flash_state = STATE_END;
				layoutDialog(&bmp_icon_error, NULL, NULL, NULL, "Error installing ", "firmware.", NULL, "Close your BITHD", "and try again.", NULL);
				return;
			}
			// read payload length
			uint8_t *p = buf + 10;
			flash_len = readprotobufint(&p);
			if (flash_len > FLASH_TOTAL_SIZE + FLASH_META_DESC_LEN - (FLASH_APP_START - FLASH_ORIGIN)) { // firmware is too big
				send_msg_failure();
				flash_state = STATE_END;
				layoutDialog(&bmp_icon_error, NULL, NULL, NULL, "Firmware is too big.", NULL, "Get official firmware", "from bithd.com/start", NULL, NULL);
				return;
			}
			flash_state = STATE_FLASHING;
			flash_pos = 0;
			wi = 0;
			flash_unlock();
			while (p < buf + 64) {
				towrite[wi] = *p;
				wi++;
				if (wi == 4) {
					const uint32_t *w = (uint32_t *)towrite;
					flash_program_word(FLASH_META_START + flash_pos, *w);
					flash_pos += 4;
					wi = 0;
				}
				p++;
			}
			flash_lock();
			return;
		}
		return;
	}

	if (flash_state == STATE_FLASHING) {
		if (buf[0] != '?') {	// invalid contents
			send_msg_failure();
			flash_state = STATE_END;
			layoutDialog(&bmp_icon_error, NULL, NULL, NULL, "Error installing ", "firmware.", NULL, "Close your BITHD", "and try again.", NULL);
			return;
		}
		const uint8_t *p = buf + 1;
		layoutProgress("INSTALLING ... Please wait", 1000 * flash_pos / flash_len);
		flash_unlock();
		while (p < buf + 64 && flash_pos < flash_len) {
			towrite[wi] = *p;
			wi++;
			if (wi == 4) {
				const uint32_t *w = (const uint32_t *)towrite;
				if (flash_pos < FLASH_META_DESC_LEN) {
					flash_program_word(FLASH_META_START + flash_pos, *w);			// the first 256 bytes of firmware is metadata descriptor
				} else {
					flash_program_word(FLASH_APP_START + (flash_pos - FLASH_META_DESC_LEN), *w);	// the rest is code
				}
				flash_pos += 4;
				wi = 0;
			}
			p++;
		}
		flash_lock();
		// flashing done
		if (flash_pos == flash_len) {
			flash_state = STATE_CHECK;
			if (!brand_new_firmware) {
				send_msg_buttonrequest_firmwarecheck();
				return;
			}
		} else {
			return;
		}
	}

	if (flash_state == STATE_CHECK) {

		if (!brand_new_firmware) {
				if (msg_id != 0x001B) {	// ButtonAck message (id 27)
					return;
				}
				SuccessAck();
				delay(100000);
				layoutProgress("INSTALLING ... Please wait", 1000);
				delay(10000000);
				if (!signatures_ok(NULL)) 
				{	// erase code area
				    layoutDialog(&bmp_icon_warning, NULL, NULL, NULL, "Firmware installation", "aborted.", NULL, "You need to repeat", "the procedure with", "the correct firmware.");
					flash_unlock();
					for (int i = FLASH_CODE_SECTOR_FIRST; i <= FLASH_CODE_SECTOR_LAST; i++) {
						flash_erase_sector(i, FLASH_CR_PROGRAM_X32);
					}
					flash_lock();
		        	send_msg_failure();
				}
				else
				{
					layoutDialog(&bmp_icon_ok, NULL, NULL, NULL, "New firmware", "successfully installed.", NULL, "You may now", "Close your BITHD.", NULL);
					send_msg_success();
					memset(meta_backup, 0, sizeof(meta_backup));
					// button.YesDown = 0;
					// button.YesUp = true;
				}
            }

		// bool hash_check_ok = brand_new_firmware || button.YesUp;

		
		// uint8_t flags = *((uint8_t *)FLASH_META_FLAGS);
		// check if to restore old storage area but only if signatures are ok
		// if ((flags & 0x01) && signatures_ok(NULL)) {
		// 	// copy new stuff
		// 	memcpy(meta_backup, (void *)FLASH_META_START, FLASH_META_DESC_LEN);
		// 	// replace "TRZR" in header with 0000 when hash not confirmed
		// 	if (!hash_check_ok) {
		// 		meta_backup[0] = 0;
		// 		meta_backup[1] = 0;
		// 		meta_backup[2] = 0;
		// 		meta_backup[3] = 0;
		// 	}
		// 	// erase storage
		// 	erase_metadata_sectors();
		// 	// restore metadata from backup
		// 	restore_metadata(meta_backup);
		// 	memset(meta_backup, 0, sizeof(meta_backup));
		// } else {
		// 	// replace "TRZR" in header with 0000 when hash not confirmed
		// 	if (!hash_check_ok) {
		// 		// no need to erase, because we are just erasing bits
		// 		flash_unlock();
		// 		flash_program_word(FLASH_META_START, 0x00000000);
		// 		flash_lock();
		// 	}
		// }
		flash_state = STATE_END;
		// if (hash_check_ok) {
		// 	layoutDialog(&bmp_icon_ok, NULL, NULL, NULL, "New firmware", "successfully installed.", NULL, "You may now", "Close your BITHD.", NULL);
		// 	send_msg_success();
		// } else {
		// 	layoutDialog(&bmp_icon_warning, NULL, NULL, NULL, "Firmware installation", "aborted.", NULL, "You need to repeat", "the procedure with", "the correct firmware.");
		// 	send_msg_failure();
		// }
		return;
	}

}

// static void hid_set_config(usbd_device *dev, uint16_t wValue)
// {
// 	(void)wValue;

// 	usbd_ep_setup(dev, ENDPOINT_ADDRESS_IN, USB_ENDPOINT_ATTR_INTERRUPT, 64, 0);
// 	usbd_ep_setup(dev, ENDPOINT_ADDRESS_OUT, USB_ENDPOINT_ATTR_INTERRUPT, 64, hid_rx_callback);

// 	usbd_register_control_callback(
// 		dev,
// 		USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
// 		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
// 		hid_control_request
// 	);
// }

// static usbd_device *usbd_dev;
// static uint8_t usbd_control_buffer[128];

void checkButtons(void)
{
	static bool btn_left = false, btn_right = false, btn_final = false;
	if (btn_final) {
		return;
	}
	uint16_t state = gpio_port_read(BTN_PORT);
	if ((state & (BTN_PIN_YES | BTN_PIN_NO)) != (BTN_PIN_YES | BTN_PIN_NO)) {
		if ((state & BTN_PIN_NO) != BTN_PIN_NO) {
			btn_left = true;
		}
		if ((state & BTN_PIN_YES) != BTN_PIN_YES) {
			btn_right = true;
		}
	}
	if (btn_left) {
		oledBox(0, 0, 3, 3, true);
	}
	if (btn_right) {
		oledBox(OLED_WIDTH - 4, 0, OLED_WIDTH - 1, 3, true);
	}
	if (btn_left || btn_right) {
		oledRefresh();
	}
	if (btn_left && btn_right) {
		btn_final = true;
	}
}

void usbLoop(bool firmware_present)
{
	brand_new_firmware = !firmware_present;
	// usbd_dev = usbd_init(&otgfs_usb_driver, &dev_descr, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	// usbd_register_set_config_callback(usbd_dev, hid_set_config);
	for (;;) {
		// usbd_poll(usbd_dev);
		UartDataSendrecive();
		if (brand_new_firmware && (flash_state == STATE_READY || flash_state == STATE_OPEN)) {
			checkButtons();
		}
	}
}

//////////////////////////////////////////////////////////////////
const BITMAP *BitpieASIIC_abc[26]={
&bitpieAcii_a,&bitpieAcii_b,&bitpieAcii_c,&bitpieAcii_d,&bitpieAcii_e,
&bitpieAcii_f,&bitpieAcii_g,&bitpieAcii_h,&bitpieAcii_i,&bitpieAcii_j,
&bitpieAcii_k,&bitpieAcii_l,&bitpieAcii_m,&bitpieAcii_n,&bitpieAcii_o,
&bitpieAcii_p,&bitpieAcii_q,&bitpieAcii_r,&bitpieAcii_s,&bitpieAcii_t,
&bitpieAcii_u,&bitpieAcii_v,&bitpieAcii_w,&bitpieAcii_x,&bitpieAcii_y,
&bitpieAcii_z,
};
const BITMAP *BitpieASIIC_ABC[26]={
&bitpieAcii_A,&bitpieAcii_B,&bitpieAcii_C,&bitpieAcii_D,&bitpieAcii_E,
&bitpieAcii_F,&bitpieAcii_G,&bitpieAcii_H,&bitpieAcii_I,&bitpieAcii_J,&bitpieAcii_K,
&bitpieAcii_L,&bitpieAcii_M,&bitpieAcii_N,&bitpieAcii_O,&bitpieAcii_P,&bitpieAcii_Q,
&bitpieAcii_R,&bitpieAcii_S,&bitpieAcii_T,&bitpieAcii_U,&bitpieAcii_V,&bitpieAcii_W,
&bitpieAcii_X,&bitpieAcii_Y,&bitpieAcii_Z,
};
const BITMAP *BitpieDigits1632[12]={
&bitpie16_32_digit0, &bitpie16_32_digit1, &bitpie16_32_digit2, &bitpie16_32_digit3, &bitpie16_32_digit4,
&bitpie16_32_digit5, &bitpie16_32_digit6, &bitpie16_32_digit7, &bitpie16_32_digit8, &bitpie16_32_digit9,
&bitpie16_32_digit_,&bitpie16_32_digit_no
};

void display_str_oled(unsigned char x,unsigned char y,unsigned char* strp,unsigned char length)
{
	unsigned char i;
	unsigned char xy=x;
    for(i=0;i<length;i++)
	{
        if(strp[i]>0x5B){oledDrawBitmap(xy,y, BitpieASIIC_abc[(strp[i]-0x61)]);}
        else{oledDrawBitmap(xy,y, BitpieASIIC_ABC[(strp[i]-0x41)]);}
        xy=xy+8;
	}
}

void blueParingdisplay(unsigned char* buf)
{
    unsigned char xy=0;
    unsigned char i;
    unsigned char pair[]="Pair";
    unsigned char password[]="Password";	

    oledClear();
    display_str_oled(12,0,pair,sizeof(pair)-1); 
    xy=(sizeof(pair))*8;
    display_str_oled(12,0,password,sizeof(password)-1); 

    xy=16;
    for(i=0;i<6;i++)
    {
        oledDrawBitmap(xy,24,BitpieDigits1632[(buf[i]&0x0f)]);
        xy=xy+16;
    }

	oledRefresh();

}
