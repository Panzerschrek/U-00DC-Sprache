Type aliases
============

There is a possibility in Ü to create a new name for an existing type.
A new name is defined with usage of ``type`` keyword and source type name after ``=``.
Such type alias may be used anywhere where a type name is required.

.. code-block:: u_spr

   type MyInt = i32;
   
   fn Bar( MyInt x );
   
   fn Foo()
   {
      var MyInt x= 0;
      Bar(x);
   }

A definition of a type alias doesn't create new type, but creates only new name for accessing an existing type.
This means that a type and its alias(es) are interchangeable.
Different type aliases are also interchangeable.

.. code-block:: u_spr

   type MyInt = i32;
   type I32 = i32;
   
   fn Bar( i32 x );
   
   fn Foo()
   {
       var MyInt x= 0;
       var I32 y= 0;
       var MyInt y_copy= y; // Use a value defined with usage of "I32" alias for initialization of a variable defined with usage of "MyInt" alias.
       var I32 x_copy= x; // Same as above, but in another direction.
       Bar(x); // "MyInt" is "i32", so, it's allowed to call function that requires "i32" parameter.
       Bar(y); // "I32" is "i32", so, it's allowed to call function that requires "i32" parameter.
   }

Type aliases may be created for any type:

.. code-block:: u_spr
   
   type IntVec3 = [ i32, 3 ]; // Alias for an array
   type FloatPair = tup[ f32, f32 ]; // Alias for a tuple
   
   struct S{}
   type SAlias = S; // Alais for a struct
   
   type AnotherVec = IntVec3; // Alias for an alias for an array
   
   fn Foo()
   {
       var IntVec3 v[ 0, 1, 2 ];
       var FloatPair p[ 0.25f, 0.33f ];
       var SAlias s{};
       var AnotherVec v2[ 55, 625, 111 ];
   }
