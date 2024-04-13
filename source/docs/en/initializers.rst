Initialization
==============

Each variable or struct/class field must be initialized, either explicitly or implicitly.
Absence of initialization isn't allowed in ``safe`` code, the compiler ensures that all is initialized.

There are several initializer kinds, that are applicable dependent on the type of initialized value.

***************************
*Expression initialization*
***************************

This is the simplest initialization kind.
An expression is defined after ``=`` for a variable.

.. code-block:: u_spr

   var i32 x= 0;
   var f32 y= 1.0f - 72.5f / 2.0f;

In combined initializers there is no ``=``, expression initialization happens if only expression itself is specified.

*************************
*Sequence initialization*
*************************

It is used for initialization of arrays and tuples.
It consists of sequence of initializers separated by comma inside ``[]``.
The number of elements in the initializer should be equal to the number of elements in an array/in a tuple.

.. code-block:: u_spr

   var [ i32, 2 ] arr0[ 0, 1, 100 ]; // Initialize an array
   var [ f32, 3 ] arr1[ 0.0f, -5.0f, 44.4f ]; // Initialize an array
   var tup[ i32, bool ] tup0[ 5, false ]; // Initialize a tuple
   var [ [ i32, 2 ], 3 ] arr2[ [ 5, 7 ], [ 50, 70 ], [ 500, 700 ] ]; // Initialize a two-dimensional array
   var tup[ f32, [ i32, 3 ] ] tup1[ 0.0f, [ 0, 0, 0 ] ]; //Initialize a tuple with an array inside

****************************
*constructor initialization*
****************************

It's used for structs/classes initialization when they have constructor(s).
It consists of ``()`` and comma-separated arguments list inside.

.. code-block:: u_spr

   struct S
   {
       fn constructor(){}
       fn constructor(i32 in_x, i32 in_y) (x= in_x, y= in_y) {}
       i32 x= 0;
       i32 y= 0;
   }
   
   fn Foo()
   {
       var S s0(); // Call the constructor with zero parameters
       var S s1( 5, 7 ); // Call the constructor with two parameters
       var [ S, 2 ] arr0[ (), ( 99, 55 ) ]; // Call different constructors for different array elements
   }

For fundamental types it's possible to perform type conversion via constructor initialization or just make a copy.

.. code-block:: u_spr

   var i32 x= 0, x_copy(x);
   var f32 y(x); // conversion i32 -> f32
   var f64 z(y), w(x_copy); // conversions f32 -> f64 and i32 -> f64
   var u16 cc("B"c8); // conversion char8 -> u16
   var char32 cccc(66); // conversion i32 -> char32
   var u16 short_int(574);
   var i64 long_signed_int(short_int); // conversion u16 -> i64

Constructor initialization allows also to make copies for arrays and tuples.

.. code-block:: u_spr

   var [i32, 2] i0 = zero_init, i1(i0);
   var tup[bool, f32] t0[true, 14.5f], t1(t0);


******************************
*Member-by-member initializer*
******************************

It's used for structs initialization.
It consists of ``{}`` and comma-separated list of initializers for fields.
Each field is specified starting with ``.``.

.. code-block:: u_spr

   struct S
   {
       i32 x;
       i32 y;
       i32 z= 0;
   }
   
   fn Foo()
   {
       var S s0{ .x= 0, .y= 0, .z= 0 }; // Initialize all fields in their definition order
       var S s1{ .y= 1, .x= 0 }; // Order may be different. For fields with default initializer an explicit initializer may not be specified.
       var tup[ S ] t[ { .z= 0, .x= 2, .y= 2 } ];
   }

It's possible to use this initializer kind in expression context in order to construct temporary values of struct types.

.. code-block:: u_spr

   struct S{ i32 x; i32 y; }
   fn Bar(S s);
   fn Foo()
   {
       Bar( S{ .x= 42, .y= 24 } ); // Create temporary value of "S" type by initializing its fields, than pass it into a function.
   }


**********************
*Empty initialization*
**********************

It's allowed to specify no initializer, if there is a default-initialization for a given type.

.. code-block:: u_spr

   struct S
   {
       fn constructor() (x= 0, y= 0) {} // Default-constructor
   
       i32 x;
       i32 y;
   }
   
   fn Foo()
   {
       var S s; // Default constructor will be called
       var [ S, 8 ] arr; // Default constructor will be called for each array element
   }

*********************
*Zero initialization*
*********************

It's used for zero-initialization for numbers, ``false`` initialization for ``bool`` values, initialization with first element for enums, ``\0`` for char types, null for pointer types.
This initialization is not allowed for classes.
It is allowed for structs which contain no reference fields and explicit constructors except copy constructor.
Zero initialization is specified with usage of ``zero_init`` keyword.

.. code-block:: u_spr

   struct S
   {
       i32 x;
       i32 y;
   }
   
   enum E{ A, B, C, }

   fn Foo()
   {
       var i32 x= zero_init;
       var S s0= zero_init; // Zero whole struct
       var S s1{ .x= 4, .y= zero_init }; // Zero one of the struct fields
       var [ f32, 128 ] arr0= zero_init; // Zero whole number array
       var [ S, 3 ] arr1= zero_init; // Zero whole struct array
       var [ S, 2 ] arr2[ { .x= 1, .y= 1 }, zero_init ]; // Zero only one struct in an array
       var tup[ E, bool, i32, i64, f64 ] t= zero_init; // Zero whole tuple
   }

******************************
*Uninitialized initialization*
******************************

It allows to skip initialization.
But this is allowed only in ``unsafe`` blocks.
it's recommended to use it only if it's absolutely necessary for performance reasons and only if values will be initialized later.

.. code-block:: u_spr

   fn Foo()
   {
       unsafe
       {
           var i32 x= uninitialized;
       }
   }

*********************************
*Constructor initialization list*
*********************************

Constructors of structs and classes may have fields initialization list.
In this list initializers for fields are specified.
Inside initializers of one fields other fields (that was already initialized) may be used.
Fields with no initializer specified will be default-initialized before explicitly-initialized fields.
The fields order is irrelevant, the only constraint is that fields should not be accessed before they will be initialized.

.. code-block:: u_spr

   struct S
   {
       fn constructor()
       ( y= z + 1, x= y / 2 ) // "z" is initialized at the beginning explicitly, that "y" is initialized (with usage of "z"), that "x" is initialized with usage of previously initialized "x" value
       {}

       i32 x;
       i32 y;
       i32 z= 0;
   }

*************************
*Fields own initializers*
*************************

Struct and class fields may have own initializers, that are defined together with field definition.
A field will be initialized with it if no other initializer is specified.

.. code-block:: u_spr

   struct A
   {
       // A default-constructor will be generated for this struct, because all fields have initializers
       i32 x= 100;
   }
   
   struct Vec
   {
      fn constructor() () {} // All fields are initialized with their own initializers
      fn constructor(i32 in_x, i32 in_y) (x= in_x, y= in_y) {} // Fields are intialized with explicit initializers, fields own initializers are not used
      i32 x= 0;
      i32 y= 0;
   }
   
   struct SimpleVec
   {
       A a; // Field has default initializer since "A" has default-constructor
       i32 x= 0;
       i32 y= 0;
   }
   
   fn Foo()
   {
       var A a; // Generated default-constructor will be called. It was generated since all fields have initializers.
       var Vec v0(), v1, v2( 10, -5 ); // In first two cases own field initializers will be used, in the third - explicitly-specified initializer values
       var SimpleVec v3{}; // In the struct initializer fields initializers are not specified thus own field initializers will be used
   }
