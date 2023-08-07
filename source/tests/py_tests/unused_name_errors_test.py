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
