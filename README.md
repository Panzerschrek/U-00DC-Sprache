## Ü-Sprache
Experimental programming language.  
It is a compilable, statically-typed C++/Rust-like language.

Here is a compiler for this language. 

### Documentation

Documentation available [here](https://u-00dc-sprache.readthedocs.io/ru/latest/contents.html). Currently only russian documentation exists.

### How to build
* Download llvm (llvm 15.0.7 used in this project).  
* Run cmake for *source/CMakeLists.txt* and generate project for your favorite IDE or build system. You must set path to llvm in cmake arguments.  
* (optional) in order to build CppHeaderConverter you need to download clang sources and set cmake variable LLVM_EXTERNAL_CLANG_SOURCE_DIR.  

### Additional components

[language server](source/language_server/README.md)

[syntax highlighting](source/syntax_highlighting/README.md)

[interpreter](source/interpreter/README.md)

[plugin for QtCreator](source/qt_creator_plugin/README.md)

### Authors
Copyright © 2016-2023 "Panzerscrek".
