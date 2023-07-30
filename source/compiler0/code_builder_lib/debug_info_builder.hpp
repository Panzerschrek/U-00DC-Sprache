#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DIBuilder.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/source_graph_loader.hpp"
#include "function_context.hpp"

namespace U
{

class DebugInfoBuilder
{
public:
	// LLVM Module must live longer, as this class.
	DebugInfoBuilder(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		const SourceGraph& source_graph,
		llvm::Module& llvm_module,
		bool build_debug_info );

	// Destructor triggers debug info finalization.
	~DebugInfoBuilder();

	void CreateVariableInfo(
		const Variable& variable,
		std::string_view variable_name,
		const SrcLoc& src_loc,
		FunctionContext& function_context );

	void CreateReferenceVariableInfo(
		const Variable& variable,
		std::string_view variable_name,
		const SrcLoc& src_loc,
		FunctionContext& function_context );

	void CreateFunctionInfo( const FunctionVariable& func_variable, std::string_view function_name );

	void SetCurrentLocation( const SrcLoc& src_loc, FunctionContext& function_context );

	void StartBlock( const SrcLoc& src_loc, FunctionContext& function_context );
	void EndBlock( FunctionContext& function_context );

private:
	llvm::DIFile* GetDIFile( const SrcLoc& src_loc );
	llvm::DIFile* GetRootDIFile();

	llvm::DIType* CreateDIType( const Type& type );
	llvm::DIType* CreateDIType( const FundamentalType& type );
	llvm::DICompositeType* CreateDIType( const ArrayType& type );
	llvm::DICompositeType* CreateDIType( const TupleType& type );
	llvm::DIDerivedType* CreateDIType( const RawPointerType& type );
	llvm::DIDerivedType* CreateDIType( const FunctionPointerType& type );
	llvm::DICompositeType* CreateDIType( ClassPtr type );
	llvm::DICompositeType* CreateDIType( EnumPtr type );
	llvm::DISubroutineType* CreateDIFunctionType( const FunctionType& type );

private:
	llvm::LLVMContext& llvm_context_;
	const llvm::DataLayout data_layout_;

	std::vector<llvm::DIFile*> source_file_entries_; // Entry for each file in sources graph.

	// Debug info builder, compile unit, types cache - unique only for current file.
	std::unique_ptr<llvm::DIBuilder> builder_;
	llvm::DICompileUnit* compile_unit_= nullptr;

	// Build debug info for classes and enums once and put it to cache.
	std::unordered_map<ClassPtr, llvm::DICompositeType*> classes_di_cache_;
	std::unordered_map<EnumPtr, llvm::DICompositeType*> enums_di_cache_;
};

} // namespace U
