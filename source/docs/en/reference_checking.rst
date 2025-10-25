Reference checking
==================

Reference checking is one of the key features of Ü, which allow to reduce number of errors in programs.
This mechanism allows to find in compile-time use-after-free errors, aliasing errors, dangling references errors, etc.

**************************
*Reference checking rules*
**************************

The main rule of the reference checking is the following: in each point of the control flow each variable or reference should have only one derived mutable reference or zero or more derived immutable references.
The compiler ensures in compile-time that this rule is not violated and generates error(s) otherwise.

********************
*Derived references*
********************

A derived reference is a reference produced with usage of source variable or reference.

.. code-block:: u_spr

   fn Foo()
   {
       var i32 x = 0;
       var i32 &y= x; // "у" is a derived from "x" reference
       var i32 &z= y; // "z" is a derived form "y" reference
   }

A reference to an array element is a derived from this array reference.

.. code-block:: u_spr

   fn Foo()
   {
       var [ f64, 4 ] a= zero_init;
       var f64 &a_ref= a[2]; // "a_ref" is a derived from "a" reference
   }

A reference that is a function call result is considered to be derived from reference arguments of the function.
Such reference may be derived from more than one source variable/reference.

.. code-block:: u_spr

   fn Pass( f32 &mut x ) : f32 &mut
   {
       return x;
   }
   
   fn Min( i32 & a, i32 & b ) : i32 &
   {
       if( a < b ) { return a; }
       return b;
   }
   
   fn Foo()
   {
       var f32 mut f= 0.5f;
       var f32 &mut f_ref= Pass(f); // "f_ref" is a derived from "f" reference
       
       var i32 a= 8, b= 7;
       var i32 &ab_ref= Min(a, b); // "ab_ref" is a derived from both "a" and "b" reference
   }

A reference stored inside a struct value is also may be derived from another variable/reference.

.. code-block:: u_spr

   struct S{ i32& r; }
   fn Foo()
   {
       var i32 x= 0;
       var S s{ .r= x }; // "s.r" is a derived from "x" reference
       var i32& r2= s.r; // "r2" is a derived from "s.r" reference
   }

******************
*Child references*
******************

Child references are different from derived references.
A child reference is a reference to a non-reference struct or class field or to a tuple element.
The main difference compared to derived references is that it's allowed to create more than one mutable child reference to a variable, but only if these references are created for different variable members (fields or tuple elements).
This allows, for example, to change simultaneously different fields of the same struct instance.

.. code-block:: u_spr

   struct S{ i32 x; i32 y; }
   fn Swap( i32 &mut a, i32 &mut b );
   fn Foo()
   {
       var S mut s= zero_init;
       var tup[i32, i32] mut t= zero_init;
       var i32 &mut x_ref= s.x; // First child reference is created - to "x" struct field.
       var i32 &mut y_ref= s.y; // Second child reference is created - to different field "y".
       Swap( t[0], t[1] ); // Mutate simultaneously different elements of the same tuple instance.
   }

******************************************
*Managing derived references in functions*
******************************************

By-default it's assumed that a reference result of a function is derived from all its reference arguments.
But there are functions which really return references that are derived only from some of the arguments.
For such cases there is a way to annotate a function in a special way in order to avoid creating unnecessary derived references for its result.

In a function declaration after specifying the return reference modifier it's possible to specify ``@`` character with a following expression in ``()``.
The expression must be constant and be an array of ``[ char8, 2 ]`` elements.
Each element of the array is a description of one of the function parameter references in some special format.
The first value is a character in the range ``0`` to ``9`` for parameter index designating.
The second value is ``_`` character for designating the reference of the parameter itself or a character in the range from ``a`` to ``z`` for designating one of the inner reference tags of the parameter type.
The whole array designates a possible set of references which this function returns.

.. code-block:: u_spr

   var [ [ char8, 2 ], 1 ] return_references_foo[ "0_" ];
   fn Foo( i32 & a, i32 & b ) : i32 & @(return_references_foo); // This function returns only a reference derived from "a" argument
   var [ [ char8, 2 ], 2 ] return_references_bar[ "0_", "2_" ];
   fn Bar( f32 & a, f32 & b, f32 & c ) : f32 & @(return_references_bar); // This function returns a reference derived from arguments "a" and "c"
   
   fn Baz()
   {
       var i32 i0= 0, i1= 0;
       var f32 f0= 0.0f, f1= 0.0f, f2= 0.0f;
       var i32 &i_ref= Foo(i0, i1); // "i_ref" is a derived from "i0" but not from "i1" reference
       var f32 &f_ref= Bar(f0, f1, f2); // "f_ref" is a derived from "f0" and "f2" but not from "f1" reference
   }

The compiler ensures that only allowed references are returned:

.. code-block:: u_spr

   var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
   fn Foo( i32 & a, i32 & b ) : i32 & @(return_references)
   {
      return b; // An error will be produced - returning unallowed reference
   }

It's possible to specify an expression inside ``@()`` after specifying a type for the return value.
This expression should be a tuple of arrays of ``[ char8, 2 ]`` elements.
Each tuple element designates a set of references for the corresponding inner reference tag of the return value.

.. code-block:: u_spr

   struct S{ i32& r; }
   
   var tup[ [ [ char8, 2 ], 2 ] ] return_inner_references[ [ "0_", "1a" ] ];
   fn Foo( i32 & a, S s, i32 & z ) : S @(return_inner_references)
   {
       if( a > s.r && z != 0 )
       {
           var S ret{ .r= a };
           return ret;
       }
       else
       {
           var S ret{ .r= s.r };
           return ret;
       }
   }

*********************
*Reference pollution*
*********************

Some functions may create derived from their arguments references inside other arguments.
This is named "reference pollution".
For a function that performs reference pollution a special notation is required - via an expression in ``@()`` after the parameters list.
This expression must be a constant array of ``[ [ char8, 2 ], 2 ]`` elements.
Each element is a pair of reference descriptions - for the destination and for the source.
References are designated like in return references notation.

.. code-block:: u_spr

   struct S{ i32& r; }
   var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
   fn Foo( S &mut s, i32& r ) @(pollution); // This function creates a derived from "r" argument reference inside "s" argument.

   fn Bar()
   {
       var i32 x= 0, y= 0;
       var S mut s{ .r= x }; // "s.r" is a derived from "x" reference
       Foo( s, y ); // After this call "s.r" is also a derived from "y" reference
   }

If a function performs reference pollution but it is not specified, the compiler will produce an error.

.. code-block:: u_spr

   struct S{ i32& r; }
   var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
   fn Foo( S &mut s, i32& r ) @(pollution); // Function creates derived from "r" argument reference inside argument "s".
   
   fn Bar( S &mut s, i32 & r )
   {
       Foo(s, r); // An error will be produced - unallowed reference pollution
   }

It's not allowed to specify reference pollution notation for copy-constructors and copy-assignment operators.
The compiler generates such notation automatically according to the copying semantics.

*******************************
*Reference notation for fields*
*******************************

Structs and classes may also have references inside.
And there is a necessity for the compiler to track these references.
Because of that the compiler creates logical references for such types (named reference tags).

A struct without reference fields and fields with references inside has 0 inner reference tags.
A struct with single reference field has 1 reference tag.
A struct with single field that contains N reference tags (N > 0) has N reference tags.

It's more complicated with a struct that contains several reference fields and/or fields with references inside.
There is a special notation in order to perform mapping of these references to the struct's reference tags.

For reference fields it's possible to specify an expression in ``@()`` after the reference modifier.
The expression should be constant and be of ``char8`` type.
Allowed values are characters in the range from ``a`` up to ``z`` that designate corresponding inner reference tags of the struct.
This expression allows to associate a reference field with a reference tag of the struct.

For non-reference fields it's possible to specify an expression in ``@()`` after the type of the field.
The expression should be constant and be an array of ``char8`` elements.
Allowed values are characters in the range from ``a`` up to ``z`` that designate corresponding inner reference tags of the struct.
This expression allows to associate inner reference tags of the field type with reference tags of the struct.

Eventually a struct will have the number of reference tags one more than maximum index of the specified tags.
But skipping some reference tags isn't allowed.

The way described above allows to specify a mapping between struct fields and reference tags that are specified in the reference notation(s) of functions.
Example:

.. code-block:: u_spr

   struct S
   {
       i32& @('a') x; // Reference points to tag "a" (#0).
       i32& @('b') y; // Reference points to tag "b" (#1).
   }
   static_assert( typeinfo</S/>.reference_tag_count == 2s );

   struct T
   {
       f32 &mut @('a') f;
       bool& @('b') b;
       // Map tags "c" (#3) and "d" (#4) to inner references of "S".
       S @("cd") s;
       // Map tag "e" (#5) to two different references.
       u64& @('e') i0;
       i64& @('e') i1;
   }
   static_assert( typeinfo</T/>.reference_tag_count == 5s );

   // Function returns a struct, different inner reference tags of which are pointing to different reference arguments.
   // "x" reference marked with "a" tag (#0) will point to reference argument "x".
   // "y" reference marked with "b" tag (#1) will point to reference argument "y".
   var tup[ [ [ char8, 2 ], 1 ], [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ], [ "1_" ] ];
   fn MakeS( i32& x, i32& y ) : S @(return_inner_references)
   {
       var S s{ .x= x, .y= y };
       return s;
   }

   // Function writes a reference to "i" argument into reference tag "e" of "t" argument.
   // This tag corresponds to reference fields "i0" and "i1".
   var [ [ [ char8, 2 ], 2 ], 1 ] pollution_seti0[ [ "0e", "1_" ] ];
   fn Seti0( T &mut t, u64& i ) @(pollution_seti0);

   // Function writes a reference to "i" argument into reference tag "d" of "t" argument.
   // This tag corresponds to reference tag "b" (#1) of "s" field, which corresponds to "y" reference field of "S" struct.
   var [ [ [ char8, 2 ], 2 ], 1 ] pollution_setsy[ [ "0d", "1_" ] ];
   fn SetSy( T &mut t, u32& i ) @(pollution_setsy);


*********************************************
*Reference checking rule violation detection*
*********************************************

It's shown in the examples below how reference protection rule is enforced.

.. code-block:: u_spr

   fn Foo()
   {
       var i32 mut x= 0;
       var i32 &mut r0= x; // "r0" is a derived from "x" mutable reference
       var i32 &imut r1= x; // Create a derived from "x" reference while another derived mutable reference exists. An error will be produced.
   }

.. code-block:: u_spr

   fn Foo()
   {
       var f32 mut x= 0.0f;
       var f32 &imut r0= x; // "r0" is a derived from "x" immutable reference
       var f32 &mut r1= x; // Create a derived from "x" mutable reference while another derived reference exists. An error will be produced.
   }

.. code-block:: u_spr

   fn MutateArgs( f64 &mut a, f64 &mut b );
   
   fn Foo()
   {
       var f64 mut x= 0.0;
       MutateArgs( x, x ); // An error will be produced. Two derived from "x" mutable references are required for the call.
   }

******************************
*Lifetime violation detection*
******************************

Reference checking allows also to find dangling references.

.. code-block:: u_spr

   struct S{ i32& r; }
   var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
   fn Foo( S &mut s, i32& r ) @(pollution); // Function creates a derived from argument "r" reference inside the "s" argument.
   
   fn Bar()
   {
       var i32 x= 0;
       var S mut s{ .r= x };
       {
           var i32 y= 0;
           Foo( s, y );
       } // An error will be produced - destroyed variable "y" still has references.
   }

Reference checking doesn't allow to return references to local variables.

.. code-block:: u_spr

   fn Foo( i32& arg ) : i32 &
   {
       var i32 x= 0;
       return x; // An error will be produced - destroyed variable "x" still has references.
   }
