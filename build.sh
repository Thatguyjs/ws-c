#!/usr/bin/bash

gcc -Werror -x c -o ./build/ws ./src/*.c

echo "-- Program Started --"
./build/ws
echo "-- Program Ended --"
