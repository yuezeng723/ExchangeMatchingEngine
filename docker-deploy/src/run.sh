#!/bin/bash
make clean
make 
echo 'running proxy as daemon...'
./server &
while true ; do continue ; done
