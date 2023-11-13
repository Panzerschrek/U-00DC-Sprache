Reference casting
=================

Ü allows to convert references of one type into references of another type - explicitly or implicitly.

***********************
*Mutability conversion*
***********************

A mutable (``mut``) reference may be converted into immutable (``imut``) reference.

.. code-block:: u_spr

   fn Bar( i32 &imut x );
   struct S{ i32 &imut r; }
   fn Foo()
   {
       var i32 mut x= 0;
       var i32 &mut r= x;
       Bar( r ); // Implicit conversion of mutable reference "r" into immutable reference - function argument.
       var S s{ .r= r }; // Implicit conversion of mutable reference "r" into immutable reference - struct member.
       var i32 &imut r2= r;// Implicit conversion of mutable reference "r" into immutable local reference.
   }

********************************************
*Conversion into a reference to parent type*
********************************************

It's possible to convert a reference of polymorph class type into reference to any of its ancestors.

.. code-block:: u_spr

   class A interface {}
   class B interface : A {}
   class C polymorph {}
   class D : C, B {}
   
   fn FooA( A& a );
   fn FooB( B& b );
   fn FooC( C& c );
   
   fn Foo()
   {
       var D d;
       FooA(d); // Convert "d" reference into reference to indirect parent (grandparent).
       FooB(d); // Convert "d" reference into reference to an implemented interface.
       FooC(d); // Convert "d" reference into reference to base class.
   }

********************************
*Explicit reference conversions*
********************************

Sometimes it's necessary to convert a reference explicitly.
Special conversion operators exist for that.

``cast_ref`` operator is used for explicit reference type conversion.
Destination type should be specified.

.. code-block:: u_spr

   class A polymorph {}
   class B : A {}
   
   fn Foo( A& a );
   fn Foo( B& b );

   fn Foo()
   {
       var B b;
       Foo( cast_ref</ A />(b) ); // "b" reference will be converted to reference of type "A" and than "fn Foo( A& a )" will be called.
   }

``cast_imut`` operator is used for mutable to immutable references conversion.

.. code-block:: u_spr

   fn Foo( i32 &mut i );
   fn Foo( i32 &imut i );

   fn Foo()
   {
       var i32 mut x=0;
       Foo( cast_imut(x) ); // Mutable reference to "x" will be converted into immutable reference, than "fn Foo( i32 &imut i )" will be called.
       var i32 imut y= 0;
       var i32 &imut y_ref= cast_imut(y); // "cast_imut" operator in this case will leave reference immutable.
   }

``cast_ref_unsafe`` is almost like ``cast_ref`` operator, but it allows to perform conversions between unrelated types.
Because of that it may be used only in ``unsafe`` code.

.. code-block:: u_spr

   fn Foo()
   {
       var i32 mut x= 0;
       unsafe
       {
           var void &x_ref_v= cast_ref_unsafe</void/>(x); // Reference to "i32" will be converted into "void" reference
           var i32 &x_ref_i= cast_ref_unsafe</i32/>(x_ref_v); // Reference to "void" will be converted into "i32" reference
       }
   }

``cast_mut`` operator is used for immutable to mutable reference conversions.
It may be used only in ``unsafe`` code.

.. code-block:: u_spr

   fn Foo()
   {
       var i32 imut x= 0;
       unsafe
       {
           ++ cast_mut(x); // Immutable reference to "x" will be converted into mutable reference, than referenced value will be mutated.
       }
   }
