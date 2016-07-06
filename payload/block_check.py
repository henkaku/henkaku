#!/usr/bin/env python3

from sys import argv, exit

# Checks that all 16-byte blocks in the file are different
# This hides the fact the we use AES in ECB mode

def main():
	if len(argv) != 2:
		print("Usage: block_check.py filename")
		return -2
	with open(argv[1], "rb") as fin:
		data = fin.read()
	blocks = [data[x:x+16] for x in range(0, len(data), 16)]
	seen = set()
	for block in blocks:
		if block in seen:
			print("FAILED: same blocks exist in {}".format(argv[1]))
			return 0  # TODO: make non-zero?
		seen.add(block)
	print("PASSED: all blocks are unique in {}".format(argv[1]))
	return 0


if __name__ == "__main__":
	exit(main())
