#!/usr/bin/env python3

from sys import argv, exit

# Checks that all 16-byte blocks in the file are different
# This hides the fact the we use AES in ECB mode

def main():
	if len(argv) != 3:
		print("Usage: write_pkg_url.py filename pkg_url_prefix")
		return -2
	with open(argv[1], "r+b") as file:
		file.seek(-256, 2)
		file.write(argv[2].encode('ascii'))
		file.write(bytes([0] * (256 - len(argv[2]))))

if __name__ == "__main__":
	exit(main())
