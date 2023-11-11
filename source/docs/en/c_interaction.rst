C interaction
=============

Ü code may interact with C code.
C functions may be called from Ü.
Ü functions also may be called from C.

For C interaction Ü has some mechanisms.

*********************************
*functions without name mangling*
*********************************

Unlike in C function names in Ü are specially encoded in order to produce different symbol names for overloaded functions and functions with the same name in different scopes.
In order to disable such encoding a function may be marked as ``nomangle``.
This marker is specified after ``fn`` and optiщnal ``constexpr`` marker.
A name of a function marked as ``nomangle`` will be the same as in C.
Thus such function may be called from C.
It's also possible to declare a ``nomangle`` prototype for an implemented in C function in order to call it from Ü.

.. code-block:: u_spr

   fn nomangle SDL_Quit() : void;
   
   fn MyQuit()
   {
       SDL_Quit(); // Call extrernal function
   }
   
   // A function that may be accessed in C code
   fn nomangle SomeFoo() : i32
   {
       return 0;
   }

``nomangle`` functions have some limitations:

* Operators can't be ``nomangle``
* Member functions of structs or classes can't be ``nomangle``
* ``nomangle`` functions may be defined only in the root namespace
* It's not allowed to overload ``nomangle`` functions

*********************
*Calling conventions*
*********************

Functions and function pointers may have a calling convention specified.
It's specified after function parameters list and after ``unsafe`` (if it is present).

.. code-block:: u_spr

   fn Foo() call_conv("C");
   fn Bar(i32 x) unsafe call_conv("fast") {}
   fn nomangle Baz(f32 x, f32 y) call_conv("system"){}
   
   var (fn() call_conv("C")) ptr = Foo;

There are following calling convention names: ``C``, ``default``, ``Ü``, ``fast``, ``cold``, ``system``.

``C`` is the default calling convention that is used in C and C++.
``default`` and ``Ü`` are aliases for ``C``.
If no calling convention is specified it is assumed to be ``C``.
This calling convention should be generally used for C interaction.

``fast`` and ``cold`` are calling conventions respectively for fast and compact calls.
They are not compatible with similar calling conventions in other languages and may be used only in Ü code if this has sense.

``system`` is a platform-dependent alias for calling system functions.
For most of the platforms it is an alias for the ``C`` calling convention.
For 32-bit Windows it is an alias for ``stdcall`` calling convention that is used in WinAPI.

*****************************
*Structs with ordered fields*
*****************************

Regular structs and classes have no strong predefined fields order.
The compiler may reorder them for better performance and/or to reduce padding.
But in C this is different - fields order is guaranteed to be in the order of fields definitions.
In order  to use same order of fields like in C Ü has special marker for structs and classes - ``ordered``.
This marker is specified in a struct or class definition after optional class kind, parents list, optional ``non_sync`` tag.
A struct with this marker will have the same order of its fields as they are defined - like in C.

In the example below both structs will have identical layout.

.. code-block:: u_spr
  :caption: Ü cide

   struct A ordered
   {
       bool x;
       i32 y;
       bool z;
   }

.. code-block:: cpp
  :caption: C++ code

   struct A
   {
       bool x;
       int32_t y;
       bool z;
   };

****************************************
*C interaction limitations and warnings*
****************************************

Not each Ü function may be called from Ü and vice versa.
There are some limitations for calls.
Ü compiler doesn't know if a function is implemented in C and thus can't check a call correctness.
Ensuring the call correctness is a programmer's responsibility.

Value-parameters and return values may be of fundamental types, enum types, function pointer types and raw pointer types.
Composite types (structs or classes, arrays, tuples) are not supported.
But it's allowed to pass and return references, they are represented internally like pointers in C.

Structs which are passed into C code or obtained from it should have the same contents and layout as in C.
Exceptions are structs which fields are not accessed within Ü code and which are passed one by one (not in arrays).
Such structs may have different field count and fields types, it's only important for them to have size and alignment not less than in C code.

Tuples in Ü are equivalent to C structs with the same fields types and fields order as in the tuple.
Because of that equivalent structs should be used in C code for Ü tuples.

There is no (obviously) reference checking in C code.
Thus it's important to pay attention in C interaction that no reference checking rules are violated.

For better safety it's recommended to mark functions implemented in C as ``unsafe``.
This forces a programmer to use ``unsafe`` blocks to call these functions and thus be more careful.
