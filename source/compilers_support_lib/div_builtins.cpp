#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/Transforms/Utils/IntegerDivision.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "div_builtins.hpp"

namespace U
{

namespace
{

llvm::Function* CreateFunction( llvm::Module& module, llvm::FunctionType* const function_type, const llvm::Twine& name )
{
	const auto function= llvm::Function::Create( function_type, llvm::GlobalValue::ExternalLinkage, name, module );

	// Create comdat. It's necessary in order to avoid redefinition errors, since built-ins are generated in all modules/object files.
	llvm::Comdat* const comdat= module.getOrInsertComdat( function->getName() );
	comdat->setSelectionKind( llvm::Comdat::Any );
	function->setComdat( comdat );

	// Set hidden visibility in order to avoid exporting built-ins in shared libraries.
	function->setVisibility( llvm::GlobalValue::HiddenVisibility );

	return function;
}

void GenerateDiv32BuiltIns( llvm::Module& module )
{
	llvm::LLVMContext& context= module.getContext();

	llvm::Type* const i32= llvm::Type::getInt32Ty( context );
	std::array<llvm::Type*, 2> const i32_args{ i32, i32 };
	llvm::FunctionType* const function_type= llvm::FunctionType::get( i32, i32_args, false );

	{
		const auto function= CreateFunction( module, function_type, "__udivsi3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto div= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::UDiv, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, div, bb );

		llvm::expandDivision( div );
	}
	{
		const auto function= CreateFunction( module, function_type, "__divsi3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto div= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::SDiv, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, div, bb );

		llvm::expandDivision( div );
	}
	{
		const auto function= CreateFunction( module, function_type, "__umodsi3" );
		const auto bb= llvm::BasicBlock::Create(context, "", function );
		const auto rem= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::URem, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, rem, bb );

		llvm::expandRemainder( rem );
	}
	{
		const auto function= CreateFunction( module, function_type, "__modsi3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto rem= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::SRem, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, rem, bb );

		llvm::expandRemainder( rem );
	}
}

void GenerateDiv64BuiltIns( llvm::Module& module )
{
	llvm::LLVMContext& context= module.getContext();

	llvm::Type* const i64= llvm::Type::getInt64Ty( context );
	std::array<llvm::Type*, 2> const i64_args{ i64, i64 };
	llvm::FunctionType* const function_type= llvm::FunctionType::get( i64, i64_args, false );

	{
		const auto function= CreateFunction( module, function_type, "__udivdi3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto div= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::UDiv, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, div, bb );

		llvm::expandDivision( div );
	}
	{
		const auto function= CreateFunction( module, function_type, "__divdi3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto div= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::SDiv, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, div, bb );

		llvm::expandDivision( div );
	}
	{
		const auto function= CreateFunction( module, function_type, "__umoddi3" );
		const auto bb= llvm::BasicBlock::Create(context, "", function );
		const auto rem= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::URem, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, rem, bb );

		llvm::expandRemainder( rem );
	}
	{
		const auto function= CreateFunction( module, function_type, "__moddi3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto rem= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::SRem, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, rem, bb );

		llvm::expandRemainder( rem );
	}
}

void GenerateDiv128BuiltIns( llvm::Module& module )
{
	llvm::LLVMContext& context= module.getContext();

	llvm::Type* const i128= llvm::Type::getInt128Ty( context );
	std::array<llvm::Type*, 2> const i128_args{ i128, i128 };
	llvm::FunctionType* const function_type= llvm::FunctionType::get( i128, i128_args, false );

	{
		const auto function= CreateFunction( module, function_type, "__udivti3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto div= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::UDiv, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, div, bb );

		llvm::expandDivision( div );
	}
	{
		const auto function= CreateFunction( module, function_type, "__divti3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto div= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::SDiv, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, div, bb );

		llvm::expandDivision( div );
	}
	{
		const auto function= CreateFunction( module, function_type, "__umodti3" );
		const auto bb= llvm::BasicBlock::Create(context, "", function );
		const auto rem= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::URem, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, rem, bb );

		llvm::expandRemainder( rem );
	}
	{
		const auto function= CreateFunction( module, function_type, "__modti3" );
		const auto bb= llvm::BasicBlock::Create( context, "", function );
		const auto rem= llvm::BinaryOperator::Create( llvm::Instruction::BinaryOps::SRem, function->getArg(0), function->getArg(1), "", bb );
		llvm::ReturnInst::Create( context, rem, bb );

		llvm::expandRemainder( rem );
	}
}

} // namespace

void GenerateDivBuiltIns( llvm::Module& module )
{
	GenerateDiv32BuiltIns( module );
	GenerateDiv64BuiltIns( module );
	GenerateDiv128BuiltIns( module );
}

bool IsDivBuiltInLikeFunctionName( const llvm::StringRef name )
{
	// Do not list here exact function names. Instead perform prefix checks. Usually it's enough.
	return
		name.startswith( "__div" ) ||
		name.startswith( "__mod" ) ||
		name.startswith( "__udiv" ) ||
		name.startswith( "__umod" );
}

} // namespace U
