#!/bin/sh

NIPSHOME=../../nips_vm/
NIPSASM="${NIPSHOME}"/nips_asm.pl
CLASSPATH=../../nips_c/lib/nips-promela-compiler.jar

mkdir -p build
for file in $@
do
    base=`basename "$file"`
    cpp "$file" | grep -v '^#' > build/"$base"
    java xml.XmlSerializer build/"$base" && \
    java CodeGen.CodeGen build/"$base".ast.xml && \
    $NIPSASM build/"$base".s && \
    mv build/"$base".b "$base".b
done
