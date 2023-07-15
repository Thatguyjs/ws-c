#!/usr/bin/bash

gcc -Werror -x c -Og -o ./build/ws-dbg ./src/*.c ./src/*/*.c

echo "-- Build Succeeded --"
valgrind --track-origins=yes --leak-check=full -s ./build/ws-dbg
echo "-- Program Ended --"
