#!/usr/bin/env python3
from sys import argv, exit
import base64
import struct

def u32(data, offset):
	return struct.unpack("<I", data[offset:offset+4])[0]

def u16(data, offset):
	return struct.unpack("<H", data[offset:offset+2])[0]

tpl_js = """
payload = [{}];
relocs = [{}];
"""

tpl_php = """<?php
$payload = array({});
$relocs = array({});
"""

def main():
	if len(argv) != 3:
		print("Usage: preprocess.py urop.bin output-payload.js")
		return -1
	with open(argv[1], "rb") as fin:
		urop = fin.read()

	while len(urop) % 4 != 0:
		urop += b"\x00"

	header_size = 0x40
	dsize = u32(urop, 0x10)
	csize = u32(urop, 0x20)
	reloc_size = u32(urop, 0x30)
	symtab_size = u32(urop, 0x38)

	if csize % 4 != 0:
		print("csize % 4 != 0???")
		return -2

	reloc_offset = header_size + dsize + csize
	symtab = reloc_offset + reloc_size
	strtab = symtab + symtab_size

	symtab_n = symtab_size // 8
	reloc_map = dict()
	for x in range(symtab_n):
		sym_id = u32(urop, symtab + 8 * x)
		str_offset = u32(urop, symtab + 8 * x + 4)
		begin = str_offset
		end = str_offset
		while urop[end] != 0:
			end += 1
		name = urop[begin:end].decode("ascii")
		reloc_map[sym_id] = name

	# mapping between roptool (symbol_name, reloc_type) and exploit hardcoded types
	# note that in exploit type=0 means no relocation
	reloc_type_map = {
		("rop.data", 0): 1,       # dest += rop_data_base
		("SceWebKit", 0): 2,      # dest += SceWebKit_base
		("SceLibKernel", 0): 3,   # dest += SceLibKernel_base
		("SceLibc", 0): 4,        # dest += SceLibc_base
		("SceLibHttp", 0): 5,     # dest += SceLibHttp_base
		("SceNet", 0): 6,         # dest += SceNet_base
	}

	# we don't need symtab/strtab/relocs
	want_len = 0x40 + dsize + csize
	relocs = [0] * (want_len // 4)

	reloc_n = reloc_size // 8
	for x in range(reloc_n):
		reloc_type = u16(urop, reloc_offset + 8 * x)
		sym_id = u16(urop, reloc_offset + 8 * x + 2)
		offset = u32(urop, reloc_offset + 8 * x + 4)
		print_dbg = lambda: print("type {} sym {} offset {}".format(reloc_type, reloc_map[sym_id], offset))
		# print_dbg()
		if offset % 4 != 0:
			print_dbg()
			print("offset % 4 != 0???")
			return -2
		if relocs[offset // 4] != 0:
			print_dbg()
			print("symbol relocated twice, not supported")
			return -2
		wk_reloc_type = reloc_type_map.get((reloc_map[sym_id], reloc_type))
		if wk_reloc_type is None:
			print_dbg()
			print("unsupported relocation type")
			return -2
		relocs[offset // 4] = wk_reloc_type

	urop_js = [u32(urop, x) for x in range(0, want_len, 4)]

	with open(argv[2], "wb") as fout:
		fout.write((want_len // 4).to_bytes(4, "little"))
		for word in urop_js:
			fout.write(word.to_bytes(4, "little"))
		for reloc in relocs:
			fout.write(reloc.to_bytes(1, "little"))

		# if argv[2].endswith("php"):
		# 	tpl = tpl_php
		# else:
		# 	tpl = tpl_js

		# fout.write(tpl.format(",".join(str(x) for x in urop_js), ",".join(str(x) for x in relocs)).encode("ascii"))

if __name__ == "__main__":
	exit(main())
