"""Read-only trace of OldVTC full-canvas HUD rendering, no client execution."""
from pathlib import Path
import hashlib,pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
path=Path(__file__).resolve().parents[2]/'05_Reverse_Engineering/Game_Binaries/(Nhi)Audition_OldVTC.exe'
data=path.read_bytes()
assert hashlib.sha256(data).hexdigest()=='398b1e1c486b1ae673f49a39734deaa32bcf3e1840a5f4b0cc5dc4bd1976d981'
pe=pefile.PE(data=data);base=pe.OPTIONAL_HEADER.ImageBase;md=Cs(CS_ARCH_X86,CS_MODE_32)
for title,a,b in [('resource descriptor width and height',0x444C9C,0x444D10),('quad dimensions from descriptor times scale',0x43F9DA,0x43FAB9),('judgement anchor mode',0xA0BEA0,0xA0BEC4),('judgement and combo draw',0xA0BFBD,0xA0C155),('song info base',0x8EB879,0x8EB896),('countdown original anchor',0x8E6D2D,0x8E6DD8)]:
 print(title)
 for i in md.disasm(pe.get_data(a-base,b-a),a):print(f'{i.address:08X} {i.mnemonic} {i.op_str}')
