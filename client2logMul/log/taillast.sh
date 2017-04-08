#!/bin/sh

DIR="./"
cd $DIR
#rm *.log
for i in $(ls $DIR/*.log)
do
    nu=$(cat $i | cut -f 1,2,3,4,5 | tail -1)
    echo $nu
done
