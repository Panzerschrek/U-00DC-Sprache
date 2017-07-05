#include <iostream>
#include <vector>

#include "lexical_analyzer.hpp"
#include "syntax_analyzer.hpp"

#include "tests/code_builder_errors_test.hpp"
#include "tests/code_builder_test.hpp"
#include "tests/inverse_polish_notation_test.hpp"

int main()
{
	U::RunIPNTests();
	U::RunCodeBuilderTests();
	U::RunCodeBuilderErrorsTests();

	std::cout << u8"Ü-Sprache Compiler" << std::endl;

	return 0;
}
