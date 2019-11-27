## Ü-Sprache
[![Build Status (master)](https://api.travis-ci.org/Panzerschrek/U-00DC-Sprache.svg?branch=master)](https://travis-ci.org/Panzerschrek/U-00DC-Sprache)

Experimental programming language.  
It is a compilable, statically-typed C++-like language.

Here is a compiler for this language. 
Currently, there are no docs for this language, all knowledges are concentrated inside compiler, tests and my mind. 
So, this is reference compiler for this language.

### How to build
* Install boost (version 1.71.0 used).  
* Download llvm (llvm 9.0.0 used in this project).  
* Run cmake for source/CMakeLists.txt and generate project for your favorite IDE or build system. You must set paths to boost and llvm in cmake arguments.  
* (optional) for CppHeaderConverter you need to download clang sources and set cmake variable LLVM_EXTERNAL_CLANG_SOURCE_DIR.  

### How to build QCreator plugin
* Download QtCreator.  
* Download QtCreator sources.  
* Run qmake for qt_creator_plugin/usprace.pro. You must set path to QtCreator sources and binaries in qmake arguments.

### Authors
Copyright © 2016-2019 Artöm "Panzerscrek" Kunz.
