#include <cstdio>
#include <iostream>
#include <vector>

#include "lexical_analyzer.hpp"
#include "syntax_analyzer.hpp"

#include "tests/code_builder_llvm_test.hpp"
#include "tests/inverse_polish_notation_test.hpp"

int main()
{
	Interpreter::RunIPNTests();
	Interpreter::RunCodeBuilderLLVMTest();

	std::cout << u8"Ü-Sprache Interpreter" << std::endl;

	return 0;
}
