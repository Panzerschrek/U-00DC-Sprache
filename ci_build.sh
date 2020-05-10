#!/bin/bash

# Get llvm
wget http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz &&\
xz -d clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz &&\
tar -xf clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04.tar &&\
\
# Configure build
mkdir build-travis &&\
cd build-travis &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_LIB_DIR=../clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04/lib/ -DSPHINX_WARNINGS_AS_ERRORS=No &&\
\
cmake --build . &&\
\
# Run ustlib tests
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/Compiler --entry-point-source source/data/entry.cpp --use-position-independent-code --input-dir source/ustlib_test
