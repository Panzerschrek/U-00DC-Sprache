#include "code_builder.hpp"

namespace U
{

Type CodeBuilder::GetGeneratorFunctionReturnType( const FunctionType& generator_function_type )
{
	// TODO - create specail class type.
	(void)generator_function_type;

	RawPointerType raw_pointer_type;
	raw_pointer_type.element_type= FundamentalType( U_FundamentalType::byte8_, fundamental_llvm_types_.byte8_ );
	raw_pointer_type.llvm_type= llvm::PointerType::get( llvm_context_, 0 );

	return std::move( raw_pointer_type );
}

void CodeBuilder::CreateGeneratorEntryBlock( FunctionContext& function_context )
{
	(void)function_context;
}

} // namespace U
