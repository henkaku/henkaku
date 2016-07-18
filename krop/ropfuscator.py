import random
import struct
import sys
from collections import defaultdict

IGNORE_WORD = 0xDEADBEEF

def draw(dist):
  x = random.uniform(0, 1)
  for k, v in dist.items():
    if x > v:
      x -= v
    else:
      return k
  raise "Invalid distribution"

def main(infile, outfile, seed=None):
  counts = defaultdict(lambda: defaultdict(lambda: 0))
  freq = defaultdict(lambda: {None: 1.0})
  random.seed(seed)
  # get frequencies
  with open(infile, mode='rb') as file:
    prev = None
    while True:
      bytes = file.read(4)
      if not bytes:
        break
      cur, = struct.unpack("<I", bytes)
      if cur == IGNORE_WORD:
        cur = None
      else:
        counts[None][cur] += 1 # count frequency
      if prev != None:
        counts[prev][cur] += 1 # count transitions
      prev = cur
    # build frequency matrix
    for key, val in counts.items():
      count = sum([v for v in val.values()])
      freq[key] = {k: float(v) / count for k, v in val.items()}
  # build output
  with open(infile, mode='rb') as input:
    with open(outfile, mode='wb') as output:
      initial = True
      prev = None
      while True:
        if prev != None or initial:
          initial = False
          bytes = input.read(4)
          if not bytes:
            break
          cur, = struct.unpack("<I", bytes)
        if cur == IGNORE_WORD or cur == None:
          cur = draw(freq[prev])
        if cur != None:
          output.write(struct.pack("<I", cur))
        prev = cur

if __name__ == "__main__":
  main(sys.argv[1], sys.argv[2], sys.argv[3] if len(sys.argv) >= 4 else None)
