Structs
=======

A structure is a named type that contains a set of named values of (possible) different types.
Additionally a struct may contain functions (including special), types, variables.

Structure definition example:

.. code-block:: u_spr

   // Struct with two fields
   struct SomeStruct
   {
       i32 x;
       f32 y;
   }
   
   struct EmptyStruct {} // it's allowed to define empty structs
   
   // Struct with multiple values of different types, including arrays and tuples
   struct ComplexStruct
   {
       i32 x;
       [ f32, 16 ] arr;
       bool b;
       tup[ i32, f64 ] t;
   }

Struct field access is performed via ``.`` operator with field name after it.

.. code-block:: u_spr

   var SomeStruct mut s= zero_init;
   s.x= 5;
   s.y= 0.25f;
   ++s.x;
   s.y= f32( s.x );

******************
*Struct functions*
******************

A struct may contain functions inside:

.. code-block:: u_spr

   struct SomeStruct
   {
       i32 x;
       f32 y;
       
       fn GetFieldCount() : size_type
       {
           return 2s;
       }
   }

In the example above a regular free function is defined, even if it is located inside a struct.
It's possible to call it like this:

.. code-block:: u_spr

   var SomeStruct s= zero_init;
   auto x= s.GetFieldCount(); // It's possible to access a function via a variable of the struct type
   auto y= SomeStruct::GetFieldCount(); // Such way of functions access is also possible, without usage of any variable of the struct type


*********
*Methods*
*********

Methods are struct/class functions with first parameter named ``this``.
The type of this parameter is not specified - it is explicitly assumed to be of struct or class type where this method if located.
It's possible to specify mutability modifier before ``this`` - ``mut`` or ``imut``.
The meaning of this modifier is like for any other parameter.
``this`` parameter always must be defined as first parameter, it's forbidden to use name ``this`` for other parameters.
Methods may be called only with a variable of struct/class type.
Inside a method it's possible to access fields directly without using explicit ``this``.

Methods definition example:

.. code-block:: u_spr

   struct Rect
   {
       u32 w;
       u32 h;
       
       fn GetArea( this ) : u32
       {
           return w * h; // Fields "w" and "h" are "this" members
       }
       
       // Mutable method, "this" fields may be modified inside it
       fn SetWidth( mut this, u32 new_w )
       {
           w= new_w;
       }
       
       fn SetHeight( mut this, u32 new_h )
       {
           this.h= new_h; // It's possible to access fields explicitly via "this"
       }
   }

Methods usage example:

.. code-block:: u_spr

   var Rect mut rect= zero_init;
   rect.SetWidth( 5u );
   rect.SetHeight( 8u );
   auto area= rect.GetArea();

``this`` parameter is by-default a reference parameter.
But this behavior may be changed via ``byval`` prefix.
In such case ``this`` will be a value parameter like any other non-this value parameters in any other function.

.. code-block:: u_spr

   struct S
   {
       fn Foo( byval this ) : i32;
       fn Bar( byval mut this, i32 y ) : f32;
       fn Baz( byval imut this ) : bool;
   }

In ``byval`` ``this`` method call a struct value will be copied or moved into the argument, if the instance of the struct that is used for call is an immediate value.
``byval`` ``this`` argument will be destroyed (its destructor will be called) at the method end, but only if it was not moved.

***********************
*Other struct contents*
***********************

It's possible to define inside structs nested types (structs, classes, enums, type aliases) and variables.
They may be accessed like elements of namespaces.
