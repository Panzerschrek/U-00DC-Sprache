#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/Linker/Linker.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "unit_tests_table.hpp"

namespace U
{

bool BuildUnitTestsTable( llvm::Module& module, const llvm::StringRef table_name )
{
	auto module_with_table= std::make_unique<llvm::Module>( "__U_tests_table_module", module.getContext() );
	module_with_table->setDataLayout( module.getDataLayout() );
	module_with_table->setTargetTriple( module.getTargetTriple() );

	const auto pointer_type= llvm::PointerType::get( module.getContext(), 0u );
	const auto table_element_type= llvm::StructType::get( pointer_type );

	llvm::SmallVector<llvm::Constant*, 256> table_initializers;

	for( llvm::Function& function : module.functions() )
	{
		if( function.hasMetadata( "__U_unit_test" ) )
			table_initializers.push_back( llvm::ConstantStruct::get( table_element_type, { &function } ) );
	}

	const auto table_type= llvm::ArrayType::get( table_element_type, table_initializers.size() );
	llvm::Constant* const table_initializer= llvm::ConstantArray::get( table_type, table_initializers );

	new llvm::GlobalVariable(
		*module_with_table,
		table_type,
		true, // is constant,
		llvm::GlobalValue::ExternalLinkage,
		table_initializer,
		table_name );

	const bool not_ok= llvm::Linker::linkModules( module, std::move(module_with_table) );
	if( not_ok )
	{
		std::cerr << "Error linking tests table!" << std::endl;
		return false;
	}
	return true;
}

} // namespace U
