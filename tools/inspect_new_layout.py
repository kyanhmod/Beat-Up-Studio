from pathlib import Path
import hashlib
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
path=Path(__file__).resolve().parents[2]/'05_Reverse_Engineering/Game_Binaries/Audition_newPH.exe'
data=path.read_bytes();pe=pefile.PE(data=data);base=pe.OPTIONAL_HEADER.ImageBase
print(path.name,hashlib.sha256(data).hexdigest())
for start,end in [(0x4584c3,0x458510),(0x455a50,0x455b39)]:
 print('BLOCK',hex(start))
 for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(pe.get_data(start-base,end-start),start):print(hex(i.address),i.mnemonic,i.op_str)
for va in [0xbcff48,0xbcff54]:print(hex(va),repr(pe.get_data(va-base,24).split(bytes([0]))[0]))
