Compiler
========

Ü compiler is a command-line tool which is used for compilation/linking of Ü code.

There are several versions of Ü compiler.
Compiler0 is written in C++.
Compiler1 is written in Ü and compiled with Compiler0.
Compiler2 is Compiler1 code compiled with Compiler1.
There is a little difference between different versions, they should generally work exactly the same.
If it's not, it's a bug in one of compiler implementations.


*******
*Usage*
*******

Basic usage is to provide one or more input files and specify output file path, like this:

.. code-block:: sh

   u.._compiler source.u -o source.o

Default input file type is Ü source file, default output file type is native object file.


**************
*Capabilities*
**************

Ü compiler can compile (obviously) Ü source files.
But it's not the only thing it can do.

It's possible to compile multiple sources into LLVM bitcode files and then combine them and run link-time optimization (via separate compiler invocation).

Also Ü compiler can produce native executables and shared libraries.
But this works only if it was compiled together with internal LLD (which is usually the case).


************************
*Command-line interface*
************************

Input files are listed directly without any option.
Zero, one or more input files may be specified.

The type of input files is specified via ``--input-filetype`` option.
Supported types are ``source`` (default) - for Ü source files, ``bc`` - for LLVM bitcode files, ``ll`` - for LLVM assembly files.

Output file is specified via ``-o`` option.
Output file type is specified via ``--filetype`` option.
Supported types are ``obj`` (default) - for native object file, ``bc`` - for LLVM bitcode file, ``ll`` - for LLVM assembly file, ``asm`` - for native assembly file, ``exe`` - for native executable, ``dll`` - for native shared library, ``null`` - to produce no file at all.
In last case it's not necessary to specify output file.

Optimization level is specified via ``-O`` options.
Supported optimization levels are ``O0``, ``O1``, ``O2``, ``Os``, ``Oz``.
They are similar to optimization options of compilers like *gcc* or *clang*.

Option ``--lto-mode`` affects link-time optimization.
``none`` option does nothing for link-time optimization.
``prelink`` option is designed for compilation of individual Ü files into LLVM bitcode files.
``link`` option is designed for link-time optimization of multiple input LLVM bitcode files.

``--internalize`` option makes public symbols (functions, global variables) private, which allows their inlining and optimizing-out.
It's useful together with ``--lto-mode=link``.
A list of symbols which shouldn't be internalized may be specified via ``--internalize-preserve`` option.
``--internalize-hidden`` option works similar, but internalizes only symbols with hidden visibility style.
``--internalize-symbols-from`` option forces internalization of symbols obtained from input file specified.

``-g`` Option enables debug information generation.
It's recommended to use it together with ``-O0``, in order to produce debuggable code without optimizations messing with debugging.

Options ``--target-arch``, ``--target-vendor``, ``--target-os``, ``--target-environment`` specify parts of target triple used for compilation.
Default target triple is usually host target triple.

``--override-data-layout`` and ``--override-target-triple`` options are designed to override data layout and/or target triple of input LLVM bitcode or LLVM assembly files.
But it's not recommended to do so.

``--mangling-scheme`` option gives control over mangling scheme used for functions/global variables mangling.
Default value is ``auto`` and selects mangling scheme based on target triple.
Value ``itanium-abi`` selects Itanium ABI mangling scheme (used on most Unix systems).
``msvc`` selects MSVC mangling scheme (with subvariant determined by target triple).
Also variants ``msvc32`` and ``msvc64`` may be specified.
Generally it's not recommended to change default mangling scheme, use this option only if it's strictly necessary for interaction with foreign code.

``-MF`` option specifies output file name for a dependency file.
Such file contains list of dependencies needed for compilation of given files, including imported and embedded files.
This file has *ninja*-compatible format.

``--include-dir`` option adds a directory for search of imported/embedded files.
It's possible to provide multiple directories.
Additionally a prefix for given directory may be specified after ``::``.

``--source-dir`` option adds a directory, files from which are considered to be source files or private imports.
It's possible to provide multiple directories.
This affects visibility style of functions with prototypes declared in files within a source directory.

If ``--prevent-imports-outside-given-directories`` option is specified, an error is generated if an imported or an embedded file is located outside provided include or source directories.

``--allow-unused-names`` option allows unused names - local variables, functions, class fields, etc.
If this option is not specified, an error is generated if a name isn't used.

``--halt-mode`` option controls behavior of language ``halt`` operator.
``trap`` value (default) means using ``llvm.trap`` instruction for ``halt``.
``abort`` value means call of C *abort* function.
``configurable_handler`` allows runtime setting of a ``halt`` handler function.
``unreachable`` value forces compiler to treat code which halts as unreachable, with undefined behavior if ``halt`` actually happens.

``--no-system-alloc`` option disables usage of system memory allocation functions (``malloc``/``HeapAlloc``) in compiled code.

``--disable-async-calls-inlining`` option disables force-inlining of async function calls.
By default async calls are inlined, which optimizes calls speed in (possible) exchange against larger binary size.

``--verify-module`` option enables correctness checks for intermediate LLVM modules produced during compilation/linking.
Generally it's needed only to detect possible compiler bugs.

``--print-llvm-asm`` and ``--print-llvm-asm-initial`` options allow to inspect generated LLVM assembly code - before and after optimizations/transformations.

``--print-prelude-code`` option allows to print compiler-generated prelude pseudo-file.
It may be useful for debugging.

``-Wl`` option allows to pass arguments to internal linker (LLD).
It's used while producing executable and shared library outputs.

``--sysroot`` option is used to provide *sysroot* for internal linker, which may be necessary for cross-compilation.


*********************************
*Additional command-line options*
*********************************

Some other command-line options are available, which are declared by the LLVM library used for Ü compiler.
Run ``u.._compiler --help`` for more information.
Available options may depend on compiler build configuration and LLVM library version.
