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

#include <stdint.h>
#include <string.h>

#include "signatures.h"
#include "ecdsa.h"
#include "secp256k1.h"
#include "sha2.h"
#include "bootloader.h"

#define PUBKEYS 5

static const uint8_t *pubkey[PUBKEYS] = {

	(uint8_t *)"\x04\x8A\x6D\xEE\xCF\x3C\x24\x3C\xA3\x73\x89\x7A\x50\x4B\x69\x10\xBE\x96\x7C\xE5\x11\xB7\xE0\x8D\xBC\x2C\xB2\x3B\x9A\x11\x0F\x98\xE5\x23\xA1\x7A\xDD\xED\x4C\x2A\x13\x3B\x2C\xBD\x7D\xF0\x65\xED\xC8\x04\x25\xCA\xE7\x1C\x90\x27\x44\x69\xE1\x7E\x0F\x63\x17\x02\xD2",
	(uint8_t *)"\x04\xE4\xF3\x7F\x1C\x2B\xEF\x33\x91\xD2\xD0\x01\x71\x07\x74\x10\xF5\xBB\x68\x02\xAB\x6E\x46\x40\x6A\x9C\x4F\x83\x4E\x73\x3E\x2C\xB7\x7D\x57\xF4\xCB\xF3\x5F\x85\x92\xBD\xE2\x01\xE6\x4B\x4A\xC8\xC0\x62\xAB\xB8\x6E\x45\x12\xA4\xAF\x34\xDE\x6E\xE8\x3A\x83\xB1\x9F",
	(uint8_t *)"\x04\x68\x30\x2E\x39\x02\x2B\xA9\xDE\x17\x81\xA8\x80\xED\x0C\xF0\x74\x1D\x57\x61\xBA\x53\x4D\xE9\x2F\x5B\xD8\xA8\x84\xFB\xDD\x7F\xEB\x05\xD4\x80\x8D\xF2\x48\xA2\x16\x1D\xAB\xF7\x89\xD8\x18\x88\x97\xF3\x2F\x40\x25\x7F\x53\xE5\xAE\xF1\x21\x19\x47\xF4\xDA\xCC\x35",
	(uint8_t *)"\x04\x1E\x61\xD0\x82\x4D\x98\x96\xF5\x7B\x6D\x5D\x0B\xC5\x3B\x6E\x72\xA7\xD7\xCD\xFA\x92\xF3\xAF\x4B\x48\x91\x69\x89\x39\x13\x0B\x41\x87\x9D\x98\xB9\xE0\x24\xE4\xEA\xF3\xB1\x54\xDE\xB2\x14\x99\x27\xA0\x7C\xF2\x3C\x43\x40\xB8\xD2\xDD\x5F\xD5\x95\xDF\xC7\x13\x71",
	(uint8_t *)"\x04\xCF\x97\xF4\x76\xD5\x84\xDD\x2C\x0F\x61\x32\x15\x99\xF1\x62\x0C\xF4\xF1\x1A\xF4\xCF\x6E\x0F\xBD\x17\x24\xA1\x60\x8C\x48\x99\xDA\x6A\xA7\xC4\x51\xC2\xF8\xAE\x3F\xEE\x92\x48\x89\xF2\x84\xAC\x48\x52\xEA\xAD\xC6\x44\xFA\x9B\x98\x8E\xD2\xD3\xD1\x31\x85\xD6\xF6",
};

#define SIGNATURES 3

int signatures_ok(uint8_t *store_hash)
{
	const uint32_t codelen = *((const uint32_t *)FLASH_META_CODELEN);
	const uint8_t sigindex1 = *((const uint8_t *)FLASH_META_SIGINDEX1);
	const uint8_t sigindex2 = *((const uint8_t *)FLASH_META_SIGINDEX2);
	const uint8_t sigindex3 = *((const uint8_t *)FLASH_META_SIGINDEX3);

	uint8_t hash[32];
	sha256_Raw((const uint8_t *)FLASH_APP_START, codelen, hash);
	if (store_hash) {
		memcpy(store_hash, hash, 32);
	}

	if (sigindex1 < 1 || sigindex1 > PUBKEYS) return 0; // invalid index
	if (sigindex2 < 1 || sigindex2 > PUBKEYS) return 0; // invalid index
	if (sigindex3 < 1 || sigindex3 > PUBKEYS) return 0; // invalid index

	if (sigindex1 == sigindex2) return 0; // duplicate use
	if (sigindex1 == sigindex3) return 0; // duplicate use
	if (sigindex2 == sigindex3) return 0; // duplicate use

	if (ecdsa_verify_digest(&secp256k1, pubkey[sigindex1 - 1], (const uint8_t *)FLASH_META_SIG1, hash) != 0) { // failure
		return 0;
	}
	if (ecdsa_verify_digest(&secp256k1, pubkey[sigindex2 - 1], (const uint8_t *)FLASH_META_SIG2, hash) != 0) { // failure
		return 0;
	}
	if (ecdsa_verify_digest(&secp256k1, pubkey[sigindex3 - 1], (const uint8_t *)FLASH_META_SIG3, hash) != 0) { // failture
		return 0;
	}

	return 1;
}
