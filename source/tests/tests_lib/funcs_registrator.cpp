#include "funcs_registrator.hpp"

namespace U
{

TestsFuncsContainer& GetTestsFuncsContainer()
{
	static TestsFuncsContainer funcs_container;
	return funcs_container;
}

TestId AddTestFuncPrivate( TestFunc* func, const char* const file_name, const char* const func_name )
{
	GetTestsFuncsContainer().emplace_back( TestFuncData{ std::string(file_name) + ":" + func_name, func } );
	return TestId();
}

} // namespace U
