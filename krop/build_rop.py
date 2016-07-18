#!/usr/bin/env python3
from sys import argv, exit
import tempfile
import os.path
import subprocess


tpl = """
.equ ENC_PAYLOAD_ADDR, {payload_addr}
.equ ENC_PAYLOAD_SIZE, {payload_size}
.equ BASE, {sysmem_base}
.equ SECOND_PAYLOAD, {second_payload}
"""

prefix = "arm-vita-eabi-"

def build(code):
    with tempfile.TemporaryDirectory() as tmp:
        src_file = os.path.join(tmp, "rop.S")
        obj_file = os.path.join(tmp, "rop.o")
        bin_file = os.path.join(tmp, "rop.bin")
        obf_file = os.path.join(tmp, "rop.obf")
        fout = open(src_file, "wb")
        fout.write(code)
        fout.close()

        subprocess.check_call([prefix + "as", src_file, "-o", obj_file])
        subprocess.check_call([prefix + "objcopy", "-O", "binary", obj_file, bin_file])
        subprocess.check_call(["python3", "krop/ropfuscator.py", bin_file, obf_file, "DxT9HVn5"])

        fin = open(obf_file, "rb")
        data = fin.read()
        fin.close()
        return data


def chunk(b, size):
    if len(b) % size != 0:
        raise RuntimeError("chunk: b % size != 0")
    return [b[x * size:(x + 1) * size] for x in range(0, len(b) // size)]


def write_c_code(krop, relocs, addr_pos, size_shift_pos, size_xor_pos, size_plain_pos, second_payload_pos):
    output = "unsigned krop[] = {"
    output += ", ".join(hex(int.from_bytes(x, 'little')) for x in krop)
    output += "};\nunsigned relocs[] = {"
    output += ", ".join(str(x) for x in relocs)
    output += "};\n"
    output += "krop[{}] = ENC_PAYLOAD_ADDR;\n".format(addr_pos)
    output += "krop[{}] = (ENC_PAYLOAD_SIZE >> 2) + 0x10;\n".format(size_shift_pos)
    output += "krop[{}] = ENC_PAYLOAD_SIZE ^ 0x40;\n".format(size_xor_pos)
    output += "krop[{}] = ENC_PAYLOAD_SIZE;\n".format(size_plain_pos)
    output += "krop[{}] = SECOND_PAYLOAD;\n".format(second_payload_pos)
    return output


def write_rop_code(krop, relocs, addr_pos, size_shift_pos, size_xor_pos, size_plain_pos, second_payload_pos):
    output = ""
    tpl = "store({}, krop + 0x{:x});\n"
    for x, (addr, reloc) in enumerate(zip(krop, relocs)):
        addr = int.from_bytes(addr, "little")
        if reloc == 0:
            s = "0x{:x}".format(addr)
        else:
            output += "add([sysmem_base], 0x{:x});\n".format(addr)
            s = "&return"
        output += tpl.format(s, x * 4)
    output += "store([kx_loader_addr], krop + 0x{:x});\n".format(addr_pos * 4)
    # I've hardcoded payload size to be 0x200, deal with it
    payload_size = 0x200
    output += "store(0x{:x}, krop + 0x{:x});\n".format((payload_size >> 2) + 0x10, size_shift_pos * 4)
    output += "store(0x{:x}, krop + 0x{:x});\n".format(payload_size ^ 0x40, size_xor_pos * 4)
    output += "store(0x{:x}, krop + 0x{:x});\n".format(payload_size, size_plain_pos * 4)
    output += "store(second_payload, krop + 0x{:x});\n".format(second_payload_pos * 4)
    return output

def main():
    if len(argv) != 3:
        print("Usage: build_rop.py rop.S output-directory/")
        return -1
    fin = open(argv[1], "rb")
    code = fin.read()
    fin.close()

    tags = {
        "payload_addr": 0xF0F0F0F0,
        "payload_size": 0x0A0A0A00,
        "sysmem_base": 0xB0B00000,
        "second_payload": 0xC0C0C0C0,
    }

    first = build(tpl.format(payload_addr=0, payload_size=0, sysmem_base=0, second_payload=0).encode("ascii") + code)
    second = build(tpl.format(**tags).encode("ascii") + code)

    if len(first) != len(second):
        print("wtf? got different krop lengths")
        return -2

    # Find differences in krops, a difference indicates either that this address depends on sysmem base or it's
    # payload addr/size
    krop = first = chunk(first, 4)
    second = chunk(second, 4)
    relocs = [0] * len(first)
    addr_pos = size_shift_pos = size_xor_pos = size_plain_pos = second_payload_pos = -1
    for i, (first_word, second_word) in enumerate(zip(first, second)):
        if first_word != second_word:
            second = int.from_bytes(second_word, "little")
            if second == tags["payload_addr"]:
                addr_pos = i
            elif second == (tags["payload_size"] >> 2) + 0x10:
                size_shift_pos = i
            elif second == tags["payload_size"] ^ 0x40:
                size_xor_pos = i
            elif second == tags["payload_size"]:
                size_plain_pos = i
            elif second == tags["second_payload"]:
                second_payload_pos = i
            else:
                relocs[i] = 1

    if -1 in [addr_pos, size_shift_pos, size_xor_pos, size_plain_pos, second_payload_pos]:
        print("unable to resolve positions: addr={}, size_shift={}, size_xor={}, size_plain={}, second_payload={}".format(
            addr_pos, size_shift_pos, size_xor_pos, size_plain_pos, second_payload_pos))
        return -2

    print("Kernel rop size: 0x{:x} bytes".format(len(krop) * 4))

    with open(os.path.join(argv[2], "krop.c"), "w") as fout:
        fout.write(write_c_code(krop, relocs, addr_pos, size_shift_pos, size_xor_pos, size_plain_pos, second_payload_pos))
    with open(os.path.join(argv[2], "krop.rop"), "w") as fout:
        fout.write(write_rop_code(krop, relocs, addr_pos, size_shift_pos, size_xor_pos, size_plain_pos, second_payload_pos))


if __name__ == "__main__":
    exit(main())
