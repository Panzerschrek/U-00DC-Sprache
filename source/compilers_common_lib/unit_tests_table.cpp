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
	llvm::LLVMContext& llvm_context= module.getContext();

	auto module_with_table= std::make_unique<llvm::Module>( "__U_tests_table_module", llvm_context );
	module_with_table->setDataLayout( module.getDataLayout() );
	module_with_table->setTargetTriple( module.getTargetTriple() );

	const auto pointer_type= llvm::PointerType::get( llvm_context, 0u );

	// Function pointer + name begin + name end.
	const auto table_element_type= llvm::StructType::get( pointer_type, pointer_type, pointer_type );

	llvm::SmallVector<llvm::Constant*, 256> table_initializers;

	const llvm::StringRef test_marker_metadata= "__U_unit_test";

	std::unordered_map< llvm::Constant*, llvm::GlobalVariable* > names_table;

	for( llvm::Function& function : module.functions() )
	{
		if( function.hasMetadata( test_marker_metadata ) )
		{
			// Create a global variable containing name.
			// Duduplicate such variables in case of identical names.

			const llvm::StringRef test_name=
					llvm::dyn_cast<llvm::MDString>( function.getMetadata( test_marker_metadata )->getOperand(0).get() )->getString();

			llvm::Constant* const name_string_initializer=
				llvm::ConstantDataArray::getString( llvm_context, test_name, false );

			if( names_table.find( name_string_initializer ) == names_table.end() )
				names_table.emplace(
					name_string_initializer,
					new llvm::GlobalVariable(
						*module_with_table,
						name_string_initializer->getType(),
						true, // is constant,
						llvm::GlobalValue::PrivateLinkage,
						name_string_initializer,
						"__U_test_name_" + test_name ) );

			// Global variable containing name is name begin pointer.
			llvm::GlobalVariable* const name_variable= names_table[ name_string_initializer ];

			llvm::Value* const name_size_value[]
			{
				llvm::ConstantInt::get( llvm_context, llvm::APInt( 32u, uint64_t( test_name.size() ) ) )
			};

			// Global variable containing name + its size is end pointer.
			const auto name_variable_end_ptr=
				llvm::ConstantExpr::getGetElementPtr(
					name_string_initializer->getType(), name_variable, name_size_value );

			table_initializers.push_back(
				llvm::ConstantStruct::get(
					table_element_type,
					{ &function, name_variable, name_variable_end_ptr } ) );
		}
	}

	// Push terminating null.
	table_initializers.push_back( llvm::Constant::getNullValue( table_element_type ) );

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
