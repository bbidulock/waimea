#!/bin/bash

rm -f cscope.*
./autogen.sh
./configure.sh
make V=0 cscope
cscope -b
make V=0 clean all
