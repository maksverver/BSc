#!/bin/bash

MODELS=../models/

[ -z "$1" ] && echo 'Missing argument: config' && exit 1

# External file declares:
MODEL=                                  # base name of model to use
SET=                                    # set configuration 
REPEAT=7                                # number of iterations
MAXVSS=unlimited                        # maximum virtual set size
INTERVAL=10000                          # reporting interval
MAXIT=10000000                          # maximum number of iterations
OUTPUT="results/`basename "$1"`"        # output file

# Read from config
source $1
[ -z "$MODEL" ] && echo 'No model given' && exit 1
[ -z "$SET" ] && echo 'No set configuration given' && exit 1
[ -z "$OUTPUT" ] && echo 'No ouput file given' && exit 1
[ ! -e "${MODELS}"/"$MODEL".b ] && echo 'Model bytecode file does not exist' \
                                && exit 1

mkdir -p "`dirname "$OUTPUT"`"
for ((n=1; $n<=$REPEAT; n=$n+1))
do 

  # Determine output filename
  if [ "$OUTPUT" = "/dev/null" ]
  then
    out=/dev/null
  else
    out="$OUTPUT"-$n
  fi

  # Run test
  ( ulimit -v $MAXVSS &&
    ../search -l "$MAXIT" -i "$INTERVAL" -m "${MODELS}"/"$MODEL".b $SET \
    > "$out" )

  # Sleep for some time to give the OS a chance to swap stuff back in
  sleep 10

done
