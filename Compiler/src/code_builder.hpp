#pragma once
#include <set>
#include <vector>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"

#include "code_builder_errors.hpp"
#include "code_builder_types.hpp"
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
	struct DestructiblesStorage final
	{
		void RegisterVariable( Variable variable );

		std::vector<Variable> variables;
	};

	struct LoopFrame final
	{
		llvm::BasicBlock* block_for_break= nullptr;
		llvm::BasicBlock* block_for_continue= nullptr;
		// Number of destructibles storages at stack before loop block creation.
		size_t destructibles_stack_size= 0u;
	};

	struct FunctionContext
	{
		FunctionContext(
			const Type return_type,
			bool return_value_is_mutable,
			bool return_value_is_reference,
			llvm::LLVMContext& llvm_context,
			llvm::Function* function );

		FunctionContext(const FunctionContext&)= delete;

		Type return_type;
		bool return_value_is_mutable;
		bool return_value_is_reference;

		const Variable* this_= nullptr; // null for nonclass functions or static member functions.
		const Variable* s_ret_= nullptr; // Value for assignment for "sret" functions.

		std::set<const ClassField*> uninitialized_this_fields;
		bool is_constructor_initializer_list_now= false;

		llvm::Function* const function;

		llvm::BasicBlock* const alloca_basic_block; // Block #0 in function. Contains all "alloca" instructions.
		llvm::IRBuilder<> alloca_ir_builder; // Use this builder for "alloca" instructions.

		llvm::BasicBlock* const function_basic_block; // Next block after all "alloca" instructions.
		llvm::IRBuilder<> llvm_ir_builder; // Use this builder for all instructions, except "alloca"

		std::vector<LoopFrame> loops_stack;

		// Stack for distructibles.
		// First entry is set of function arguments.
		// Each block adds new storage for it`s destructible variables.
		// Also, evaluation of some operators and expressions adds their destructibles storages.
		std::vector<DestructiblesStorage> destructibles_stack;
		llvm::BasicBlock* destructor_end_block= nullptr; // exists, if function is destructor
	};

	struct BlockBuildInfo
	{
		bool have_unconditional_return_inside= false;
		bool have_uncodnitional_break_or_continue= false;
	};

private:
	void FillGlobalNamesScope( NamesScope& global_names_scope );
	Type PrepareType( const FilePos& file_pos, const TypeName& type_name, NamesScope& names_scope );

	// Returns nullptr on fail.
	ClassPtr PrepareClass( const ClassDeclaration& class_declaration, NamesScope& names_scope );

	// Templates
	void PrepareClassTemplate( const ClassTemplateDeclaration& class_template_declaration, NamesScope& names_scope );

	// Returns nullptr in case of fail.
	NamesScope::InsertedName* GenTemplateClass(
		const ClassTemplate& class_template,
		const std::vector<IExpressionComponentPtr>& template_arguments,
		NamesScope& names_scope );

	void TryGenerateDefaultConstructor( Class& the_class, const Type& class_type );
	void TryGenerateCopyConstructor( Class& the_class, const Type& class_type );
	void TryGenerateDestructor( Class& the_class, const Type& class_type );

	void BuildCopyConstructorPart(
		llvm::Value* src, llvm::Value* dst,
		const Type& type,
		FunctionContext& function_context );

	void TryCallCopyConstructor(
		const FilePos& file_pos,
		llvm::Value* this_, llvm::Value* src,
		const ClassPtr& class_,
		FunctionContext& function_context );

	// Generates for loop from 0 to iteration_count - 1
	// Calls callback with argument - i32 with index
	void GenerateLoop(
		size_t iteration_count,
		const std::function<void(llvm::Value* counter_value)>& loop_body,
		FunctionContext& function_context);

	void CallDestructors(
		const DestructiblesStorage& destructibles_storage,
		FunctionContext& function_context );

	void CallDestructor(
		llvm::Value* ptr,
		const Type& type,
		FunctionContext& function_context );

	void CallDestructorsForLoopInnerVariables( FunctionContext& function_context );
	void CallDestructorsBeforeReturn( FunctionContext& function_context );
	void CallMembersDestructors( FunctionContext& function_context );

	void BuildNamespaceBody(
		const ProgramElements& body_elements,
		NamesScope& names_scope );

	void PrepareFunction(
		const FunctionDeclaration& func,
		bool force_prototype,
		ClassPtr base_class,
		NamesScope& scope );

	// Code build methods.
	// Methods without "noexcept" can throw exceptions.
	// Methods with "noexcept" can not throw exceptions and must catch exceptions.
	// Throw only in places, where you can not just make continue/return.

	void BuildFuncCode(
		FunctionVariable& func,
		ClassPtr base_class,
		NamesScope& parent_names_scope,
		const ProgramString& func_name,
		const FunctionArgumentsDeclaration& args,
		const Block* block, // null for prototypes.
		const StructNamedInitializer* constructor_initialization_list ) noexcept;

	void BuildConstructorInitialization(
		const Variable& this_,
		const Class& base_class,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const StructNamedInitializer& constructor_initialization_list ) noexcept;

	BlockBuildInfo BuildBlockCode(
		const Block& block,
		NamesScope& names,
		FunctionContext& function_context ) noexcept;

	// Expressions.

	Value BuildExpressionCodeAndDestroyTemporaries(
		const IExpressionComponent& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildExpressionCode(
		const IExpressionComponent& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const FilePos& file_pos,
		FunctionContext& function_context );
		
	Variable BuildLazyBinaryOperator(
		const IExpressionComponent& l_expression,
		const IExpressionComponent& r_expression,
		const BinaryOperator& binary_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildNamedOperand( const NamedOperand& named_operand, NamesScope& names, FunctionContext& function_context );
	Variable BuildNumericConstant( const NumericConstant& numeric_constant );
	Variable BuildBooleanConstant( const BooleanConstant& boolean_constant );

	Variable BuildIndexationOperator(
		const Value& value,
		const IndexationOperator& indexation_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildMemberAccessOperator(
		const Value& value,
		const MemberAccessOperator& member_access_operator,
		FunctionContext& function_context );

	Variable BuildCallOperator(
		const Value& function_value,
		const CallOperator& call_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildTempVariableConstruction(
		const Type& type,
		const CallOperator& call_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildUnaryMinus(
		const Value& value,
		const UnaryMinus& unary_minus,
		FunctionContext& function_context );

	Variable BuildLogicalNot(
		const Value& value,
		const LogicalNot& logical_not,
		FunctionContext& function_context );

	Variable BuildBitwiseNot(
		const Value& value,
		const BitwiseNot& bitwise_not,
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
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildAdditiveAssignmentOperatorCode(
		const AdditiveAssignmentOperator& additive_assignment_operator,
		NamesScope& block_names,
		FunctionContext& function_context );

	// ++ and -- operations
	void BuildDeltaOneOperatorCode(
		const IExpressionComponent& expression,
		const FilePos& file_pos,
		bool positive, // true - increment, false - decrement
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildReturnOperatorCode(
		const ReturnOperator& return_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildWhileOperatorCode(
		const WhileOperator& while_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildBreakOperatorCode(
		const BreakOperator& break_operator,
		FunctionContext& function_context ) noexcept;

	void BuildContinueOperatorCode(
		const ContinueOperator& continue_operator,
		FunctionContext& function_context ) noexcept;

	BlockBuildInfo BuildIfOperatorCode(
		const IfOperator& if_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildStaticAssert(
		const StaticAssert& static_assert_,
		NamesScope& names );

	// Functions

	FunctionVariable* GetFunctionWithExactSignature(
		const Function& function_type,
		OverloadedFunctionsSet& functions_set );

	// Throws, if can not apply function.
	void ApplyOverloadedFunction(
		OverloadedFunctionsSet& functions_set,
		const FunctionVariable& function,
		const FilePos& file_pos );

	// Throws, if can not select function.
	const FunctionVariable& GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		const std::vector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this,
		const FilePos& file_pos );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.

	llvm::Constant* ApplyInitializer(
		const Variable& variable,
		const IInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyEmptyInitializer(
		const ProgramString& variable_name,
		const FilePos& file_pos,
		const Variable& variable,
		FunctionContext& function_context );

	llvm::Constant* ApplyArrayInitializer(
		const Variable& variable,
		const ArrayInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyStructNamedInitializer(
		const Variable& variable,
		const StructNamedInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const Variable& variable,
		const CallOperator& call_operator,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyExpressionInitializer(
		const Variable& variable,
		const ExpressionInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyZeroInitializer(
		const Variable& variable,
		const ZeroInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	const NamesScope::InsertedName* ResolveName( NamesScope& names_scope, const ComplexName& complex_name );
	const NamesScope::InsertedName* ResolveName(
		NamesScope& names_scope,
		const ComplexName::Component* components, size_t component_count );

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

	Type invalid_type_;
	Type void_type_;
	Type bool_type_;

	FunctionContext* dummy_function_context_= nullptr;

	std::unique_ptr<llvm::Module> module_;
	unsigned int error_count_= 0u;
	std::vector<CodeBuilderError> errors_;
};

} // namespace CodeBuilderPrivate


using CodeBuilderPrivate::CodeBuilder;

} // namespace U
