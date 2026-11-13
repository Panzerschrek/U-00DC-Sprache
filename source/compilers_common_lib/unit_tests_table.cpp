#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constants.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/assert.hpp"
#include "unit_tests_table.hpp"

namespace U
{

void BuildUnitTestsTable( llvm::Module& module, const llvm::StringRef table_name )
{
	llvm::LLVMContext& llvm_context= module.getContext();

	const auto pointer_type= llvm::PointerType::get( llvm_context, 0u );
	const auto size_type= module.getDataLayout().getIntPtrType( llvm_context );

	// Function pointer + name begin + name size.
	const auto table_element_type= llvm::StructType::get( pointer_type, pointer_type, size_type );

	llvm::SmallVector<llvm::Constant*, 256> table_initializers;

	 // This should match mentadata used by both compilers for marking unit-test functions.
	const llvm::StringRef test_marker_metadata_name= "__U_unit_test";

	std::unordered_map< llvm::Constant*, llvm::GlobalVariable* > names_table;

	for( llvm::Function& function : module.functions() )
	{
		if( function.hasMetadata( test_marker_metadata_name ) )
		{
			// Create a global variable containing name.
			// Duduplicate such variables in case of identical names.

			llvm::MDNode* const test_marker_metadata= function.getMetadata( test_marker_metadata_name );
			U_ASSERT( test_marker_metadata != nullptr );
			U_ASSERT( test_marker_metadata->getNumOperands() == 1 );

			const auto md_string= llvm::dyn_cast<llvm::MDString>( test_marker_metadata->getOperand(0).get() );
			U_ASSERT( md_string != nullptr );

			const llvm::StringRef test_name= md_string->getString();

			llvm::Constant* const name_string_initializer=
				llvm::ConstantDataArray::getString( llvm_context, test_name, false /* not null-terminated */ );

			if( names_table.find( name_string_initializer ) == names_table.end() )
				names_table.emplace(
					name_string_initializer,
					new llvm::GlobalVariable(
						module, // Ownership is transferred to the result module.
						name_string_initializer->getType(),
						true, // is constant,
						llvm::GlobalValue::PrivateLinkage,
						name_string_initializer,
						"__U_test_name_" + test_name ) );

			// Global variable containing name is name begin pointer.
			llvm::GlobalVariable* const name_variable= names_table[ name_string_initializer ];

			llvm::Constant* const name_size_value= llvm::ConstantInt::get( size_type, uint64_t( test_name.size() ) );

			table_initializers.push_back(
				llvm::ConstantStruct::get(
					table_element_type,
					{ &function, name_variable, name_size_value } ) );
		}
	}

	// Push terminating null. It's necessary to be able to calculate table size in run-time.
	table_initializers.push_back( llvm::Constant::getNullValue( table_element_type ) );

	const auto table_type= llvm::ArrayType::get( table_element_type, table_initializers.size() );

	const auto table_variable=
		new llvm::GlobalVariable(
			module, // Ownership is transferred to the result module.
			table_type,
			true, // is constant,
			llvm::GlobalValue::ExternalLinkage,
			llvm::ConstantArray::get( table_type, table_initializers ),
			"" /* set name later */ );

	// In case if there is already a variable with this name, replace it and remove it.
	if( const auto prev_variable= module.getNamedGlobal( table_name ) )
	{
		prev_variable->replaceAllUsesWith( table_variable );
		prev_variable->eraseFromParent();
	}

	table_variable->setName( table_name );
}

} // namespace U
