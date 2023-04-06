#!/bin/bash

for i in {1..5000}
do
    ./client &
done

wait