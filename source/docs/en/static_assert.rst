static_assert
=============

``static_assert`` is a way to check during compile-time that some condition is true.
This operator consists of ``static_assrt`` keyword and a constant expression in ``()`` after this keyword.
The type of the expression must be ``bool``.
If the result of the expression is false the compiler will produce an error.

.. code-block:: u_spr

   fn Foo()
   {
       static_assert( 75 / 3 == 25 ); // Ok, expression is true
       static_assert( 0.0f != 0.0f ); // Expression is false - an error will be produced
   }

``static_assert`` may me used inside function bodies, inside structs and classes, inside namespaces.

.. code-block:: u_spr

   static_assert( true ); // Inside root namespace
   namespace N
   {
       static_assert( 1 + 2 == 3 ); // Inside namespace
   }
   
   struct S
   {
       static_assert( typeinfo</i32/>.size_of == 4s ); // Inside struct
   }
   
   fn Foo()
   {
       static_assert( 8u % 3u == 2u ); // Inside function
   }

There is also ``static_assert`` with a message, that is specified after comma after the condition expression.
If the expression result is false, the compiler will display this message together with the error.

.. code-block:: u_spr

   static_assert( false, "es tut mir leid" );

The compiler will produce something like this:

   error: Static assertion failed: Es tut mir leid.