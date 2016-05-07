#include <algorithm>

#include "vm.hpp"

namespace Interpreter
{

VM::VM( VmProgram program )
	: program_( std::move( program ) )
{
	U_ASSERT( std::is_sorted( program_.import_funcs ) );
	U_ASSERT( std::is_sorted( program_.export_funcs ) );
}

Funcs::iterator VM::FindExportedFunc( const ProgramString& name )
{
	// Perform binary search.
	// Warning, program_.export_funcs must be sorted.

	auto it= std::lower_bound(
		program_.export_funcs.begin(),
		program_.export_funcs.end(),
		name,
		[]( const FuncEntry& func, const ProgramString& name ) -> bool
		{
			return func.name < name;
		});

	return it;
}

bool VM::PushArgs(
	StackFrame& stack,
	const std::vector<U_FundamentalType> params,
	unsigned int n)
{
	U_UNUSED(stack);

	return n == params.size();
}

} // namespace Interpreter
