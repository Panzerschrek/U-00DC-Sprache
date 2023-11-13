Types
=====

Ü is a statically-type language, that means that each variable should have some known at compile-time type.

Ü has several type categories:

*******************
*Fundamental types*
*******************

Fundamental types are base of the type system.
All other types are based either on fundamental types or types, that based on fundamental types, etc.
There are following fundamental types in Ü:

* ``void`` - is used as return type for functions that return nothing. It has zero size.
* ``bool`` - logical type, possible values are ``true`` and ``false``.
* Signed integer types - ``i8``, ``i16``, ``i32``, ``i64``, ``i128``.
* Unsigned integer types - ``u8``, ``u16``, ``u32``, ``u64``, ``u128``.
* Floating point types - ``f32``, ``f64``.
* Char types - ``char8``, ``char16``, ``char32``.
* Raw data types - ``byte8``, ``byte16``, ``byte32``, ``byte64``, ``byte128``.

Numeric suffix of some type names means size in bits.

********
*Arrays*
********

Arrays are sequences of single type values with size known at compile-time.

Array types declaration examples:

.. code-block:: u_spr

   [ i32, 4 ] // Array of 4 "i32" elements
   [ char8, 8 ] // Array of 8 "char8" elements
   [ [ f32, 4 ] , 4 ] // 2x2 array of "f32" elements
   [ bool, 0 ] // Empty array

Array elements are accessed via ``[]`` operator.
``[]`` should contain an expression of unsigned integer type, that contains index of an element needed to be accessed.

.. code-block:: u_spr

   var [ i32, 4 ] mut arr= zero_init;
   var u32 i= 1;
   arr[0]= 66;
   arr[i]= 55;
   arr[2]= arr[0] + arr[3];
   

********
*Tuples*
********

Tuples are sequences of values.
Each element in the sequence may have different type.

Tuple types declaration examples:

.. code-block:: u_spr

   tup[ i32, f32 ] // Tuple of two elements of different types
   tup[] // Empty tuple
   tup[ i32, i32, i32 ] // Tuple of three elements of same type
   tup[ i32, f32, f32, bool, [ i32, 2 ], char8, [ char16, 8 ], tup[ bool, f64 ], u64 ] // Tuple may contain arrays and other tuples
   

Tuple elements are accessed via ``[]`` operator.
``[]`` should contain a compile-time expression of integer type.

.. code-block:: u_spr

   var tup[ i32, f32, bool ] mut t= zero_init;
   t[0]= 55;
   t[1]= 3.7f;
   t[2]= f32(t[0]) == t[1];

It's possible to iterate over tuple elements via ``for`` operator.

*********************
*Structs and classes*
*********************

Structs and classes are sets of named values of different types and associated functions, types, variables.
See :doc:`structs` и :doc:`classes`.

*******
*Enums*
*******

See :doc:`enums`.

*******************
*Function pointers*
*******************

See :doc:`function_pointers`.


**************
*Raw pointers*
**************

See :doc:`raw_pointers`.
