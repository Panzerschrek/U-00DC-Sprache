#!/bin/bash

# Get llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
xz -d clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
tar -xf clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar &&\
\
# Configure build
mkdir build_dir &&\
cd build_dir &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_LIB_DIR=../clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04/lib/ -DU_BUILD_COMPILER2=NO -DU_BUILD_COMPILER3=NO -DSPHINX_WARNINGS_AS_ERRORS=NO -DU_BUILD_DOCS=YES &&\
\
cmake --build . &&\
\
# install
mkdir install &&\
cmake --install build_dir --prefix install
