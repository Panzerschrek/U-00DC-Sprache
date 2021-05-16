﻿## Ü-Sprache
Experimental programming language.  
It is a compilable, statically-typed C++-like language.

Here is a compiler for this language. 

### Documentation

Documentation available [here](https://u-00dc-sprache.readthedocs.io/ru/latest/contents.html). Currently only russian documentation exists.

### How to build
* Download llvm (llvm 9.0.0 used in this project).  
* Run cmake for source/CMakeLists.txt and generate project for your favorite IDE or build system. You must set path to llvm in cmake arguments.  
* (optional) for CppHeaderConverter you need to download clang sources and set cmake variable LLVM_EXTERNAL_CLANG_SOURCE_DIR.  

### How to build QCreator plugin
* Download QtCreator.  
* Download QtCreator sources.  
* Run qmake for qt_creator_plugin/usprace.pro. You must set path to QtCreator sources and binaries in qmake arguments.

### Authors
Copyright © 2016-2021 Artöm "Panzerscrek" Kunz.
