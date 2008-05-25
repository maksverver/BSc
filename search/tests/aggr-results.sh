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
    rm /tmp/aggregated.txt
}

aggregate eratosthenes vss    base 01 02 03 04 05 06 07 08 09
aggregate eratosthenes wctime base 01 02 03 04 05 06 07 08 09
