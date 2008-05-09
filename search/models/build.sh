#!/bin/sh

NIPS_HOME=../../nips_vm/
NIPSASM="${NIPS_HOME}"/nips_asm.pl
CLASSPATH=../../nips_c/lib/nips-promela-compiler.jar

export NIPS_HOME CLASSPATH

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
