#!/bin/sh

mkdir -p aggregated

aggregate() {
    program=$1
    column=$2
    base=$3
    shift 3
    base_path=aggregated/"$program"-"$base"_"$column"
    ./aggregate.py "$column" results/"$program"-"$base"-* > "$base_path"
    for ext in $@
    do
        ./aggregate.py "$column" results/"$program"-"$ext"-* > /tmp/aggregated.txt
        ./subtract.py "$base_path" /tmp/aggregated.txt > aggregated/"$program"-"$ext"_"$column"
    done
    rm -f /tmp/aggregated.txt
}

aggregate eratosthenes qsz    base
aggregate eratosthenes trans  base
aggregate eratosthenes vss    base 01 02 03 04 05 06 07 08 09 10 11
aggregate eratosthenes wctime base 01 02 03 04 05 06 07 08 09 10 11

aggregate leader2      qsz    base
aggregate leader2      trans  base
aggregate leader2      vss    base 01 02 03 04
aggregate leader2      wctime base 01 02 03 04

aggregate peterson     qsz    base
aggregate peterson     trans  base
aggregate peterson     vss    base 01 02 03 04
aggregate peterson     wctime base 01 02 03 04
