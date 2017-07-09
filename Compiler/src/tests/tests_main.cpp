#include <iostream>
#include <vector>

#include "code_builder_errors_test.hpp"
#include "code_builder_test.hpp"
#include "initializers_test.hpp"
#include "inverse_polish_notation_test.hpp"

int main()
{
	U::RunIPNTests();
	U::RunCodeBuilderTests();
	U::RunInitializersTest();
	U::RunCodeBuilderErrorsTests();

	return 0;
}
