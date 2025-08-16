C++ header converter
====================

C++ header converter is a tool, which allows to generate Ü bindings for C or C++ header files.
C headers are fully supported, but only limited subset of C++ headers may be converted.
This tool is designed for simplification of C and Ü code interaction.
The output file, produced by it, may be imported directly in Ü code or it may be manually tweaked first, if it's necessary.

C++ header converter is based on *clang* source code and thus can parse any valid C header properly, exactly like real C compilers do this.
So, there is no problem of mis-parsing some headers, what often happens with some hand-written C parsers.


*******************************
*Possibilities and limitations*
*******************************

C++ header converter can create prototypes for functions, it can convert structs, enums, typedefs, some constant variables.
But there are still a lot of limitations caused by differences between C and Ü:

* Most functions are translated properly as ``nomangle`` functions with ``C`` calling convention.
* Functions with names matching an Ü keyword are ignored - they can't be used.
  Also functions starting with ``_`` are ignored, since in Ü identifiers can't start with ``_``.
* Functions with variadic arguments in Ü aren't supported, C variadic functions become non-variadic in Ü.
* C built-in types are mapped to Ü types (where it's possible).
* Structs are translated properly as Ü structs with ``ordered`` marker.
* C unions and structs with bit-fields are translated as structs with single field - array of ``byte``-type elements, so that this struct matches original union/struct size and alignment.
  It's done in this way, since Ü has no unions and bit-fields.
* Incomplete types are translated as empty classes with disabled default constructor, since Ü has no incomplete types.
* Both ``const`` and non-``const`` pointers are translated as raw pointers in Ü, which are always assumed to be mutable (non-``const``).
* Enums are always translated as their underlying types, enum members are translated as global constant variables.
* Names except function names may be renamed, if a name can't be valid Ü name or in case of name conflicts.
* Nested structs are moved into the global namespace (for simplicity), possible with renaming to avoid name conflicts.
* Global variables are translated, but only if they are ``const`` and have compile-time initializer.
  Variables of scalar types are supported.
  Arrays are supported too, but only with number of initializers matching array size.
  Struct/union types aren't supported.
* Simple defines containing integer, char or string literals are translated as global constants.
  More complex defines are ignored, even defines for negative numbers.

Possibilities and limitations for conversion of C++ headers:

* Only ``extern "C"`` functions are supported, with argument types/return types compatible with C.
* Non-templated ``using`` definitions are translated properly as type aliases.
* Classes are translated as structs, visibility labels and member functions are ignored.
* Polymorph classes/classes with inheritance aren't handled properly.
* Scoped enums are translated as type alias for their underlying integer type.
  A namespace with ``_`` suffix is created for members of such enums.
* Namespaces aren't supported, they will likely break the tool.
* No templates are supported, they will likely break the tool.


************************
*Command-line interface*
************************

Basic usage is following:

.. code-block:: sh

   u.._cpp_header_converter some_c_header.h -o some_c_header_converted.uh -- -std=c11

Input file(s) are specified directly.

Available command-line options:

``-o`` - specify output file.

``--force-import`` - create an import statement inside output file, importing file specified.

``--skip-declarations-from-includes`` - skip converting declarations from includes.

Additionally C or C++ options may be specified after ``--``.
Since C++ header converter is based on *clang*, many *clang* options are supported, see its documentation for more details.
The most common of these options, that are used in C++ header converter, are standard options (``-std=c11``, ``-std=c++17``, etc.) and include directories option (``-I``).

Usually C++ header converter is shipped together with *clang* internal headers, which are located in directory like *lib/clang/17/include* of the Ü installation.
This directory is automatically added into include search paths.
But if this is not the case, the path to this directory should be specified manually via ``-I`` option.

``--force-import`` and ``--skip-declarations-from-includes`` options may be used in combination, if one want to convert each header file of a complex include hierarchy separately and recreate such hierarchy of Ü imports.
