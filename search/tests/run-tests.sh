#!/bin/sh

for file in config/*
do
	./run-test.sh "$file"
done
