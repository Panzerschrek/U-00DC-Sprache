#!/bin/bash

# Get llvm
wget http://releases.llvm.org/9.0.0/clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz &&\
xz -d clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04.tar.xz &&\
tar -xf clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04.tar &&\
\
# Configure build
mkdir build-travis &&\
cd build-travis &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_LIB_DIR=../clang+llvm-9.0.0-x86_64-linux-gnu-ubuntu-16.04/lib/ -DU_BUILD_COMPILER2=YES -DU_BUILD_COMPILER3=YES -DSPHINX_WARNINGS_AS_ERRORS=No &&\
\
cmake --build . &&\
\
# Run ustlib tests
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler0/Compiler  --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler1/Compiler1 --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler2/Compiler2 --use-position-independent-code --input-dir source/ustlib_test &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/compiler3/Compiler3 --use-position-independent-code --input-dir source/ustlib_test
