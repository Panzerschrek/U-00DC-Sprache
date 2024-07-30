#!/bin/bash

# Get llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
xz -d clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
tar -xf clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar &&\
\
# Configure build
mkdir build_dir &&\
cd build_dir &&\
# Build only tests - do not count coverage from later compiler0 launches.
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLLVM_LIB_DIR=$PWD/../clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04/lib/ -DU_BUILD_COMPILER=NO -DU_BUILD_DOCS=NO -DU_COVERAGE_ENABLED=YES &&\
\
cmake --build . &&\
\
# Run the coverage preparation code
cd .. &&\
lcov -o code_coverage.info -c -d build_dir -d source --no-external &&\
genhtml -o code_coverage_report code_coverage.info
