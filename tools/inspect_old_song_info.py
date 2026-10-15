"""Read-only OldVTC HUD trace. Requires pefile and capstone.

Usage: python tools/inspect_old_song_info.py [path-to-OldVTC.exe]
Addresses below are specific to the supplied OldVTC, not other clients.
"""
import hashlib
from pathlib import Path
import struct
import sys
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

path = Path(sys.argv[1]) if len(sys.argv) > 1 else (
    Path(__file__).resolve().parents[2]
    / '05_Reverse_Engineering/Game_Binaries/(Nhi)Audition_OldVTC.exe'
)
data = path.read_bytes()
pe = pefile.PE(data=data)
base = pe.OPTIONAL_HEADER.ImageBase
print(path.name, 'SHA256', hashlib.sha256(data).hexdigest())
for name in ('blank', 'full', 'base', 'bar', 'feather'):
    needle = f'song_info_{name}'.encode() + b'\0'
    off = data.find(needle)
    if off < 0:
        raise SystemExit(f'Missing {needle!r}; unsupported binary')
    va = base + pe.get_rva_from_offset(off)
    print(needle[:-1].decode(), f'VA 0x{va:08X}')
    if name == 'base' and va != 0xE7DF04:
        raise SystemExit('Different binary layout; locate new functions before disassembling')
for start, end in ((0x8EB820, 0x8EB944), (0x43EA20, 0x43EA60),
                   (0x4405B3, 0x440655), (0x8EB47C, 0x8EB507)):
    off = pe.get_offset_from_rva(start - base)
    print(f'\nBlock 0x{start:08X}')
    for ins in Cs(CS_ARCH_X86, CS_MODE_32).disasm(data[off:off + end-start], start):
        print(f'{ins.address:08X}  {ins.mnemonic:8} {ins.op_str}')
for va in (0xE7DC88, 0xE23DB0, 0xE214F8, 0xE21C20):
    print(f'constant 0x{va:08X}', struct.unpack_from('<d', data, pe.get_offset_from_rva(va-base))[0])
for va in (0xE7DEC8, 0xE7DE8C):
    off = pe.get_offset_from_rva(va-base)
    print(f'label 0x{va:08X}', repr(data[off:data.index(b'\0', off)]))
