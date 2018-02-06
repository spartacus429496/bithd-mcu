#!/usr/bin/python
from __future__ import print_function
import argparse
import hashlib
import struct
import binascii
import ecdsa

try:
    raw_input
except:
    raw_input = input

SLOTS = 3

signingbuf={
    1:'5CC814E31F8EF9A58281D29C05E98CBFCF32438D9E332EF8581A62876E5F91518C91BFD4AAB65885A9BF58870B1F14F3FE0947C0D9F14CB7E87929E8222614EB',
    2:'3BD9622BC72033ECF45F0E65E4305FAC142CC4E0AF34CBC0CE611AD3D04123DE6EB07A554F75B19C86883E6D7AEECA3CB1881F4EE7AF5F588AFA74EA8669ABB0',
    3:'3BD9622BC72033ECF45F0E65E4305FAC142CC4E0AF34CBC0CE611AD3D04123DE6EB07A554F75B19C86883E6D7AEECA3CB1881F4EE7AF5F588AFA74EA8669ABB0',
    
    #1:'8707b9d16446eee43efb1c1725dd072f41b96511ea81003980bb69da6a9176f5ac0e1a2ff8398130bf61375834a99d04bbb92d270885961c5b8cb10ede0e2abd',
    #2:'d4c814f74a31281640754697f0435e470dad22140e5bb76e55aae6efcd70471f1df21bf7157a74a94eac03d644e0eca61e6fbe32a41349b9e473bf33df0f4825',
    #3:'c3bd7dc34cbaf56b8b338b0ab533ee437d264abc444862e87644c428a3ec3708968b3697e2b0efaabac619713c564463b217f971b318c2eb79081d75ad5b6836',
}

pubkeys = {
    1: '041E61D0824D9896F57B6D5D0BC53B6E72A7D7CDFA92F3AF4B4891698939130B41879D98B9E024E4EAF3B154DEB2149927A07CF23C4340B8D2DD5FD595DFC71371',
    2: '048EF545ED4CE4EDE4231DAFBFE510FCE5C75ABFBC98D99459A4F875D5685D31E59112A38742A1002EE4BB98A097BE41DBAFC17044C914192B9BA88F28F75B2844',
    3: '048EF545ED4CE4EDE4231DAFBFE510FCE5C75ABFBC98D99459A4F875D5685D31E59112A38742A1002EE4BB98A097BE41DBAFC17044C914192B9BA88F28F75B2844',
    
    #1: '045601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7ccc136c1dc0cbeb930e9e298043589351d81d8e0bc736ae2a1f5192e5e8b061d58',
    #2: '042b4ea0a797a443d293ef5cff444f4979f06acfebd7e86d277475656138385b6c85e89bc037945d93b343083b5a1c86131a01f60c50269763b570c854e5c09b7a',
    #3: '044ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c9712ba26dcb10ec1625da61fa10a844c676162948271d96967450288ee9233dc3a',
    4: '045601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7ccc136c1dc0cbeb930e9e298043589351d81d8e0bc736ae2a1f5192e5e8b061d58',
    5: '045601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7ccc136c1dc0cbeb930e9e298043589351d81d8e0bc736ae2a1f5192e5e8b061d58',
}

INDEXES_START = len('TRZR') + struct.calcsize('<I')
SIG_START = INDEXES_START + SLOTS + 1 + 52

def parse_args():
    parser = argparse.ArgumentParser(description='Commandline tool for signing Trezor firmware.')
    parser.add_argument('-f', '--file', dest='path', help="Firmware file to modify")
    parser.add_argument('-s', '--sign', dest='sign', action='store_true', help="Add signature to firmware slot")
    parser.add_argument('-p', '--pem', dest='pem', action='store_true', help="Use PEM instead of SECEXP")
    parser.add_argument('-g', '--generate', dest='generate', action='store_true', help='Generate new ECDSA keypair')

    return parser.parse_args()

def prepare(data):
    # Takes raw OR signed firmware and clean out metadata structure
    # This produces 'clean' data for signing

    meta = b'TRZR'  # magic
    if data[:4] == b'TRZR':
        meta += data[4:4 + struct.calcsize('<I')]
    else:
        meta += struct.pack('<I', len(data))  # length of the code
    meta += b'\x00' * SLOTS  # signature index #1-#3
    meta += b'\x01'       # flags
    meta += b'\x00' * 52  # reserved
    meta += b'\x00' * 64 * SLOTS  # signature #1-#3

    if data[:4] == b'TRZR':
        # Replace existing header
        out = meta + data[len(meta):]
    else:
        # create data from meta + code
        out = meta + data

    return out

def check_signatures(data):
    # Analyses given firmware and prints out
    # status of included signatures

    try:
        indexes = [ ord(x) for x in data[INDEXES_START:INDEXES_START + SLOTS] ]
    except:
        indexes = [ x for x in data[INDEXES_START:INDEXES_START + SLOTS] ]

    to_sign = prepare(data)[256:] # without meta
    fingerprint = hashlib.sha256(to_sign).hexdigest()

    print("Firmware fingerprint:", fingerprint)

    used = []
    for x in range(SLOTS):
        signature = data[SIG_START + 64 * x:SIG_START + 64 * x + 64]

        if indexes[x] == 0:
            print("Slot #%d" % (x + 1), 'is empty')
        else:
            pk = pubkeys[indexes[x]]
            verify = ecdsa.VerifyingKey.from_string(binascii.unhexlify(pk)[1:],
                        curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

            try:
                verify.verify(signature, to_sign, hashfunc=hashlib.sha256)

                if indexes[x] in used:
                    print("Slot #%d signature: DUPLICATE" % (x + 1), binascii.hexlify(signature))
                else:
                    used.append(indexes[x])
                    print("Slot #%d signature: VALID" % (x + 1), binascii.hexlify(signature))

            except:
                print("Slot #%d signature: INVALID" % (x + 1), binascii.hexlify(signature))


def modify(data, slot, index, signature):
    # Replace signature in data
    # Put index to 
    print("signature:")
    print(binascii.hexlify(signature))
    data = data[:INDEXES_START + slot - 1 ] + chr(index) + data[INDEXES_START + slot:]

    # Put signature to data
    data = data[:SIG_START + 64 * (slot - 1) ] + signature + data[SIG_START + 64 * slot:]
    print("slot:",slot)
    print("data:",binascii.hexlify(data[SIG_START:SIG_START+192]))
    return data

def sign(data, is_pem):
    # Ask for index and private key and signs the firmware

    slot = int(raw_input('Enter signature slot (1-%d): ' % SLOTS))
    if slot < 1 or slot > SLOTS:
        raise Exception("Invalid slot")

    if is_pem:
        print("Paste ECDSA private key in PEM format and press Enter:")
        print("(blank private key removes the signature on given index)")
        pem_key = ''
        while True:
            key = raw_input()
            pem_key += key + "\n"
            if key == '':
                break
        if pem_key.strip() == '':
            # Blank key,let's remove existing signature from slot
            return modify(data, slot, 0, '\x00' * 64)
        key = ecdsa.SigningKey.from_pem(pem_key)
    else:
        print("Paste SECEXP (in hex) and press Enter:")
        print("(blank private key removes the signature on given index)")
        secexp = raw_input()
        if secexp.strip() == '':
            # Blank key,let's remove existing signature from slot
            return modify(data, slot, 0, '\x00' * 64)
        key = ecdsa.SigningKey.from_secret_exponent(secexp = int(secexp, 16), curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

    to_sign = prepare(data)[256:] # without meta

    # Locate proper index of current signing key
    pubkey = b'04' + binascii.hexlify(key.get_verifying_key().to_string())
    index = None
    for i, pk in pubkeys.items():
        if pk == pubkey:
            index = i
            break

    if index == None:
        raise Exception("Unable to find private key index. Unknown private key?")

    signature = key.sign_deterministic(to_sign, hashfunc=hashlib.sha256)
    print("slot",slot)
    print("index",index)
    return modify(data, slot, index, signature)

def main(args):
    if args.generate:
        print("BITHD produce")
        for slot in range(1,4):
            data = open(args.path, 'rb').read()
            if data[:4] != b'TRZR':
                print("Metadata has been added...")
                data = prepare(data)

            if data[:4] != b'TRZR':
                raise Exception("Firmware header expected")
            data =modify(data, slot, slot, signingbuf[slot].decode("hex"))
            print("datasave:",binascii.hexlify(data[SIG_START:SIG_START+192]))
            fp = open(args.path, 'wb')
            fp.write(data)
            fp.close()
        #key = ecdsa.SigningKey.generate(
        #    curve=ecdsa.curves.SECP256k1,
        #    hashfunc=hashlib.sha256)

        #print("PRIVATE KEY (SECEXP):")
        #print(binascii.hexlify(key.to_string()))
        #print()

        #print("PRIVATE KEY (PEM):")
        #print(key.to_pem())

        #print("PUBLIC KEY:")
        #print('04' + binascii.hexlify(key.get_verifying_key().to_string()))
        return

    if not args.path:
        raise Exception("-f/--file is required")

    data = open(args.path, 'rb').read()
    print(args.path)
    assert len(data) % 4 == 0

    if data[:4] != b'TRZR':
        print("Metadata has been added...")
        data = prepare(data)

    if data[:4] != b'TRZR':
        raise Exception("Firmware header expected")

    print("Firmware size %d bytes" % len(data))

    check_signatures(data)

    if args.sign:
        data = sign(data, args.pem)
        check_signatures(data)

    fp = open(args.path, 'wb')
    fp.write(data)
    fp.close()

if __name__ == '__main__':
    args = parse_args()
    chose = int(input("chose porduce/debug (1/0):"))
    if chose == 1:
        args.generate=1
    main(args)
