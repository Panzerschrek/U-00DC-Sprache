#pragma once
#include <list>
#include <set>
#include <vector>
#include <unordered_map>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"

#include "code_builder_errors.hpp"
#include "code_builder_types.hpp"
#include "constexpr_function_evaluator.hpp"
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

	using OverloadingResolutionCache=
		std::unordered_map< const Synt::SyntaxElementBase*, boost::optional<FunctionVariable> >;

	struct FunctionContext;

	// Usage - create this struct on stack. FunctionContext::stack_variables_stack will be controlled automatically.
	// But you still need call "CallDestructors" manually.
	struct StackVariablesStorage final
	{
		StackVariablesStorage( FunctionContext& function_context );
		~StackVariablesStorage();

		void RegisterVariable( const StoredVariablePtr& variable );

		FunctionContext& function_context;
		std::vector<StoredVariablePtr> variables;
	};

	struct LoopFrame final
	{
		llvm::BasicBlock* block_for_break= nullptr;
		llvm::BasicBlock* block_for_continue= nullptr;
		// Number of stack variable storages at stack before loop block creation.
		size_t stack_variables_stack_size= 0u;
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

		// For reference-returned functions - references of returning reference.
		// For value-returned functions - references inside value.
		std::unordered_set<StoredVariablePtr> allowed_for_returning_references;

		const Variable* this_= nullptr; // null for nonclass functions or static member functions.
		llvm::Value* s_ret_= nullptr; // Value for assignment for "sret" functions.

		std::set<const ClassField*> uninitialized_this_fields;
		bool base_initialized= false;
		bool whole_this_is_unavailable= false; // May be true in constructor initializer list, in body of constructors and destructors of abstract classes.
		bool have_non_constexpr_operations_inside= false; // While building code, may set to "true".

		llvm::Function* const function;

		llvm::BasicBlock* const alloca_basic_block; // Block #0 in function. Contains all "alloca" instructions.
		llvm::IRBuilder<> alloca_ir_builder; // Use this builder for "alloca" instructions.

		llvm::BasicBlock* const function_basic_block; // Next block after all "alloca" instructions.
		llvm::IRBuilder<> llvm_ir_builder; // Use this builder for all instructions, except "alloca"

		std::vector<LoopFrame> loops_stack;
		bool is_in_unsafe_block= false;

		// Stack for stack variables.
		// First entry is set of function arguments.
		// Each block adds new storage for it`s variables.
		// Also, evaluation of some operators and expressions adds their variables storages.
		// Do not push/pop to t his stack manually!
		std::vector<StackVariablesStorage*> stack_variables_stack;
		VariablesState variables_state;

		OverloadingResolutionCache overloading_resolutin_cache;

		llvm::BasicBlock* destructor_end_block= nullptr; // exists, if function is destructor
	};

	struct BlockBuildInfo
	{
		bool have_unconditional_return_inside= false;
		bool have_uncodnitional_break_or_continue= false;
	};

	struct TemplateTypeGenerationResult
	{
		TypeTemplatePtr type_template;
		NamesScope::InsertedName* type= nullptr;
		std::vector<DeducedTemplateParameter> deduced_template_parameters;
	};

	struct GlobalThing // TODO - move struct out of here
	{
		const void* thing_ptr= nullptr;
		ProgramString name;
		FilePos file_pos;
		TypeCompleteness completeness= TypeCompleteness::Incomplete;

		GlobalThing( const void* const in_thing_ptr, const ProgramString& in_name, const FilePos& in_file_pos, const TypeCompleteness in_completeness )
			: thing_ptr(in_thing_ptr), name(in_name), file_pos(in_file_pos), completeness(in_completeness)
		{}
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
	Type PrepareType( const Synt::ITypeNamePtr& type_name, NamesScope& names_scope );

	llvm::FunctionType* GetLLVMFunctionType( const Function& function_type );

	// Virtual stuff
	void ProcessClassParentsVirtualTables( Class& the_class );
	void TryGenerateDestructorPrototypeForPolymorphClass( Class& the_class, const Type& class_type );
	void ProcessClassVirtualFunction( Class& the_class, FunctionVariable& function );
	void PrepareClassVirtualTableType( Class& the_class );
	void BuildClassVirtualTables_r( Class& the_class, const Type& class_type, const std::vector< ClassProxyPtr >& dst_class_path, llvm::Value* dst_class_ptr_null_based );
	void BuildClassVirtualTables( Class& the_class, const Type& class_type ); // Returns type of vtable pointer or nullptr.

	std::pair<Variable, llvm::Value*> TryFetchVirtualFunction( const Variable& this_, const FunctionVariable& function, FunctionContext& function_context );

	void SetupVirtualTablePointers_r(
		llvm::Value* this_,
		const std::vector< ClassProxyPtr >& class_path,
		const std::map< std::vector< ClassProxyPtr >, llvm::GlobalVariable* > virtual_tables,
		FunctionContext& function_context );

	void SetupVirtualTablePointers(
		llvm::Value* this_,
		const Class& the_class,
		FunctionContext& function_context );

	// Templates
	void PrepareTypeTemplate(
		const Synt::TypeTemplateBase& type_template_declaration,
		TypeTemplatesSet& type_templates_set,
		NamesScope& names_scope );

	void PrepareFunctionTemplate(
		const Synt::FunctionTemplate& function_template_declaration,
		OverloadedFunctionsSet& functions_set,
		NamesScope& names_scope,
		const ClassProxyPtr& base_class );

	void ProcessTemplateArgs(
		const std::vector<Synt::TemplateBase::Arg>& args,
		NamesScope& names_scope,
		const FilePos& file_pos,
		std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		NamesScope& template_parameters_namespace,
		std::vector<bool>& template_parameters_usage_flags );

	void PrepareTemplateSignatureParameter(
		const FilePos& file_pos,
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope,
		const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	void PrepareTemplateSignatureParameter(
		const Synt::IExpressionComponentPtr& template_parameter,
		NamesScope& names_scope,
		const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	void PrepareTemplateSignatureParameter(
		const Synt::ITypeName& template_parameter,
		NamesScope& names_scope,
		const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	// Resolve as deep, as can, but does not instantiate last component, if it is template.
	NamesScope::InsertedName* ResolveForTemplateSignatureParameter(
		const FilePos& file_pos,
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope );

	// Returns deduced parameter, if all ok.
	DeducedTemplateParameter DeduceTemplateArguments(
		const TemplateBase& template_,
		const TemplateParameter& template_parameter,
		const Synt::ComplexName& signature_parameter,
		const FilePos& signature_parameter_file_pos,
		DeducibleTemplateParameters& deducible_template_parameters,
		NamesScope& names_scope );

	DeducedTemplateParameter DeduceTemplateArguments(
		const TemplateBase& template_,
		const TemplateParameter& template_parameter,
		const Synt::IExpressionComponent& signature_parameter,
		const FilePos& signature_parameter_file_pos,
		DeducibleTemplateParameters& deducible_template_parameters,
		NamesScope& names_scope );

	DeducedTemplateParameter DeduceTemplateArguments(
		const TemplateBase& template_,
		const TemplateParameter& template_parameter,
		const Synt::ITypeName& signature_parameter,
		const FilePos& signature_parameter_file_pos,
		DeducibleTemplateParameters& deducible_template_parameters,
		NamesScope& names_scope );

	// Returns nullptr in case of fail.
	NamesScope::InsertedName* GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatesSet& type_templates_set,
		const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
		NamesScope& arguments_names_scope );

	// Returns nullptr in case of fail.
	TemplateTypeGenerationResult GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatePtr& type_template_ptr,
		const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
		NamesScope& arguments_names_scope,
		bool skip_type_generation );

	const FunctionVariable* GenTemplateFunction(
		const FilePos& file_pos,
		const FunctionTemplatePtr& function_template_ptr,
		const std::vector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this,
		bool skip_arguments= false );

	NamesScope::InsertedName* GenTemplateFunctionsUsingTemplateParameters(
		const FilePos& file_pos,
		const std::vector<FunctionTemplatePtr>& function_templates,
		const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
		NamesScope& arguments_names_scope );

	bool NameShadowsTemplateArgument( const ProgramString& name, NamesScope& names_scope );

	bool TypeIsValidForTemplateVariableArgument( const Type& type );


	void ReportAboutIncompleteMembersOfTemplateClass( const FilePos& file_pos, Class& class_ );

	// Constructors/destructors
	void TryGenerateDefaultConstructor( Class& the_class, const Type& class_type );
	void TryGenerateCopyConstructor( Class& the_class, const Type& class_type );
	FunctionVariable GenerateDestructorPrototype( Class& the_class, const Type& class_type );
	void GenerateDestructorBody( Class& the_class, const Type& class_type, FunctionVariable& destructor_function );
	void TryGenerateDestructor( Class& the_class, const Type& class_type );
	void TryGenerateCopyAssignmentOperator( Class& the_class, const Type& class_type );

	void BuildCopyConstructorPart(
		llvm::Value* src, llvm::Value* dst,
		const Type& type,
		FunctionContext& function_context );

	void BuildCopyAssignmentOperatorPart(
		llvm::Value* src, llvm::Value* dst,
		const Type& type,
		FunctionContext& function_context );

	void CopyBytes(
		llvm::Value* src, llvm::Value* dst,
		const Type& type,
		FunctionContext& function_context );

	void MoveConstantToMemory(
		llvm::Value* ptr, llvm::Constant* constant,
		FunctionContext& function_context );

	void TryCallCopyConstructor(
		const FilePos& file_pos,
		llvm::Value* this_, llvm::Value* src,
		const ClassProxyPtr& class_proxy,
		FunctionContext& function_context );

	bool IsDefaultConstructor( const Function& function_type, const Type& base_class );
	bool IsCopyConstructor( const Function& function_type, const Type& base_class );
	bool IsCopyAssignmentOperator( const Function& function_type, const Type& base_class );

	// Generates for loop from 0 to iteration_count - 1
	// Calls callback with argument - size_type with index
	void GenerateLoop(
		SizeType iteration_count,
		const std::function<void(llvm::Value* counter_value)>& loop_body,
		FunctionContext& function_context);

	// Store counter of destroyed references to this variable.
	typedef std::unordered_map<StoredVariablePtr, size_t> DestroyedVariableReferencesCount;

	void CallDestructorsImpl(
		const StackVariablesStorage& stack_variables_storage,
		FunctionContext& function_context,
		DestroyedVariableReferencesCount& destroyed_variable_references,
		const FilePos& file_pos );

	void CallDestructors(
		const StackVariablesStorage& stack_variables_storage,
		FunctionContext& function_context,
		const FilePos& file_pos );

	void CallDestructor(
		llvm::Value* ptr,
		const Type& type,
		FunctionContext& function_context,
		const FilePos& file_pos );

	void CallDestructorsForLoopInnerVariables( FunctionContext& function_context, const FilePos& file_pos );
	void CallDestructorsBeforeReturn( FunctionContext& function_context, const FilePos& file_pos );
	void CallMembersDestructors( FunctionContext& function_context, const FilePos& file_pos );

	void PrepareFunction(
		NamesScope& names_scope,
		const ClassProxyPtr& base_class,
		OverloadedFunctionsSet& functions_set,
		const Synt::Function& function_declaration,
		bool is_out_of_line_function );

	void CheckOverloadedOperator(
		const ClassProxyPtr& base_class,
		const Function& func_type,
		OverloadedOperator overloaded_operator,
		const FilePos& file_pos );

	void BuildFuncCode(
		FunctionVariable& func,
		const ClassProxyPtr& base_class,
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

	// Returns Value, if overloaded operator selected or if arguments are template dependent or argumens are error values.
	// Returns boost::none, if all ok, but there is no overloaded operator.
	// In success call of overloaded operator arguments evaluated in left to right order.
	boost::optional<Value> TryCallOverloadedBinaryOperator(
		OverloadedOperator op,
		const Synt::SyntaxElementBase& op_syntax_element,
		const Synt::IExpressionComponent&  left_expr,
		const Synt::IExpressionComponent& right_expr,
		bool evaluate_args_in_reverse_order,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const FilePos& file_pos,
		FunctionContext& function_context );
		
	Value BuildLazyBinaryOperator(
		const Synt::IExpressionComponent& l_expression,
		const Synt::IExpressionComponent& r_expression,
		const Synt::BinaryOperator& binary_operator,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildCastRef( const Synt::CastRef& cast_ref, NamesScope& names, FunctionContext& function_context );
	Value BuildCastRefUnsafe( const Synt::CastRefUnsafe& cast_ref_unsafe, NamesScope& names, FunctionContext& function_context );
	Value DoReferenceCast(
		const FilePos& file_pos,
		const Synt::ITypeNamePtr& type_name,
		const Synt::IExpressionComponentPtr& expression,
		bool enable_unsafe,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildCastImut( const Synt::CastImut& cast_imut, NamesScope& names, FunctionContext& function_context );
	Value BuildCastMut( const Synt::CastMut& cast_mut, NamesScope& names, FunctionContext& function_context );

	Value BuildNamedOperand( const Synt::NamedOperand& named_operand, NamesScope& names, FunctionContext& function_context );
	Value BuildMoveOpeator( const Synt::MoveOperator& move_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildNumericConstant( const Synt::NumericConstant& numeric_constant, FunctionContext& function_context );
	Value BuildStringLiteral( const Synt::StringLiteral& string_literal, FunctionContext& function_context );
	Variable BuildBooleanConstant( const Synt::BooleanConstant& boolean_constant, FunctionContext& function_context );

	Value BuildIndexationOperator(
		const Value& value,
		const Synt::IndexationOperator& indexation_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildMemberAccessOperator(
		const Value& value,
		const Synt::MemberAccessOperator& member_access_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildCallOperator(
		const Value& function_value,
		const Synt::CallOperator& call_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Value DoCallFunction(
		llvm::Value* function,
		const Function& function_type,
		const FilePos& call_file_pos,
		const Variable* first_arg,
		std::vector<const Synt::IExpressionComponent*> args,
		const bool evaluate_args_in_reverse_order,
		NamesScope& names,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

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

	// Typeinfo

	Value BuildTypeinfoOperator( const Synt::TypeInfo& typeinfo_op, NamesScope& names );
	Variable BuildTypeInfo( const Type& type, NamesScope& root_namespace );
	ClassProxyPtr CreateTypeinfoClass( NamesScope& root_namespace );
	Variable BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace );
	void BuildFullTypeinfo( const Type& type, Variable& typeinfo_variable, NamesScope& root_namespace );
	const Variable& GetTypeinfoListEndNode( NamesScope& root_namespace );
	void AddTypeinfoNodeIsEndVariable( Class& node_class, bool is_end= false );
	void FinishTypeinfoClass( Class& class_, const ClassProxyPtr class_proxy, const std::vector<llvm::Type*>& fields_llvm_types );
	Variable BuildTypeinfoEnumElementsList( const Enum& enum_type, NamesScope& root_namespace );
	void CreateTypeinfoClassMembersListNodeCommonFields(
		const Class& class_, const ClassProxyPtr& node_class_proxy,
		const ProgramString& member_name,
		std::vector<llvm::Type*>& fields_llvm_types, std::vector<llvm::Constant*>& fields_initializers );
	Variable BuildTypeinfoClassFieldsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassTypesList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassFunctionsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildeTypeinfoClassParentsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoFunctionArguments( const Function& function_type, NamesScope& root_namespace );

	// Block elements

	void BuildVariablesDeclarationCode(
		const Synt::VariablesDeclaration& variables_declaration,
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildAutoVariableDeclarationCode(
		const Synt::AutoVariableDeclaration& auto_variable_declaration,
		NamesScope& block_names,
		FunctionContext& function_context );

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

	void BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names );
	void BuildStaticAssert( const Synt::StaticAssert& static_assert_, NamesScope& names );

	BlockBuildInfo BuildStaticIfOperatorCode(
		const Synt::StaticIfOperator& static_if_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildHalt( const Synt::Halt& halt, FunctionContext& function_context );
	void BuildHaltIf( const Synt::HaltIf& halt_if, NamesScope& names, FunctionContext& function_context );

	void BuildTypedef(
		const Synt::Typedef& typedef_,
		NamesScope& names );

	// Functions

	FunctionVariable* GetFunctionWithSameType(
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

	const FunctionVariable* GetOverloadedOperator(
		const std::vector<Function::Arg>& actual_args,
		OverloadedOperator op,
		const FilePos& file_pos );

	const TemplateTypeGenerationResult* SelectTemplateType(
		const std::vector<TemplateTypeGenerationResult>& candidate_templates,
		size_t arg_count );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.

	llvm::Constant* ApplyInitializer(
		const Variable& variable,
		const StoredVariablePtr& variable_storage,
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
		const StoredVariablePtr& variable_storage,
		const Synt::ArrayInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyStructNamedInitializer(
		const Variable& variable,
		const StoredVariablePtr& variable_storage,
		const Synt::StructNamedInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const Variable& variable,
		const StoredVariablePtr& variable_storage,
		const Synt::CallOperator& call_operator,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyExpressionInitializer(
		const Variable& variable,
		const StoredVariablePtr& variable_storage,
		const Synt::ExpressionInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyZeroInitializer(
		const Variable& variable,
		const Synt::ZeroInitializer& initializer,
		FunctionContext& function_context );

	llvm::Constant* ApplyUninitializedInitializer(
		const Variable& variable,
		const Synt::UninitializedInitializer& initializer,
		FunctionContext& function_context );

	llvm::Constant* InitializeReferenceField(
		const Variable& variable,
		const StoredVariablePtr& variable_storage,
		const ClassField& field,
		const Synt::IInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* InitializeFunctionPointer(
		const Variable& variable,
		const Synt::IExpressionComponent& initializer_expression,
		NamesScope& block_names,
		FunctionContext& function_context );

	// Reference-checking.
	void ProcessFunctionArgReferencesTags(
		const Synt::FunctionType& func,
		Function& function_type,
		const Synt::FunctionArgument& in_arg,
		const Function::Arg& out_arg,
		size_t arg_number );

	void ProcessFunctionReturnValueReferenceTags( const Synt::FunctionType& func, const Function& function_type );

	void TryGenerateFunctionReturnReferencesMapping(
		const Synt::FunctionType& func,
		Function& function_type );

	void ProcessFunctionReferencesPollution(
		const Synt::Function& func,
		Function& function_type,
		const ClassProxyPtr& base_class );

	void ProcessFunctionTypeReferencesPollution(
		const Synt::FunctionType& func,
		Function& function_type,
		bool first_arg_is_implicit_this= false );

	void CheckReferencedVariables( const Variable& reference, const FilePos& file_pos );
	void CheckVariableReferences( const StoredVariable& var, const FilePos& file_pos );
	std::vector<VariableStorageUseCounter> LockReferencedVariables( const Variable& reference );

	VariablesState MergeVariablesStateAfterIf( const std::vector<VariablesState>& bracnhes_variables_state, const FilePos& file_pos );
	void CheckWhileBlokVariablesState( const VariablesState& state_before, const VariablesState& state_after, const FilePos& file_pos );

	// Name resolving.
	enum class ResolveMode
	{
		Regular,
		ForDeclaration,
		ForTemplateSignatureParameter,
	};
	NamesScope::InsertedName* ResolveName( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name, ResolveMode resolve_mode= ResolveMode::Regular );

	NamesScope::InsertedName* ResolveName(
		const FilePos& file_pos,
		NamesScope& names_scope,
		const Synt::ComplexName::Component* components,
		size_t component_count,
		ResolveMode resolve_mode );

	// NamesScope fill

	void NamesScopeFill( NamesScope& names_scope, const Synt::ProgramElements& namespace_elements );
	void NamesScopeFill( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::AutoVariableDeclaration& variable_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Function& function_declaration, const ClassProxyPtr& base_class, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_declaration, const ClassProxyPtr& base_class, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	ClassProxyPtr NamesScopeFill( NamesScope& names_scope, const Synt::Class& class_declaration, const ProgramString& override_name= ""_SpC );
	void NamesScopeFill( NamesScope& names_scope, const Synt::TypeTemplateBase& type_template_declaration, const ClassProxyPtr& base_class, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Enum& enum_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Typedef& typedef_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::StaticAssert& static_assert_ );
	void NamesScopeFillOutOfLineElements( NamesScope& names_scope, const Synt::ProgramElements& namespace_elements );

	// Global things build

	bool EnsureTypeCompleteness( const Type& type, TypeCompleteness completeness ); // Returns true, if all ok

	void GlobalThingBuildNamespace( NamesScope& names_scope );
	void GlobalThingBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, bool build_body );
	void GlobalThingBuildClass( ClassProxyPtr class_type, TypeCompleteness completeness );
	void GlobalThingBuildEnum( const EnumPtr& enum_, TypeCompleteness completeness );
	void GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set );
	void GlobalThingBuildTypedef( NamesScope& names_scope, Value& typedef_value );
	void GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value );
	size_t GlobalThingDetectloop( const GlobalThing& global_thing ); // returns loop start index or ~0u
	void GlobalThingReportAboutLoop( size_t loop_start_stack_index, const ProgramString& last_loop_element_name, const FilePos& last_loop_element_file_pos );

	// Other stuff

	static U_FundamentalType GetNumericConstantType( const Synt::NumericConstant& number );

	llvm::Type* GetFundamentalLLVMType( U_FundamentalType fundmantal_type );

	// If variable is on stack, creates move to rigister instruction.
	// If variable already in register - does nothing.
	llvm::Value* CreateMoveToLLVMRegisterInstruction( const Variable& variable, FunctionContext& function_context );

	llvm::Value* CreateReferenceCast( llvm::Value* ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context );

	llvm::GlobalVariable* CreateGlobalConstantVariable( const Type& type, const std::string& mangled_name, llvm::Constant* initializer= nullptr );

	void SetupGeneratedFunctionLinkageAttributes( llvm::Function& function );

private:
	llvm::LLVMContext& llvm_context_;
	std::string target_triple_str_;
	const llvm::TargetMachine* target_machine_= nullptr;

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

		llvm::IntegerType* char8 ;
		llvm::IntegerType* char16;
		llvm::IntegerType* char32;

		llvm::IntegerType* void_;
		llvm::Type* void_for_ret_;
		llvm::Type* invalid_type_;
		llvm::IntegerType* bool_;

		llvm::IntegerType* int_ptr; // Type with width of pointer.
	} fundamental_llvm_types_;

	llvm::Function* halt_func_= nullptr;

	Type invalid_type_;
	Type void_type_;
	Type void_type_for_ret_;
	Type bool_type_;
	Type size_type_; // Alias for u32 or u64

	ConstexprFunctionEvaluator constexpr_function_evaluator_;

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

	// We needs to generate same typeinfo classes for same types. Use cache for it.
	// TODO - create hasher for type and use unordered_map.
	std::list< std::pair< Type, Variable > > typeinfo_cache_;
	boost::optional< Variable > typeinfo_list_end_node_; // Lazy initialized.
	llvm::GlobalVariable* typeinfo_is_end_variable_[2u]= { nullptr, nullptr }; // Lazy initialized.

	std::vector<GlobalThing> global_things_stack_;
};

using MutabilityModifier= Synt::MutabilityModifier;
using ReferenceModifier= Synt::ReferenceModifier;

} // namespace CodeBuilderPrivate

using CodeBuilderPrivate::CodeBuilder;

} // namespace U
