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
	llvm::Value* const null= llvm::ConstantPointerNull::get( llvm::PointerType::get( llvm_context_, 0 ) );

	llvm::Value* const coro_id= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_id ),
		{
			llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, uint64_t(0) ) ),
			null,
			null,
			null,
		},
		"coro_id" );

	llvm::Value* const coro_frame_size= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_size, { fundamental_llvm_types_.int_ptr } ),
		{},
		"coro_frame_size" );

	if( false )
	{
		llvm::Value* const coro_frame_align= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_align, { fundamental_llvm_types_.int_ptr } ),
			{},
			"coro_frame_align" );
		(void)coro_frame_align;
	}

	llvm::Value* const coro_frame_memory= function_context.llvm_ir_builder.CreateCall(
		coro_.malloc,
		{ coro_frame_size },
		"coro_frame_memory" );

	llvm::Value* const coro_handle= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_begin ),
		{ coro_id, coro_frame_memory },
		"coro_handle" );

	function_context.coro_id= coro_id;
	function_context.coro_handle= coro_handle;

	// TODO - make here initial suspend?
}

void CodeBuilder::CreateGeneratorEndBlock( FunctionContext& function_context )
{
	llvm::Value* const mem_for_free= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_free ),
		{ function_context.coro_id, function_context.coro_handle },
		"coro_frame_memory_for_free" );

	function_context.llvm_ir_builder.CreateCall( coro_.free, { mem_for_free } );

	function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_end ),
		{ function_context.coro_handle, llvm::ConstantInt::getFalse( llvm_context_ ) } );

	function_context.llvm_ir_builder.CreateRet( function_context.coro_handle );
}

} // namespace U
