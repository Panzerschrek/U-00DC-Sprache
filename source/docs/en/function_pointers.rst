Function pointers
=================

There is a possibility in Ü to save a function address into a pointer and than call this function with it.

Function pointer is a separate type kind.
Its name is started with ``fn`` keyword, than follow parameters description, (optional) reference pollution, (optional) ``unsafe`` modifier, return value description.

.. code-block:: u_spr

   type simple_fn = fn();
   type int_ret_fn = fn() : i32;
   type f_arg_fn = fn(f32 x) : f32;
   type ref_ret_fn = fn(i32 &x) : i32&;
   type binary_fn = fn(i64 x, i64 y) : i64;
   type unsafe_fn = fn(u32 x) unsafe : bool;

It's necessary to initialize function pointer.
It's possible to initialize it with ``zero_init``, but program execution will be terminated in case if such zero function pointer is called.
Usually a function pointer is initialized with a function.

.. code-block:: u_spr

   fn Bar();
   
   fn Foo()
   {
       var (fn()) fn_ptr = Bar;
       fn_ptr(); // "Bar" function will be called
   }

When multiple functions with the same name exist, the function with matching type will be chosen.

.. code-block:: u_spr

   fn Bar(i32 x) : i32;
   fn Bar(f32 x) : f32;
   
   fn Foo()
   {
       var ( fn(f32 x) : f32 ) fn_ptr = Bar; // "Bar(f32 x) : f32" function will be chosen
       var f32 res= fn_ptr(0.0f);
   }

It's possible to initialize a function pointer with another function pointer.

.. code-block:: u_spr

   fn Bar0();
   fn Bar1();
   fn Bar2();
   
   type fn_ptr_type= fn();
   fn Foo()
   {
       var fn_ptr_type mut fn_ptr= zero_init;
       fn_ptr = fn_ptr_type(Bar0);
       fn_ptr(); // "Bar0" function will be called
       fn_ptr = fn_ptr_type(Bar1);
       fn_ptr(); // "Bar1" function will be called
   }

There is a possibility to perform implicit conversion from a function into a pointer, but only if there is only one function with such name.

.. code-block:: u_spr

   fn Bar();
   fn Baz( ( fn() ) ptr );
   fn Foo()
   {
       var ( fn() ) mut ptr= zero_init;
       ptr= Bar; // Implicitly convert "Bar" into a pointer in assignment.
       Baz( Bar ); // Implicitly convert "Bar" into a pointer in function argument passing.
   }


******************************
*Function pointer conversions*
******************************

It's allowed to initialize a function pointer with a value of different function pointer type.
But the type of this function pointer must be compatible.

Compatibility rules are following:

* Return types must be equal
* Return reference modifiers must be the same
* It's allowed to convert a function pointer returning ``mut`` reference to a function pointer returning ``imut`` reference. Backward conversion isn't allowed.
* The number of parameters must be the same
* All parameter types must be the same
* Reference modifiers of parameters must be the same
* It's allowed to convert a function pointer with ``imut`` reference parameter to a function pointer with ``mut`` reference parameter. Backward conversion is not allowed.
* Conversion is possible while converting a pointer returning less logical references to function pointer returning more logical references
* Conversion is possible while converting a pointer performing less reference pollution to function pointer performing more reference pollution
* It's possible to convert pointer to safe function into pointer to ``unsafe`` function.

.. code-block:: u_spr

   fn IMutArgFn( i32 &imut x );
   var ( fn( i32 &mut x ) ) mut_arg_fn_ptr = IMutArgFn; // Convert parameter mutability
   
   fn MutRetFn( f32 &mut x ) : f32 &mut;
   var ( fn( f32 &mut x ) : f32 &imut ) imut_ret_fn_ptr = MutRetFn;  // Convert return reference mutability
   
   fn SafeFn();
   var ( fn() unsafe ) unsafe_fn_ptr = SafeFn;  // Convert unsafe modifier
   
   var [ [ char8, 2 ], 1 ] return_references_first[ "0_" ];
   fn FirstRetFn( i32& x, i32& y ) : i32 & @(return_references_first);
   var [ [ char8, 2 ], 2 ] return_references_first_and_second[ "0_", "1_" ];
   var ( fn( i32& x, i32& y ) : i32 & @(return_references_first_and_second ) ) all_ret_fn_ptr = FirstRetFn; // Convert with different return references

During function pointer initialization the compiler ensures that this conversion is possible.
If multiple conversions are possible an error will be generated.

.. code-block:: u_spr

   fn Foo( i32 &imut x, i32 &mut y );
   fn Foo( i32 &mut x, i32 &imut y );
   
   var ( fn( i32 &mut x, i32 &mut y ) ) foo_ptr = Foo; // Error: can't select matching function - too many candidates


******************************
*Function pointers comparison*
******************************

There are equality compare operators for function pointers.
But they are a little bit tricky.
All function pointers obtained from the same function pointer will be the same.
Function pointers may be or may not be equal if they point to the same function in different places of the program.
Function pointers may be or may not be equal if they point to different functions.

.. code-block:: u_spr

   fn Bar0(){}
   fn Bar1(){}
   
   fn Foo()
   {
       var (fn()) ptr0= Bar0;
       var (fn()) ptr1 = ptr0;
       var (fn()) ptr2= Bar0;
       var (fn()) ptr3= Bar1;
       auto cmp0 = ptr0 == ptr1; // Result is true
       auto cmp1 = ptr0 != ptr1; // Result is false
       auto cmp2 = ptr0 == ptr2; // May be true or false
       auto cmp3 = ptr3 == ptr0; // May be true or false
   }
