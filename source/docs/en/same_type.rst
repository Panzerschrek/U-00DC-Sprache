Type comparison
===============

There is operator ``same_type`` in Ü, that allows to compare passed types.
This operator accepts two type-arguments and returns a constant value of ``bool`` type.
The value is true if types are equal and false otherwise.

Usage example:

.. code-block:: u_spr

   static_assert( same_type</ u32, size_type /> || same_type</ u64, size_type /> );

This operator is indended for unsage in template code for cases, where it is necessary to check if two types are equal.
Like this, for example:

.. code-block:: u_spr

   template</ type T /> fn Process( T& arg )
   {
       if( same_type</ T, ust::string8 /> )
       {
           ProcessString(arg);
       }
       else
       {
           ProcessGeneric(arg);
       }
   }

The comparison result is the same to type comparison inside the compiler.
Different type kinds are considered to be different.
For example, arrays/tuples/structs with the same contents are considered to be different.
Different from each other are all structs and classes - their identity is determined by their full name (including full namespace path).
This affects also inheritance - a child and its parent are different.
Different are also all enums.

Fundamental types are also different from each other.
Exception - ``size_type``, which is for now a type alias for one of unsigned integers.

Function pointer types are different if function types are different.
But there are some caveats.
Implicit ``void`` for return value is equal to explicitely-specified ``void``.
``imut`` and ``mut`` modifiers for value params doesn't affect function type.
There are also calling convention names that are different but internally represent the same calling convention, like "Ü" and "C".
``this`` param is equal to non-``this`` reference param of the struct or class type.

It's also important to mention that type aliases *doesn't* create different types.
For example:

.. code-block:: u_spr

   type Int= i32;
   static_assert( same_type</ Int, i32 /> ); // type comparison returns true
