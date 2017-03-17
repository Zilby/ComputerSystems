#!/bin/bash
# Initial Author: Jason Booth
# Modified By: Alex Zilbersher

t1_start=9324875923874598
t1_count=59
t2_start=14254234534549
t2_count=648

for ii in 0 1 2
do
    printf "test 1.$(($ii+1)), threads: $((4**$ii)), start: $t1_start, count: $t1_count"
    time (./main $((2**$ii)) $t1_start $t1_count 1>> results.txt 2>> errors.txt)
done


for jj in 0 1 2
do
    printf "\n"
    printf "test 2.$(($jj+1)), threads: $((4**$jj)), start: $t2_start, count: $t2_count"
    time (./main $((2**$jj)) $t2_start $t2_count 1>> results.txt 2>> errors.txt)
done

if [ -s ./errors.txt ]; then
    echo "ERROR: error in computation"
fi
