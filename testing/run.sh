#!/bin/bash

for i in {1..2}
do
    ./testing/client &
done

wait