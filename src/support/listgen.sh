#!/bin/bash
# This is for creating list of monsters and items
# for checking which entries have lore.
for (( i=0; i<2; ++i )); do
    if [ $i -eq 0 ]; then
        list=`grep kObj gen/ObjectIlk.h | grep -v \=`
    else
        list=`grep kMon gen/MonsterIlk.h | grep -v \=`
    fi
    sum=0;
    numlore=0;
    for entry in $list; do
        sum=$((sum+1));
        lore=`grep $entry gen/Lore.cpp | grep -v glb_`
        if [ -z "$lore" ]; then
            lore='     (no lore)';
        else
            numlore=$((numlore+1));
        fi
        printf "%-30s %s\n" $entry "$lore"
    done
    echo
    printf "%d/%d\n" $numlore $sum
    echo
done
