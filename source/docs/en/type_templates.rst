Type templates
==============

It's possible to create abstract types in Ü, that are parameterized with other types and values.
Such types are named template types.

****************************
*Struct and class templates*
****************************

It's needed to use ``template`` keyword with template parameters listed in ``<//>`` in order to create a struct or class template.
There are two kinds of parameters - types and values.
For a type parameter ``type`` keyword should be specified before the parameter name.
For a value parameter type name should be used before the parameter name.

.. code-block:: u_spr

   template</ type T />
   struct Box{ T t; }
   
   template</ type T, size_type s />
   struct Arr{ [ T, s ] arr; }

For usage of type template its instantiation is required.
Template arguments in ``<//>`` should be specified after the name of the template in order to do this.

.. code-block:: u_spr

   template</ type T />
   struct Box{ T t; }
   
   template</ type T, size_type s />
   struct Arr{ [ T, s ] arr; }
   
   fn Foo()
   {
       var Box</i32/> b_i{ .t= 0 };
       var Box</f32/> b_f{ .t= 0.0f };
       var Arr</ bool, 3s /> a{ .arr[ false, true, false ] };
   }


***********************************************
*Type templates specialization and overloading*
***********************************************

In examples above short form of type templates is used.
But there is also a full form of type templates.
This form contains signature parameters of the template, which are specified after the name of the template struct or class.
It's possible to specify complex type names - arrays, tuples, functions and other templates.
Default signature arguments are also possible.

.. code-block:: u_spr

   template</ type T />
   struct TypeExtractor</ [ T, 4s ] />
   {
       type ElementType= T;
   }
   
   // Specify an array type in template instantiation - this template requires it.
   type E= TypeExtractor</ [ f32, 4s ] />::ElementType;
   
   fn Foo() : E
   {
       return 3.14f;
   }

.. code-block:: u_spr

   // Template with default signature argument.
   template</ type T, type U />
   struct S</ T, U= i32 />
   {
       T t;
       U u;
   }
   
   fn Foo()
   {
       var S</ f32 /> s0{ .t= 0.0f, .u= 0 }; // No second argument specified - default signature argument "i32" will be used.
       var S</ i32, bool /> s1{ .t= 5, .u= false }; // The second argument is specified.
   }

It's allowed to define multiple type templates with the same name, but different signature parameters.
In case if it's possible to instantiate more than one template, more specialized one will be chosen.

Specialization rules are the following: a specific type is better than an array, tuple, template type. An array, tuple, template type is better that a template parameter.

.. code-block:: u_spr

   template</ type T, size_type S />
   struct S</ [ T, S ] />
   {
       auto x= 1;
   }
   
   template</ type T />
   struct S</ T />
   {
       auto x= 2;
   }
   
   static_assert( S</ i32 />::x == 2 );
   static_assert( S</ [ f32, 64s ] />::x == 1 );

**************************
*Template value arguments*
**************************

As mentioned above, value template arguments are also possible (not only type arguments).
These values should be of one of the allowed types - ``bool``, integer types, character types, ``byte``-types.
Arrays and tuples are also possible, if their element types are types, listed above.

.. code-block:: u_spr

   enum E { A, B, C }
   type ArgType= tup[ [ i32, 2 ], char8, bool, E ];

   // Struct template with composite value parameter.
   template</ ArgType arg /> struct S {}

   var ArgType constexpr my_arg[ [ 7, -5 ], "y"c8, true, E::B ];

   // Parameterize the template with a composite value.
   type MyS= S</ my_arg />;

**********************
*Type alias templates*
**********************

There are also templates for type aliases.
They are similar to struct or class templates.

.. code-block:: u_spr

   template</ type T />
   struct Box{ T t; }
   
   template</ type T /> type BoxAlias= Box</ T />; // A template for a template struct.
   template</ type T /> type MyVec3= [ T, 3 ]; // A template for an array.
