#pragma once
#include <exception>

#include "../push_disable_llvm_warnings.hpp"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include "../pop_llvm_warnings.hpp"

#include "../code_builder.hpp"

namespace U
{

typedef unsigned int TestId;
typedef void (TestFunc)();
class TestException  final : public std::runtime_error
{
public:
	TestException( const char* const what )
		: std::runtime_error( what )
	{}
};

TestId AddTestFuncPrivate( TestFunc* func, const char* const file_name, const char* const func_name );

/*
Create test. Usage:

U_TEST(TestName)
{
// test body
}

Test will be registered at tests startup and executed lately.
 */
#define U_TEST(NAME) \
static void NAME##Func();\
static const TestId NAME##variable= AddTestFuncPrivate( NAME##Func, __FILE__, #NAME );\
static void NAME##Func()

// Main tests assertion handler. Aborts test.
#define U_TEST_ASSERT(x) \
	if( !(x) )\
	{\
		throw TestException( #x );\
	}

// Tests launch procedure.
void RunAllTests();

// Utility tests functions.
typedef std::unique_ptr<llvm::ExecutionEngine> EnginePtr;

std::unique_ptr<llvm::Module> BuildProgram( const char* text );
CodeBuilder::BuildResult BuildProgramWithErrors( const char* text );
EnginePtr CreateEngine( std::unique_ptr<llvm::Module> module, bool needs_dump= false );

} // namespace U
