#!/bin/bash

[ ! -d "./build" ] && mkdir build

compiler_flags="-std=c++11 -g -Isrc -O3"
linker_flags="-lm"

g++ src/cvm/cvm_main.cpp $compiler_flags $linker_flags -o build/cvm
aarch64-linux-gnu-as snake.S
