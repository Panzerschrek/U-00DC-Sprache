from py_tests_common import *


def OrderIndependentFunctions_Test0():
	c_program_text= """
		fn Baz() : i32 { return 81; }
		fn Foo() : i32
		{
			return Baz() * Bar();  // Functions "Bar" and "Baz" visible here and can be called.
		}
		fn Bar() : i32 { return 52414; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52414 * 81 )


def OrderIndependentClasses_Test0():
	c_program_text= """
		fn Foo()
		{
			var S s;
		}

		struct T{}
		struct S{ F f; T t; }  // Here used classes abowe and below.
		struct F{}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def OrderIndependent_OutOfLineFunction_Test0():
	c_program_text= """
		fn Bar() : i32;
		fn Foo() : i32  { return Bar(); }
		fn Bar() : i32  { return 88524; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88524 )


def OrderIndependent_OutOfLineFunction_Test1():
	c_program_text= """
		fn Bar() : i32  { return 665235; }
		fn Foo() : i32  { return Bar(); }
		fn Bar() : i32; // ok, prototype after body

	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 665235 )


def OrderIndependent_OutOfLineFunction_Test2():
	c_program_text= """
		fn Foo() : i32  { return N::Bar(); }
		namespace N
		{
			fn Bar() : i32;
		}
		fn N::Bar() : i32 { return 5632478; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5632478 )


def OrderIndependent_OutOfLineFunction_Test3():
	c_program_text= """
		fn Foo() : i32  { return N::Bar(); }
		namespace N
		{
			fn Bar() : i32;
			fn Bar() : i32  { return 5623222; }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5623222 )


def OrderIndependent_Enums_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			return i32(E::C);
		}
		enum E{ A, B, C, D }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2 )


def OrderIndependent_Enums_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			var SS ss= SS::D;
			return i32(ss);
		}
		enum SS{ A, B, C, D }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3 )


def OrderIndependent_TypeAlias_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var I r= 51245;
			return r;
		}
		type I= i32;
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 51245 )


def OrderIndependent_TypeAlias_Test1():
	c_program_text= """
		type A= RRR;
		fn Foo() : A
		{
			return 653524;
		}
		type FFF= i32;
		type RRR= FFF;
		type UnusedType= [ bool, 1024 ];
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 653524 )


def OrderIndependent_RecursiveTypeAlias_Test0():
	c_program_text= """
		struct S
		{
			type Self= S;
		}
	"""
	tests_lib.build_program( c_program_text )


def OrderIndependent_ClassInternalDependencies_Test0():
	c_program_text= """
		struct A
		{
			auto x = B::y;
			auto z= B::w;
		}
		struct B
		{
			auto y= A::z;
			auto w= 66;
		}
		static_assert( A::x == 66 );
		static_assert( B::y == 66 );
		static_assert( A::z == 66 );
		static_assert( B::w == 66 );
	"""
	tests_lib.build_program( c_program_text )


def OrderIndependent_ClassInternalDependencies_Test1():
	c_program_text= """
		class A polymorph
		{
			auto x = B::y;
			auto z= B::w;
		}
		class B : A
		{
			auto y= A::z;
			auto w= 99995;
		}
		static_assert( A::x == 99995 );
		static_assert( B::y == 99995 );
		static_assert( A::z == 99995 );
		static_assert( B::w == 99995 );
	"""
	tests_lib.build_program( c_program_text )


def OrderIndependent_ClassInternalDependencies_Test2():
	c_program_text= """
		class A polymorph
		{
			type Q= B::T;
		}
		class B : A
		{
			type T= f32;
		}
		var A::Q q= 0.25f;
	"""
	tests_lib.build_program( c_program_text )


def GlobalsLoopDetected_Test0():
	c_program_text= """
		struct S{ S s; }  // Type depends on itself
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test1():
	c_program_text= """
		struct A{ B b; }
		struct B{ C c; }
		struct C{ A a; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test2():
	c_program_text= """
		var i32 x= x; // Variable self-initialization
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test3():
	c_program_text= """
		var i32 a= b + 1; // Variables loop.
		var i32 b= c + 1;
		var i32 c= d + 1;
		var i32 d= a + 1;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test4():
	c_program_text= """
		type T= T; // Type alias self-initialization.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test5():
	c_program_text= """
		type A= B; // Type alias loop.
		type B= C;
		type C= [ A, 2 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test6():
	c_program_text= """
		fn constexpr Foo() : u32 { return 2u; }
		fn Foo( [ i32, Foo() ]& arr ) {}  // Declaration of function inside functions set depends on first function.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test7():
	c_program_text= """
		template</type T/> struct Rec</ Rec</T/> /> {}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def GlobalsLoopDetected_Test8():
	c_program_text= """
		// Globals loop for type alias - it referrs to itself.
		template</type T/>
		type Rec= Rec</T/>;
		type Instantiation= Rec</i32/>;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 4 )


def MethodsCompletenessForClass_Test0():
	c_program_text= """
		struct S
		{
			// Require complete type to fetch typeinfo for "S".
			// But method "Foo" itself isn't required for "S" completeness.
			// So, there is no globals loop here.
			fn Foo() : [ byte8, typeinfo</S/>.size_of ];
		}
	"""
	tests_lib.build_program( c_program_text )


def MethodsCompletenessForClass_Test1():
	c_program_text= """
		struct S
		{
			// Require complete type to fetch typeinfo for "S".
			// But method "Foo" itself isn't required for "S" completeness.
			// So, there is no globals loop here.
			fn enable_if( typeinfo</S/>.size_of < 16s ) Foo( this, f32& y ) : bool;
		}
	"""
	tests_lib.build_program( c_program_text )


def MethodsCompletenessForClass_Test2():
	c_program_text= """
		struct S
		{
			i32 x= 0;
			// Require complete type to build global variable "s" of type "S".
			// But method "Foo" itself isn't required for "S" completeness.
			// So, there is no globals loop here.
			fn  Foo( mut this, typeof(s) arg ) : i32;
		}
		var S s{ .x= 4 };
	"""
	tests_lib.build_program( c_program_text )


def MethodsCompletenessForClass_Test3():
	c_program_text= """
		struct S
		{
			// Has global loop, since default constructor is required for type completeness.
			fn enable_if( typeinfo</S/>.size_of < 16s ) constructor( this );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def MethodsCompletenessForClass_Test4():
	c_program_text= """
		struct S
		{
			// Has global loop, since constructors are required for type completeness.
			fn enable_if( typeinfo</S/>.size_of < 16s ) constructor( i32 x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def MethodsCompletenessForClass_Test5():
	c_program_text= """
		struct S
		{
			// Has global loop, since assignment operators are required for type completeness.
			op=( mut this, [ byte8, typeinfo</S/>.size_of ] data );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def MethodsCompletenessForClass_Test6():
	c_program_text= """
		struct S
		{
			// Has global loop, since equality compare operators are required for type completeness.
			op enable_if( typeinfo</S/>.is_class ) == ( S& l, S& r ) : bool;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def MethodsCompletenessForClass_Test7():
	c_program_text= """
		struct S
		{
			// Has global loop, since virtual methods require completeness.
			fn virtual Foo( this, [ byte8, typeinfo</S/>.size_of ]& data );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 2 ) )


def MethodsCompletenessForClass_Test8():
	c_program_text= """
		struct S
		{
			// Accessing typeinfo functions list triggers function declarations completeness and thus a global loop,
			// But not for the class itself, but for the function.
			fn Foo( typeof( typeinfo</S/>.functions_list ) & arg );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 6 ) )


def TypesCompletenessForClass_Test0():
	c_program_text= """
		struct S
		{
			// "typeinfo" triggers "S" completeness, but this doesn't trigger dependency loop,
			// since type alias completeness isn't required for type completeness.
			type TypeBytes= [ byte8, typeinfo</S/>.size_of ];
			i32 x;
			f32 y;
		}
	"""
	tests_lib.build_program( c_program_text )


def TypesCompletenessForClass_Test1():
	c_program_text= """
		struct S
		{
			// Types list depends on types - so, a global loop is created.
			type TypesListType= typeof( typeinfo</S/>.types_list );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "GlobalsLoopDetected", 5 ) )
