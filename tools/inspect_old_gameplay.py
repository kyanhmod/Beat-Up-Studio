"""Read-only, reproducible OldVTC motion/input trace (pefile + capstone)."""
from pathlib import Path
import hashlib
import struct
import sys
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

path = Path(sys.argv[1]) if len(sys.argv) > 1 else (
    Path(__file__).resolve().parents[2]
    / '05_Reverse_Engineering/Game_Binaries/(Nhi)Audition_OldVTC.exe'
)
data = path.read_bytes()
digest = hashlib.sha256(data).hexdigest()
if digest != '398b1e1c486b1ae673f49a39734deaa32bcf3e1840a5f4b0cc5dc4bd1976d981':
    raise SystemExit('Different binary: these addresses must be located again')
pe = pefile.PE(data=data)
base = pe.OPTIONAL_HEADER.ImageBase
print(path.name, digest)
for name, va in [('direction thresholds',0xE93130), ('Space thresholds',0xE93144),
                 ('direction base score',0xE93170), ('Space base score',0xE93184)]:
    print(name, struct.unpack_from('<5i',data,pe.get_offset_from_rva(va-base)))
for va in [0xE7E720,0xB5DF68,0xE214D8,0xE934A0,0xE8D558]:
    print(f'double 0x{va:08X}',struct.unpack_from('<d',data,pe.get_offset_from_rva(va-base))[0])
blocks = [
    ('ready and start voice phase gates',0x8E638E,0x8E63EF),
    ('countdown start voice gate',0x8E64F0,0x8E6584),
    ('chance prompt and Delete activation',0xA11AE8,0xA11D1F),
    ('chance generation note flag',0xA11226,0xA112BB),
    ('finish tick to measure comparison',0xA10EE0,0xA10EF7),
    ('native hit levels and miss tier state',0xA0D834,0xA0DAA6),
    ('native ready start countdown phases',0x8E6320,0x8E638E),
    ('native beatup ready start countdown drawing',0x8E6CC8,0x8E6DD8),
    ('song record BPM and offset',0x963F51,0x963F81),
    ('smooth clock native offset phase',0x9BC22C,0x9BC25F),
    ('integer clock native offset phase',0x9BC343,0x9BC399),
    ('tick clock',0x964664,0x964695),
    ('motion speed',0xA125E0,0xA12619),
    ('direction judge',0xA0BA20,0xA0BA6E),
    ('Space judge',0xA0BA70,0xA0BAAA),
    ('input takes first pending direction',0xA131D0,0xA132F0),
    ('Space input gate',0xA0EAA0,0xA0EAEB),
    ('direction draw',0xA0D333,0xA0D56B),
    ('hit arrow and glow lifecycle',0xA0D090,0xA0D2B5),
    ('arrow rendering followed by end caps',0xA1190F,0xA11A21),
    ('track layout',0xA10A1B,0xA10BA1),
    ('Space bar origin',0xA10D42,0xA10D63),
    ('Space note centers',0xA0E338,0xA0E400),
    ('judge and combo',0xA0BFBD,0xA0C155),
]
md = Cs(CS_ARCH_X86, CS_MODE_32)
for label, start, end in blocks:
    print('\n'+label)
    offset = pe.get_offset_from_rva(start-base)
    for ins in md.disasm(data[offset:offset+end-start],start):
        print(f'{ins.address:08X} {ins.mnemonic:8} {ins.op_str}')
