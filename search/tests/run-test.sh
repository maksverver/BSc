#!/bin/sh

MODELS=../models/

[ -z "$1" ] && echo 'Missing argument: config' && exit 1

# External file declares:
MODEL=                                  # base name of model to use
SET=                                    # set configuration 
MAXVSZ=unlimited                        # maximum virtual set size
INTERVAL=1000                           # reporting interval
MAXIT=10000000                          # maximum number of iterations
OUTPUT="results/`basename "$1"`"        # output file

# Read from config
source $1
[ -z "$MODEL" ] && echo 'No model given' && exit 1
[ -z "$SET" ] && echo 'No set configuration given' && exit 1
[ -z "$OUTPUT" ] && echo 'No ouput file given' && exit 1
[ ! -e "${MODELS}"/"$MODEL".b ] && echo 'Model bytecode file does not exist' \
                                && exit 1

mkdir -p "`basename "$OUTPUT"`"
( ulimit -v $MAXVSZ &&
  ../search -l "$MAXIT" -i "$INTERVAL" -m "${MODELS}"/"$MODEL".b $SET \
  > "$OUTPUT" )

