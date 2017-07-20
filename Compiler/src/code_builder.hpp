#pragma once
#include <vector>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"

#include "code_builder_errors.hpp"
#include "code_builder_types.hpp"
#include "inverse_polish_notation.hpp"
#include "syntax_elements.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

class CodeBuilder final
{
public:
	CodeBuilder();
	~CodeBuilder();

	struct BuildResult
	{
		std::vector<CodeBuilderError> errors;
		std::unique_ptr<llvm::Module> module;
	};

	BuildResult BuildProgram( const ProgramElements& program_elements );

private:
	struct FunctionContext
	{
		FunctionContext(
			const Type return_type,
			bool return_value_is_mutable,
			bool return_value_is_reference,
			llvm::LLVMContext& llvm_context,
			llvm::Function* function );

		Type return_type;
		bool return_value_is_mutable;
		bool return_value_is_reference;

		llvm::Function* const function;
		llvm::BasicBlock* const function_basic_block;
		llvm::IRBuilder<> llvm_ir_builder;

		llvm::BasicBlock* block_for_break;
		llvm::BasicBlock* block_for_continue;
	};

	struct BlockBuildInfo
	{
		bool have_unconditional_return_inside= false;
		bool have_uncodnitional_break_or_continue= false;
	};

private:
	Type PrepareType( const FilePos& file_pos, const TypeName& type_name );
	ClassPtr PrepareClass( const ClassDeclaration& class_declaration );

	// Code build methods.
	// Methods without "noexcept" can throw exceptions.
	// Methods with "noexcept" can not throw exceptions and must catch exceptions.
	// Throw only in places, where you can not just make continue/return.

	void BuildFuncCode(
		Variable& func,
		const ProgramString& func_name,
		const FunctionArgumentsDeclaration& args,
		const Block* block ) noexcept;

	BlockBuildInfo BuildBlockCode(
		const Block& block,
		const NamesScope& names,
		FunctionContext& function_context ) noexcept;

	// Expressions.

	Variable BuildExpressionCode(
		const BinaryOperatorsChain& expression,
		const NamesScope& names,
		FunctionContext& function_context );

	Variable BuildExpressionCode_r(
		const InversePolishNotation& ipn,
		unsigned int ipn_index,
		const NamesScope& names,
		FunctionContext& function_context );

	Variable BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperator binary_operator,
		const FilePos& file_pos,
		FunctionContext& function_context );

	Variable BuildNamedOperand( const NamedOperand& named_operand, const NamesScope& names );
	Variable BuildNumericConstant( const NumericConstant& numeric_constant );
	Variable BuildBooleanConstant( const BooleanConstant& boolean_constant );

	Variable BuildIndexationOperator(
		const Variable& variable,
		const IndexationOperator& indexation_operator,
		const NamesScope& names,
		FunctionContext& function_context );

	Variable BuildMemberAccessOperator(
		const Variable& variable,
		const MemberAccessOperator& member_access_operator,
		FunctionContext& function_context );

	Variable BuildCallOperator(
		const Variable& function_variable,
		const CallOperator& call_operator,
		const NamesScope& names,
		FunctionContext& function_context );

	Variable BuildUnaryMinus(
		const Variable& variable,
		const UnaryMinus& unary_minus,
		FunctionContext& function_context );

	// Block elements

	void BuildVariablesDeclarationCode(
		const VariablesDeclaration& variables_declaration,
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildAutoVariableDeclarationCode(
		const AutoVariableDeclaration& auto_variable_declaration,
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildAssignmentOperatorCode(
		const AssignmentOperator& assignment_operator,
		const NamesScope& block_names,
		FunctionContext& function_context );

	void BuildReturnOperatorCode(
		const ReturnOperator& return_operator,
		const NamesScope& names,
		FunctionContext& function_context );

	void BuildWhileOperatorCode(
		const WhileOperator& while_operator,
		const NamesScope& names,
		FunctionContext& function_context );

	void BuildBreakOperatorCode(
		const BreakOperator& break_operator,
		FunctionContext& function_context ) noexcept;

	void BuildContinueOperatorCode(
		const ContinueOperator& continue_operator,
		FunctionContext& function_context ) noexcept;

	BlockBuildInfo BuildIfOperatorCode(
		const IfOperator& if_operator,
		const NamesScope& names,
		FunctionContext& function_context );

	// Functions

	Variable* GetFunctionWithExactSignature(
		const Function& function_type,
		OverloadedFunctionsSet& functions_set );

	// Throws, if can not apply function.
	void ApplyOverloadedFunction(
		OverloadedFunctionsSet& functions_set,
		const Variable& function,
		const FilePos& file_pos );

	// Throws, if can not select function.
	const Variable& GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		const std::vector<Function::Arg>& actual_args,
		const FilePos& file_pos );

	// Initializers.

	void ApplyInitializer_r(
		const Variable& variable,
		const IInitializer* initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyArrayInitializer(
		const Variable& variable,
		const ArrayInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyStructNamedInitializer(
		const Variable& variable,
		const StructNamedInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyConstructorInitializer(
		const Variable& variable,
		const ConstructorInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyExpressionInitializer(
		const Variable& variable,
		const ExpressionInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyZeroInitializer(
		const Variable& variable,
		const ZeroInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	static U_FundamentalType GetNumericConstantType( const NumericConstant& number );

	llvm::Type* GetFundamentalLLVMType( U_FundamentalType fundmantal_type );

	// If variable is on stack, creates move to rigister instruction.
	// If variable already in register - does nothing.
	llvm::Value* CreateMoveToLLVMRegisterInstruction( const Variable& variable, FunctionContext& function_context );

private:
	llvm::LLVMContext& llvm_context_;

	struct
	{
		llvm::IntegerType*  i8;
		llvm::IntegerType*  u8;
		llvm::IntegerType* i16;
		llvm::IntegerType* u16;
		llvm::IntegerType* i32;
		llvm::IntegerType* u32;
		llvm::IntegerType* i64;
		llvm::IntegerType* u64;

		llvm::Type* f32;
		llvm::Type* f64;

		llvm::Type* void_;
		llvm::Type* invalid_type_;
		llvm::IntegerType* bool_;
	} fundamental_llvm_types_;

	std::unique_ptr<llvm::Module> module_;
	unsigned int error_count_= 0u;
	std::vector<CodeBuilderError> errors_;

	NamesScope global_names_;
};

} // namespace CodeBuilderPrivate


using CodeBuilderPrivate::CodeBuilder;

} // namespace U
