## Ü-Sprache

Experimental programming language.  
It is a compilable, statically-typed C++-like language.

Here placed compiler for this language. 
Currently, there are no any docs for this language, all knowledges are concentrated inside compiler, tests and my mind. 
So, this is reference-compiler for this language.

### How to build
* Install boost (version 1.60.0 used).  
* Download llvm (llvm 3.7.1 used in this project).  
* Set path to your llvm installation in CmakeLists.txt.  
* Run cmake for source/CMakeLists.txt and generate project for your favorite IDE or build system. You must set paths to boost and llvm in cmake arguments.

### How to build QCreator plugin
* Download QtCreator.  
* Download QtCreator sources.  
* Run qmake for qt_creator_plugin/usprace.pro. You must set path to QtCreator sources and bianries in qmake arguments.

### Authors
Copyright © 2016-2019 Artöm "Panzerscrek" Kunz.
