### About

*CPPHeaderConverter* - a tool that generates Ü bindings for declarations in C++ header files.
The main reason for this tool to exist is to automate Ü bindings creation for C functions.
For now C++ headers support is limited, only C-like declarations can be converted.


### How to build

This tool is based on *clang* sources.
In order to build it you may need to build the project with *clang* by providing LLVM_EXTERNAL_CLANG_SOURCE_DIR in cmake options.
Alternatively it is possible to build the tool with prebuilt LLVM library, that includes necessary clang libraries.


### Usage

Run the tool with provided input and output file names.
Specify C (or C++) compiler-specific options after --.

Result Ü file will include declarations for all functions, types and some define-constants, including declarations from included files.
Alternatively it is possible to disable taking declarations from included files via *--skip-declarations-from-includes* option.
Also it's possible to add imports section into result file, using option *--force-import*.

It is preferred to use this tool with *--skip-declarations-from-includes* option.
Without this option result Ü file will include declarations not only from given input file, but from all included files.
This may cause a problem when multiple Ü files for multiple headers are generated.
Ü compiler considers declarations with same name in different files distinct and produces redefinition error.
In order to obtain shared declarations properly it is recommended to create some common-declarations C (or C++) header, with all necessary includes (like C standard library), generate common Ü file for it and add import of this file in other generated Ü bindings via option *--force-import*.


### Limitations

The tool has a lot of limitations, since C and Ü are pretty different languages:

* Functions with invalid for Ü names are skipped - like functions starting with underscore.
* Variadic functions aren't supported - variadic parameters are removed.
* Types with names invalid for Ü are renamed.
* Types with names identical to function are renamed. C allows such types (at least structs), Ü - doesn't.
* Nested structs are moved into the global namespace and (if necessary) renamed, in order to fix some name resolution problems.
* Fields with names identical to type names are renamed in order to avoid naming conflicts.
* Contents of unions is replaced with simple byte arrays - Ü doesn't support C-style unions.
* Sequential named enums are mapped to Ü enums, but other enums - doesn't. Variable declarations are used for anonymous enums, wrapper structs used for non-sequential named enums.
* `size_t` isn't translated like `size_type` in Ü, since in C it's just a type alias.
* Typedefs for function types are broken. But typedefs for function pointer types work fine.
* Some C calling conventions aren't supported by Ü, so, functions declarations may have wrong calling conventions.
* Constants for `#define`s are created, but only limited defines are supported - numeric literals and strings.

Also *CPPHeaderConverter* doesn't preserve declarations order, it emits symbols sorted by kind and by name instead.
It's done due to implementation reasons.
And it's not necessary to preserve original order, since Ü has order-independent top level declarations.


### Clang headers

In order to work properly this tool requires *clang* headers.
Normally they should be shipped together with it - in directory like `../lib/clang/15.0.7/include` relative to the tool executable.
But when you use this tool in other context, like in build scripts, you may need to provide path to these headers via `-I` option after `--`.
