Generators
==========

A generator is a coroutine which may return zero or more values.
Generator-functions are defined with usage of ``generator`` keyword.

It's possible to use operator ``yield`` inside a generator, which is used for producing of a new value by the generator.
A value (or a reference) passed into ``yield`` is returned to a code that calls this generator.
This operator is not terminal - execution is possible after it.

Generator-function with operator ``yield`` usage:

.. code-block:: u_spr

   fn generator GenNumbers() : i32
   {
       yield 1;
       yield 2;
       yield 3;
   }

Low-level functionality of the ``yield`` operator is the following: it fills result value, pauses generator execution and returns control flow to a generator caller.
A caller extracts the value produced by the ``yield`` operator.

It's also possible to use ``return`` operator inside generators.
A ``retrun`` operator without a value just finishes a generator.
A ``return`` operator with a value is identical to a combination of ``yield`` and empty ``return``.
If a generator function has no ``return`` operator(s) it will be produced implicitly - at the end of the function, like for regular functions returning ``void``.

.. code-block:: u_spr

   fn generator GenNumbers(bool cond) : i32
   {
       yield 1;
       if( cond )
       {
           return;
       }
       else
       {
           return 2; // is equivalent to  yield 2; return;
       }
   }


****************
*Generator call*
****************

Values are obtained from a generator with :ref:`if-coro-advance` operator.
Usually it is used in a loop, because many generators produce sequences of values.
If no control flow was transferred to a block of ``if_coro_advance``, means that a generator was finished.

****************
*Generator type*
****************

Generator type is a type of generator-objects.
Generator functions return generator-type objects.

Ü has a special syntax for specifying of generator types.
It consists of ``generator`` keyword, optional notation for inner references specification, optional ``non_sync`` tag, return type (with/without reference modifier).

.. code-block:: u_spr

   type IntGen= generator : i32; // Simplest generator
   var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
   type FloatRefGen= generator'imut' : f32 & @(return_references); // A generator that returns a reference and stores references inside.
   type NonSyncRefGen= generator'mut' non_sync : u64 &mut @(return_references); // non_sync generator that returns immutable reference and stores mutable references inside.

As it can be seen generator type isn't strictly affected by the details of a specific generator-function (by which it was created).
This allows to use the same variable for storing of generators produced by calls to different generator-functions - with different bodies and parameters.

.. code-block:: u_spr

   // Generator-functions. Their return type is (generator : i32).
   fn generator Foo(i32 x, i32 y) : i32;
   fn generator Bar() : i32;
   // A function which returns generator-object but which is not a generator-function.
   fn CreateGen(bool cond) : (generator : i32)
   {
       return select(cond ? Foo( 14, 56 ) : Bar() );
   }
