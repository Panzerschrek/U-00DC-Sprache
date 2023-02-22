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
	DebugInfoBuilder(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		const SourceGraph& source_graph,
		llvm::Module& llvm_module,
		bool build_debug_info );

	void Finalize();

	llvm::DIFile* GetDIFile(size_t file_index);

	void CreateVariableDebugInfo(
		const Variable& variable,
		const std::string& variable_name,
		const SrcLoc& src_loc,
		FunctionContext& function_context );

	void CreateReferenceVariableDebugInfo(
		const Variable& variable,
		const std::string& variable_name,
		const SrcLoc& src_loc,
		FunctionContext& function_context );

	void CreateFunctionDebugInfo(
		const FunctionVariable& func_variable,
		const std::string& function_name );

	void SetCurrentDebugLocation(
		const SrcLoc& src_loc,
		FunctionContext& function_context );

	void DebugInfoStartBlock( const SrcLoc& src_loc, FunctionContext& function_context );
	void DebugInfoEndBlock( FunctionContext& function_context );

	llvm::DIType* CreateDIType( const Type& type );
	llvm::DIType* CreateDIType( const FundamentalType& type );
	llvm::DICompositeType* CreateDIType( const ArrayType& type );
	llvm::DICompositeType* CreateDIType( const TupleType& type );
	llvm::DISubroutineType* CreateDIType( const FunctionType& type );
	llvm::DIDerivedType* CreateDIType( const RawPointerType& type );
	llvm::DIDerivedType* CreateDIType( const FunctionPointerType& type );
	llvm::DICompositeType* CreateDIType( ClassPtr type );
	llvm::DICompositeType* CreateDIType( EnumPtr type );

private:
	llvm::LLVMContext& llvm_context_;
	const llvm::DataLayout data_layout_;
	const bool build_debug_info_;

	// Debug info.
	struct
	{
		std::vector<llvm::DIFile*> source_file_entries; // Entry for each file in sources graph.

		// Debug info builder, compile unit, types cache - unique only for current file.
		std::unique_ptr<llvm::DIBuilder> builder;
		llvm::DICompileUnit* compile_unit= nullptr;

		// Build debug info for classes and enums once and put it to cache.
		std::unordered_map<ClassPtr, llvm::DICompositeType*> classes_di_cache;
		std::unordered_map<EnumPtr, llvm::DICompositeType*> enums_di_cache;
	} debug_info_;
};

} // namespace U
