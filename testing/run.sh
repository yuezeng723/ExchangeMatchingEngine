#!/bin/bash

for i in {1..500}
do
    ./testing/client &
done

wait