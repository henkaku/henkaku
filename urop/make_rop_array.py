#!/usr/bin/env python3
from sys import argv, exit

tpl = """buffer {name}[{size}] = {{ {data} }};\nsymbol {name}_size = {size};"""

def main():
	if len(argv) != 4:
		print("Usage: make_rop_array.py input.bin array-name output.rop")
		return -1
	with open(argv[1], "rb") as fin:
		data = fin.read()
	while len(data) % 4 != 0:
		data += b"\x00"
	txt_data = ", ".join(hex(x) for x in data)
	txt = tpl.format(name=argv[2], size=len(data), data=txt_data)
	with open(argv[3], "w") as fout:
		fout.write(txt)

if __name__ == "__main__":
	exit(main())
