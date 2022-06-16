#!/bin/sh

# sh make.sh /usr/local/php/bin/php-config

set -x

if [ $# -eq 1 ]; then
    PHP_DIR=$1
else
   echo "usage: $0 <php-config path>"
   exit
fi

rm -rf *.o

echo "building interface.."
g++ -std=c++11  -fpic  `$PHP_DIR --includes` -c ../../src/process/dict.cpp ../../src/process/hashtable.cpp ../../src/process/shmem.cpp ../../src/loader.cpp ../../src/dictapi.cpp -I ../../src -I ../../src/process -lpthread -lrt
gcc -std=c++11 -fpic  `$PHP_DIR --includes`   -c ../../src/process/lockfile.cpp  -I ../../src/process
gcc -fpic  `$PHP_DIR --includes`   -c  dictx.c

echo "linking.. "
g++ -std=c++11 -shared *.o  -o dictx.so
echo "success"

sudo cp -f dictx.so /usr/local/php/lib/php/extensions/no-debug-non-zts-20060613/

