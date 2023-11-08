String literals
===============

String literals exist for specifying of some string values in an Ü program.
They are declared with ``""``.

.. code-block:: u_spr

   "simple string"
   "в строке может встречаться почти любой юникод"

For some characters escaping is used:

.. code-block:: u_spr

   "\b return"
   "\t tabulation"
   "\n newline"
   "\r carriage return"
   "\f new page"
   "\" quote"
   "\\ backslash"
   "\0 binary zero"

It's also possible to specify symbol code via its hexadecimal number:

.. code-block:: u_spr

   "\u0000 zero"
   "\u00DC Ü"
   "\u0068 h"

*********************
*String literal type*
*********************

A string literal is an array with one of types ``char8``, ``char16`` or ``char32``as elemet type.
Array size is determined by the literal size.
By-default element type is ``char8``, but it's possible to specifiy it via special suffixes - ``u8`` for ``char8``, ``u16`` for ``char16``, ``u32`` for ``char32``.
``u8`` means UTF-8, ``u16`` - UTF-16, ``u32`` - UTF-32.

.. code-block:: u_spr

   "abc абв"u8 // UTF-8 string, array element type is "char8"
   "abc абв"u16 // UTF-16 string, array element type is "char16"
   "abc абв"u32 // UTF-32 string, array element type is "char32"

******************
*Terminating zero*
******************

Unlike in C, C++ and other languages string literals in Ü are NOT terminating with zero.
If terminating zero is required, for C code interaction for example, it may be explicitely specified via ``\0`` at the literal end.

.. code-block:: u_spr

   "/etc/shadow" // No terminating zero, string size is 11
   "/etc/shadow\0" // There is a terminating zero, string size is 12
   "" // No terminating zero, string size is 0
   "\0" // There is a terminating zero, string size is 1

**************************************
*String literals for single character*
**************************************

A string literal specifies usually an array of symbols.
But if it is necessary to specifiy literal for single symbol, one of special suffixes ``c8``, ``c16``, ``c32`` may be used.
Full type names as suffixes may be used too - ``char8``, ``char16``, ``char32``.

.. code-block:: u_spr

   "Q"c8 // char8
   "~"char8 // char8
   "Ё"c16 // char16
   "й"char16 // char16
   "\u00DC"c32 // char32
   "ß"char32 // char32
   
