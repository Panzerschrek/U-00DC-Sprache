C++ header converter
====================

C++ header converter is a tool, which allows to generate Ü bindings for C or C++ header files.
C headers are fully supported, but only limited subset of C++ headers may be converted.


*******************************
*Possibilities and limitations*
*******************************

C++ header converter can create bindings for functions, it can convert structs, enums, typedefs, some constant variables.
But there are still a lot of limitations caused by differences between C and Ü:

* Functions with names matching an Ü keyword are ignored - they can't be used.
  Also functions starting with ``_`` are ignored, since in Ü identifiers can't start with ``_``.
* Functions with variadic arguments in Ü aren't supported, C variadic functions become non-variadic in Ü.
* Ü has no unions, so C unions are translated as structs with single field - array of ``byte``-type elements.
* Ü has no bit-fields in structs, so structs containing bit-fields are translated as structs with single field - array of ``byte``-type elements.
* Both ``const`` and non-``const`` pointers from C are translated as raw pointers in Ü, which are always assumed to be mutable (non-``const``).
* Enums are always translated as their underlying types, including C++ ``enum class``.
* Names except function names may be renamed, if for exampe a name can't be valid Ü name or in case of name conflicts.
* Nested structs are flattened for simplicity, possible with renaming to avoid name conflicts.
* Global variables are translated, but only if they are ``const`` and have ``constexpr`` initializer.
  Varaibles of sclar types are supported.
  Arrays are supported too, but only with number of initializers matching array size.
  Struct/union types aren't supported.


************************
*Command-line interface*
************************

Basic usage is following:

.. code-block:: sh

   u.._cpp_header_converter some_c_header.h -o some_c_header_converted.uh -- -std=c11


Available command-line options:

``-o`` - specify input file.

``--force-import`` - create an import statement inside output file, importing file specified.

``--skip-declarations-from-includes`` - skip converting declarations from includes.

``--force-import`` and ``--skip-declarations-from-includes`` may be used in combination, if one want to convert each header file of a complex include hierarchy separately and recreate such hierarchy of Ü imports.

Additionally C or C++ arguments may be specified after ``--``.
Since C++ header converter is based on *clang*, many *clang* options are supported.
See *clang*'s documentation for more details.
Most common options, used in C++ header converter, are standard options (``-std=c11``, ``-std=c++17``, etc) and include dirs option (``-I``).
