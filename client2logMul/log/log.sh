#!/bin/sh

DIR="./"
cd $DIR
rm "./*.log"
while true
do
    for i in $(ls $DIR/*.log)
    do
        cat $i
    done
    sleep 4
done

