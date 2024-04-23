Numeric literals
================

Ü has a flexible way to define numeric literals.

There are following numeric literals properties:

* base
* presence/absence of fractional point
* presence/absence of exponent
* type

There are following numeric literals bases:

* 2. Literal starts with ``0b`` prefix. Digits 0-1 are possible.
* 8. Literal starts with ``0o`` prefix. Digits 0-7 are possible.
* 10. Literal has no prefix. Digits 0-9 are possible.
* 16. Literal starts with ``0x`` prefix. Digits 0-9 and a-f or A-F are possible.

Fractional part is specified via ``.`` after integer part of the literal.

Exponent is allowed only for decimal literals.
It is specified after ``e`` symbol, following by optional sign (``+`` or ``-``) and a value of the exponent.

Numeric literals examples:

.. code-block:: u_spr

   42 // Simplest decimal literal
   3.1415926535 // Decimal literal with fractional part
   1e5 // Decimal literal without fractional part but with exponent
   1.5e20 // Decimal literal with fractional part and with exponent
   25.3e-5 // Decimal literal with fractional part and with negative exponent
   695e+7 // Decimal literal with explicitly-specified positive exponent
   
   0b1110 // Binary literal
   0b1.101 // Binary literal with fractional part
   
   0o7124 // Octal literal
   0o7124.1005 // Octal literal with fractional part
   
   0xAB054FE // Hexadecimal literal
   0xAb054Fe // There is no difference between lowercase a-f and uppercase A-F letters
   0x70.F4 // Hexadecimal literal with fractional part

**********************
*Numeric literal type*
**********************

Each numeric literal has a type.
A numeric literal type may be explicitly specified - via type suffix.
If a literal has no suffix, its type will be automatically chosen.

A name of any fundamental type may be used as type suffix.
There are also additional type suffixes:

* ``u`` - suffix for ``u32`` type.
* ``s`` - suffix for ``size_type`` type.
* ``f`` - suffix for ``f32`` type.
* ``c8`` - suffix for ``char8`` type.
* ``c16`` - suffix for ``char16`` type.
* ``c32`` - suffix for ``char32`` type.

If no explicit suffix is specified, the type will be chosen according to the following rule: if a literal has fractional part it is assumed to be of ``f64`` type, otherwise - of ``i32`` type.

.. code-block:: u_spr

   55 // i32
   64125 // i32
   1e5 // i32
   52.0 // f64
   
   0b110 // i32
   0b110.1 // f64
   
   0o07741 // i32
   0o0.722 // f64
   
   0xFF // i32
   0x541E.aa // f64
   
   0u // u32
   0x100u // u32
   
   128s // size_type
   0o521s // size_type
   
   0.5f // f32
   0b100f // f32
   
   88c8 // char8
   0b0c16 // char16
   220c32 // char32
   
   0i16 // i16
   55f64 // f64
   925u64 // u64
   220char16 // char16
