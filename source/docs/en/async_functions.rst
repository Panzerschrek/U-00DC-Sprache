Async functions
===============

Async functions are coroutines which return single value.
They are declared with usage of ``async`` keyword.

Async functions return values like regular functions - via ``return`` operator.
Its usage rules are the same as in regular functions.

Simplest async function example:

.. code-block:: u_spr

   fn async DoubleIt(u32 x) : u32
   {
       return x * 2u;
   }

In async functions it's possible to use ``yield`` operator without a value.
Its usage pauses an async function execution, which may be resumed later with code following ``yield`` operator.

***********************
*Async functions usage*
***********************

Async function call returns an async function object.
It's possible to start/resume execution of an async fuction object via :ref:`if-coro-advance` operator for it.

Each usage of this operator resumes async function execution, but only if it was not finished yet.
Control flow may be passed to a block of ``if_coro_advance`` only once - when an async function finished and thus returned a result.

.. code-block:: u_spr

   // This async function makes several pauses before returning a result.
   fn async SimpleFunc() : i32
   {
       yield;
       yield;
       yield;
       return 555444;
   }
   
   fn Foo()
   {
       auto mut f= SimpleFunc();
       auto mut result= 0;
       // Execute "if_coro_advance" operator until the async function is not finished.
       // 3 full loop iteration will be executed (equal to the number of "yield" operators inside the async function body), a break from the loop will happen at 4th iteration.
       loop
       {
           if_coro_advance( x : f )
           {
               result= x;
               break;
           }
       }
   }

It's important to be careful with usage of ``if_coro_advance`` in a loop until a result will be obtained.
If an async function object is already finished, ``if_coro_advance`` will never return a result and thus the loop will be infinite.
In order to avoid this it's needed to check an async function object if it is already finished before entering the loop with ``if_coro_advance``.

****************
*await operator*
****************

There is ``await`` operator that simplifies async function calls.
This operator is a postfix operator that consists of dot (``.``) and ``await`` keyword and may be used for an async function object inside another async function.

``await`` operator works like this: it resumes passed async function execution, if it is finished - extracts its result, else the caller async function pauses its execution and after it will be resumed, control flow will be passed to a code, that again resumes passed async function execution etc., until passed async function execution isn't finished.

This operator is somewhat equivalent to the following code:

.. code-block:: u_spr

   loop
   {
       if_coro_advance( x : f )
       {
           // x - await operator result.
           break;
       }
       else
       {
           yield;
       }
   }

``await`` operator requires passed value to be an immediate value of an async function type.
It's also necessary that a passed function is not finished yet, otherwise ``halt`` will be executed.
After obtaining of the execution result passed async function object is destroyed properly.

``await`` operator usage example:

.. code-block:: u_spr

   fn async Foo( i32 x ) : i32;

   fn async Bar( i32 x, i32 y ) : i32
   {
       auto foo_res= Foo( x * y ).await;
       return foo_res / 3;
   }

In fact ``await`` operator is just a way to simplify an async function call from another async function.
Where for regular functions just regular call operator is used, for async function call operator with following ``await`` operator is used instead.

*********************
*Async function type*
*********************

Async function type is a type of an async function object.
Async functions return async function-type objects.

Ü has a special syntax for specifying of async function types.
It consists of ``async`` keyword, optional notation for inner references specification, optional ``non_sync`` tag, return type (with/without reference modifier).

.. code-block:: u_spr

   type IntAsyncFunc= async : i32; // Simplest async function
   var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
   type FloatRefAsyncFunc= async'imut' : f32 & @(return_references); // An async function that returns a reference and stores references inside.
   type NonSyncRefAsyncFunc= async'mut' non_sync : u64 &mut @(return_references); // non_sync async function that returns immutable reference and stores mutable references inside.

As it can be seen async function type isn't strictly affected by the details of a specific async function (by which it was created).
This allows to use the same variable for storing of async function object produced by calls to different async functions - with different bodies and parameters.

.. code-block:: u_spr

    // Async functions. Their return type is (async : i32).
   fn async Foo(i32 x, i32 y) : i32;
   fn async Bar() : i32;
    // A function which returns async function object but which is not async.
   fn CreateFunc(bool cond) : (async : i32)
   {
       return ( cond ? Foo( 14, 56 ) : Bar() );
   }
