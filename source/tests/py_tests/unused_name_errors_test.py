from py_tests_common import *


def UnusedLocalTypeAlias_Test0():
	c_program_text= """
		fn nomangle Foo()
		{
			type I= i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test1():
	c_program_text= """
		fn nomangle Foo()
		{
			type SS= SomeStruct;
		}
		struct SomeStruct{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test2():
	c_program_text= """
		fn nomangle Foo()
		{
			type SC= SomeClass;
		}
		class SomeClass polymorph{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test3():
	c_program_text= """
		fn nomangle Foo()
		{
			{
				type AliasInsideScope= void;
			}
		}	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalTypeAlias_Test4():
	c_program_text= """
		fn nomangle Foo(bool cond)
		{
			if(cond)
			{
				type AliasInsideIf= bool;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )


def UnusedLocalTypeAlias_Test5():
	c_program_text= """
		fn nomangle Foo()
		{
			while(Bar())
			{
				type AliasInsideLoop= typeof(666);
			}
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )


def UnusedValueArgument_Test0():
	c_program_text= """
		fn nomangle Foo(void v) // Void arg. Technically it is just regular arg, so, error about unused name must be produced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test1():
	c_program_text= """
		fn nomangle Foo(i32 x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test2():
	c_program_text= """
		fn nomangle Foo(f64 x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test3():
	c_program_text= """
		fn nomangle Foo(E e) // Enum value arg. Still must be referenced.
		{}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test4():
	c_program_text= """
		fn nomangle Foo( (fn()) f_ptr ) // Function pointer arg also must be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test5():
	c_program_text= """
		fn nomangle Foo($(u64) ptr) // Raw pointer arg must be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test6():
	c_program_text= """
		fn nomangle Foo([i16, 8] a) // Aray of fundamental type elements arg should be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test7():
	c_program_text= """
		fn nomangle Foo( tup[ bool, void, f32, [i32, 7], char16, tup[ u32, i16 ] ] t ) // Tuple of trivial element types arg should be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test8():
	c_program_text= """
		fn nomangle Foo(S s) // Trivial struct arg - must be referenced.
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test9():
	c_program_text= """
		fn nomangle Foo([S, 16] s) // Array of trivial struct arg - must be referenced.
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test10():
	c_program_text= """
		fn nomangle Foo(C c) // Classes considered to be non-trivial and possibly may have non-trivial destructor. Avoid reporting about unreferenced arg.
		{}
		class C{}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedValueArgument_Test11():
	c_program_text= """
		fn nomangle Foo(tup[C, bool] c) // Classes considered to be non-trivial and possibly may have non-trivial destructor. Avoid reporting about unreferenced arg.
		{}
		class C{}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedValueArgument_Test12():
	c_program_text= """
		fn nomangle Foo(S s) // Avoid reporting about unreferenced arg, since its destructor is not trivial.
		{}
		struct S{ fn destructor(); }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedValueArgument_Test13():
	c_program_text= """
		fn nomangle Foo([S, 7] s) // Avoid reporting about unreferenced arg, since its destructor is not trivial.
		{}
		struct S{ fn destructor(); }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedReferenceArgument_Test0():
	c_program_text= """
		fn nomangle Foo(void& v)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test1():
	c_program_text= """
		fn nomangle Foo(i32& x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test2():
	c_program_text= """
		fn nomangle Foo(f64 &mut x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test3():
	c_program_text= """
		fn nomangle Foo(E& e)
		{}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test4():
	c_program_text= """
		fn nomangle Foo( (fn()) &mut f_ptr )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test5():
	c_program_text= """
		fn nomangle Foo($(u64) & ptr)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test6():
	c_program_text= """
		fn nomangle Foo([i16, 8] &mut a)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test7():
	c_program_text= """
		fn nomangle Foo( tup[ bool, void, f32, [i32, 7], char16, tup[ u32, i16 ] ] & t )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test8():
	c_program_text= """
		fn nomangle Foo(S &mut s)
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test9():
	c_program_text= """
		fn nomangle Foo([S, 16] & s)
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test10():
	c_program_text= """
		fn nomangle Foo(C & c) // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		class C{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test11():
	c_program_text= """
		fn nomangle Foo(tup[C, bool] &mut c) // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		class C{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test12():
	c_program_text= """
		fn nomangle Foo(S &imut s)  // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		struct S{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test13():
	c_program_text= """
		fn nomangle Foo([S, 7] &mut s) // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		struct S{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedLocalVariable_Test0():
	c_program_text= """
		fn nomangle Foo()
		{
			var i32 constexpr x= 0; // Trivial constexpr local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test1():
	c_program_text= """
		fn nomangle Foo(f32 in_x)
		{
			var f32 x= in_x; // Trivial immutable local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test2():
	c_program_text= """
		fn nomangle Foo()
		{
			auto mut ok= false; // Trivial mutable local auto-variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test3():
	c_program_text= """
		fn nomangle Foo()
		{
			var [ f64, 16 ] arr= zero_init; // Trivial constexpr array.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test4():
	c_program_text= """
		fn nomangle Foo()
		{
			var tup[ bool, char8, f32 ] mut t[ false, "&"c8, 0.25f ]; // Trivial mutable tuple.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test5():
	c_program_text= """
		fn nomangle Foo(S s)
		{
			auto s_copy= s; // Trivial immutable struct.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test6():
	c_program_text= """
		fn nomangle Foo()
		{
			var S mut s{ .x= 6, .y= 675.8f }; // Trivial mutable struct.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test7():
	c_program_text= """
		fn nomangle Foo()
		{
			var S s{}; // Non-trivial immutable struct.
		}
		struct S{ fn destructor(); }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedLocalVariable_Test8():
	c_program_text= """
		fn nomangle Foo()
		{
			var C mut c; // Non-trivial class.
		}
		class C{}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedLocalVariable_Test9():
	c_program_text= """
		fn nomangle Foo()
		{
			{
				auto x= 675; // Trivial constexpr auto-variable inside block.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalVariable_Test10():
	c_program_text= """
		fn nomangle Foo(bool cond)
		{
			if(cond)
			{
				var u64 mut z= Bar(); // Mutable variable inside "if" block.
			}
		}
		fn Bar() : u64;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )


def UnusedLocalVariable_Test11():
	c_program_text= """
		fn nomangle Foo(bool cond)
		{
			if(cond) {}
			else
			{
				var void mut v= zero_init; // Mutable variable inside "else" block.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 7 ) )


def UnusedLocalVariable_Test12():
	c_program_text= """
		fn nomangle Foo()
		{
			var tup[ i32, f32 ] t[ 0, 0.0f ];
			for( v : t ) // Unused tuple iteration variable.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalVariable_Test13():
	c_program_text= """
		fn nomangle Foo()
		{
			with( x : 42 ) // Unused "with" operator variable.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test14():
	c_program_text= """
		fn nomangle Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( mut x : gen ) // Unused "if_coro_advance" operator variable.
			{}
		}
		fn generator SomeGen() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalVariable_Test15():
	c_program_text= """
		fn nomangle Foo()
		{
			unsafe
			{
				var char8 c= Bar(); // Unused unsafe labeled block variable.
			} label some_label
		}
		fn Bar() : char8;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )


def UnusedLocalReference_Test0():
	c_program_text= """
		fn nomangle Foo(i32 x)
		{
			auto& x_ref= x; // Unused auto-reference to arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalReference_Test1():
	c_program_text= """
		fn nomangle Foo([f64, 4] vec)
		{
			var [f64, 4] & vec_ref= vec; // Unused reference to arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalReference_Test2():
	c_program_text= """
		fn nomangle Foo()
		{
			var $(i32) ptr= zero_init;
			auto& ptr_ref= ptr; // Auto-reference for local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalReference_Test3():
	c_program_text= """
		fn nomangle Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s; // Unused mutable reference to local variable.
		}
		struct S{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalReference_Test4():
	c_program_text= """
		fn nomangle Foo()
		{
			var S s= zero_init;
			var S & s_ref= s; // Unused reference to local variable of non-trivial type.
		}
		struct S{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalReference_Test5():
	c_program_text= """
		fn nomangle Foo( (fn() : i32) mut fn_ptr )
		{
			while(true)
			{
				auto &mut ptr= fn_ptr; // Unused mutable reference to arg inside a loop.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )


def UnusedLocalReference_Test6():
	c_program_text= """
		fn nomangle Foo()
		{
			var tup[ f32, bool ] t= zero_init;
			for( &el : t ) // Unreferenced reference in tuple iteration.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalReference_Test7():
	c_program_text= """
		fn nomangle Foo(C& c)
		{
			with( &c_ref : c ) // Unused reference to a non-trivial type reference argument inside "with".
			{}
		}
		class C{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalReference_Test8():
	c_program_text= """
		fn nomangle Foo()
		{
			auto mut gen= Gen();
			if_coro_advance( &x : gen ) // Unused reference inside "if_coro_advance".
			{}
		}
		fn generator Gen() : i32 &;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def VariableUsage_Test0():
	c_program_text= """
		fn nomangle Foo(i32 x)
		{
			Bar(x); // Use variable - pass it as argument to another function.
		}
		fn Bar(i32 x);
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test1():
	c_program_text= """
		fn nomangle Foo(f32 &mut x)
		{
			x += 0.25f; // Use variable - change it.
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test2():
	c_program_text= """
		fn nomangle Foo()
		{
			auto size= 4s;
			var [ C, size ] arr; // Use local constexpr variable inside type name.
		}
		class C{}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test3():
	c_program_text= """
		fn nomangle Foo()
		{
			auto size= 4s;
			auto& size_ref= size;
			var C</size_ref/> c; // Use local constexpr reference as template argument.
		}
		template</size_type s/> class C{}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test4():
	c_program_text= """
		fn nomangle Foo()
		{
			var i32 mut x= 0;
			var typeof(x) x_copy= zero_init; // Use local variable inside "typeof".
			Bar(x_copy);
		}
		fn Bar(i32 x);
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test5():
	c_program_text= """
		fn nomangle Foo()
		{
			var S mut s= zero_init;
			move(s); // Use local variable inside "move".
		}
		struct S{ }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test6():
	c_program_text= """
		fn nomangle Foo()
		{
			var i32 mut x= 1;
			x+= x; // Use variable to modify itself.
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test7():
	c_program_text= """
		auto x= 0;
		fn nomangle Foo() : i32 { return x; } // Ok - use global variable.
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def VariableUsage_Test8():
	c_program_text= """
		var i32 x= 0;
		type I= typeof(x); // Ok - use global variable for "typeof".
		fn nomangle Foo() : I { return 0; }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedGlobalVariable_Test0():
	c_program_text= """
		var i32 x= 0;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test1():
	c_program_text= """
		var i32 x= 0, y= 0; // Only "y" is unused here
		var f32 z(x);
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test2():
	c_program_text= """
		auto must_be_true= true;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test3():
	c_program_text= """
		var (fn()) f_ptr(Foo);
		fn Foo();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test4():
	c_program_text= """
		var [ i8, 64 ] arr= zero_init;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test5():
	c_program_text= """
		var tup[ char8, bool ] t[ "a"c8, false ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test6():
	c_program_text= """
		auto qwerty = "qwerty";
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test7():
	c_program_text= """
		var S s{ .x= 1, .y= 2 };
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test8():
	c_program_text= """
		var i32 mut x= 0; // Even for mutable global variable unused name error still must be generated.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobalVariable_Test9():
	c_program_text= """
		namespace SomeNamespace
		{
			var u8 x= zero_init;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedGlobalReference_Test0():
	c_program_text= """
		var i32 x= 0;
		var i32& x_ref= x;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedGlobalReference_Test1():
	c_program_text= """
		var i32 x= 0;
		auto& x_ref= x;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedGlobalReference_Test2():
	c_program_text= """
		var S s{};
		auto& s_ref= s;
		struct S{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedGlobalReference_Test3():
	c_program_text= """
		var [ i32, 3 ] x[ 5, 6, 7 ];
		var [ i32, 3 ] & x_ref= x;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedGlobalReference_Test4():
	c_program_text= """
		var tup[] t[];
		auto& t_ref= t;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedGlobalReference_Test5():
	c_program_text= """
		namespace SomeNamespace
		{
			var f32 x(7.5f);
			auto& x_ref= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedGlobaTypeAlias_Test0():
	c_program_text= """
		type I= i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobaTypeAlias_Test1():
	c_program_text= """
		type I= J;
		type J= u32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobaTypeAlias_Test2():
	c_program_text= """
		namespace SomeNamespace
		{
			type Vec= [ f32, 3 ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedGlobaTypeAlias_Test3():
	c_program_text= """
		type IBox= Box</i32/>;
		template</type T/> struct Box{ T t; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedGlobaTypeAlias_Test4():
	c_program_text= """
		struct S
		{
			type Lol= i32;
		}
		type SAlias= S;
		fn nomangle Foo() : S
		{
			var SAlias::Lol mut i= 0; // Use type alias here as name lookup base.
			++i;
			return S();
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClass_Test0():
	c_program_text= """
		struct S{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedClass_Test1():
	c_program_text= """
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedClass_Test2():
	c_program_text= """
		struct S{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedClass_Test3():
	c_program_text= """
		class C{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedClass_Test4():
	c_program_text= """
		namespace SomeNamespace
		{
			class C polymorph {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClass_Test5():
	c_program_text= """
		class C interface {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedEnum_Test0():
	c_program_text= """
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedEnum_Test1():
	c_program_text= """
		namespace SomeNamespace
		{
			enum E{ A, B, C }
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedFunction_Test0():
	c_program_text= """
		fn Foo() {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedFunction_Test1():
	c_program_text= """
		fn Foo(); // Unused local prototype.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedFunction_Test2():
	c_program_text= """
		fn Foo( i32 &mut x ) { x= 0; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedFunction_Test3():
	c_program_text= """
		fn Bar( i32 x ){}
		fn Bar( f32 x ){}
		fn Foo()
		{
			var i32 x= 0;
			Bar(x); // Call only "i32" overloading. "f32" variant is unused.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedFunction_Test4():
	c_program_text= """
		fn Bar( i32 x ){}
		fn Bar( f32 x ){}
		fn Foo()
		{
			var f32 x= 0.0;
			Bar(x); // Call only "f32" overloading. "i32" variant is unused.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedFunction_Test5():
	c_program_text= """
		fn Bar( i32 x );
		fn Bar( f32 x ); // Unused local prototype.
		fn Foo()
		{
			var i32 x= 0;
			Bar(x); // Call only "i32" overloading. "f32" variant is unused.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedFunction_Test6():
	c_program_text= """
		fn Bar( i32 x ); // Unused local prototype.
		fn Bar( f32 x );
		fn Foo()
		{
			var f32 x= 0.0;
			Bar(x); // Call only "f32" overloading. "i32" variant is unused.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedFunction_Test7():
	c_program_text= """
		namespace SomeNamespace
		{
			fn Foo() {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedFunction_Test8():
	c_program_text= """
		fn nomangle Foo() {} // Do not generate error - this function is available externally.
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedFunction_Test9():
	c_program_text= """
		fn Foo();
		fn Foo() {} // Generate error - only local prototype exists - thus the function is not externally-available.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedFunction_Test10():
	c_program_text= """
		fn Bar(){} // Ok - reference it in "Foo".
		fn nomangle Foo(){ Bar(); }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedFunction_Test11():
	c_program_text= """
		fn Bar(); // Ok - reference it in "Foo".
		fn nomangle Foo(){ Bar(); }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedFunction_Test12():
	c_program_text= """
		fn Bar(i32 x); // Ok - reference it in "Foo".
		fn Bar(f32 x); // Ok - reference it in "Foo".
		fn nomangle Foo()
		{
			Bar(0);
			Bar(0.0f);
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedFunction_Test13():
	c_program_text= """
		fn Bar();
		fn nomangle Foo() : (fn())
		{
			return (fn())(Bar); // Ok - take pointer to function.
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedTypeTemplate_Test0():
	c_program_text= """
		template</type T/> type Vec3= [ T, 3 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedTypeTemplate_Test1():
	c_program_text= """
		template</size_type S/> type IVec= [ i32, S ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedTypeTemplate_Test1():
	c_program_text= """
		template</type T/> struct Box { T t; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedTypeTemplate_Test2():
	c_program_text= """
		template</size_type S/> struct ArraySizeHelper</ [ i32, S ] /> { auto size= S; }  // This is unused.
		template</size_type S/> struct ArraySizeHelper</ [ f32, S ] /> { auto size= S; }
		auto size= ArraySizeHelper</ [ f32, 66 ] />; // Use only "f32" variant.
		static_assert( size == 66 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )
	assert( not HaveError( errors_list, "UnusedName", 3 ) )


def UnusedTypeTemplate_Test3():
	c_program_text= """
		template</size_type S/> struct ArraySizeHelper</ [ i32, S ] /> { auto size= S; }
		template</size_type S/> struct ArraySizeHelper</ [ f32, S ] /> { auto size= S; } // This is unused.
		auto size= ArraySizeHelper</ [ i32, 66 ] />; // Use only "i32" variant.
		static_assert( size == 66 );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( not HaveError( errors_list, "UnusedName", 2 ) )
	assert( HaveError( errors_list, "UnusedName", 3 ) )


def UnusedTypeTemplate_Test4():
	c_program_text= """
		template</type T/> struct Box { T t; } // Ok - type template is used.
		fn nomangle MakeBox(i32 x) : Box</i32/>
		{
			var Box</i32/> res{ .t= x };
			return res;
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedTypeTemplate_Test5():
	c_program_text= """
		template</type T/> struct Box { T t; } // Ok - type template is used.
		type FBox= Box</f32/>;
		fn nomangle MakeBox(f32 x) : FBox
		{
			var FBox res{ .t= x };
			return res;
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassVariable_Test0():
	c_program_text= """
		struct S
		{
			var i32 c_zero= 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassVariable_Test1():
	c_program_text= """
		class C
		{
			auto c_zero= 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassVariable_Test2():
	c_program_text= """
		struct S
		{
			auto& c_x= g_x;
		}
		var i32 g_x= 0;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassTypeAlias_Test0():
	c_program_text= """
		struct S
		{
			type I= i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassTypeAlias_Test1():
	c_program_text= """
		class C
		{
			type this_type= C;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassTypeAlias_Test2():
	c_program_text= """
		class C interface
		{
			type SomeUnused= f32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassInternalClass_Test0():
	c_program_text= """
		struct S
		{
			struct Inner{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassInternalClass_Test1():
	c_program_text= """
		class C
		{
			class InnerC{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test0():
	c_program_text= """
		struct S{} // Some generated methods are unreferences, but is it ok.
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test1():
	c_program_text= """
		struct S
		{
			// It is still ok to declare special methods manually with "default".
			fn constructor(mut this)= default;
			fn constructor(mut this, S &imut other)= default;
			op=(mut this, S &imut other)= default;
			op==(S &imut l, S &imut r) : bool = default;

		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test2():
	c_program_text= """
		struct S
		{
			// It is still ok to define special methods manually.
			fn constructor(mut this) (x= 0) {}
			fn constructor(mut this, S &imut other) (x= other.x) {}
			fn destructor() {}
			op=(mut this, S &imut other) { x= other.x; }
			op==(S &imut l, S &imut r) : bool { return l.x == r.x; }

			i32 x;
		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test3():
	c_program_text= """
		struct S
		{
			fn Bar() {} // Unused static method.
		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test4():
	c_program_text= """
		struct S
		{
			fn Bar(imut this) {} // Unused imut-this method.
		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test5():
	c_program_text= """
		struct S
		{
			fn Bar(mut this) {} // Unused mut-this method.
		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test6():
	c_program_text= """
		struct S
		{
			fn Bar( mut this) : i32 { return 2; } // Unused overloading.
			fn Bar(imut this) : i32 { return 3; }
		}
		fn nomangle Foo() : i32
		{
			var S s;
			return s.Bar(); // use "imut" overloading.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test7():
	c_program_text= """
		struct S
		{
			fn Bar( mut this) : i32 { return 2; }
			fn Bar(imut this) : i32 { return 3; } // Unused overloading.
		}
		fn nomangle Foo() : i32
		{
			var S mut s;
			return s.Bar(); // use "mut" overloading.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedClassFunction_Test8():
	c_program_text= """
		class S polymorph
		{
			fn virtual Foo(this) : i32 { return 0; } // Unused virtual mehod. For now ignore virtual methods.
		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test9():
	c_program_text= """
		class S interface
		{
			fn virtual pure Foo(this) : i32; // Unused virtual pure. For now ignore virtual methods.
		}
		fn nomangle Foo() : size_type { return typeinfo</S/>.size_of; }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test10():
	c_program_text= """
		class A interface
		{
			fn virtual pure Foo(this) : i32; // Ok - use it by overloading.
		}
		class B : A
		{
			fn virtual override Foo(this) : i32;
		}
		fn nomangle GetB() : B { return B(); }
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test11():
	c_program_text= """
		class S
		{
			fn Foo(this){}
		}
		fn nomangle Bar()
		{
			var S s;
			S::Foo(s); // Ok - use method by accessing it directly.
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test12():
	c_program_text= """
		struct S
		{
			op()(this) {} // Unused overloaded postfix operator.
			fn destructor() {}
		}
		fn nomangle Foo() : size_type
		{
			return typeinfo</S/>.size_if;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test12():
	c_program_text= """
		struct S
		{
			op++(mut this) {} // Unused overloaded prefix operator.
			fn destructor() {}
		}
		fn nomangle Foo() : size_type
		{
			return typeinfo</S/>.size_if;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test14():
	c_program_text= """
		struct S
		{
			op+(S a, S b) { return S(); } // Unused binary operator
			fn destructor() {}
		}
		fn nomangle Foo() : size_type
		{
			return typeinfo</S/>.size_if;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test15():
	c_program_text= """
		struct S
		{
			op()(this) {}
			op+(S a, S b) : S { return S(); }
			op++(mut this) {}
			fn destructor() {}
		}
		fn nomangle Foo()
		{
			var S mut s0, mut s1;
			++s0; // Ok, use ++ operator
			var S s2= s0 + s1; // Ok, use binary + operator
			s2(); // Ok - use () operator
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassField_Test0():
	c_program_text= """
		struct S
		{
			i32 x; // Totally unused.
		}
		fn nomangle Foo() : S
		{
			var S s= zero_init;
			return s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassField_Test1():
	c_program_text= """
		struct S
		{
			i32 x; // Used only for initialization - this counts as unused.
		}
		fn nomangle Foo() : S
		{
			var S s{ .x= 42 };
			return s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassField_Test2():
	c_program_text= """
		struct S
		{
			i32 x; // Used only for initialization via constructor initializer list  - this counts as unused.
			fn constructor() ( x= 77 ) {}
		}
		fn nomangle Foo() : S
		{
			var S s;
			return s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedClassFunction_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
		}
		fn nomangle Foo() : S
		{
			var S mut s= zero_init;
			s.x= 66; // Ok - used via member access operator.
			return s;
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			fn SetX( mut this, i32 in_x )
			{
				x= in_x; // Ok - access via simple name.
			}
		}
		fn nomangle Foo() : S
		{
			var S mut s= zero_init;
			s.SetX( 123 );
			return s;
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )


def UnusedClassFunction_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			fn SetX( mut this, i32 in_x )
			{
				S::x= in_x; // Ok - access via complex name.
			}
		}
		fn nomangle Foo() : S
		{
			var S mut s= zero_init;
			s.SetX( 123 );
			return s;
		}
	"""
	tests_lib.build_program_unused_errors_enabled( c_program_text )
