#include <iostream>
#include <vector>

#include "code_builder_errors_test.hpp"
#include "code_builder_test.hpp"
#include "initializers_errors_test.hpp"
#include "initializers_test.hpp"
#include "inverse_polish_notation_test.hpp"

int main()
{
	U::RunIPNTests();
	U::RunCodeBuilderTests();
	U::RunInitializersTest();
	// Run tests with code builder errors after tests without errors.
	U::RunCodeBuilderErrorsTests();
	U::RunInitializersErrorsTest();

	return 0;
}
