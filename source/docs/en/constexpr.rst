Constant expressions
====================

There are expressions in Ü, that are named constant expressions, which value may be calculated in compile-time.
Such expressions may be used in some places where it is necessary - for array sizes, for template arguments, in constructions like ``static_assert`` and ``static_if``.

An expression may be constant if:

* All variables inside it are constant
* Functions and operators in this expression are ``constexpr``.

*****************
*constexpr types*
*****************

Variables inside constant expressions should be constant.
Constant can be only variables of one of ``constexpr`` types.

``constexpr`` types are:

* All fundamental types
* All enums
* All function pointers
* Arrays with ``constexpr`` element types
* Tuples with all element types ``constexpr``
* Some struct types

``constexpr`` structs requirements:

* All field types should be ``constexpr``
* There should be no mutable reference fields
* There should be no explicit copy-constructors, copy-assignment operators, destructors

*********************
*constexpr functions*
*********************

Functions that may be used in constant expressions should be marked with ``constexpr`` keyword.
Such functions should have a body, ``constexpr`` for prototype declarations is not allowed.

``constexpr`` function requirements:

* Parameter types and return types should be ``constexpr``, but not function pointers
* They can't be marked as ``unsafe``
* They should not contain ``unsafe`` blocks
* They should not do reference pollution (see corresponding chapter)
* They can't contain definitions of variables of non-``constexpr`` types
* They can't contain calls to non-``constexpr`` functions
* They can't call functions via pointers

``constexpr`` functions may have mutable reference parameters, but such functions can't be called with ``constexpr`` arguments in order to produce ``constexpr`` result.
Almost all constructions are allowed inside ``constexpr`` functions (except some forbidden ones) this includes conditions, loops, ``halt``, etc.

``constexpr`` function example:

.. code-block:: u_spr

   fn constexpr ArrSumm( [ u32, 16 ]& arr ) : u32
   {
       auto mut res= 0u;
       auto mut i= 0s;
       while( i < 16s )
       {
           res+= arr[i];
           ++i;
       }
       return res;
   }

*********************
*constexpr variables*
*********************

Immutable variables of ``constexpr`` type which are constantly-initialized are considered to be ``constexpr``.
If it's necessary to ensure that a variable is ``constexpr``, ``constexpr`` mutability modifier should be used.

.. code-block:: u_spr

   auto x= 0; // Variable is immutable, its initializer is constant, thus variable is constexpr
   auto imut y= x + 5; // Same as above, but variable is explicitly marked as immutable
   var i32 z(66), imut v(-5), w= y / 2; // All these variables are constexpr
   var [ i32, 2 ] arr[ 0 + z, w * 10 ]; // These are constexpr too
   auto constexpr x_copy= x; // Explicitly specify constexpr
   var i32 constexpr ensure_constant(y); // Explicitly specify constexpr
   
   var i32 mut nonconst= 0; // Initializer is constant, but the variable itself isn't because it is mutable
   auto constexpr wtf= nonconst; // An error will be generated - initializer of constexpr variable is not constexpr
