#!/bin/sh

if [ "$#" -ne 2 ]; then
  echo "usage: $0 stage2_url pkg_prefix_url" >&2
  exit 1
fi

cp loader.rop.bin host/stage1.bin
python3 preprocess.py exploit.rop.bin host/stage2.bin
python3 write_pkg_url.py host/stage1.bin "$1"
python3 write_pkg_url.py host/stage2.bin "$2"
python3 preprocess.py host/stage1.bin host/payload.js
echo "done." >&2
