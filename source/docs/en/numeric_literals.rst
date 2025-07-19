Numeric literals
================

Ü supports numeric literals in various formats.
There are two kinds of numeric literals - integers and floating point-literals.


*********************************
*Floating-point numeric literals*
*********************************

Floating-point numeric literals are decimal numbers with fractional point and/or exponent specified.

Type suffix may be specified for a floating-point numeric literal.
Supported suffixes are ``f`` and ``f32`` for ``f32`` type, ``f64`` for ``f64`` type.
If no suffix is specified for a floating-point numeric literal, it's assumed to be of type ``f64``.

Examples of floating-point numeric literals:

.. code-block:: u_spr

   var f64 a = 0.0; // Has fractional point.
   var f64 b = 1234.56; // Has several integer and fractional digits.
   var f32 c = 87.33f; // Has suffix "f".
   var f32 d = 0.000253f; // Has leading zeros in fractional part.
   var f64 e = 67e7; // Has exponent.
   var f64 f = 3.5e-14; // Has fractional point and negative exponent.
   var f32 g = 12323.7f32; // Has suffix "f32".
   var f64 h = 908754.24556f64; // Has suffix "f64".


**************************
*Integer numeric literals*
**************************

Decimal numbers without fractional point and exponent are considered to be integer numeric literals.

There is also support of non-decimal numeric literals:

* base-2. Literal starts with ``0b`` prefix. Digits 0-1 are possible.
* base-8. Literal starts with ``0o`` prefix. Digits 0-7 are possible.
* base-16. Literal starts with ``0x`` prefix. Digits 0-9 and a-f or A-F are possible.

Non-decimal numeric literals are always integers, fractional point and exponent can't be specified, overflow is treated as error.

Integer numeric literals with value greater than 18446744073709551615 (2\ :sup:`64` - 1) aren't supported.

Type suffix may be specified for an integer numeric literal.
Specifying suffix equal to the name of some built-in integer type means, that this numeric literal is of this type.
There are also additional suffixes ``u`` for ``u32`` type and ``s`` for ``size_type`` type.
If no suffix is specified for an integer numeric literal, it's assumed to be of type ``i32`` (if it fits inside it), ``i64`` or ``i128``.

Examples of integer numeric literals:

.. code-block:: u_spr

   var i32 a = 7; // Decimal number with no suffix - is "i32".
   var i32 b = 644; // Several decimal digits.
   var u32 c = 0u; // Suffix "u" for "u32".
   var u64 d = 0b1100101101101110110111011100u64; // Binary number with suffix "u64".
   var i8 f = 0x3ei8; // Hexadecimal number with suffix "u8".
   var u16 g = 0o74u16; // Octal number with suffix "u16".
   var i64 h = 0x868E5B7F; // Hexadecimal number too large to fit into "i32", so it's "i64".
   var i128 i = 17446748073702551613; // Decimal number too large to fit into "i64", so it's "i128".
   var size_type j = 1234s; // Decimal number with suffix "s".
   var i32 k = 0x67ab4e; // Hexadecimal number with no suffix - is "i32".
