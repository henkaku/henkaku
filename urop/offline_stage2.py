#!/usr/bin/env python3

from sys import exit, argv

# This script reads simple relocable rop chain (generated with webkit/preprocess.py) from argv[1] and writes:
# * same ropchain without size field to argv[2]
# * ropchain info: size in words, csize, dsize, code_start, data_start -- as rop-script include-able to argv[3]

def u32(buf, off):
	return int.from_bytes(buf[off:off+4], "little")

def main():
	with open(argv[1], "rb") as fin:
		size_words = int.from_bytes(fin.read(4), "little")
		ropchain = fin.read()

	with open(argv[2], "wb") as fout:
		fout.write(ropchain)

	with open(argv[3], "w") as fout:
		fout.write("symbol rop_size_words = 0x{:x};\n".format(size_words))
		fout.write("symbol dsize = 0x{:x};\n".format(u32(ropchain, 0x10)))
		fout.write("symbol csize = 0x{:x};\n".format(u32(ropchain, 0x20)))
		fout.write("symbol data_start = 0x40;\n")
		fout.write("symbol code_start = 0x{:x};\n".format(0x40 + u32(ropchain, 0x10)))

if __name__ == "__main__":
	exit(main())
