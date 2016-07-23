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

def analyze(infile, dbfile, seed=None):
  counts = defaultdict(lambda: defaultdict(lambda: 0))
  freq = defaultdict(lambda: {None: 1.0})
  indexes = {}
  random.seed(seed)
  # get frequencies
  with open(infile, mode='rb') as file:
    prev = None
    i = 0
    while True:
      bytes = file.read(4)
      if not bytes:
        break
      cur, = struct.unpack("<I", bytes)
      if cur == IGNORE_WORD:
        cur = None
      else:
        indexes[cur] = i
        counts[None][cur] += 1 # count frequency
      if prev != None:
        counts[prev][cur] += 1 # count transitions
      prev = cur
      i += 1
    # build frequency matrix
    for key, val in counts.items():
      count = sum([v for v in val.values()])
      freq[key] = {k: float(v) / count for k, v in val.items()}
  # build output
  with open(infile, mode='rb') as input:
    with open(dbfile, mode='wb') as output:
      i = 0
      prev = None
      while True:
        if prev != None or i == 0:
          bytes = input.read(4)
          if not bytes:
            break
          cur, = struct.unpack("<I", bytes)
        if cur == IGNORE_WORD:
          cur = draw(freq[prev])
          while cur == None:
            cur = draw(freq[cur])
          output.write(struct.pack("<I", indexes[cur]))
        else:
          output.write(struct.pack("<I", i))
        prev = cur
        i += 1

def generate(infile, outfile, dbfile):
  chain = []
  with open(infile, mode='rb') as input:
    while True:
      bytes = input.read(4)
      if not bytes:
        break
      cur, = struct.unpack("<I", bytes)
      chain.append(cur)
  with open(dbfile, mode='rb') as db:
    with open(outfile, mode='wb') as output:
      while True:
        bytes = db.read(4)
        if not bytes:
          break
        i, = struct.unpack("<I", bytes)
        output.write(struct.pack("<I", chain[i]))

if __name__ == "__main__":
  if sys.argv[1] == "analyze":
    analyze(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) >= 5 else None)
  elif sys.argv[1] == "generate":
    generate(sys.argv[2], sys.argv[3], sys.argv[4])
  else:
    raise "Invalid operation"
