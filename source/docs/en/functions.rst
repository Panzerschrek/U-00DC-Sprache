Functions
=========

Ü supports free functions.
Functions have zero or more params and can return values.

Exaples of function declarations:

.. code-block:: u_spr

   fn Add( i32 x, i32 y ) : i32; // Function has two params of type "i32" and returns a value of type "i32".
   fn Sleep( f32 sec ); // Function has single param of type "f32" and returns no value.
   fn GetPi() : f64; // Function has no params and returns a value of type "f64".
   fn Max( i64& x, i64& y ) : i64&; // Function has two reference params of type "i64" and returns a reference of the same type.

********
*Params*
********

Functions can have value-params and reference-params.
Reference params are defined with ``&`` after the name of the param.
Params may have mutability modifiers ``mut`` or ``imut``, that are specified after the type name or after ``&`` (for reference params).
If there is no mutability modifier specified for the param it is assumed to be ``imut``.

For reference params mutability modifiers are important for the caller.
An immutable reference can't be passed to mutable reference param.
For value params mutability modifiers have a meaning only inside function body.
Immutable value arguments can't be changed inside function body.

Params example:

.. code-block:: u_spr

   fn Add( i32 imut x, i32 y ) : i32; // Immutable value params. The second param is by-default immutable.
   fn Inc( u32 &mut x ); // Mutable reference param
   fn GetLength( ust::string8 &imut str ) : size_type; // Immutable reference param
   fn GetNumZeros( ust::string8 & str ) : size_type;  // Immutable by-default reference param

**************
*Return value*
**************

A function may return nothing.
In order to do that it should be declared without any return type or with ``void``.

.. code-block:: u_spr

   fn Sleep( f32 sec );
   fn USleep( u64 ns ) : void;

A function may return a value.
A return type should be specified in order to do that:

.. code-block:: u_spr

   fn Add( i32 x, i32 y ) : i32; // return type is "i32"

A function may return a reference - mutable or immutable.
It should be declared with ``&`` after the return type name in order to do that.
A mutability modifier ``mut`` or ``imut`` may be optionally specified.
If there is no mutability modifier it is assumed to be ``imut``.

.. code-block:: u_spr

   fn Max( i64& x, i64& y ) : i64 &imut; // Function returns immutable reference of type "i64"
   fn Max( u64& x, u64& y ) : u64 &; // Function returns immutable reference of type  "u64", "imut" modifier is implicitely applied
   fn Max( u64 &mut x, u64 &mut y ) : i64 &mut; // Function returns mutable reference of type "u64"

***********************
*Functions overloading*
***********************

It's possible to declare more than one function with the same name in one namespace.
But all these functions should have different params - different number of params, different types, different mutability (for reference params).

.. code-block:: u_spr

   fn Foo();
   fn Foo(i32 x); // Ok - an overloading with different number of params
   fn Foo(f32 &imut x); // Ok - an overloading with different param type
   fn Foo(f32 &mut x); // Ok - an overloading with different param mutability

If functions differ by non-param properties (return value, ``unsafe`` modifier), overloading isn't possible.
Also it's impossible to overoad a function with only difference in the mutability of a value param or with overloading of a value param by an immutable reference param.

.. code-block:: u_spr

   fn Foo() : i32;
   fn Foo() unsafe : f32; // Error, overliading is not possible - same signature

   fn Bar(i32 mut x);
   fn Bar(i32 imut x); // Error, overliading is not possible - only mutability modifier of a value param differs

   fn Baz(i32 imut x);
   fn Baz(i32 &imut x); // Error, overliading is not possible - the only difference is a reference modifier of an immutable param

********************************
*Prototypes and implementations*
********************************

A function declaration with ``;`` at  is only a prototype.
But if after the function declaration follows a body block, this is a function implementation.

.. code-block:: u_spr

   // prototype declaration
   fn Add( i32 x, i32 y ) : i32;
   
   // implementation
   fn Add( i32 x, i32 y ) : i32
   {
        return x + y;
   }

*****************************
*return value type deduction*
*****************************

It's possible to specify ``auto`` as function return type.
In that case the compiler can automatically deduce the return type.

.. code-block:: u_spr

   fn Div( i32 x, i32 y ) : auto
   {
       return x / y; // Return type is"i32"
   }
   
   fn Abs( f32 x ) : auto
   {
       // In all "return" operators return type is "f32"
       if( x >= 0.0f ) { return x; }
       return -x;
   }

Functions with return type deduction have some limitations:

* They must have a body
* They can't recursivelly call itself
* They can't be struct or class members

*******************************
*Conditional function presense*
*******************************

Sometimes it is necessary to control presense of a function - depending on some condition.
Especially it may be usefull in template code.
For that Ü has a special syntax construction - ``enable_if``.
This construction may be specified after ``fn`, optional ``constexpr``, ``virtual``, ``nomangle`` modifiers in a function declaration.
After that follows an expression in ``()``.
The expression must be a compile-time constant of type ``bool``.
If the result of the expression is ``false``, this function will not be compiled (nether the declaration nor the body).

.. code-block:: u_spr

   auto constexpr is_32bit = typeinfo</size_type/>.size_of == 4s;
   // This function exists only on a 32-bit platform
   fn enable_if( is_32bit ) Bar();
   
   fn Foo()
   {
       Bar(); // A compilation error "function not found" will be producesd on a 64-bit platform.
   }
