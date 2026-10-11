#!/bin/sh

mkdir -p test/output/

for i in test/tests/* ; do
  echo $i
  if [ -d "$i" ]; then
    output=test/output/$(basename "$i").txt
    ./build/rash ${i}/input.txt > $output
    cmp ${i}/output.txt $output
  fi
done
