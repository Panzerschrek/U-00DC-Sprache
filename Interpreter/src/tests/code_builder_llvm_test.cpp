#include <cstdlib>
#include <iostream>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>

#include "../code_builder_llvm.hpp"
#include "../lexical_analyzer.hpp"
#include "../syntax_analyzer.hpp"

#include "code_builder_llvm_test.hpp"

namespace Interpreter
{

static std::unique_ptr<llvm::Module> BuildProgram( const char* const text )
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

	CodeBuilderLLVM::BuildResult build_result=
		CodeBuilderLLVM().BuildProgram( syntax_analysis_result.program_elements );

	for( const std::string& error_message : build_result.error_messages )
		std::cout << error_message << "\n";

	return std::move( build_result.module );
}

static llvm::ExecutionEngine* CreateEngine(
	std::unique_ptr<llvm::Module> module, const bool needs_dump= false )
{
	U_ASSERT( module != nullptr );

	if( needs_dump )
		module->dump();

	llvm::EngineBuilder builder( std::move(module) );
	llvm::ExecutionEngine* const engine= builder.create();

	U_ASSERT( engine != nullptr );
	return engine;
}

static void SimpleProgramTest()
{
	static const char c_program_text[]=
	"\
	fn Foo( a : i32, b : i32, c : i32 ) : i32\
	{\
		return a + b + c;\
	}"
	;

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ) );

	U_i32 arg0= 100500, arg1= 1488, arg2= 42;

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_ASSERT( static_cast<uint64_t>( arg0 + arg1 + arg2 ) == result_value.IntVal.getLimitedValue() );
}

static void BasicBinaryOperationsTest()
{
	static const char c_program_text[]=
	"\
	fn Foo( a : i32, b : i32, c : i32 ) : i32\
	{\
		return a * a + b / b - c;\
	}"
	;

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	int arg0= 77, arg1= 1488, arg2= 42;

	llvm::GenericValue args[3];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );
	args[2].IntVal= llvm::APInt( 32, arg2 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 3 ) );

	U_ASSERT( static_cast<uint64_t>( arg0 * arg0 + arg1 / arg1 - arg2 ) == result_value.IntVal.getLimitedValue() );
}

static void VariablesTest()
{
	static const char c_program_text[]=
	"\
	fn Foo( a : i32, b : i32 ) : i32\
	{\
		let tmp : i32;\
		tmp = a - b;\
		return tmp;\
	}"
	;

	llvm::ExecutionEngine* const engine= CreateEngine( BuildProgram( c_program_text ), true );

	llvm::Function* function= engine->FindFunctionNamed( "Foo" );
	U_ASSERT( function != nullptr );

	int arg0= 1488, arg1= 77;

	llvm::GenericValue args[2];
	args[0].IntVal= llvm::APInt( 32, arg0 );
	args[1].IntVal= llvm::APInt( 32, arg1 );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>( args, 2 ) );

	U_ASSERT( static_cast<uint64_t>( arg0 - arg1 ) == result_value.IntVal.getLimitedValue() );
}

void RunCodeBuilderLLVMTest()
{
	SimpleProgramTest();
	BasicBinaryOperationsTest();
	VariablesTest();
}

} // namespace Interpreter
