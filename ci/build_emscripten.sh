#!/bin/bash

# Get emscripten
git clone https://github.com/emscripten-core/emsdk.git &&\
cd emsdk &&\
./emsdk install latest &&\
./emsdk activate latest &&\
source ./emsdk_env.sh &&\
cd .. &&\
\
# Get llvm src
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/llvm-17.0.6.src.tar.xz &&\
xz -d llvm-17.0.6.src.tar.xz &&\
tar -xf llvm-17.0.6.src.tar &&\
\
# Get llvm cmake modules
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/cmake-17.0.6.src.tar.xz &&\
xz -d cmake-17.0.6.src.tar.xz &&\
tar -xf cmake-17.0.6.src.tar &&\
mv cmake-17.0.6.src cmake &&\
\
# Get binary llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz &&\
xz -d clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz &&\
tar -xf clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar &&\
\
# Configure build
mkdir build &&\
cd build &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$PWD/../emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DEMSCRIPTEN_SYSTEM_PROCESSOR=wasm -DCMAKE_C_COMPILER=emcc -DCMAKE_CXX_COMPILER=emcc -DLLVM_SRC_DIR=../llvm-17.0.6.src/ -DU_EXTERNAL_LLVM_AS=$PWD/../clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04/bin/llvm-as -DU_BUILD_COMPILER=NO -DU_BUILD_COMPILER1=NO -DU_BUILD_CPP_HEADER_CONVERTER=NO -DU_BUILD_LANGUAGE_SERVER=NO -DU_BUILD_TESTS=NO -DU_BUILD_PY_TESTS=NO -DU_BUILD_LINKAGE_TESTS=NO -DUBUILD_DOCS=NO -DU_BUILD_BUILD_SYSTEM=NO -DU_BUILD_BUILD_SYSTEM_TESTS=NO -DU_BUILD_INTERPRETER=YES -DLLVM_TARGETS_TO_BUILD="" -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_BUILD_TESTS=OFF &&\
\
# Run build
cmake --build . 
