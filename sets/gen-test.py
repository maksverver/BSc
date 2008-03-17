#!/usr/bin/env python

from random import Random

chrs = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
rnd = Random(1)
N = 100000

# Generate a bunch of random strings
ls = []
while len(ls) < N:
    n = rnd.randint(5,15)
    s = ''.join([chrs[rnd.randint(0, len(chrs)-1)] for _ in range(n)])
    ls.append(s)

# Output a bunch of random queries
for _ in range(4*N):
    s = ls[rnd.randint(0,len(ls)-1)]
    if rnd.randint(0,1):
        print s
    else:
        print '?%s'%s

