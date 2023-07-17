#!/usr/bin/bash

[ ! -d ./build ] && mkdir ./build

gcc -Werror -x c -O2 -o ./build/ws ./src/*.c ./src/*/*.c

echo "-- Program Started --"
./build/ws "$@"
echo "-- Program Ended --"
