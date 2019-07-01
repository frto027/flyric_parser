#!/bin/sh
make bin/test.memcheck
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./bin/test.memcheck 'lrc.txt'
