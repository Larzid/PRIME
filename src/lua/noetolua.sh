#!/bin/bash
for i in *.noe; do
    sed 's/\.noe/\.lua/' $i > ${i%.noe}.lua
    rm $i
done
