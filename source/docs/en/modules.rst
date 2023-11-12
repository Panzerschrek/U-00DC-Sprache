Modules
=======

*************
*Build model*
*************

An Ü program consists of one or more source files.
Each file is built by the compiler into platform-specific object file.
Result object files are linked together into result executable, static or shared library.

Technically linking is possible with object files produced by compilers of other languages like C or C++.

********
*Import*
********

Sometimes it's necessary to use same declarations/definitions (for functions, classes, etc.) in different source files.
It's possible to move these definitions into a separate file and import it in all source files where it is necessary.

Imports are specified at the beginning of a source file via ``import`` directive that contains a file path in ``""``.
A path may be absolute (starting with ``/``) or relative otherwise.
If a path is absolute imported file will be searched starting from one of the root import directories that are specified via the corresponding compiler option.
If a path is relative it will be searched relative to the current file.

.. code-block:: u_spr

   import "a.u" // Import a file located in the same directory as the current file
   import "../b.u" // Import a file located in one directory above the directory of the current file
   import "cc/c.u" // Import a file from subdirectory "cc" of directory containing the current file
   import "/d.u" // Import a file that is located in one of the root directories (specified in the compiler options)

*******************
*How imports works*
*******************

An imported file is compiled as usual, sometimes even with imports of another files.
Than definitions/declarations from each imported files are merged into this file.
Import order doesn't matter for both imported files and current file.
With any imports order result imported names set will be the same.

Functions defined in imported files have ``private`` linkage in order to avoid linkage conflicts in linking of several object files produced by compilation of source files that all import some common file.
Also generated functions (class methods), template function and any functions within templates have ``private`` linkage.

Usage of ``private`` linkage allows to define functions in common imported files without linking problems.
For example, there are three files and contents of one of them is imported by two others:

.. code-block:: u_spr
  :caption: a.u

   fn GetX() : i32 { return 42; }

.. code-block:: u_spr
  :caption: b.u

   import "a.u"

.. code-block:: u_spr
  :caption: c.u

   import "a.u"

In program linking from ``"b.u"`` and ``"c.u"`` files there will be no linking conflicts caused by ``GetX()`` function.

Also ``private`` visibility have functions defined in a main (compiled) file if they have no prototype in one of imported files.
The only exception - ``nomangle`` functions.
Such feature allows to define functions with private visibility without linking conflicts even if another function(s) with the same name and signature exists in other files.

.. code-block:: u_spr
  :caption: a.u

   fn SomeLocal(){}

.. code-block:: u_spr
  :caption: b.u

   fn SomeLocal(){}

In program linking from ``"b.u"`` and ``"c.u"`` files there will be no linking conflicts caused by ``SomeLocal()`` function.

*********************
*One definition rule*
*********************

Each thing in the result Ü program must be defined exactly once.
It's not allowed for many things with the same name (and signature, for functions) to exist in one program.
Exceptions are things with ``private`` visibility defined in different source files.
During import the compiler checks that things from different imported files are not conflicting with each other - names are not redefined, there is no function with the same name and signature, etc.

But the compiler can't always check that one definition rule is not violated.
A programmer may technically define in two source files things with non-``private`` visibility and the same name, compile these files and try to link them.
The compiler can't find this error and it's not guaranteed either that a linked will find it.
Thus a programmer is responsible for absence of the one definition rule violations.
