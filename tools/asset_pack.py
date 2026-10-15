"""Windows XPRESS-Huffman resource container; Python standard library only."""
import ctypes as c
from ctypes import wintypes as w
import struct

MAX_BYTES=64*1024*1024

def codec(data,decompress=False,size=0):
    api=c.WinDLL('cabinet',use_last_error=True);handle=c.c_void_p()
    create=api.CreateDecompressor if decompress else api.CreateCompressor
    run=api.Decompress if decompress else api.Compress
    close=api.CloseDecompressor if decompress else api.CloseCompressor
    create.argtypes=[w.DWORD,c.c_void_p,c.POINTER(c.c_void_p)]
    run.argtypes=[c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
    close.argtypes=[c.c_void_p]
    if not create(4,None,c.byref(handle)):raise c.WinError(c.get_last_error())
    try:
        src=c.create_string_buffer(data);needed=c.c_size_t(size)
        if not decompress:run(handle,src,len(data),None,0,c.byref(needed))
        if not 0<needed.value<=MAX_BYTES:raise ValueError('Invalid resource size')
        dst=c.create_string_buffer(needed.value)
        if not run(handle,src,len(data),dst,len(dst),c.byref(needed)):raise c.WinError(c.get_last_error())
        if decompress and needed.value!=size:raise ValueError('Resource size mismatch')
        return dst.raw[:needed.value]
    finally:close(handle)

def write_pack(path,assets):
    raw=bytearray()
    for key,data in sorted(assets.items()):
        name=key.encode('utf-8');raw+=struct.pack('<II',len(name),len(data))+name+data
    if len(raw)>MAX_BYTES:raise ValueError('Resource bundle exceeds limit')
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(struct.pack('<Q',len(raw))+codec(bytes(raw)))

def read_pack(path):
    data=path.read_bytes()
    if len(data)<9:raise ValueError('Resource bundle is truncated')
    size=struct.unpack_from('<Q',data)[0]
    if not 0<size<=MAX_BYTES:raise ValueError('Invalid resource bundle length')
    raw=codec(data[8:],True,size);pos=0;assets={}
    while pos<len(raw):
        if len(raw)-pos<8:raise ValueError('Truncated resource header')
        n,length=struct.unpack_from('<II',raw,pos);pos+=8
        if n>len(raw)-pos or length>len(raw)-pos-n:raise ValueError('Truncated resource data')
        name=raw[pos:pos+n].decode('utf-8');pos+=n
        if not name or '/' in name or chr(92) in name or name in {'.','..'} or name in assets:raise ValueError('Invalid resource name')
        assets[name]=raw[pos:pos+length];pos+=length
    return assets
