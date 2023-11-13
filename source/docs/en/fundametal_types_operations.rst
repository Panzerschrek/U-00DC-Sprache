Fundamental types and operations for them
=========================================

***********
*void type*
***********

``void`` type is used mostly as return type for functions which return nothing.
It has zero size.
It may be initialized with empty initializer.
All ``void`` values are equal.
It's possible to assign ``void`` values, pass them by value or by reference into a function, return values and references of this type.

**************
*boolean type*
**************

Ü has ``bool`` type.
The only possible values are ``true`` and ``false``.
There are following operations for ``bool`` values:

* ``&`` - Binary logical AND.
* ``|`` - Binary logical OR.
* ``^`` - Binary logical XOR.
* ``!`` - Unary logical NOT.
* ``&&`` - Lazy logical AND.
* ``||`` - Lazy logical OR.

Result of each of these operations for ``bool`` is also of ``bool`` type.

***************
*Numeric types*
***************

Ü has several numeric types - integer and floating point.
Integer types are divided into signed and unsigned.

Floating point types:

* ``f32``
* ``f64``

Signed integer types:

* ``i8``
* ``i16``
* ``i32``
* ``i64``
* ``i128``

Unsigned integer types:

* ``u8``
* ``u16``
* ``u32``
* ``u64``
* ``u128``

Numeric suffix of the type name means its size in bits.

Following arithmetic operations are possible for values of numeric types:

* ``+`` - binary addition.
* ``-`` - binary difference.
* ``*`` - binary multiplication.
* ``/`` - binary division.
* ``%`` - binary reminder.
* ``-`` - Unary minus. It is equivalent to subtraction of a value from 0.

The exact behavior of an arithmetic operation is different for different numeric type kinds.
Operations for integer types may overflow.
Operations for floating point types may be saturated instead.

The result of the division for floating point values is nearest floating point value, for integers - nearest integer value.
For integers division result by zero is undefined, for floating point types result is ±infinity or ``NaN``.

**************************************
*bitwise operations for integer types*
**************************************

For values of integer types following bitwise operations may be used:

* ``&`` - Binary bitwise AND.
* ``|`` - Binary bitwise OR.
* ``^`` - Binary bitwise XOR.
* ``~`` - Unary bitwise inversion.


********
*Shifts*
********

It's possible to use bit shifts for values of integer types - left shift ``<<`` and right shift ``>>``.
The first operand is a shifted value, the second - number of bits to shift.

``<<`` operator performs left shift.
The most left bits are missing, the most right bits are replaced by zeros.

``>>`` operator performs right shift.
The most right bits are missing, the most left bits are replaced by 0 (for unsigned types) or by sign bit (for signed types).

***********
*size_type*
***********

``size_type`` is a built-in alias for one of unsigned integer types.
It aliases to a type with size equal to size of pointer on the target platform.
It is used in containers code, for indexing, etc.

************
*char types*
************

There are following char types in Ü:

* ``char8``
* ``char16``
* ``char32``

Numeric suffix of the type name means its size in bits.

Unlike numeric types there is no arithmetic and bitwise operations for char types.
But it is possible to compare values of char types.

*******************************
*raw data representation types*
*******************************

Ü has following types for raw data representation:

* ``byte8``
* ``byte16``
* ``byte32``
* ``byte64``
* ``byte128``

They exist to represent raw bytes and sequences of bytes (2 bytes, 4 bytes, etc.).
The only possible operation for values of these types is equality comparison.
It is possible to convert values of `byte`-types into numeric and char values and vice versa.
Conversion is performed by simply reinterpret bit-representation (even for floating-point types).

************
*Comparison*
************

There are several comparison operators in Ü.
Result of all of them is of ``bool`` type.

There are equality comparison operators ``==`` and ``!=`` for each fundamental type.

There are also order comparison operators for types except ``bool``, ``void`` and ``byte`` types:

* ``<`` - Less.
* ``<=`` - Less or equal.
* ``>`` - Greater.
* ``>=`` - Greater or equal.

For numeric values natural comparison order is used.
For chars comparison follows char number.

There are some caveats for floating point comparisons:

* ``+0`` and ``-0`` have different bit representation but in comparison are equal.
* Any comparison against ``NaN`` excluding ``!=`` returns ``false``.
  ``!=`` against ``NaN`` returns always ``true``. Also ``==`` with both ``NaN`` operands returns ``false``.
  From all this follows that ``NaN`` isn't equal to any value, even to itself.

For all types, for which order compare operators are supported, special operator ``<=>`` exists.
It returns result of ``i32`` type, -1 if left operand is less than right operand, +1 if left operand is greater that right operand, 0 if operands are equal.

********
*select*
********

Ü has operator for selection of one of two variants - ``select``.
It consists of ``select`` keyword and a body inside ``()``.
The body consists of a logical expression, of an expression for true value after ``?`` and of an expression after ``:`` for false value.
It works like this: evaluates first expression (that should be of ``bool`` type, then if its result is true - evaluate the second expression - else - the third.

.. code-block:: u_spr

   fn Foo()
   {
       auto x= select( true ? 1 : 2 ); // ``x`` will be equal 1
       auto y= select( false ? 0.5f : 3.5f ); // ``y`` will be equal 3.5
       var i32 mut z= 0, mut w= 0;
       select( x == 1 ? z : w )= 666; // ``select`` operator may be applied even for mutable references
   }

********************
*operators priority*
********************

In a complex expressions consisting of multiple operators calculation is performed in order of operators priority.
Unary operators have highest priority - are applied before any others.
Binary operators are calculated in priority from weak to strong:

* ``/``, ``*``, ``%``
* ``+``, ``-``
* ``<<``, ``>>``
* ``<=>``
* ``<``, ``<=``, ``>``, ``>=``
* ``==``, ``!=``
* ``&``
* ``^``
* ``|``
* ``&&``
* ``||``

The priorities above are like in C++.
Binary operator with the same priority are evaluated in left-to-right order.
It is possible to use ``()`` in order to change default priority.
