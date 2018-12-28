#!/bin/bash
pass="/home/joey/mProv/ui-work/mpi-pass/build/partition/libPartitionPass.so"

clang-4.0 -c -emit-llvm $1
opt-4.0  -load $pass -PartitionPass < ${1%.c}.bc > tmp.bc
llc-4.0 -filetype=obj tmp.bc
gcc -c hello.c -o hello.o
gcc /home/joey/mProv/ui-work/context-lib/src/*.o -I/home/joey/mProv/ui-work/context-lib/include tmp.o -o ${1%.c}-inst -lpthread
