#!/bin/bash

for i in {1..1000}
do
    ./testing/client &
done

wait