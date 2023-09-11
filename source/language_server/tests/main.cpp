#include <iostream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"

// Entry point for language server tests executable.
int main(int argc, char* argv[])
{
	using namespace U;

	const llvm::InitLLVM llvm_initializer(argc, argv);

	const TestsFuncsContainer& funcs_container= GetTestsFuncsContainer();

	std::cout << "Run " << funcs_container.size() << " language server tests" << std::endl << std::endl;

	uint32_t passed= 0u;
	uint32_t disabled= 0u;
	uint32_t failed= 0u;

	for(const TestFuncData& func_data : funcs_container )
	{
		try
		{
			func_data.func();
			++passed;
		}
		catch( const DisableTestException& )
		{
			std::cout << "Test " << func_data.name << " disabled\n";
			disabled++;
		}
		catch( const TestException& ex )
		{
			std::cout << "Test " << func_data.name << " failed: " << ex.what() << "\n" << std::endl;
			failed++;
		}
	}

	std::cout << std::endl <<
		passed << " tests passed\n" <<
		disabled << " tests disabled\n" <<
		failed << " tests failed" << std::endl;

	return -int(failed);
}
