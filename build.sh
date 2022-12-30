#!/bin/bash
###
 # @Author: Sugar 45682h@gmail.com
 # @Date: 2022-12-30 15:23:25
 # @Describe: 
### 

cd build && cmake ../

if test $# -gt 0 && test $1 = "clean"
then
    echo "make clean"
    make clean
else
    echo "make"
    make
fi