#!/bin/bash

for i in {1..100} 
do
    ./client &
done

wait