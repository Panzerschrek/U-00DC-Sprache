#include "unit_tests_table.hpp"

namespace U
{

void BuildUnitTestsTable( llvm::Module& module , const llvm::StringRef table_name )
{
	llvm::SmallVector<llvm::Function*, 256> test_functions;

	for( llvm::Function& function : module.functions() )
	{
		if( function.hasMetadata( "__U_unit_test" ) )
			test_functions.push_back( &function );
	}

	(void)table_name;
}

} // namespace U
