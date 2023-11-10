.. _unsafe-blocks:

Unsafe code
===========

Ü by-default doesn't allow some operations, that are considered to be unsafe:

* Explicit constructors and destructors access
* Accessing global mutable variables
* Raw pointer dereference
* ``cast_ref_unsafe`` operator
* ``cast_mut`` operator
* ``ununitialized`` initializer
* ``unsafe`` function call

But if such operation is necessary it should be used inside ``unsafe`` block.
This block consists of ``unsafe`` keyword and block body after it.
In such block all unsafe operations are allowed.

.. code-block:: u_spr

   fn Foo()
   {
       unsafe
       {
           var i32 mut x= uninitialized;
           x= 42;
       }
   }

It's possible to use ``safe`` block inside ``unsafe`` block in order to temporary forbid unsafe operations.

.. code-block:: u_spr

   fn Bar() unsafe;
   fn Foo()
   {
       unsafe
       {
           Bar(); // Ok, it's allowed to call an unsafe function
           safe
           {
               Bar(); // Error - here it's forbidden to call an unsafe function
           }
           Bar(); // Ok, now it's again allowed to call an unsafe function
       }
   }

There are also ``unsafe`` expressions.
They are similar to ``unsafe`` blocks but work inside expressions.
There are also ``safe`` expressions.

.. code-block:: u_spr

   fn Bar() unsafe : i32;
   fn Foo()
   {
      var i32 x= 2 * unsafe(Bar()); // Ok - unsafe context affects only unsafe function call.
   }

******************
*Unsafe functions*
******************

Unsafe functions are functions that can't be simple called without any consideration, it's required to satisfy some preconditions in order to call them.
Unsafe functions are marked with usage of ``unsafe`` keyword - after parameters list and reference pollution notation.
Such functions may be called only within an ``unsafe`` block or expression.
The body of an ``unsafe`` function is not a ``unsafe`` block, if it is necessary to perform some unsafe operations inside it ``unsafe`` block or expression should be used explicitely.

.. code-block:: u_spr

   fn Bar( i32 d ) unsafe : i32
   {
       unsafe
       {
           var i32 mut x= uninitialized;
           x= 100 / d;
           return x;
       }
   }
   
   fn Foo()
   {
       unsafe
       {
           Bar( 7 );
       }
   }
