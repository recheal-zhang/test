#!/bin/sh

DIR="./bin/"
cd $DIR

while true
do
    i = 0
    while i < 2
    do
        client
        i = i+1
    done
    
done

