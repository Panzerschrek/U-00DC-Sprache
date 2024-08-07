#!/bin/bash

# Get llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/llvm-15.0.7.src.tar.xz &&\
xz -d llvm-15.0.7.src.tar.xz &&\
tar -xf llvm-15.0.7.src.tar &&\
\
# Get llvm cmake modules
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/cmake-15.0.7.src.tar.xz &&\
xz -d cmake-15.0.7.src.tar.xz &&\
tar -xf cmake-15.0.7.src.tar &&\
# Get lld
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/lld-15.0.7.src.tar.xz &&\
xz -d lld-15.0.7.src.tar.xz &&\
tar -xf lld-15.0.7.src.tar &&\
# Get libunwind
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/libunwind-15.0.7.src.tar.xz &&\
xz -d libunwind-15.0.7.src.tar.xz &&\
tar -xf libunwind-15.0.7.src.tar &&\
\
# lld Mach-O library includes libunwind using path relative to llvm src directory and without a version. So, rename it.
mv libunwind-15.0.7.src libunwind &&\
# Configure build
mkdir build_dir &&\
cd build_dir &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MODULE_PATH=$PWD/../cmake-15.0.7.src/Modules/ -DCMAKE_LINKER="ld.gold" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=gold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=gold" -DCMAKE_MODULE_LINKER_FLAGS="-fuse-ld=gold" -DLLVM_SRC_DIR=$PWD/../llvm-15.0.7.src/ -DLLVM_EXTERNAL_LLD_SOURCE_DIR=$PWD/../lld-15.0.7.src/ -DLLVM_TOOL_LLD_BUILD=ON -DU_BUILD_COMPILER2=YES -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_NATIVE_ARCH="X86" -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF &&\
\
# Run build
cmake --build . -- -j 1 &&\
\
# Run ustlib tests
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler0/Compiler  --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler1/Compiler1 --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler2/Compiler2 --use-position-independent-code --input-dir source/ustlib_test
