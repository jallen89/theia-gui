#!/bin/bash
pass="/home/joey/markdowns/Theia/gui-notes/mpi/llvm-pass-skeleton/build/skeleton/libSkeletonPass.so"

clang-4.0 -c -emit-llvm $1
opt-4.0  -load $pass -SkeletonPass < ${1%.c}.bc > test.bc
