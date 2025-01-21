#!/bin/bash

# Get llvm
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz &&\
xz -d clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar.xz &&\
tar -xf clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04.tar &&\
\
# Configure build
mkdir build_dir &&\
cd build_dir &&\
cmake ../source/ -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_LIB_DIR=$PWD/../clang+llvm-17.0.6-x86_64-linux-gnu-ubuntu-22.04/lib/ -DU_BUILD_COMPILER2=YES -DU_BUILD_COMPILER3=YES -DSPHINX_WARNINGS_AS_ERRORS=NO -DU_BUILD_DOCS=YES &&\
\
cmake --build . &&\
\
# Run ustlib tests
cd .. &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler0/Compiler  --add-library=build_dir/ustlib0/libustlib.a  --add-library=-lpthread --use-position-independent-code --input-dir source/ustlib/tests &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler1/Compiler1 --add-library=build_dir/ustlib1/libustlib1.a --add-library=-lpthread --use-position-independent-code --input-dir source/ustlib/tests &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler2/Compiler2 --add-library=build_dir/ustlib2/libustlib2.a --add-library=-lpthread --use-position-independent-code --input-dir source/ustlib/tests &&\
python3 source/annotated_tests_run.py --compiler-executable build_dir/compiler3/Compiler3 --add-library=build_dir/ustlib2/libustlib2.a --add-library=-lpthread --use-position-independent-code --input-dir source/ustlib/tests &&\
# install
mkdir install &&\
cmake --install build_dir --prefix install
