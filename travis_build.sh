#!/bin/bash

# Get llvm
wget http://releases.llvm.org/3.7.1/llvm-3.7.1.src.tar.xz &&\
xz -d llvm-3.7.1.src.tar.xz &&\
tar -xf llvm-3.7.1.src.tar &&\
\
# Get Boost
wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz &&\
gzip -d boost_1_66_0.tar.gz &&\
tar -xf boost_1_66_0.tar &&\
\
# Buuld boost
cd boost_1_66_0 &&\
./bootstrap.sh &&\
./b2 --with-system --with-filesystem --with-program_options &&\
cd .. &&\
\
# Configure build
mkdir build-travis &&\
cd build-travis &&\
cmake ../source/ -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=../boost_1_66_0 -DLLVM_SRC_DIR=../llvm-3.7.1.src -DLLVM_TARGETS_TO_BUILD=X86 &&\
\
# Build it
# travis-ci has 2 cpu cores
make -j 2  &&\
\
# Run tests
./Tests &&\
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build-travis/Compiler --entry-point-source source/data/entry.cpp --use-position-independent-code --input-dir source/ustlib_test