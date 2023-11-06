Control flow
============

Ü has some constructions for control flow.

************
*Conditions*
************

``if`` operator allows to execute some code part conditionally.
The condition inside ``if`` operator must be of type ``bool``.

.. code-block:: u_spr

   if( x > y )
   {
       ++x;
   }

It's possible to specify alternative action if condition in ``if`` is false:

.. code-block:: u_spr

   if( x > y )
   {
       ++x;
   }
   else
   {
       ++y;
   }

It's also possible to specify several conditions and actions for them:

.. code-block:: u_spr

   if( x > y )
   {
       ++x;
   }
   else if( y > x )
   {
       ++y;
   }
   else if( z )
   {
       return;
   }
   else
   {
       ++x;
       ++y;
   }

*******************
*Static conditions*
*******************

There is a special form of the conditional operator - ``static_if``.
It accepts only ``constexpr`` conditions.
The branch which will be executed is determined entirely in compile-time based on provided condition(s).
Branches that should not be executed are skipped by the compiler entirely.

.. code-block:: u_spr

   static_if( typeinfo</size_type/>.size_of == 8s )
   {
       return 0; // Either this branch will be compiled
   }
   else static_if( typeinfo</size_type/>.size_of == 4s )
   {
       return 1; // Or this
   }
   else
   {
       // In strange cases even this branch may be compiled
       static_assert(false); // but compilation will fail because of this assert
       halt;
   }

**********************
*Conditions combining*
**********************

Different conditional operators - ``if``, ``static_if`` and also :ref:`if-coro-advance` may be combined together in any way.
After ``else`` of any of these operators may follow any other.
An usage of any conditional operator after ``else`` is equivalent to nesting of the second conditional operator inside a block after ``else`` of the first operator.

For example this code:

.. code-block:: u_spr

   static_if( static_condition )
   {
       Action0();
   }
   else if( dynamic_condition )
   {
       Action1();
   }
   else if_coro_advance( x : some_gen )
   {
       Action2(x);
   }
   else
   {
       Action3(x);
   }

Is equivalent to this code:

.. code-block:: u_spr

   static_if( static_condition )
   {
       Action0();
   }
   else
   {
       if( dynamic_condition )
       {
           Action1()
       }
       else
       {
           if_coro_advance( x : some_gen )
           {
               Action2(x);
           }
           else
           {
               Action3(x);
           }
       }
   }


*****************
*switch operator*
*****************

Ü has ``switch`` operator, which allows to transfer control flow to a block depending on a value of some variable.

An example of simple ``switch``:

.. code-block:: u_spr

   switch(x)
   {
       0 -> { return -1; },
       1 -> {},
       2 -> { halt; },
       // other handles following
   }


This operator works only with values of integer and enum types.
Values to compare must be ``constexpr``.

``switch`` operator allows to specify several values for single block of code - via comma.

It's also possible to specify value ranges.
Control flow will be transfered to a block if a value is greater or equial to minimum range value and less or equal to maximum range value.
It's allowed to skip specifying lower/upper range values, in such cases minimum/maximum value of the type will be used.

For not listed values ``default`` handler may be used.
It may not be the last, it is only important to have no more than one default handler.

.. code-block:: u_spr

   switch( x )
   {
       33, ... -7, 66 ... 78, 999 ... -> { return 777; }, // value 33, range [-int_max; -7], range [66; 78], range [999; int_max]
       96 ... 108, 80 -> { return 888; }, // range [96; 108], value 80
       82, 200 ... 300 -> { return 999; }, // value 82, range [200; 300]
       default -> { return 1000; }, // all other values outside values/ranges listed above
   }

The compiler checks that ``switch`` operator handles all possible cases.
If this is not true - an error will be produced.

.. code-block:: u_spr

   enum E{ A, B, C }

   fn Foo( E e )
   {
       switch( e )
       {
            E::A -> {},
            E::B -> {},
            // error - E::C is not handled
       }
   }

If a ``default`` handler exists it is assumed that it handles all values not listed explicitely.
But if ``default`` if unnecessary, the compiler will produce an error.

.. code-block:: u_spr

   fn Foo( u8 x )
   {
       switch( x )
       {
            0u8 -> {},
            1u8 -> {},
            2u8 ... -> {}, // Handles all values from 2 to the maximum
            default -> {}, // error - this handler is unreachable
       }
   }


************
*while loop*
************

``while`` operator allows to repeat some operations until the condition is true.
The condition should be ``bool`` type.

.. code-block:: u_spr

   while( x > 0 )
   {
       --x;
   }

It's possible to break from a loop early with usage of ``break`` operator:

.. code-block:: u_spr

   while( x > 0 )
   {
       x /= 5;
       if( x == 1 )
       {
           break;
       }
   }

It's also possible to continue to the next loop operation with usage of ``continue`` operator:

.. code-block:: u_spr

   while( x > 0 )
   {
       x /= 3;
       if( x == 5 )
       {
           continue;
       }
       --x;
   }


``while`` loop (and other loop kinds) may have a label (with usage of ``label`` keyword) and use it in ``break`` and ``continue`` operators.
This allows to continue to the next iteration of outer loop or break from it.

.. code-block:: u_spr

   while( Cond0() ) label outer
   {
       while( Cond1() )
       {
           if( Cond2() )
           {
               continue label outer; // continue outer loop
           }
           if( Cond3() )
           {
               break label outer; // break from outer loop
           }
       }
   }


**********
*for loop*
**********

There is also ``for`` loop in Ü, that is similar to such loop in C++.
It consists of three parts - variables declaration part, condition part, iteration part.
Parts are separated by ``;``.
Each part is optional.
If no condition is specified a loop is considered to be unconditional - ends only with ``break`` or ``return``.
``for`` loop allows to perform some actions always at the and of any iteration, each ``continue`` operator will jump to the iteration part.

``for`` loop examples:

.. code-block:: u_spr

   auto mut x= 0;
   for( auto mut i= 0; i < 10; ++i )  // Declare a variable via "auto"
   {
      x+= i * i;
   }

.. code-block:: u_spr

   auto mut x= 0;
   for( var i32 mut i= 0, mut j= 2; i < 5; ++i, j*= 2 ) // Declare variables via "var", more that one action in iteration part
   {
      x+= i * j;
   }

.. code-block:: u_spr

   for( auto mut i = 1; ; i <<= 1u ) // Unconditional loop
   {
      if( i < 0 ){ break; }
   }

.. code-block:: u_spr

   for( ; ; ) // A loop without any elements
   {
      break;
   }

.. code-block:: u_spr

   for( var u32 mut x= 0u; x < 100u; ++x )
   {
      if( SomeFunc(x) ){ continue; } // After "continue" "++x" will be executed
      SomeFunc2(x);
   }

*************
*simple loop*
*************

Ü has simple undconditional loop - ``loop``
It is (almost) equivalent to the ``while`` loop with always true condition.
The only way to end this loop is to use ``break``, ``return`` or ``continue`` to some other outer loop.
There is a reason to use it in cases when a loop end/continue condition may be calculated only inside the body of the loop.

.. code-block:: u_spr

   loop
   {
      // Some code
      if( SomeCondition() )
      {
         break;
      }
   }

It's important to know that if ``loop`` has no ``break`` (for this loop) any code after this loop is considered to be unreachable.

.. code-block:: u_spr

   loop
   {
      if( SomeCondition() )
      {
         return;
      }
   }
   auto x = 0; // The compiler will produce here an error, because this code is unreachable.

**********************
*return from function*
**********************

The execution of a function that returns no value ends when control flow reaches the end of the function.
But if it necessary to end it earlier, ``return`` operator may be used.

.. code-block:: u_spr

   fn Clamp( i32 &mut x )
   {
       if( x >= 0 )
       {
           return;
       }
       x= 0;
   }

Functions that return a value should always end with ``return`` with a value.
A type of ``return`` expession must be the same as function return type (or convertible to it).

.. code-block:: u_spr

   fn Add( i32 x, i32 y ) : i32
   {
       return x + y;
   }

The compiler ensures that a function returns always.
Otherwise an error will be generated.

.. code-block:: u_spr

   fn Clamp( i32 &mut x ) : bool
   {
       if( x >= 0 )
       {
           return false;
       }
       x= 0;
       // Error, function returns not always.
   }

.. code-block:: u_spr

   fn Clamp( i32 &mut x ) : bool
   {
       if( x >= 0 )
       {
           return false;
       }
       else
       {
           x= 0;
           return true;
       }
       // All ok - function always returns.
   }
