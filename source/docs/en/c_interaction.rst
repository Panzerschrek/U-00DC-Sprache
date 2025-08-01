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

   fn nomangle SDL_Quit() call_conv( "C" ) : void;
   
   fn MyQuit()
   {
       SDL_Quit(); // Call extrernal function
   }
   
   // A function that may be accessed in C code
   fn nomangle SomeFoo() call_conv( "C" ) : i32
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
Any expression which results into constexpr ``char8`` elements array may be used for calling convention name.

.. code-block:: u_spr

   fn Foo() call_conv("C");
   fn Bar(i32 x) unsafe call_conv("fast") {}
   fn nomangle Baz(f32 x, f32 y) call_conv("system"){}
   
   var (fn() call_conv("C")) ptr = Foo;

There are following calling convention names: ``default``, ``Ü``, ``C``, ``fast``, ``cold``, ``system``.

``default`` is the default calling convention.
Functions with no calling convention specified have such convention.
Some special methods can use only this calling convention.
``Ü`` is just alias for ``default``, they are equal to each other.

``C`` is the calling convention, that is used in C and C++.
This calling convention should be generally used for C interaction.

``fast`` and ``cold`` are calling conventions respectively for fast and compact calls.
They are not compatible with similar calling conventions in other languages and may be used only in Ü code if this has sense.

``system`` is the calling convention for calling system functions.
For most of the platforms it's equivalent to ``C`` calling convention (but doesn't equal to it).
For 32-bit Windows it's equivalent to ``stdcall`` calling convention, that is used in WinAPI.

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
  :caption: Ü code

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


********************
*External functions*
********************

Ü has a special operator designed for accessing external functions (defined outside Ü code).
It consists of keywords ``import`` and ``fn``, function type in ``<//>`` and function name as string in ``()``.
This operator returns a function pointer for the requested function.

.. code-block:: u_spr

   fn Foo()
   {
       unsafe
       {
           auto f= import fn</ fn() : i32 />( "_some_external_function" ); // Obtain a pointer for specified function
           var i32 x= f(); // Call it
       }
   }

It's allowed to use this operator only in ``unsafe`` blocks and expressions.
It's necessary, since a programmer should ensure that the type specified is compatible with the type of the function defined externally and that there is no name conflicts with functions written in Ü.

This operator is intended to be used in cases, where it's not possible to write a prototype for some external function.
This may be the case, if the name of such function isn't correct Ü name, like it starts with ``_``, contains forbidden symbols or is an Ü keyword.


***************************
*External global variables*
***************************

Ü has a special operator designed for accessing external global variables (defined outside Ü code).
It consists of keywords ``import`` and ``var``, variable type in ``<//>`` and variable name as string in ``()``.
This operator returns a mutable reference for the requested global variable.

.. code-block:: u_spr

   fn Foo()
   {
       unsafe
       {
           var i32 &mut x= import var</ i32 />( "__some_var" ); // Obtain a reference to required variable
           ++x; // Can modify this variable
       }
   }

This operator is necessary for accessing external global variables (defined outside Ü code), because there are no other ways in the language to do this.
A returned reference is always mutable, if an external variable is defined as constant, changing it via this mutable reference isn't allowed.
Thread-local variables aren't supported.
One need to use this operator with caution.
It's allowed only within ``unsafe`` blocks and expressions.


****************************************
*C interaction limitations and warnings*
****************************************

Not each Ü function may be called from Ü and vice versa.
There are some limitations for calls.
Ü compiler doesn't know if a function is implemented in C and thus can't check a call correctness.
Ensuring the call correctness is a programmer's responsibility.

Structs which are passed into C code or obtained from it should have the same contents and layout as in C.
Exceptions are structs which fields are not accessed within Ü code and which are passed one by one (not in arrays).
Such structs may have different field count and fields types, it's only important for them to have size and alignment not less than in C code.

Tuples in Ü are equivalent to C structs with the same fields types and fields order as in the tuple.
Because of that equivalent structs should be used in C code for Ü tuples.

There is no (obviously) reference checking in C code.
Thus it's important to pay attention in C interaction that no reference checking rules are violated.

For better safety it's recommended to mark functions implemented in C as ``unsafe``.
This forces a programmer to use ``unsafe`` blocks to call these functions and thus be more careful.
