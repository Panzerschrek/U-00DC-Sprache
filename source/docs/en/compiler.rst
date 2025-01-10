Compiler
========

Ü compiler is a command-line tool which is used for compilation/linking of Ü code.
There are several versions of Ü compiler.
Compiler0 is written in C++.
Compiler1 is written in Ü and compiled with Compiler1.
Compiler2 is Compiler1 code compiled with Compiler1.


*******
*Usage*
*******

Basic usage is to provide one or more input file and one output file, like this:

.. code-block:: sh

   Compiler source.u -o source.o

Default input file type is Ü source file, default output is native object file.


************************
*Command-line interface*
************************

Input files are listed without any option.
It's expected at least one input file.

Type of input files is specified via ``--input-filetype`` option.
Supported types are ``source`` - for Ü files, ``bc`` - for LLVM bitcode files, ``ll`` - for LLVM assembly files.

Output file is specified via ``-o`` option.
Output file type is specified via ``--filetype`` option.
Supported types are ``obj`` - for native object file, ``bc`` - for LLVM bitcode file, ``ll`` - for LLVM assembly file, ``asm`` - for native assembly file, ``exe`` - for native executable, ``dll`` - for native shared library, ``null`` - to produce no file at all.
In last case it's not necessary to specify output file.

Optimization level is specified via options ``-O``.
Supported optimization levels are ``O0``, ``O1``, ``O2``, ``Os``, ``Oz``.
They are similar to optimization options of compilers like *gcc* or *clang*.


Option ``--lto-mode`` affects link-time optimization.
``none`` option does nothing for link-time optimization.
``prelink`` option is designed for compilation of individual Ü files into LLVM bitcode files.
``link`` option is  designed for link-time optimisation of multiple input LLVM bitcode files.

``-g`` Option enables debug information generation.
It's recommended to use it together with ``-O0``, in order to produce debuggable code without optimizations messing with debugging.
