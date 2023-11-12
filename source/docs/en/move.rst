Moving
======

In Ü there is a possibility in same places to replace costly copying operations with moving operations.
Moving is a change of object's location in the address space without changing object's contents.

Moving happens in cases where a movable object is an immediate value.
An immediate value is created via temp variable construction or as function call result.
During moving destructor for destination may be called (if there is an object in this place already) and than simple memory copy from source to destination is performed.

Moving may happen in the following cases:

* Passing a value argument into a function
* Assignment
* Initialization
* Value returning

.. code-block:: u_spr

   struct S{ i32 x = 0; }
   
   fn GetS() : S
   {
       // Temporary object of type "S" is created and than moved into return value.
       return S();
   }
   
   fn TakeS( S s ){}
   
   fn Foo()
   {
       // Temporary object of type "S" is created and than moved into "s0" variable during its initialization.
       auto s0= S();
       // A temporary value obtained via function call is moved into "s1" variable during its initialization.
       var S s1(GetS());
       var S mut s2;
       // A temporary object of type "S" is created and than moved into "s2" variable. Before moving destructor for "s2" is called.
       s2= S();
       // A temporary object of type "S" is created and than moved into function argument.
       TakeS( S() );
   }

***************
*move operator*
***************

Moving happens only for immediate values.
If an object is a stack variable or its part, a function argument, etc. it can't be simple moved.
But there is a way to force it to move - ``move`` operator.
This operator accepts a name of a local variable or function argument (including ``byval`` ``this``) and returns immediate value.
Moved variable is assumed to be destroyed and can't be accessed further.
Destructor for a moved variable will not be called.

.. code-block:: u_spr

   struct S{ i32 x = 0; }
   
   fn TakeS( S s ){}
   
   fn Foo( S mut s_arg )
   {
       var S mut s0, mut s1;
       // Copying of the local variable "s0" happens.
       TakeS( s0 );
       // Moving of the local variable "s1" happens.
       TakeS( move(s1) );
       // Moving of the argument "s_arg" happens.
       TakeS( move(s_arg) );
       // Destructor will be called for "s0" here, but not for moved variables "s1" and "s_arg".
   }

``move`` operator has some limitations.
It's allowed to move only local mutable variables and function arguments.
It's forbidden to move references, structs/arrays/tuples members, immutable variables.
It's not possible to move outer relative to a loop variable inside this loop.
It's not allowed to perform conditional moving - a variable must be moved in nono or in all branches of ``if-else``.

***************
*take operator*
***************

This operator is similar to ``move`` operator, but has some differences.
It accepts arbitrary expressions, not only a variable name.
It moves the result of this expression and than performs default-construction for the source.
If a moved value type isn't default-constructible an error will be produced.

.. code-block:: u_spr

   struct S
   {
       i32 x;
       fn constructor() ( x= 0 ) {}
       fn constructor( i32 in_x ) ( x= in_x ) {}
   }
   fn Foo()
   {
       var [ S, 3 ] mut arr[ (55), (77), (99) ];
       var S s= take(arr[1]); // The value inside "arr[1]" will be moved into a new variable "s". For "arr[1]" a default-constructor will be called.
   }
