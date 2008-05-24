#!/usr/bin/env python
#
# Script to subtract base results from a data file.
#
# Arguments are as follows:
#  <base>      The base data file
#  <data>      The actual data file
#
# Both input files are assumed to be in the format produced by aggregate.py
# For each row that occurs in both 'base' and 'data', the median value from
# 'base' is subtracted from the median, minimum and maximum values in 'data'
# and the result is printed.
#

from sys import argv, stderr, exit

def usage():
    'Print usage information, then terminate.'

    print 'Usage: subtract <base> <data>'
    exit(0)

def parse(str):
    'Parses a string into an int or float'

    if str.find('.') < 0:
        return int(str)
    else:
        return float(str)


# Check number of arguments (exactly 3, including script name)
if len(argv) <> 3:
    print 'Not enough arguments.'
    usage()

# Read base values
base = { }
for line in file(argv[1]):
    row = line.split()
    key = int(row[0])
    val = parse(row[1])
    base[key] = val

# Read data values
for line in file(argv[2]):
    row = line.split()
    key  = int(row[0])
    if key not in base:
        print >>stderr, 'WARNING: missing base value for key', key
        continue
    medi = parse(row[1]) - base[key]
    mini = parse(row[2]) - base[key]
    maxi = parse(row[3]) - base[key]
    print "%19s %19s %19s %19s" % (str(key), str(medi), str(mini), str(maxi))
