from py_tests_common import *


def UnusedLocalTypeAlias_Test0():
	c_program_text= """
		fn Foo()
		{
			type I= i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test1():
	c_program_text= """
		fn Foo()
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
		fn Foo()
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
		fn Foo()
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
		fn Foo(bool cond)
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
		fn Foo()
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
		fn Foo(void v) // Void arg. Technically it is just regular arg, so, error about unused name must be produced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test1():
	c_program_text= """
		fn Foo(i32 x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test2():
	c_program_text= """
		fn Foo(f64 x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test3():
	c_program_text= """
		fn Foo(E e) // Enum value arg. Still must be referenced.
		{}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test4():
	c_program_text= """
		fn Foo( (fn()) f_ptr ) // Function pointer arg also must be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test5():
	c_program_text= """
		fn Foo($(u64) ptr) // Raw pointer arg must be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test6():
	c_program_text= """
		fn Foo([i16, 8] a) // Aray of fundamental type elements arg should be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test7():
	c_program_text= """
		fn Foo( tup[ bool, void, f32, [i32, 7], char16, tup[ u32, i16 ] ] t ) // Tuple of trivial element types arg should be referenced.
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test8():
	c_program_text= """
		fn Foo(S s) // Trivial struct arg - must be referenced.
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test9():
	c_program_text= """
		fn Foo([S, 16] s) // Array of trivial struct arg - must be referenced.
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedValueArgument_Test10():
	c_program_text= """
		fn Foo(C c) // Classes considered to be non-trivial and possibly may have non-trivial destructor. Avoid reporting about unreferenced arg.
		{}
		class C{ i32 x; i32 y; }
	"""
	tests_lib.build_program( c_program_text )


def UnusedValueArgument_Test11():
	c_program_text= """
		fn Foo(tup[C, bool] c) // Classes considered to be non-trivial and possibly may have non-trivial destructor. Avoid reporting about unreferenced arg.
		{}
		class C{ i32 x; i32 y; }
	"""
	tests_lib.build_program( c_program_text )


def UnusedValueArgument_Test12():
	c_program_text= """
		fn Foo(S s) // Avoid reporting about unreferenced arg, since its destructor is not trivial.
		{}
		struct S{ fn destructor(); }
	"""
	tests_lib.build_program( c_program_text )


def UnusedValueArgument_Test13():
	c_program_text= """
		fn Foo([S, 7] s) // Avoid reporting about unreferenced arg, since its destructor is not trivial.
		{}
		struct S{ fn destructor(); }
	"""
	tests_lib.build_program( c_program_text )


def UnusedReferenceArgument_Test0():
	c_program_text= """
		fn Foo(void& v)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test1():
	c_program_text= """
		fn Foo(i32& x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test2():
	c_program_text= """
		fn Foo(f64 &mut x)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test3():
	c_program_text= """
		fn Foo(E& e)
		{}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test4():
	c_program_text= """
		fn Foo( (fn()) &mut f_ptr )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test5():
	c_program_text= """
		fn Foo($(u64) & ptr)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test6():
	c_program_text= """
		fn Foo([i16, 8] &mut a)
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test7():
	c_program_text= """
		fn Foo( tup[ bool, void, f32, [i32, 7], char16, tup[ u32, i16 ] ] & t )
		{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test8():
	c_program_text= """
		fn Foo(S &mut s)
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test9():
	c_program_text= """
		fn Foo([S, 16] & s)
		{}
		struct S{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test10():
	c_program_text= """
		fn Foo(C & c) // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		class C{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test11():
	c_program_text= """
		fn Foo(tup[C, bool] &mut c) // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		class C{ i32 x; i32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test12():
	c_program_text= """
		fn Foo(S &imut s)  // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		struct S{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedReferenceArgument_Test13():
	c_program_text= """
		fn Foo([S, 7] &mut s) // Even if type is non-trivial, error about unreferenced arg should be produced, since it is a reference arg.
		{}
		struct S{ fn destructor(); }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 2 ) )


def UnusedLocalVariable_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 constexpr x= 0; // Trivial constexpr local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test1():
	c_program_text= """
		fn Foo(f32 in_x)
		{
			var f32 x= in_x; // Trivial immutable local variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut ok= false; // Trivial mutable local auto-variable.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test3():
	c_program_text= """
		fn Foo()
		{
			var [ f64, 16 ] arr= zero_init; // Trivial constexpr array.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test4():
	c_program_text= """
		fn Foo()
		{
			var tup[ bool, char8, f32 ] mut t[ false, "&"c8, 0.25f ]; // Trivial mutable tuple.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalVariable_Test5():
	c_program_text= """
		fn Foo(S s)
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
		fn Foo()
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
		fn Foo()
		{
			var S s{}; // Non-trivial immutable struct.
		}
		struct S{ fn destructor(); }
	"""
	tests_lib.build_program( c_program_text )


def UnusedLocalVariable_Test8():
	c_program_text= """
		fn Foo()
		{
			var C mut c; // Non-trivial class.
		}
		class C{}
	"""
	tests_lib.build_program( c_program_text )


def UnusedLocalVariable_Test9():
	c_program_text= """
		fn Foo()
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
		fn Foo(bool cond)
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
		fn Foo(bool cond)
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
		fn Foo()
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
		fn Foo()
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
		fn Foo()
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
		fn Foo()
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
		fn Foo(i32 x)
		{
			auto& x_ref= x; // Unused auto-reference to arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalReference_Test1():
	c_program_text= """
		fn Foo([f64, 4] vec)
		{
			var [f64, 4] & vec_ref= vec; // Unused reference to arg.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalReference_Test2():
	c_program_text= """
		fn Foo()
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
		fn Foo()
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
		fn Foo()
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
		fn Foo( (fn() : i32) mut fn_ptr )
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
		fn Foo()
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
		fn Foo(C& c)
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
		fn Foo()
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
		fn Foo(i32 x)
		{
			Bar(x); // Use variable - pass it as argument to another function.
		}
		fn Bar(i32 x);
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test1():
	c_program_text= """
		fn Foo(f32 &mut x)
		{
			x += 0.25f; // Use variable - change it.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test2():
	c_program_text= """
		fn Foo()
		{
			auto size= 4s;
			var [ C, size ] arr; // Use local constexpr variable inside type name.
		}
		class C{}
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test3():
	c_program_text= """
		fn Foo()
		{
			auto size= 4s;
			auto& size_ref= size;
			var C</size_ref/> c; // Use local constexpr reference as template argument.
		}
		template</size_type s/> class C{}
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test4():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var typeof(x) x_copy= zero_init; // Use local variable inside "typeof".
			Bar(x_copy);
		}
		fn Bar(i32 x);
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test5():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			move(s); // Use local variable inside "move".
		}
		struct S{ i32 x; }
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test6():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 1;
			x+= x; // Use variable to modify itself.
		}
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test7():
	c_program_text= """
		auto x= 0;
		fn nomangle Foo() : i32 { return x; } // Ok - use global variable.
	"""
	tests_lib.build_program( c_program_text )


def VariableUsage_Test8():
	c_program_text= """
		var i32 x= 0;
		type I= typeof(x); // Ok - use global variable for "typeof".
		fn nomangle Foo() : I { return 0; }
	"""
	tests_lib.build_program( c_program_text )


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
