#!/bin/bash

# Get llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
xz -d clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar.xz &&\
tar -xf clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04.tar &&\
\
# Configure build
mkdir build_dir &&\
cd build_dir &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLLVM_LIB_DIR=../clang+llvm-15.0.6-x86_64-linux-gnu-ubuntu-18.04/lib/ -DU_BUILD_COMPILER2=NO -DU_BUILD_COMPILER3=NO -DU_BUILD_DOCS=NO -DU_COVERAGE_ENABLED=YES &&\
\
cmake --build . &&\
\
# Run ustlib tests
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler0/Compiler  --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler1/Compiler1 --use-position-independent-code --input-dir source/ustlib_test &&\
# Run the coverage preparation code
lcov -o code_coverage.info -c -d build_dir -d source --no-external &&\
genhtml -o code_coverage_report code_coverage.info


