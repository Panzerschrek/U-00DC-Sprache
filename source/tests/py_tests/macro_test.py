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


def MacroExpansion_Test4():
	c_program_text= """
	// Class context macro.
	?macro <? MAKE_NONCOPYABLE:class ( ?class_name:ident ) ?>  ->
	<?
	fn constructor( mut this, ?class_name& other )= delete;
	op=( mut this, ?class_name& other )= delete;
	?>

	struct S
	{
		i32 x;
		MAKE_NONCOPYABLE( S )
	}

	static_assert( !typeinfo</S/>.is_copy_constructible );
	static_assert( !typeinfo</S/>.is_copy_assignable );
	"""
	tests_lib.build_program( c_program_text )


def MacroExpansion_Test5():
	c_program_text= """
	// Namespace context macro.
	?macro <? DEFINE_CONSTANT:namespace ?name:ident ?value:expr ; ?>  ->
	<?
		auto constexpr ?name= ?value;
	?>

	DEFINE_CONSTANT pi 3.14;

	namespace NS
	{
		DEFINE_CONSTANT k1 1024;
	}

	static_assert( pi == 3.14 );
	static_assert( NS::k1 == 1024 );
	"""
	tests_lib.build_program( c_program_text )


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
	?macro <? DECLARE_ZERO_VAR:block ( ?t:ty, ?name:ident ) ?>  ->  <? var ?t ?name=zero_init; ?>

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


def MacroOptional_Test0():
	c_program_text= """
	?macro <? ADD:expr ( ?a:expr, ?b:expr ?o:opt<? ,?c:expr ?> ) ?>  ->  <? (?a) + (?b) ?o<? + (?c) ?> ?>

	static_assert( ADD( 985, 11024 ) == 985 + 11024 );
	static_assert( ADD( 81, 47, -11 ) == 81 + 47 - 11 );
	"""
	tests_lib.build_program( c_program_text )


def MacroOptional_Test1():
	c_program_text= """
	?macro <? ADD:expr ( ?a:expr ?o:opt<? FF ?> TO ?b:expr ) ?>  ->  <? (?a) + ?o<? - ?> (?b) ?>

	static_assert( ADD( 985 TO 11024 ) == 985 + 11024 );
	static_assert( ADD( 985 FF TO 11024 ) == 985 - 11024 );
	"""
	tests_lib.build_program( c_program_text )


def MacroOptional_Test2():
	c_program_text= """
	// Inside optional visible outer macro variables.
	?macro <? DO:expr ( ?a:expr ?o:opt<? SQUARE ?> ) ?>  ->  <? (?a) ?o<? * (?a) ?> ?>

	static_assert( DO( 12411 ) == 12411 );
	static_assert( DO( 66 SQUARE ) == 66 * 66 );
	"""
	tests_lib.build_program( c_program_text )


def MacroOptional_Test3():
	c_program_text= """
	// Expand optional as is.
	?macro <? DO:expr ( ?a:expr ?o:opt<? * ?b:expr ?> ) ?>  ->  <? (?a) ?o ?>   // All given lexems will be expanded in ?o

	static_assert( DO( 512 ) == 512 );
	static_assert( DO( 47 * ( 99 / 5 ) ) == 47 * ( 99 / 5 ) );
	"""
	tests_lib.build_program( c_program_text )


def MacroRepeated_Test0():
	c_program_text= """
	?macro <? SUM:expr ( ?sequence:rep<? ?e:expr ?> ) ?>  ->  <?  ?sequence<? (?e) + ?> 0  ?>

	static_assert( SUM( ) == 0 );
	static_assert( SUM( 854 ) == 854 );
	static_assert( SUM( 5 23 ) == 5 + 23 );
	static_assert( SUM( 11 -4 854 77 ) == 11 -4 + 854 + 77 );
	"""
	tests_lib.build_program( c_program_text )


def MacroRepeated_Test1():
	c_program_text= """
	// Macro with separator
	?macro <? SUM:expr ( ?sequence:rep<? ?e:expr ?><?,?> ) ?>  ->  <?  ?sequence<? (?e) + ?> 0  ?>

	static_assert( SUM( ) == 0 );
	static_assert( SUM( 854 ) == 854 );
	static_assert( SUM( 5, 23 ) == 5 + 23 );
	static_assert( SUM( 11, -4, 854, 77 ) == 11 -4 + 854 + 77 );
	"""
	tests_lib.build_program( c_program_text )


def MacroRepeated_Test2():
	c_program_text= """
	// Macro with separator
	?macro <? DO_CALL:expr ?foo:ident ( ?args:rep<? ?e:expr ?><?,?> ) ?>  ->  <?  ?foo( ?args<? (?e) ?><?,?> ) ?>

	fn constexpr Zero() : i32 { return 0; }
	fn constexpr Pass( i32 x ) : i32 { return x; }
	fn constexpr Mul( i32 x, i32 y ) : i32 { return x * y; }

	static_assert( ( DO_CALL Zero() ) == 0 );
	static_assert( ( DO_CALL Pass( -854 + 48 ) ) == -854 + 48 );
	static_assert( ( DO_CALL Mul( 85, 11 ) ) == 85 * 11 );
	"""
	tests_lib.build_program( c_program_text )


def MacroRepeated_Test3():
	c_program_text= """
	// Macro with identifier separator.
	?macro <? SUM:expr ( ?args:rep<? ?e:expr ?><?AND?> ) ?>  ->  <?  ?args<? (?e) ?><?+?> ) ?>

	static_assert( SUM( -999854 ) == -999854 );
	static_assert( SUM( 595 AND 1123 ) == 595 + 1123 );
	static_assert( SUM( 11 AND -4 AND 854 AND 77 ) == 11 -4 + 854 + 77 );
	"""
	tests_lib.build_program( c_program_text )


def MacroRepeated_Test4():
	c_program_text= """
	// Expand sequence as is.
	?macro <? CALL:expr ( ?first_arg:expr ?seq:rep<?, ?b:expr ?> ) ?>  ->  <? Mul( ?first_arg ?seq ) ?>   // All given lexems will be expanded in ?seq

	fn constexpr Mul( i32 x ) : i32 { return x; }
	fn constexpr Mul( i32 x, i32 y ) : i32 { return x * y; }
	fn constexpr Mul( i32 x, i32 y, i32 z ) : i32 { return x * y * z; }
	fn constexpr Mul( i32 x, i32 y, i32 z, i32 w ) : i32 { return x * y * z * w; }

	static_assert( CALL( 675 ) == 675 );
	static_assert( CALL( 2, 5 ) == 2 * 5 );
	static_assert( CALL( 2, (5/3), 3, (7-0) ) == 2 * (5/3) * 3 * (7-0) );
	"""
	tests_lib.build_program( c_program_text )


def StartBlockLexemAsOptionalIndicator_Test0():
	c_program_text= """
	?macro <? DOUBLE_IT:expr ( ?quad_factor:opt<? QUAD ?> ?e:expr ) ?>  ->  <? ?e * 2 ?quad_factor<? *2 ?> ?>

	static_assert( DOUBLE_IT( 31 ) == 62 );
	static_assert( DOUBLE_IT( QUAD 120 ) == 480 );
	"""
	tests_lib.build_program( c_program_text )


def StartBlockLexemAsOptionalIndicator_Test1():
	c_program_text= """
	?macro <? DOUBLE_IT:expr ( ?quad_factor:opt<? QUAD ?> & ?e:expr ) ?>  ->  <? ?e * 2 ?quad_factor<? *2 ?> ?>

	static_assert( DOUBLE_IT( & -14 ) == -28 );
	static_assert( DOUBLE_IT( QUAD & 63 * 74 ) == 63 * 74 * 4 );
	"""
	tests_lib.build_program( c_program_text )


def StartBlockLexemAsRepeatedIndicator_Test0():
	c_program_text= """
	?macro <? POW2:expr ( ?r:rep<? DO it ?> ?e:expr ) ?>  ->  <? ?e ?r<? *2 ?> ?>

	static_assert( POW2( 11 ) == 11 );
	static_assert( POW2( DO it 98 ) == 98 * 2 );
	static_assert( POW2( DO it DO it DO it -51 ) == -51 * 2 * 2 * 2 );
	"""
	tests_lib.build_program( c_program_text )


def StartBlockLexemAsRepeatedIndicator_Test1():
	c_program_text= """
	?macro <? SEQ:expr ( ?r:rep<? MUL ?n:expr ?><?,?> ?e:expr ) ?>  ->  <? ?e ?r<? * (?n) ?> ?>

	static_assert( SEQ( 11 ) == 11 );
	static_assert( SEQ( MUL 5 94 ) == 5 * 94 );
	static_assert( SEQ( MUL -1 + 2, MUL 89, MUL 74 66 ) == ( -1 +2 ) * 89 * 74 * 66 );
	"""
	tests_lib.build_program( c_program_text )


def MacroForIfAlternative_Test0():
	c_program_text= """
		?macro <? EMPTY_BLOCK:block ?> -> <? {} ?>

		fn Foo( bool cond )
		{
			if( cond ) {}
			else EMPTY_BLOCK
		}
	"""
	tests_lib.build_program( c_program_text )


def MacroForIfAlternative_Test1():
	c_program_text= """
		?macro <? TRUE_IF:block ?> -> <? if( true ) {} else { halt; } ?>

		fn Foo( bool cond )
		{
			if( cond ) {}
			else TRUE_IF
		}
	"""
	tests_lib.build_program( c_program_text )


def UniqueMacroLexem_Test0():
	c_program_text= """
	?macro <? forN:block ( ?n:expr ) ?b:block ?>
	->
	<?
		{
			var size_type mut ??i(0);
			auto ??count= ?n;
			while( ??i < ??count )
			{
				?b
				++ ??i;
			}
		}
	?>

	fn Foo()
	{
		var i32 mut i= 0;
		forN( size_type(66) ) { ++i; } // loop variable "??i" invisible here, because it replaced with unique identifier.
		halt if( i != 66 );
	}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def UniqueMacroLexem_Test1():
	c_program_text= """
	?macro <? CreateI:block ?> -> <? var size_type mut ??i(0); ?>
	fn Foo()
	{
		CreateI
		i; // Not found
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].src_loc.line == 6 )
	assert( errors_list[0].text.find("i") != -1 )


def UniqueMacroLexemIsGloballyUnique_Test0():
	# Each macro expansion should produce unique identifiers.
	c_program_text= """
	?macro <? DECLARE_S:namespace ?> -> <? struct ??s{} ?>

	DECLARE_S
	DECLARE_S
	"""
	tests_lib.build_program( c_program_text )


def UniqueMacroLexemIsGloballyUnique_Test1():
	# Each macro expansion should produce unique identifiers even if ed expansion is inside macro.
	c_program_text= """
	?macro <? DECLARE_S:namespace ?> -> <? struct ??s{} ?>
	?macro <? CALL_DECLARE_S:namespace ?> -> <? DECLARE_S ?>

	CALL_DECLARE_S
	CALL_DECLARE_S
	"""
	tests_lib.build_program( c_program_text )


def UniqueMacroLexemIsGloballyUnique_Test2():
	# Each macro expansion should produce unique identifiers even if ed expansion is inside macro.
	c_program_text= """
	?macro <? DECLARE_S:namespace ?> -> <? struct ??s{} ?>
	?macro <? CALL_DECLARE_S:namespace ?> -> <? DECLARE_S DECLARE_S ?>

	CALL_DECLARE_S
	"""
	tests_lib.build_program( c_program_text )


def MacroExpansionContext_Test0():
	c_program_text= """
	?macro <? TEST:namespace ?name:ident ?> -> <? fn ?name () { a; } ?>

	TEST Foo
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( errors_list[0].src_loc.line == 4 )
	assert( errors_list[0].template_errors.errors[0].error_code == "NameNotFound" )
	assert( errors_list[0].template_errors.errors[0].src_loc.line == 2 )


def MacroExpansionContext_Test1():
	c_program_text= """
	?macro <? TEST_IMPL:namespace ?name:ident ?> -> <? fn ?name () { a; } ?>
	?macro <? TEST:namespace ?name:ident ?> -> <? TEST_IMPL ?name ?>

	TEST Foo
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( errors_list[0].src_loc.line == 5 )
	assert( errors_list[0].template_errors.errors[0].error_code == "MacroExpansionContext" )
	assert( errors_list[0].template_errors.errors[0].src_loc.line == 3 )
	assert( errors_list[0].template_errors.errors[0].template_errors.errors[0].error_code == "NameNotFound" )
	assert( errors_list[0].template_errors.errors[0].template_errors.errors[0].src_loc.line == 2 )


def MacroExpansionContext_Test2():
	c_program_text= """
		?macro <? ZERO_RET:block ?> -> <? var T ret = 0; return ret; ?>

		template</ type T /> fn Foo() : T
		{
			ZERO_RET
		}

		fn Bar()
		{
			Foo</f32/>();
		}
	"""

	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( errors_list[0].error_code == "TemplateContext" )
	assert( errors_list[0].src_loc.line == 11 )
	assert( errors_list[0].template_errors.errors[0].error_code == "MacroExpansionContext" )
	assert( errors_list[0].template_errors.errors[0].src_loc.line == 6 )
	assert( errors_list[0].template_errors.errors[0].template_errors.errors[0].error_code == "TypesMismatch" )
	assert( errors_list[0].template_errors.errors[0].template_errors.errors[0].src_loc.line == 2 )


def MacroExpansionContext_Test3():
	# Expand macro parameter multiple times
	c_program_text= """
		?macro <? REPEAT:block ?b : block ?> -> <? ?b ?>
		?macro <? TEST:block ?b:block ?> -> <? static_if(false) ?b else ?b ?>

		fn Foo()
		{
			TEST { REPEAT{ unknown; } }
		}
	"""

	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "MacroExpansionContext" )
	assert( errors_list[0].src_loc.line == 7 )
	assert( errors_list[0].template_errors.errors[0].error_code == "MacroExpansionContext" )
	assert( errors_list[0].template_errors.errors[0].src_loc.line == 7 )
	assert( errors_list[0].template_errors.errors[0].template_errors.errors[0].error_code == "NameNotFound" )
	assert( errors_list[0].template_errors.errors[0].template_errors.errors[0].src_loc.line == 7 )
