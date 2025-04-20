#include "../../../lex_synt_lib_common/assert.hpp"
#include "../../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../../code_builder_lib_common/return_value_optimization.hpp"

extern "C"
{

LLVMValueRef U1_ConstantTokenNone( const LLVMContextRef C )
{
	return llvm::wrap( llvm::ConstantTokenNone::get( *llvm::unwrap(C) ) );
}

LLVMTypeRef U1_GetFunctionType(const LLVMValueRef f)
{
	return llvm::wrap(llvm::dyn_cast<llvm::Function>(llvm::unwrap(f))->getFunctionType());
}

bool U1_BasicBlockHasPredecessors(const LLVMBasicBlockRef basic_block)
{
	return llvm::unwrap(basic_block)->hasNPredecessorsOrMore(1);
}

LLVMValueRef U1_CreateOrphanInBoundsGEP( const LLVMTypeRef t, const LLVMValueRef poiter, LLVMValueRef* const indices, const uint32_t num_indices )
{
	return
		llvm::wrap(
			llvm::GetElementPtrInst::CreateInBounds(
				llvm::unwrap(t),
				llvm::unwrap(poiter),
				llvm::ArrayRef<llvm::Value*>( reinterpret_cast<llvm::Value**>(indices), num_indices ) ) );
}

void U1_InsertInstructionAfterAnother( const LLVMValueRef src_instructuon, const LLVMValueRef instruction_for_insert )
{
	llvm::dyn_cast<llvm::Instruction>( llvm::unwrap(instruction_for_insert) )->insertAfter( llvm::dyn_cast<llvm::Instruction>( llvm::unwrap( src_instructuon ) ) );
}

void U1_FunctionAddDereferenceableAttr(const LLVMValueRef function, const uint32_t index, const uint64_t bytes)
{
	const auto f= llvm::dyn_cast<llvm::Function>(llvm::unwrap(function));

	llvm::AttrBuilder builder( f->getContext() );
	builder.addDereferenceableAttr( bytes );

	if( index == llvm::AttributeList::ReturnIndex )
		f->addRetAttrs(builder);
	else
		f->addParamAttrs(index - llvm::AttributeList::FirstArgIndex, builder);
}

bool U1_IsLegalUTF8String( const char* const start, const size_t length )
{
	auto ptr= reinterpret_cast<const llvm::UTF8*>(start);
	return llvm::isLegalUTF8String( &ptr, ptr + length ) != 0;
}

LLVMValueRef U1_ConstDataArray(LLVMTypeRef t, const char* const data, const size_t size, const size_t element_count)
{
	return llvm::wrap(
		llvm::ConstantDataArray::getRaw(
			llvm::StringRef(data, size),
			element_count,
			llvm::unwrap(t)));
}

void U1_TryToPerformReturnValueAllocationOptimization( const LLVMValueRef function )
{
	const auto function_really= llvm::dyn_cast<llvm::Function>( llvm::unwrap(function) );
	U::TryToPerformReturnValueAllocationOptimization( *function_really );
}

void U1_ReplaceMetadataNodes( const std::pair< LLVMMetadataRef, LLVMMetadataRef >* const nodes_start, const size_t num_nodes )
{
	// Put nodes into tracked container, in order to avoid their invalidation.
	std::vector< std::pair< llvm::TrackingMDNodeRef, llvm::TrackingMDNodeRef > > nodes_tracked( num_nodes );
	for( size_t i= 0; i < num_nodes; ++i )
	{
		nodes_tracked[i].first.reset( llvm::dyn_cast<llvm::MDNode>( llvm::unwrap( nodes_start[i].first ) ) );
		nodes_tracked[i].second.reset( llvm::dyn_cast<llvm::MDNode>( llvm::unwrap( nodes_start[i].second ) ) );
	}

	for( size_t i= 0; i < num_nodes; ++i )
	{
		llvm::MDNode* const node_to_delete= nodes_tracked[i].first;
		llvm::MDNode* const new_node= nodes_tracked[i].second;
		node_to_delete->replaceAllUsesWith( new_node );
		U_ASSERT( nodes_tracked[i].first == new_node ); // Should replace tracked value.
		llvm::MDNode::deleteTemporary( node_to_delete );
	}
}

} // extern "C"
