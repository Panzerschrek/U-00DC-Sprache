### About

This directory contains Ü compilers (Compiler0, Compiler1 and further generations) common code.

The Ü Compiler is a core part of the Ü project.
It performs proper Ü sources compilation.


### Usage

Build a simple object file:

```
Compiler test.u -o test.o
```

It's possible to specify multiple files:

```
Compiler test0.u test1.u test2.u -o test.o
```

Import directories may be specified:

```
Compiler test.u --include-dir dir0 --include-dir ../some_path/dir1 -o test.o
```

There is an option to enable debug information generation:

```
Compiler test.u -o test.o -g
```

There is also an option to control optimization level, like in C compilers:

```
Compiler test.u -o test.o -O2
```

It's possible to produce an ll file (to inspect it, for example):

```
Compiler test.u -o test.ll --filetype=ll
```

LLVM bitcode output is also supported:

```
Compiler test.u -o test.bc --filetype=bc
```

Also the compiler may work as LLVM bitcode linker:

```
Compiler test.bc -o test.o --input-filetype=bc
```

The compiler supports link time optimization:

```
# Build a module for further LTO.
Compiler test.u -o test.bc --lto-mode=prelink
# Run LTO. Internalize all functions except "main".
Compiler test.bc --input-filetype=bc -o test.o --lto-mode=link --internalize --internalize-preserve=main
```

If the compiler was built with internal LLD it's possible to output a native executable file:

```
Compiler test.u -o test.exe --filetype=exe
```

Shared libraries are also supported:

```
Compiler test.u -o test.so --filetype=dll
```

It's possible to specify options for internal LLD via `-Wl` option:

```
Compiler test.u -o test.exe --filetype=exe -Wl=-lpthread
```

Run the compiler with --help option to know all supported options.
There are a lot of internal LLVM options, including options for target-specific optimizations.


### Internal LLD

If the compiler was built with LLD libraries this internal LLD may be used for producing executable and shared library files.

When building the project form the LLVM sources, in order to build the compiler with LLD you need to specify path to LLD sources via LLVM_EXTERNAL_LLD_SOURCE_DIR cmake option.
LLD libraries build must be also enabled via LLVM_TOOL_LLD_BUILD cmake option.
Also libunwind sources (a part of the LLVM project) must exist near LLVM source directory.

When building with prebuilt LLVM, LLD will be included in the compiler, if this prebuilt LLVM libraries contain LLD libraries.

It's important to mention that the compiler uses its internal LLD in a very limited way.
It produces single temporary object file and performs result executable or shared library generation from it.
It's possible to specify some linker options manually, but it's not possible to disable options produced by the compiler itself.
Thus this internal LLD can't be used as general linker for producing arbitrary executables from arbitrary input object files.
A proper external linker should be used instead (ld, link.exe, etc).

Generally if an Ü program uses some parts written in C or C++ it's recommended to use a C++ linker to produce result executable file, rather than using the Ü compiler as linker.


### Embedded ustlib implementation functions

The compiler includes some implementation functions for ustlib (see ustlib/src).
It's required to recompile the compiler if these sources were changed.
