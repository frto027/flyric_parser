#!/bin/sh
make test.memcheck
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./test.memcheck 'lrc.txt'
