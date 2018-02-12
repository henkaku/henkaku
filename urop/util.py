import struct

try:
    long        # Python 2
except NameError:
    long = int  # Python 3

def u32(b, off=0):
    return struct.unpack("<I", b[off:off+4])[0]

def p32(x):
    return struct.pack("<I", x)

def p8(x):
    return struct.pack("<B", x)

def u64(b, off=0):
    """ Unpack 64-bit """
    return struct.unpack("<Q", b[off:off+8])[0]

def p16(x):
    """ Pack 16-bit """
    return struct.pack("<H", x)

def c_str(b, off=0):
    b = b[off:]
    out = ""
    x = 0
    while b[x] != "\x00":
        out += b[x]
        x += 1
    return out

def isint(x):
    return isinstance(x, (int, long))
