#!/usr/bin/bash

gcc -Werror -x c -o ./build/cws ./src/main.c

echo "-- Program Started --"
./build/cws
echo "-- Program Ended --"
