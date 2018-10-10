from py_tests_common import *

def MacroDefenition_Test0():
	c_program_text= """
	?macro <? Pass:expr ?e:expr ?>  ->  <? ?e ?>
	"""
	tests_lib.build_program( c_program_text )


def MacroDefenition_Test1():
	c_program_text= """
	?macro <? PassInBrackets:block  ( ?e:expr ) ?>  ->  <? while(true) { ?e; break; } ?>
	"""
	tests_lib.build_program( c_program_text )


def MacroExpansion_Test0():
	c_program_text= """
	// Expression context macro.
	?macro <? Double:expr ( ?e:expr ) ?>  ->  <? ?e * 2 ?>

	fn Foo() : i32
	{
		return Double(42);
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 42 * 2 )


def MacroExpansion_Test1():
	c_program_text= """
	// Expression context macro with no ?parameters.
	?macro <? ZeroInt:expr () ?>  ->  <? i32(0) ?>

	fn Foo() : i32
	{
		return ZeroInt();
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def MacroExpansion_Test2():
	c_program_text= """
	// Block context macro with block argument.
	?macro <? FiveTimes:block ?b:block ?>  ->  <? ?b ?b ?b ?b ?b ?>

	fn Foo() : i32
	{
		auto mut x= 0;
		FiveTimes { ++x; }
		return x;
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5 )


def MacroExpansion_Test3():
	c_program_text= """
	// Block context macro with expression argument.
	?macro <? Inc:block ?e:expr ?>  ->  <? ++?e; ?>

	fn Foo() : i32
	{
		auto mut x= 0;
		Inc(x)
		return x;
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 1 )


def ExpandMacroWhileExpandingMacro_Test0():
	c_program_text= """
	?macro <? FiveTimes:block ?b:block ?>  ->  <? ?b ?b ?b ?b ?b ?>
	?macro <? TenTimes:block ?b:block ?>  ->  <? FiveTimes ?b FiveTimes ?b ?>

	fn Foo() : i32
	{
		auto mut x= 0;
		TenTimes { ++x; }
		return x;
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 10 )


def ExpandMacroWhileExpandingMacro_Test1():
	c_program_text= """
	?macro <? MulFive:expr ( ?e:expr ) ?>  ->  <? ( ?e * 5 ) ?>
	?macro <? MulTen:expr ( ?e:expr ) ?>  ->  <? ( MulFive(?e) * 2 ) ?>

	fn Foo() : i32
	{
		return MulTen(31);
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 31 * 10 )


def ExpandMacroInArgumentOfOtherMacro_Test0():
	c_program_text= """
	?macro <? FourTimes:block ?b:block ?>  ->  <? ?b ?b ?b ?b ?>
	?macro <? FiveTimes:block ?b:block ?>  ->  <? ?b ?b ?b ?b ?b ?>
	?macro <? TenTimes:block ?b:block ?>  ->  <? FiveTimes ?b FiveTimes ?b ?>

	fn Foo() : i32
	{
		auto mut x= 0;
		TenTimes { FourTimes{ ++x; } }
		return x;
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 10 * 4 )


def ExpandMacroInArgumentOfOtherMacro_Test1():
	c_program_text= """
	?macro <? MulFive:expr ( ?e:expr ) ?>  ->  <? ( ?e * 5 ) ?>

	fn Foo() : i32
	{
		return MulFive( MulFive(11) );
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5 * 5 * 11 )


def MacroParamIdentifier_Test0():
	c_program_text= """
	?macro <? ADD_COUNTER:block ( ?i:ident ) ?>  ->  <? var size_type mut ?i(0); ?>

	fn Foo() : i32
	{
		auto mut x= 7;
		ADD_COUNTER( counter )
		while( counter < size_type(4) )
		{
			x*= 3;
			++counter;
		}

		return x;
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 7 * 3 * 3 * 3 * 3 )


def MacroParamTypeName_Test0():
	c_program_text= """
	?macro <? DECLARE_ZERO_VAR:block ( ?type:ty, ?name:ident ) ?>  ->  <? var ?type ?name=zero_init; ?>

	struct Vec{ f32 x; f32 y; }
	fn Foo()
	{
		DECLARE_ZERO_VAR( i32, x )
		DECLARE_ZERO_VAR( [ i32, 4 ], arr )
		DECLARE_ZERO_VAR( (fn()), fn_ptr )
		DECLARE_ZERO_VAR( Vec, vec )
	}
	"""
	tests_lib.build_program( c_program_text )


def MacroParamExpression_Test0():
	c_program_text= """
	?macro <? SET:block ( ?dst:expr TO ?src:expr ) ?>  ->  <? ?dst= ?src; ?>

	fn Foo() : i32
	{
		var i32 mut x= 0;
		SET( x TO 2 + 2 * 2 )
		return x;
	}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2 + 2 * 2 )
