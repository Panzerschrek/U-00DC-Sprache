#!/bin/bash

# Get llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.1/llvm-10.0.1.src.tar.xz &&\
xz -d llvm-10.0.1.src.tar.xz &&\
tar -xf llvm-10.0.1.src.tar &&\
\
# Configure build
mkdir build-travis &&\
cd build-travis &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_LINKER="ld.gold" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=gold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=gold" -DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=gold" -DLLVM_SRC_DIR=../llvm-10.0.1.src/ -DU_BUILD_COMPILER2=YES -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF &&\
\
# Run build
cmake --build . -- -j 1 &&\
\
# Run ustlib tests
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler0/Compiler  --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler1/Compiler1 --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler2/Compiler2 --use-position-independent-code --input-dir source/ustlib_test
