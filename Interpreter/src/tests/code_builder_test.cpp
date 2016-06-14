#include <iostream>

#include "../code_builder.hpp"
#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"

#include "code_builder_test.hpp"

namespace Interpreter
{

static VmProgram BuildProgram( const char* text )
{
	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( text ) );

	for( const std::string& lexical_error_message : lexical_analysis_result.error_messages )
		std::cout << lexical_error_message << "\n";
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );

	for( const std::string& syntax_error_message : syntax_analysis_result.error_messages )
		std::cout << syntax_error_message << "\n";
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	const CodeBuilder::BuildResult build_result=
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );

	for( const std::string& error_message : build_result.error_messages )
		std::cout << error_message << "\n";

	return build_result.program;
}

static void SimpleProgramTest()
{
	static const char c_program_text[]=

"fn A(){}\
fn Foo( a : i32, b : i32, c : i32 ) : i32\
{\
	A();\
	a;\
	a - a;\
	return (a) - -b / c ;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 arg0= 100500, arg1= 1488, arg2= 42, func_result= 0;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, arg0, arg1, arg2 );
	U_ASSERT( call_result.ok );

	U_ASSERT( arg0 - -arg1 / arg2 == func_result );
}

static void FuncCallTest()
{
	static const char c_program_text[]=

"\
fn Bar( x : i32, y : i32 ) : i32\
{\
	return y + 1 - 1;\
}\
\
fn Foo( a : i32, b : i32 ) : i32\
{\
	return a * Bar(a + 0e0, b + 0x000i32) ;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 arg0= 100, arg1= 17, func_result= 0;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, arg0, arg1 );
	U_ASSERT( call_result.ok );

	U_ASSERT( arg0 * arg1 == func_result );
}

static void IfOperatorTest()
{
	static const char c_program_text[]=

"fn Factorial(a : i32) : i32\
{\
	if( a > 1e-0i32 )\
	{\
		return a * Factorial( a - 1 );\
	}\
	else\
	{\
		return 0o01;\
	}\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	for( U_i32 i= 1,expected_result= 1; i < 13; i++ )
	{
		expected_result*= i;

		U_i32 func_result;

		const VM::CallResult call_result =
			vm.CallRet( ToProgramString("Factorial"), func_result, i );
		U_ASSERT( call_result.ok );

		U_ASSERT( func_result == expected_result );
	}
}

static void VariablesTest()
{
	static const char c_program_text[]=

"\
fn Foo( c : i32 ) : i32\
{\
	let x : i32 = c;\
	let y : i32;\
	y= c;\
	x= 0b00i32;\
	c= c * y;\
	return y * c + x;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 arg0= 100, func_result= 0;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, arg0 );
	U_ASSERT( call_result.ok );

	U_ASSERT( arg0 * arg0 * arg0 == func_result );
}

static void StackVariablesPlacementTest()
{
	static const char c_program_text[]=

"\
fn Foo( x : i32 ) : i32\
{\
	let a : i32= x;\
	{\
		let unused_b : bool;\
		let b : i32 = a;\
		{\
			{\
				let AA : i32; let BB : i32; let CC : i32; let DD : i64;  let EE : bool = false;\
			}\
			let c : i32 = b;\
			let unused_c : i64;\
			{\
				let some_boolean : bool = true;\
				return c;\
			}\
		}\
	}\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 arg0= 1488, func_result= 0;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, arg0 );
	U_ASSERT( call_result.ok );

	U_ASSERT( arg0 == func_result );
}

static void WhileOperatorTest()
{
	static const char c_program_text[]=

"fn Factorial(x : u32) : u32\
{\
	let result : u32 = 1u32;\
	let i : u32= 1u32;\
	while( i <= x )\
	{\
		while( false ){}\
		result= result * i;\
		i= i + 1u32;\
	}\
	return result;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	for( U_u32 i= 1, expected_result= 1; i < 13; i++ )
	{
		expected_result*= i;

		U_u32 func_result;

		const VM::CallResult call_result =
			vm.CallRet( ToProgramString("Factorial"), func_result, i );
		U_ASSERT( call_result.ok );

		U_ASSERT( func_result == expected_result );
	}
}

static void BreakOperatorTest()
{
	static const char c_program_text[]=

"\
fn Foo( c : i32 ) : i32\
{\
	while( true )\
	{\
		if( c * c < c + c ) { break; }\
		return c + 1;\
	}\
	return c + c + c;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 arg0, func_result= 0;
	VM::CallResult call_result;

	arg0= 1;
	call_result= vm.CallRet( ToProgramString("Foo"), func_result, arg0 );
	U_ASSERT( call_result.ok );
	U_ASSERT( func_result == 3);

	arg0= 3;
	call_result= vm.CallRet( ToProgramString("Foo"), func_result, arg0 );
	U_ASSERT( call_result.ok );
	U_ASSERT( func_result == 4);
}

static void ContinueOperatorTest()
{
	static const char c_program_text[]=

"fn Factorial(x : u32) : u32\
{\
	let result : u32 = 1u32;\
	let i : u32= 1u32;\
	while( true )\
	{\
		result= result * i;\
		i= i + 1u32;\
		if( i <= x ) { continue; }\
		break;\
	}\
	return result;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	for( U_u32 i= 1, expected_result= 1; i < 13; i++ )
	{
		expected_result*= i;

		U_u32 func_result;

		const VM::CallResult call_result =
			vm.CallRet( ToProgramString("Factorial"), func_result, i );
		U_ASSERT( call_result.ok );

		U_ASSERT( func_result == expected_result );
	}
}

static void BitOperatorsTest()
{
	static const char c_program_text[]=

"\
fn Foo( a : i32, b : i32, c : i32 ) : i32\
{\
	return (a | b) & c;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 func_result, a= 0b00110011, b= 0b11001100, c= 0b1101;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, a, b, c );
	U_ASSERT( call_result.ok );

	U_ASSERT( ( (a|b) & c ) == func_result );
}

static void BooleanOperatorsTest()
{
	static const char c_program_text[]=

"\
fn Foo( a : bool, b : bool, c : bool ) : bool\
{\
	return (a | b) & c;\
}"
;

	VM vm{ BuildProgram( c_program_text ) };

	for( unsigned int a= 0; a < 1; a++ )
	for( unsigned int b= 0; b < 1; b++ )
	for( unsigned int c= 0; c < 1; c++ )
	{
		bool func_result;

		const VM::CallResult call_result =
			vm.CallRet( ToProgramString("Foo"), func_result, bool(a), bool(b), bool(c) );
		U_ASSERT( call_result.ok );

		U_ASSERT( ( (bool(a) | bool(b)) & bool(c) ) == func_result );
	}
}


static void MultipleCallTest()
{
	static const char c_program_text[]=

R"RAW(
fn X0(){} // no args and void result
fn X1() : i32 { return 1; } // no args and nonzero result
fn X2( x : i32 ) : i32 // args and nonzero result
{
	return x * 2;
}
fn X3( x : i32 ) : i32
{
	return x * 3;
}
fn X99( x : i32 ) {} // args and void result
fn Z( x : i32 ) : i32
{
	((((X0))))(); // Func name inside brackets
	X0; // Func name without call
	X0();
	X99(x);
	return X1() * X2(x) + X3(x);
}

)RAW"
;

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 func_result, x= 451;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Z"), func_result, x );
	U_ASSERT( call_result.ok );

	U_ASSERT( x * 5 == func_result );
}

static void ArraysTest0()
{
	static const char c_program_text[]=
R"(
fn Foo( a : i32 ) : i32
{
	let x : [ i32, 2 ];
	x[ 1u32 ]= a;
	return x[ 1u8 ] * x[ 1u16 ] + x[ 0u32 ] - x[ 0u64 ];
}
)";

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 func_result, x= -42;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, x );
	U_ASSERT( call_result.ok );
	U_ASSERT( func_result == x * x );
}

static void ArraysTest1()
{
	static const char c_program_text[]=
R"(
fn Foo( a : u8 ) : u8
{
	let x : [ u8, 2 ];
	x[ 1u32 ]= a;
	return x[ 1u32 ];
}
)";

	VM vm{ BuildProgram( c_program_text ) };

	U_u8 func_result, x= 196;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, x );
	U_ASSERT( call_result.ok );
	U_ASSERT( func_result == x );
}

static void MultidimensionalArraysTest()
{
	static const char c_program_text[]=
R"(
fn Foo( a : i32 ) : i32
{
	let x : [ [ [ i32, 3 ], 2 ], 2 ];
	x[ 0u32 ][ 1u8 ][ 2u32 ]= a - 34;
	return x[ 0u16 ][ 1u32 ][ 2u64 ] + 42;
}
)";

	VM vm{ BuildProgram( c_program_text ) };

	U_i32 func_result, x= -42 - 500264;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, x );
	U_ASSERT( call_result.ok );
	U_ASSERT( func_result == x - 34 + 42 );
}

void RunCodeBuilderTest()
{
	SimpleProgramTest();
	FuncCallTest();
	IfOperatorTest();
	VariablesTest();
	StackVariablesPlacementTest();
	WhileOperatorTest();
	BreakOperatorTest();
	ContinueOperatorTest();
	BitOperatorsTest();
	BooleanOperatorsTest();
	MultipleCallTest();
	ArraysTest0();
	ArraysTest1();
	MultidimensionalArraysTest();
}

} // namespace Interpreter
