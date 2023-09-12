### About

*CPPHeaderConverter* - a tool that generates Ü bindings for declarations in C++ header files.
The main reason for this tool to exist is to automate Ü bindings creation for C functions.
For now C++ headers support is limited, only C-like declarations can be converted.


### How to build

This tool is based on clang sources.
In order to build it you may need to build project with clang by providing LLVM_EXTERNAL_CLANG_SOURCE_DIR in cmake options.
Alternatively it is possible to build a tool with prebuilt LLVM library, that includes necessary clang libraries.


### Usage

Run the tool with provided input and output file names.
Specify C (or C++) compiler-specific options after --.

Result Ü file will include declarations for all functions, types and some define-constants, including declarations from included files.
Alternatively it is possible to disable taking declarations from included files via *--skip-declarations-from-includes* option.
Also it is possible to add imports section into result file, using option *--force-import*.

It is preferred to use this tool with *--skip-declarations-from-includes* option.
Without this option result Ü file will include declarations not only from input file, but from all included files.
This may cause a problem when multiple Ü files for multiple headers are generated.
Ü compiler considers declarations wit same name in different files distinct and produces redefinition error.
In order to obtain shared declarations properly it is recommended to create some common-declarations C (or C++) header, with all necessary includes (like C standard library), generate common Ü file for it and add import of this file in other generated Ü bindings via option *--force-import*.
