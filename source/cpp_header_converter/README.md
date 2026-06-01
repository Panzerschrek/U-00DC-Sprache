### About

*CPPHeaderConverter* - a tool that generates Ü bindings for declarations in C++ header files.
The main reason for this tool to exist is to automate Ü bindings creation for C functions.
For now C++ headers support is limited, mostly only C-like declarations can be converted.


### How to build

This tool is based on *clang* sources.
In order to build it you may need to build the project with *clang* by providing LLVM_EXTERNAL_CLANG_SOURCE_DIR in cmake options.
Alternatively it is possible to build the tool with prebuilt LLVM library, that includes necessary clang libraries.


### Usage

Run the tool with provided input and output file names.
Specify C (or C++) compiler-specific options after --.
The result Ü file will include declarations for functions, types and some define-constants, including declarations from included files.

It's possible to disable emitting some declarations using `#define U_CPP_HEADER_CONVERTER_IGNORE`.
It may be useful, for example, in order to skip declarations from some specific header.
But skipped declarations are still referenced, so that you may need to generate them via a separate invocation of C++ header converter and to import the result file using *--force-import* option.


### Limitations

The tool has a lot of limitations, since C and Ü are pretty different languages:

* Functions with invalid for Ü names are skipped, this includes functions starting with underscore or having name which is a keyword in Ü.
* Other names (types, variables, fields, etc.) are renamed if their original name isn't a valid identifier in Ü or if it may create a name conflict with some special names used by C++ header converter.
* Variadic functions aren't fully supported - variadic parameters are removed.
* Contents of unions is replaced with simple byte arrays - Ü doesn't support C-style unions.
* Contents of structs having bit-fields is replaced with simple byte arrays - Ü doesn't support bit-fields.
* `size_t` isn't translated like `size_type` in Ü, since in C it's just a type alias for some fundamental integer type.
* Typedefs for function types are broken, but typedefs for function pointer types work fine.
* Some C calling conventions aren't supported by Ü, so, functions declarations may have wrong calling conventions.
* Constants for `#define`s are created, but only limited defines are supported - numeric literals, char literals and strings.
* Alias defines (like #define X Y) can be created for functions, types, variables, enum members, but more complex defines can't be translated.


### Clang headers

In order to work properly this tool requires *clang* headers.
Normally they should be shipped together with it - in directory like `../lib/clang/17/include` relative to the tool executable.
But when you use this tool in other context, like in build scripts, you may need to provide path to these headers via `-I` option after `--`.
