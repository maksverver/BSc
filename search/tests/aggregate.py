#!/usr/bin/env python
#
# Script to aggregate results from multiple identical test runs.
#
# Arguments are as follows:
#  <col>       The column to aggregate on
#  <file>...   Data files
# Run the script without arguments for a list of possible columns.
#
# The first value on each line is interpreted as a key; for each key, the
# median, minimum and maximum values of the data files are reported.
#

COLS = [ 'its', 'qsz', 'trans', 'wctime', 'utime', 'stime', 'rss', 'vss' ]

from sys import argv, exit

def usage():
    'Print usage information, then terminate.'

    print 'Usage: aggregate <col> <files>+'
    print 'Columns:', '/'.join(COLS)
    exit(0)


# Check number of arguments (at least 3, including script name)
if len(argv) < 3:
    print 'Not enough arguments.'
    usage()

try:
    col = COLS.index(argv[1])
except ValueError:
    print 'Invalid column: ', argv[1]
    usage()

values = { }    # For each key, a list of values is stored

# Process all files
for path in argv[2:]:

    for line in file(path):

        # Parse line
        if not line or line[0] == '#':
            continue
        row = line.split()
        if not row or len(row) <= col:
            continue

        # Extract key and value
        key = int(row[0])
        val = row[col]

        # Interpret value
        if val.find('.') < 0:
            val = int(row[col])
        else:
            val = float(row[col])

        # Store result
        if key not in values:
            values[key] = [ ]
        values[key].append(val)


# Output results
items = values.items()
items.sort()
for key, vals in items:
    vals.sort()
    mini = vals[0]
    maxi = vals[-1]
    if len(vals)&1 == 0:
        medi = (vals[len(vals)/2 - 1] + vals[len(vals)/2])/2
    else:
        medi = vals[len(vals)/2]

    print "%19s %19s %19s %19s" % (str(key), str(medi), str(mini), str(maxi))
