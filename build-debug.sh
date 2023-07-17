#!/usr/bin/bash

[ ! -d ./build ] && mkdir ./build

gcc -Werror -x c -ggdb3 -o ./build/ws-dbg ./src/*.c ./src/*/*.c

echo "-- Build Succeeded --"
valgrind --track-origins=yes --leak-check=full -s ./build/ws-dbg "$@"
echo "-- Program Ended --"
