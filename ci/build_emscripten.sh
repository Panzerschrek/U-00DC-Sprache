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
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/llvm-15.0.7.src.tar.xz &&\
xz -d llvm-15.0.7.src.tar.xz &&\
tar -xf llvm-15.0.7.src.tar &&\
\
# Get llvm cmake modules
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/cmake-15.0.7.src.tar.xz &&\
xz -d cmake-15.0.7.src.tar.xz &&\
tar -xf cmake-15.0.7.src.tar &&\
mv cmake-15.0.7.src cmake &&\
\
# Get binary llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
xz -d clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
tar -xf clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar &&\
\
# Perform hacks for native tools build
sed -i -e "s/-DLLVM_TARGET_IS_CROSSCOMPILE_HOST=TRUE/-DLLVM_TARGET_IS_CROSSCOMPILE_HOST=TRUE -DLLVM_INCLUDE_BENCHMARKS=FALSE/g" llvm-15.0.7.src/cmake/modules/CrossCompile.cmake &&\
\
# Configure build
mkdir build &&\
cd build &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$PWD/../emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DEMSCRIPTEN_SYSTEM_PROCESSOR=wasm -DCMAKE_C_COMPILER=emcc -DCMAKE_CXX_COMPILER=emcc -DLLVM_SRC_DIR=../llvm-15.0.7.src/ -DU_EXTERNAL_LLVM_AS=$PWD/../clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04/bin/llvm-as -DU_BUILD_COMPILER=NO -DU_BUILD_COMPILER1=NO -DU_BUILD_CPP_HEADER_CONVERTER=NO -DU_BUILD_LANGUAGE_SERVER=NO -DU_BUILD_TESTS=NO -DU_BUILD_PY_TESTS=NO -DU_BUILD_LINKAGE_TESTS=NO -DUBUILD_DOCS=NO -DU_BUILD_INTERPRETER=YES -DLLVM_TARGETS_TO_BUILD="" -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF &&\
\
# Run build
cmake --build . 
