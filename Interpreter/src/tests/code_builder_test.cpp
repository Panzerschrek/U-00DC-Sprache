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
	return a - b / c ;\
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

	U_ASSERT( arg0 - arg1 / arg2 == func_result );
}

void RunCodeBuilderTest()
{
	SimpleProgramTest();
}

} // namespace Interpreter
