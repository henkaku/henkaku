#!/usr/bin/env python3

from sys import argv, exit

# Checks that all 16-byte blocks in the file are different
# This hides the fact the we use AES in ECB mode

def main():
  if len(argv) != 3:
    print("Usage: write_url.py filename url")
    return -2
  if len(argv[2]) >= 255:
    print("url must be at most 255 characters!")
    return -2
  with open(argv[1], "r+b") as file:
    data = file.read()
    pos = data.find(bytes([0x78]*256))
    file.seek(pos, 0)
    file.write(argv[2].encode('ascii'))
    file.write(bytes([0] * (256 - len(argv[2]))))

if __name__ == "__main__":
  exit(main())
