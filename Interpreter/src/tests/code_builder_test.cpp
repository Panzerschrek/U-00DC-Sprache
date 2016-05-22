#include "../code_builder.hpp"
#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"

#include "code_builder_test.hpp"

namespace Interpreter
{

static void SimpleProgramTest()
{
	static const char c_program_text[]=

"fn Foo( a : i32, b : i32, c : i32 ) : i32\
{\
	return a - -b / c ;\
}"
;

	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( c_program_text ) );
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	const CodeBuilder::BuildResult build_result=
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );
	U_ASSERT( build_result.error_messages.empty() );

	VM vm{ build_result.program };

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
	return y + x / x - y / y;\
}\
\
fn Foo( a : i32, b : i32 ) : i32\
{\
	return a * Bar(a + b - b, b + a - a) ;\
}"
;

	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( c_program_text ) );
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	const CodeBuilder::BuildResult build_result=
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );
	U_ASSERT( build_result.error_messages.empty() );

	VM vm{ build_result.program };

	U_i32 arg0= 100, arg1= 17, func_result= 0;

	const VM::CallResult call_result =
		vm.CallRet( ToProgramString("Foo"), func_result, arg0, arg1 );
	U_ASSERT( call_result.ok );

	U_ASSERT( arg0 * arg1 == func_result );
}

static void IfOperatorTest()
{
	static const char c_program_text[]=

"fn Factorial(a : i32, one : i32) : i32\
{\
	if( a > one )\
	{\
		return a * Factorial( a - one, one );\
	}\
	else\
	{\
		return one;\
	}\
}"
;

	const LexicalAnalysisResult lexical_analysis_result=
		LexicalAnalysis( ToProgramString( c_program_text ) );
	U_ASSERT( lexical_analysis_result.error_messages.empty() );

	const SyntaxAnalysisResult syntax_analysis_result=
		SyntaxAnalysis( lexical_analysis_result.lexems );
	U_ASSERT( syntax_analysis_result.error_messages.empty() );

	const CodeBuilder::BuildResult build_result=
		CodeBuilder().BuildProgram( syntax_analysis_result.program_elements );
	U_ASSERT( build_result.error_messages.empty() );

	VM vm{ build_result.program };

	for( U_i32 i= 1,expected_result= 1; i < 13; i++ )
	{
		expected_result*= i;

		U_i32 one= 1, func_result;

		const VM::CallResult call_result =
			vm.CallRet( ToProgramString("Factorial"), func_result, i, one );
		U_ASSERT( call_result.ok );

		U_ASSERT( func_result == expected_result );
	}
}

void RunCodeBuilderTest()
{
	SimpleProgramTest();
	FuncCallTest();
	IfOperatorTest();
}

} // namespace Interpreter
