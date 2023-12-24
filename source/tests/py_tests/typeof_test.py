from py_tests_common import *

def TypeofOperatorDeclaration_Test0():
	c_program_text= """
		type T= typeof(0);
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test1():
	c_program_text= """
		type T= typeof( 55 * 88 );
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test2():
	c_program_text= """
		type T= [ typeof( 0.25 ), 64 ];
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test3():
	c_program_text= """
		type T= typeof( "str" );
	"""
	tests_lib.build_program( c_program_text )


def TypeofOperatorDeclaration_Test5():
	c_program_text= """
		fn Foo() : i32;
		type T= typeof( Foo() );
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test0():
	c_program_text= """
		fn Baz() : i32 { return 666; }
		fn Foo()
		{
			var typeof( Baz() ) x= Baz(); // Type will be "i32"
			var i32 x_copy= x;
		}
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test1():
	c_program_text= """
		fn Pass( f64& x ) : f64& { return x; }
		fn Foo()
		{
			var f64 x= 0.52;
			var typeof( Pass(x) ) x_copy= x; // Type will be "f64", function reference modifier ignored
		}
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test2():
	c_program_text= """
		type PiType= typeof(3.14f); // Typeof for global type alias
		var PiType e= 2.718281828f;
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test3():
	c_program_text= """
		struct S {}
		var S constexpr s{};
		fn GetS() : typeof(s)& // Typeof for function return type
		{
			return s;
		}
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test4():
	c_program_text= """
		struct S {}
		var S constexpr s{};
		fn CopyS( typeof(s) mut arg ) : S // Typeof for function argument type
		{
			return move(arg);
		}
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test5():
	c_program_text= """
		struct S
		{
			auto constexpr SomeConstant= "8"c8;
			typeof(SomeConstant) field; // Typeof for class field
		}
	"""
	tests_lib.build_program( c_program_text )


def Typeof_Test6():
	c_program_text= """
		fn Foo()
		{
			auto &constexpr str= "Some String";
			var typeof(str) str_storage= zero_init; // Typeof for string type
			static_assert( typeinfo</ typeof(str) />.element_count == size_type(11) ); // Typeof for typeinfo
		}
	"""
	tests_lib.build_program( c_program_text )


def TypeofHasNoEffects_Test0():
	c_program_text= """
		fn Inc( i32 &mut x ) : i32 { ++x; return x; }
		fn Foo()
		{
			var i32 mut x= 666;
			var typeof( Inc(x) ) x_copy= x; // Only type evalueated for expression 'Inc(x)', no actual code generated.
			halt if( x != 666 );
			halt if( x_copy != 666 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def Typeof_ChecksExpression_Test0():
	c_program_text= """
		type T= typeof( CallUnknownFunction() );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 2 )
