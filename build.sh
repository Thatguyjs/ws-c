#!/usr/bin/bash

gcc -Werror -x c -o ./build/ws ./src/main.c

echo "-- Program Started --"
./build/ws
echo "-- Program Ended --"
