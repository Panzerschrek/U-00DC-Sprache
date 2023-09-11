#pragma once
#include <string>
#include <vector>

namespace U
{

struct TestId // Use struct with non-trivial constructor adn destructor for prevent "unused-variable" warning.
{
	TestId(){}
	~TestId() {}
};

using TestFunc= void();

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
inline const TestId NAME##variable= AddTestFuncPrivate( NAME##Func, __FILE__, #NAME );\
static void NAME##Func()

// For tests launcher:

struct TestFuncData
{
	std::string name;
	TestFunc* func;
};

using TestsFuncsContainer= std::vector<TestFuncData>;

TestsFuncsContainer& GetTestsFuncsContainer();

} // namespace U
