#!/usr/bin/env python3
"""Write synthetic format seeds only; contains no executable firmware."""
import pathlib
import struct
import sys
out = pathlib.Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
def put(b, off, value):
    struct.pack_into('<I', b, off, value)
for version in (2, 3):
    r = bytearray(2048)
    r[0:2] = b'\x55\xaa'; r[24] = 32
    r[32:36] = b'PCIR'; r[48] = 4; r[53] = 128
    r[64:70] = b'\xff\xb8BIT\0'
    r[72:75] = bytes((12, 6, 1)); r[75] = (-sum(r[64:75])) & 255
    r[76:82] = bytes((0x70, 2, 4, 0, 96, 0))
    put(r, 96, 128); r[128:132] = bytes((1, 6, 6, 1))
    r[134] = 0x85; put(r, 136, 256)
    if version == 2:
        for off, val in {0:(60<<16)|(2<<8)|1,4:512,24:256,36:256,40:256,48:256}.items():
            put(r, 256+off, val)
    else:
        for off, val in {0:(428<<16)|(3<<8)|1,4:768,20:256,32:512}.items():
            put(r, 256+off, val)
        r[292:297] = bytes((1,0,1,1,1))
    (out / f'fwsec-v{version}').write_bytes(r)
b = bytearray(72+59136+16)
fields = [0x314c5442,1,0,256,34048,25088,1,256,33792,256,0,34560,13,1,1,16,59136,16]
for i,v in enumerate(fields): put(b,4*i,v)
(out/'booter').write_bytes(b)
b=bytearray(76); put(b,0,4); (out/'boot-v4').write_bytes(b)
b=bytearray(256); b[:7]=b'\x7fELF\x02\x01\x01'
for off,val in {20:1,40:64,52:64,58:64,64+24:192,64+32:8,128:1,128+24:208,128+32:4}.items():
    if off in (52,58): struct.pack_into('<H',b,off,val)
    else: put(b,off,val)
struct.pack_into('<HH',b,60,2,0); b[192:199]=b'\0.text\0'
(out/'elf').write_bytes(b)
