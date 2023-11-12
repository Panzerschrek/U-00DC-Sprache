Function templates
==================

It's possible to create abstract functions and methods in Ü, that are parameterized with types and values.
Such functions are template functions.

It's needed to use ``template`` keyword with template parameters listed in ``<//>`` in order to create a function template - much like in type templates.
It's possible to instantiate a template function by calling it.

.. code-block:: u_spr

   template</ type T />
   fn min( T &imut a, T &imut b ) : T &imut
   {
       if( a < b ) { return a; }
       return b;
   }
   
   template</ type T, size_type S />
   fn FillWithZeros( [ T, S ] &mut arr )
   {
       foreach( &mut el : arr )
       {
           el= T(0);
       }
   }
   
   fn Foo()
   {
       var i32 x= min( 55, 9 );
       var [ f64, 4 ] mut arr[ 1.0, 1.0, 1.0, 1.0 ];
       FillWithZeros( arr );
   }

Template arguments are deduced basing on the provided function arguments.
If such deduction is not possible, template arguments may be specified explicitly or at least some of them.

.. code-block:: u_spr

   template</ type T />
   fn GetPi() : T
   {
       return T(3.1415926535);
   }
   
   auto constexpr pi_f= GetPi</f32/>();
   auto constexpr pi_d= GetPi</f64/>();

***************************************************
*Function templates specialization and overloading*
***************************************************

It's possible to define multiple template and non-template functions with the same name in the same scope.
In a call the most specialized template function will be instantiated.
Specialization rules are similar to specialization rules of type templates.

.. code-block:: u_spr

   template</ type T />
   fn GetSequenceSize( T& t ) : size_type // A function for an arbitrary type
   {
       return 0s;
   }
   
   template</ type T, size_type S />
   fn GetSequenceSize( [T, S] &arr ) : size_type // A function specialized for arrays
   {
       return S;
   }
   
   fn constexpr GetSequenceSize( tup[] &t ) : size_type // Non-template function that is specialized for empty tuples. Is is considered to be more specialized as previous template function.
   {
       return 0s;
   }
   
   template</ type T />
   fn GetSequenceSize( tup[T] &t ) : size_type // Specialization for tuples of size 1.
   {
       return 1s;
   }
   
   template</ type T, type U />
   fn GetSequenceSize( tup[T, U] &t ) : size_type // Specialization for tuples of size 2.
   {
       return 2s;
   }
   
   template</ type T, type U, type V />
   fn GetSequenceSize( tup[T, U, V] &t ) : size_type // Specialization for tuples of size 3.
   {
        return 3s;
   }
   
   var i32 constexpr i= 0;
   static_assert( GetSequenceSize(i) == 0s );
   
   var [ bool, 16 ] constexpr arr= zero_init;
   static_assert( GetSequenceSize(arr) == 16s );
   
   var tup[] constexpr t0= zero_init;
   static_assert( GetSequenceSize(t0) == 0s );
   
   var tup[ f32 ] constexpr t1= zero_init;
   static_assert( GetSequenceSize(t1) == 1s );
   
   var tup[ bool, i32 ] constexpr t2= zero_init;
   static_assert( GetSequenceSize(t2) == 2s );
   
   var tup[ f32, u64, i32 ] constexpr t3= zero_init;
   static_assert( GetSequenceSize(t3) == 3s );

**********************************
*constexpr for template functions*
**********************************

It's possible to mark a template function as ``constexpr``.
In such case the compiler will ensure that the body of an instantiated template function may be ``constexpr``.
But if a template function isn't marked as ``constexpr`` its instantiations still may be ``constexpr``, if ``constexpr`` requirements are met for the function with its body and with given template arguments.
