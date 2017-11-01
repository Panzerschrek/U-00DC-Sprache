#pragma once
#include <set>
#include <vector>
#include <unordered_map>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"

#include "code_builder_errors.hpp"
#include "code_builder_types.hpp"
#include "i_code_builder.hpp"
#include "syntax_elements.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

class CodeBuilder final : public ICodeBuilder
{
public:
	CodeBuilder();
	virtual ~CodeBuilder() override;

	virtual BuildResult BuildProgram( const SourceGraph& source_graph ) override;

private:
	typedef std::unordered_map< ClassProxyPtr, std::shared_ptr<Class> > ClassTable;
	struct BuildResultInternal
	{
		std::unique_ptr<NamesScope> names_map;
		std::unique_ptr<ClassTable> class_table;
	};

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
	BuildResultInternal BuildProgramInternal( const SourceGraph& source_graph, size_t node_index );

	void MergeNameScopes( NamesScope& dst, const NamesScope& src, ClassTable& dst_class_table );

	void CopyClass(
		const FilePos& file_pos, // FilePos or original class.
		const ClassProxyPtr& src_class,
		ClassTable& dst_class_table,
		NamesScope& dst_namespace );
	void SetCurrentClassTable( ClassTable& table );

	void FillGlobalNamesScope( NamesScope& global_names_scope );
	Type PrepareType( const FilePos& file_pos, const Synt::TypeName& type_name, NamesScope& names_scope );

	// Returns nullptr on fail.
	ClassProxyPtr PrepareClass(
		const Synt::Class& class_declaration,
		const Synt::ComplexName& class_complex_name,
		NamesScope& names_scope,
		bool force_forward_declaration= false );

	// Templates
	void PrepareTypeTemplate( const Synt::TemplateBase& type_template_declaration, NamesScope& names_scope );

	void PrepareTemplateSignatureParameter(
		const FilePos& file_pos,
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope,
		const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	// Resolve as deep, as can, but does not instantiate last component, if it is template.
	const NamesScope::InsertedName* ResolveForTemplateSignatureParameter(
		const FilePos& file_pos,
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope );

	// Returns true, if all ok.
	bool DuduceTemplateArguments(
		const TypeTemplatePtr& type_template_ptr,
		const TemplateParameter& template_parameter,
		const Synt::ComplexName& signature_parameter,
		const FilePos& signature_parameter_file_pos,
		DeducibleTemplateParameters& deducible_template_parameters,
		NamesScope& names_scope );

	// Returns nullptr in case of fail.
	NamesScope::InsertedName* GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatePtr& type_template_ptr,
		const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
		NamesScope& template_names_scope,
		NamesScope& arguments_names_scope );

	NamesScope& PushTemplateArgumentsSpace();
	void PopTemplateArgumentsSpace();
	bool NameShadowsTemplateArgument( const ProgramString& name, NamesScope& names_scope );

	TemplateDependentType GetNextTemplateDependentType();
	bool TypeIsValidForTemplateVariableArgument( const Type& type );

	// Removes llvm-functions and functions of subclasses.
	// Warning! Class must be not used after call of this function!
	void RemoveTempClassLLVMValues( Class& class_ );

	void ReportAboutIncompleteMembersOfTemplateClass( const FilePos& file_pos, Class& class_ );

	// Constructors/destructors
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
		const ClassProxyPtr& class_proxy,
		FunctionContext& function_context );

	// Generates for loop from 0 to iteration_count - 1
	// Calls callback with argument - i32 with index
	// TODO - allow 64bit indeces?
	void GenerateLoop(
		SizeType iteration_count,
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
		const Synt::ProgramElements& body_elements,
		NamesScope& names_scope );

	struct PrepareFunctionResult
	{
		const Synt::Function* func_syntax_element= nullptr;
		OverloadedFunctionsSet* functions_set= nullptr;
		size_t function_index= 0u;
	};

	PrepareFunctionResult PrepareFunction(
		const Synt::Function& func,
		bool force_prototype,
		ClassProxyPtr base_class,
		NamesScope& scope );

	void BuildFuncCode(
		FunctionVariable& func,
		ClassProxyPtr base_class,
		NamesScope& parent_names_scope,
		const ProgramString& func_name,
		const Synt::FunctionArgumentsDeclaration& args,
		const Synt::Block* block, // null for prototypes.
		const Synt::StructNamedInitializer* constructor_initialization_list );

	void BuildConstructorInitialization(
		const Variable& this_,
		const Class& base_class,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const Synt::StructNamedInitializer& constructor_initialization_list );

	BlockBuildInfo BuildBlockCode(
		const Synt::Block& block,
		NamesScope& names,
		FunctionContext& function_context );

	// Expressions.

	Value BuildExpressionCodeAndDestroyTemporaries(
		const Synt::IExpressionComponent& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildExpressionCode(
		const Synt::IExpressionComponent& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		Synt::BinaryOperatorType binary_operator,
		const FilePos& file_pos,
		FunctionContext& function_context );
		
	Value BuildLazyBinaryOperator(
		const Synt::IExpressionComponent& l_expression,
		const Synt::IExpressionComponent& r_expression,
		const Synt::BinaryOperator& binary_operator,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildNamedOperand( const Synt::NamedOperand& named_operand, NamesScope& names, FunctionContext& function_context );
	Value BuildNumericConstant( const Synt::NumericConstant& numeric_constant );
	Variable BuildBooleanConstant( const Synt::BooleanConstant& boolean_constant );

	Value BuildIndexationOperator(
		const Value& value,
		const Synt::IndexationOperator& indexation_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildMemberAccessOperator(
		const Value& value,
		const Synt::MemberAccessOperator& member_access_operator,
		FunctionContext& function_context );

	Value BuildCallOperator(
		const Value& function_value,
		const Synt::CallOperator& call_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildTempVariableConstruction(
		const Type& type,
		const Synt::CallOperator& call_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildUnaryMinus(
		const Value& value,
		const Synt::UnaryMinus& unary_minus,
		FunctionContext& function_context );

	Value BuildLogicalNot(
		const Value& value,
		const Synt::LogicalNot& logical_not,
		FunctionContext& function_context );

	Value BuildBitwiseNot(
		const Value& value,
		const Synt::BitwiseNot& bitwise_not,
		FunctionContext& function_context );

	// Block elements

	void BuildVariablesDeclarationCode(
		const Synt::VariablesDeclaration& variables_declaration,
		NamesScope& block_names,
		FunctionContext& function_context,
		bool global= false );

	void BuildAutoVariableDeclarationCode(
		const Synt::AutoVariableDeclaration& auto_variable_declaration,
		NamesScope& block_names,
		FunctionContext& function_context,
		bool global= false );

	void BuildAssignmentOperatorCode(
		const Synt::AssignmentOperator& assignment_operator,
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildAdditiveAssignmentOperatorCode(
		const Synt::AdditiveAssignmentOperator& additive_assignment_operator,
		NamesScope& block_names,
		FunctionContext& function_context );

	// ++ and -- operations
	void BuildDeltaOneOperatorCode(
		const Synt::IExpressionComponent& expression,
		const FilePos& file_pos,
		bool positive, // true - increment, false - decrement
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildReturnOperatorCode(
		const Synt::ReturnOperator& return_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildWhileOperatorCode(
		const Synt::WhileOperator& while_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildBreakOperatorCode(
		const Synt::BreakOperator& break_operator,
		FunctionContext& function_context );

	void BuildContinueOperatorCode(
		const Synt::ContinueOperator& continue_operator,
		FunctionContext& function_context );

	BlockBuildInfo BuildIfOperatorCode(
		const Synt::IfOperator& if_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildStaticAssert(
		const Synt::StaticAssert& static_assert_,
		NamesScope& names );

	void BuildHalt( const Synt::Halt& halt, FunctionContext& function_context );

	void BuildTypedef(
		const Synt::Typedef& typedef_,
		NamesScope& names );

	// Functions

	FunctionVariable* GetFunctionWithExactSignature(
		const Function& function_type,
		OverloadedFunctionsSet& functions_set );

	// Returns "false" on error.
	bool ApplyOverloadedFunction(
		OverloadedFunctionsSet& functions_set,
		const FunctionVariable& function,
		const FilePos& file_pos );

	const FunctionVariable* GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		const std::vector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this,
		const FilePos& file_pos );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.

	llvm::Constant* ApplyInitializer(
		const Variable& variable,
		const Synt::IInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyEmptyInitializer(
		const ProgramString& variable_name,
		const FilePos& file_pos,
		const Variable& variable,
		FunctionContext& function_context );

	llvm::Constant* ApplyArrayInitializer(
		const Variable& variable,
		const Synt::ArrayInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyStructNamedInitializer(
		const Variable& variable,
		const Synt::StructNamedInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const Variable& variable,
		const Synt::CallOperator& call_operator,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyExpressionInitializer(
		const Variable& variable,
		const Synt::ExpressionInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyZeroInitializer(
		const Variable& variable,
		const Synt::ZeroInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	// Name resolving.

	// PreResolve function.
	// Returns name (if found) and parent namespace, if name found not in cache.
	typedef
		std::function<
			std::pair<const NamesScope::InsertedName*, NamesScope*>(
				NamesScope& names_scope,
				const Synt::ComplexName::Component* components,
				size_t component_count,
				size_t& out_skip_components ) > PreResolveFunc;

	void PushCacheFillResolveHandler( ResolvingCache& resolving_cache, NamesScope& start_namespace );
	void PushCacheGetResolveHandelr( const ResolvingCache& resolving_cache );
	void PopResolveHandler();

	const NamesScope::InsertedName* ResolveName( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name );

	const NamesScope::InsertedName* ResolveName(
		const FilePos& file_pos,
		NamesScope& names_scope,
		const Synt::ComplexName::Component* components,
		size_t component_count );

	const NamesScope::InsertedName* PreResolve(
		NamesScope& names_scope,
		const Synt::ComplexName::Component* components,
		size_t component_count,
		size_t& out_skip_components );

	// Finds namespace, where are name. Do not search in classes (returns class itself)
	std::pair<const NamesScope::InsertedName*, NamesScope*> PreResolveDefault(
		NamesScope& names_scope,
		const Synt::ComplexName::Component* components,
		size_t component_count,
		size_t& out_skip_components );

	static U_FundamentalType GetNumericConstantType( const Synt::NumericConstant& number );

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

	llvm::Function* halt_func_= nullptr;

	Type invalid_type_;
	Type void_type_;
	Type bool_type_;

	FunctionContext* dummy_function_context_= nullptr;

	std::unique_ptr<llvm::Module> module_;
	unsigned int error_count_= 0u;
	std::vector<CodeBuilderError> errors_;

	std::unordered_map< size_t, BuildResultInternal > compiled_sources_cache_;
	ClassTable* current_class_table_= nullptr;

	// Cache needs for generating same classes as template instantiation result in different source files.
	// We can use same classes in different files, because template classes are logically unchangeable after instantiation.
	// Unchangeable they are because incomplete template classes ( or classes inside template classes, etc. ) currently forbidden.
	TemplateClassesCache template_classes_cache_;

	std::vector<std::unique_ptr<PreResolveFunc>> resolving_funcs_stack_;
	size_t next_template_dependent_type_index_= 1u;
};

using MutabilityModifier= Synt::MutabilityModifier;
using ReferenceModifier= Synt::ReferenceModifier;

} // namespace CodeBuilderPrivate

using CodeBuilderPrivate::CodeBuilder;

} // namespace U
