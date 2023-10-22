from py_tests_common import *

def FieldInitializerDeclaration_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 0; // Expression initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test1():
	c_program_text= """
		struct S
		{
			i32 x(0); // Constructor initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test2():
	c_program_text= """
		struct S
		{
			i32 x= zero_init; // Zero initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test2():
	c_program_text= """
		struct S
		{
			[ i32, 2 ] arr[ 55, 66 ]; // Array initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
		}
		struct T
		{
			S s{ .x= 0 }; // Struct named initializerg
		}
	"""
	tests_lib.build_program( c_program_text )


def InClassFieldInitializer_InStructNamedInitializer_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 66;
		}
		fn Foo() : i32
		{
			var S s{};
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66 )


def InClassFieldInitializer_InStructNamedInitializer_Test1():
	c_program_text= """
		struct S
		{
			f32 a= 480.0f;
			f32 b= 3.0f + 2.0f;
		}
		fn Foo() : f32
		{
			var S s{};
			return s.a / s.b;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 96.0 )


def InClassFieldInitializer_InStructNamedInitializer_Test2():
	c_program_text= """
		struct S
		{
			[ i32, 2 ] arr[ 77, 14 ];
		}
		fn Foo() : i32
		{
			var S s{};
			return s.arr[0u] - s.arr[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 77 - 14 )


def InClassFieldInitializer_InConstructorInitializer_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 3258;
			fn constructor()
			// In default initializer list initializer for 'x' called
			{}
		}
		fn Foo() : i32
		{
			return S().x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3258 )


def InClassFieldInitializer_InConstructorInitializer_Test1():
	c_program_text= """
		struct S
		{
			i32 x= 89;
			i32 y;
			fn constructor()
			( y= x * 5 ) // 'x' initialized before 'y'
			{}
		}
		fn Foo() : i32
		{
			var S s;
			return s.y - s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 89 * 5 - 89 )


def InClassFieldInitializer_InDefaultConstructor_Test0():
	c_program_text= """
		struct S
		{
			i32 x= -1;
			f32 y= -99.0f;
		}
		fn Foo() : i32
		{
			var S s; // Default constructor generated, because struct have initializer for all fields.
			return i32(f32(s.x) * s.y);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99 )


def InClassFieldInitializer_InDefaultConstructor_Test1():
	c_program_text= """
		struct S
		{
			i32 x= 0;
		}
		struct T
		{
			S s; // Have default constructor.
			bool b= false;
		}
		fn Foo()
		{
			var T t; // Default constructor generated.
			halt if( t.s.x != 0 );
			halt if( t.b );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def InClassFieldInitializer_ForReferenceField_Test0():
	c_program_text= """
		auto global_constant= 1917;
		struct S
		{
			i32& r= global_constant;
		}
		fn Foo() : i32
		{
			var S s{}; // Initialize reference field in struct initializer.
			return s.r;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1917 )


def InClassFieldInitializer_ForReferenceField_Test1():
	c_program_text= """
		auto pi= 3.1415926535;
		struct S
		{
			f64& r= pi;
		}
		fn Foo() : i32
		{
			var S s; // Call generated default constructor.
			return i32( s.r * 100.0 );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 314 )


def InClassFieldInitializer_ForReferenceField_Test2():
	c_program_text= """
		auto e= 2.718281828f;
		struct S
		{
			f32& r= e;
			fn constructor()
			() {} // Call here initializer for reference field.
		}
		fn Foo() : i32
		{
			var S s; // Call generated default constructor.
			return i32( s.r * 100.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 271 )


def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 95;
		}
		fn Foo() : i32
		{
			var S s{ .x= 77 };
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 77 )


def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test1():
	c_program_text= """
		struct S
		{
			[ i64, 6 ] arr= zero_init;
		}
		fn Foo() : i64
		{
			var S s{ .arr[ 4i64, 8i64, 15i64, 16i64, 23i64, 42i64 ] };
			return s.arr[0u] + s.arr[1u] * s.arr[2u] / s.arr[3u] + s.arr[4u] - s.arr[5u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 4 + int(8 * 15 / 16) + 23 - 42 )


def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test2():
	c_program_text= """
		struct S
		{
			i32 x= 0;
			fn constructor( i32 in_x )
			( x(in_x) )
			{}
		}
		fn Foo() : i32
		{
			var S s( 33369 );
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 33369 )


def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test3():
	c_program_text= """
		auto constexpr global_zero= 0;
		struct S
		{
			i32& x= global_zero;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this'a', i32&'b in_x ) @(pollution)
			( x(in_x) )
			{}
		}
		fn Foo() : i32
		{
			auto non_zero= 666;
			var S s( non_zero );
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 666 )


def InClassFieldInitializer_MayBeConstexpr_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 99951;
		}
		fn Foo()
		{
			var S constexpr s{};
			static_assert( s.x == 99951 );
		}
	"""
	tests_lib.build_program( c_program_text )


def InClassFieldInitializer_MayBeConstexpr_Test1():
	c_program_text= """
		struct S
		{
			auto constant= 1953;
			i32& x= constant;
		}
		fn Foo()
		{
			var S constexpr s{}; // Reference field must be constexpr here.
			static_assert( s.x == 1953 );
		}
	"""
	tests_lib.build_program( c_program_text )


def InClassFieldInitializer_EvaluatesInClassScope_Test0():
	c_program_text= """
		struct S
		{
			auto constant= 2019;
			i32 x= constant;
		}
		auto constant= 0;
		fn Foo() : i32
		{
			var S s{}; // 's.x' must be initialized, using 'S::constant', not '::constant'
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2019 )


def InClassFieldInitializer_EvaluatesInClassScope_Test1():
	c_program_text= """
		struct S
		{
			fn Baz() : i32 { return 99965; }
			i32 x= Baz();
		}
		fn Baz();
		fn Foo() : i32
		{
			var S s; // S::Baz() must be called from generated default constructor.
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99965 )


def InClassFieldInitializer_EvaluatesInClassScope_Test2():
	c_program_text= """
		namespace N
		{
			struct S
			{
				f32 x= Baz();
			}
			fn Baz() : f32 { return 9.8f; }
		}
		fn Foo() : f32
		{
			var N::S s{}; // 'N::Baz' must be called for initialization here.
			return s.x * 10.0f;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 98.0 )


def InClassFieldInitializer_EvaluatesInClassScope_Test3():
	c_program_text= """
		class C
		{
		private:
			auto constant= 999;
		public:
			i32 x= constant; // Ok, initializer called in 'C' scope
		}
		fn Foo() : i32
		{
			var C c;
			return c.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999 )
