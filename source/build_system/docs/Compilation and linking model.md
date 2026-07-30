This document describes roughly how the Ü  build system performs code compilation and linking.

The Ü build system has the following build target types:

* executable - regular native executable
* library - Ü library, which can be used by other build targets
* shared library - regular native shared library
* object file - regular object file using native format

The way how each target is built depends on its type.

For each build target its own source files are compiled into LLVM bitcode files.
Then, when producing the result file of the build target, all LLVM bitcode files are read and combined together.
*ustlib* (also built into LLVM bitcode) is linked into build targets except libraries.
*--internalize-hidden-functions* option is used for all build targets except executables to make internal all functions with public linkage which don't have hidden visibility.
Hidden visibility is used for functions with public linkage (declared in an imported file).
Default (non-hidden) visibility is used for functions with public linkage declared in an imported file within public import directories of a build target.
*--internalize* option is used for executables to make all functions internal except *main*.
Internalization for executables is needed to remove all unused functions and encourage inlining.
Such internalization is needed in order to hide as many functions are possible and to minimize the possibility of name conflicts in build targets having many dependencies.

For executables, shared libraries and object files public library dependencies (including transitive public library dependencies) are also linked.
Their LLVM bitcode files are read and combined with LLVM bitcode of the build target itself prior to generating machine code.
Such bitcode-based linking is used to make some optimizations (like inlining) possible.

For all build targets private library dependencies (including their public library dependencies) are also linked.
*--internalize-functions-from* option is used to internalize all functions from these dependent libraries.
It's necessary to do so in order to allow having multiple different versions of the same library dependency (usually having functions with identical mangled names) linked into the same build target without having linking conflicts.
It also can help preventing name conflicts even for different libraries which happen to use the same mangled function names.

The necessity of internalization of private library dependencies in library targets makes it hard or even impossible to generate machine code for libraries on per-library or per-source-file basis.
It's impossible with current LLVM tooling to take an object file or shared library, make functions from it private and re-package it back without breaking calling of these functions between different object files within a static library.

Machine code is generated for all build targets except libraries.
Internal compiler linker is executed for executables and shared libraries, but not for object files.

For libraries no code generation is performed.
Their LLVM bitcode files are just combined into single LLVM bitcode file, which is saved on disk for later usage by other build targets.

For executables and shared libraries shared library dependencies and external static and shared libraries are also provided to the compiler (more precise, to its built-in linker).
If there is a dependency on a shared library target, *-rpath=$ORIGIN* option is also set.

For shared libraries an interface library (*.lib*) can be generated (usually for Windows builds).
If it exists, such interface library is used instead of the shared library itself for linking build targets depending on this shared library target.

For shared library targets functions with default visibility are preserved, so that users of this shared library can call them.
On Windows *dllexport* storage class is set for functions with default visibility, so that these functions appear in the export table of the result DLL.
