#pragma once
#include <list>
#include <set>
#include <vector>
#include <unordered_map>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"
#include "code_builder_errors.hpp"
#include "constexpr_function_evaluator.hpp"
#include "function_context.hpp"
#include "i_code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

class CodeBuilder final : public ICodeBuilder
{
public:
	CodeBuilder(
		llvm::LLVMContext& llvm_context,
		std::string target_triple_str,
		const llvm::DataLayout& data_layout );
	virtual ~CodeBuilder() override= default;

	virtual BuildResult BuildProgram( const SourceGraph& source_graph ) override;

private:
	using ClassTable= std::unordered_map< ClassProxyPtr, std::unique_ptr<Class> >;
	struct BuildResultInternal
	{
		std::unique_ptr<NamesScope> names_map;
		std::unique_ptr< ProgramStringMap< Value > > generated_template_things_storage;
		std::unique_ptr<ClassTable> class_table;
	};

	struct BlockBuildInfo
	{
		bool have_terminal_instruction_inside= false;
	};

	struct TemplateTypeGenerationResult
	{
		TypeTemplatePtr type_template;
		Value* type= nullptr;
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

	class ReferencesGraphNodeHolder final
	{
	public:
		ReferencesGraphNodeHolder( ReferencesGraphNodePtr node, FunctionContext& function_context );
		ReferencesGraphNodeHolder( const ReferencesGraphNodeHolder& )= delete;
		ReferencesGraphNodeHolder( ReferencesGraphNodeHolder&& other ) noexcept;

		ReferencesGraphNodeHolder& operator=( const ReferencesGraphNodeHolder& )= delete;
		ReferencesGraphNodeHolder& operator=( ReferencesGraphNodeHolder&& other ) noexcept;
		~ReferencesGraphNodeHolder();

		const ReferencesGraphNodePtr& Node() const { return node_; }

	private:
		ReferencesGraphNodePtr node_;
		FunctionContext& function_context_;
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

	// Function context required for accesing local constexpr variables.
	Type PrepareType( const Synt::TypeName& type_name, NamesScope& names_scope, FunctionContext& function_context );

	llvm::FunctionType* GetLLVMFunctionType( const Function& function_type );

	// Virtual stuff
	void ProcessClassParentsVirtualTables( Class& the_class );
	void TryGenerateDestructorPrototypeForPolymorphClass( Class& the_class, const Type& class_type );
	void ProcessClassVirtualFunction( Class& the_class, FunctionVariable& function );
	void PrepareClassVirtualTableType( const ClassProxyPtr& class_type );
	void BuildClassVirtualTables_r( Class& the_class, const Type& class_type, const std::vector< ClassProxyPtr >& dst_class_path, llvm::Value* dst_class_ptr_null_based );
	void BuildClassVirtualTables( Class& the_class, const Type& class_type ); // Returns type of vtable pointer or nullptr.

	std::pair<Variable, llvm::Value*> TryFetchVirtualFunction(
		const Variable& this_,
		const FunctionVariable& function,
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

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
		const Synt::Expression& template_parameter,
		NamesScope& names_scope,
		const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	void PrepareTemplateSignatureParameter(
		const Synt::TypeName& template_parameter,
		NamesScope& names_scope,
		const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	// Resolve as deep, as can, but does not instantiate last component, if it is template.
	Value* ResolveForTemplateSignatureParameter(
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
		const Synt::Expression& signature_parameter,
		const FilePos& signature_parameter_file_pos,
		DeducibleTemplateParameters& deducible_template_parameters,
		NamesScope& names_scope );

	DeducedTemplateParameter DeduceTemplateArguments(
		const TemplateBase& template_,
		const TemplateParameter& template_parameter,
		const Synt::TypeName& signature_parameter,
		const FilePos& signature_parameter_file_pos,
		DeducibleTemplateParameters& deducible_template_parameters,
		NamesScope& names_scope );

	// Returns nullptr in case of fail.
	Value* GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatesSet& type_templates_set,
		const std::vector<Synt::Expression>& template_arguments,
		NamesScope& arguments_names_scope );

	// Returns nullptr in case of fail.
	TemplateTypeGenerationResult GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatePtr& type_template_ptr,
		const std::vector<Synt::Expression>& template_arguments,
		NamesScope& arguments_names_scope,
		bool skip_type_generation );

	const FunctionVariable* GenTemplateFunction(
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos,
		const FunctionTemplatePtr& function_template_ptr,
		const ArgsVector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this,
		bool skip_arguments= false );

	Value* GenTemplateFunctionsUsingTemplateParameters(
		const FilePos& file_pos,
		const std::vector<FunctionTemplatePtr>& function_templates,
		const std::vector<Synt::Expression>& template_arguments,
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
		llvm::Value* dst, llvm::Value* src,
		const Type& type,
		FunctionContext& function_context );

	void BuildCopyAssignmentOperatorPart(
		llvm::Value* dst, llvm::Value* src,
		const Type& type,
		FunctionContext& function_context );

	void CopyBytes_r(
		llvm::Value* src, llvm::Value* dst,
		const llvm::Type* const llvm_type,
		FunctionContext& function_context );

	void CopyBytes(
		llvm::Value* src, llvm::Value* dst,
		const Type& type,
		FunctionContext& function_context );

	void MoveConstantToMemory(
		llvm::Value* ptr, llvm::Constant* constant,
		FunctionContext& function_context );

	void TryCallCopyConstructor(
		CodeBuilderErrorsContainer& errors_container,
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
		uint64_t iteration_count,
		const std::function<void(llvm::Value* counter_value)>& loop_body,
		FunctionContext& function_context);

	void CallDestructorsImpl(
		const StackVariablesStorage& stack_variables_storage,
		FunctionContext& function_context,
		ReferencesGraph& variables_state_copy,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	void CallDestructors(
		const StackVariablesStorage& stack_variables_storage,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const FilePos& file_pos );

	void CallDestructor(
		llvm::Value* ptr,
		const Type& type,
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	void CallDestructorsForLoopInnerVariables( NamesScope& names_scope, FunctionContext& function_context, const FilePos& file_pos );
	void CallDestructorsBeforeReturn( NamesScope& names_scope, FunctionContext& function_context, const FilePos& file_pos );
	void CallMembersDestructors( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos );

	// Returns index of function in set, if function successfuly prepared and inserted. Returns ~0 on fail.
	size_t PrepareFunction(
		NamesScope& names_scope,
		const ClassProxyPtr& base_class,
		OverloadedFunctionsSet& functions_set,
		const Synt::Function& function_declaration,
		bool is_out_of_line_function );

	void CheckOverloadedOperator(
		const ClassProxyPtr& base_class,
		const Function& func_type,
		OverloadedOperator overloaded_operator,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	// Returns type of return value.
	Type BuildFuncCode(
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
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildExpressionCodeEnsureVariable(
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildExpressionCode(
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	// Returns Value, if overloaded operator selected or if arguments are template dependent or argumens are error values.
	// Returns boost::none, if all ok, but there is no overloaded operator.
	// In success call of overloaded operator arguments evaluated in left to right order.
	boost::optional<Value> TryCallOverloadedBinaryOperator(
		OverloadedOperator op,
		const Synt::SyntaxElementBase& op_syntax_element,
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		bool evaluate_args_in_reverse_order,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value CallBinaryOperatorForTuple(
		OverloadedOperator op,
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );
		
	Value BuildLazyBinaryOperator(
		const Synt::Expression& l_expression,
		const Synt::Expression& r_expression,
		const Synt::BinaryOperator& binary_operator,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildCastRef( const Synt::CastRef& cast_ref, NamesScope& names, FunctionContext& function_context );
	Value BuildCastRefUnsafe( const Synt::CastRefUnsafe& cast_ref_unsafe, NamesScope& names, FunctionContext& function_context );
	Value DoReferenceCast(
		const FilePos& file_pos,
		const Synt::TypeName& type_name,
		const Synt::Expression& expression,
		bool enable_unsafe,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildCastImut( const Synt::CastImut& cast_imut, NamesScope& names, FunctionContext& function_context );
	Value BuildCastMut( const Synt::CastMut& cast_mut, NamesScope& names, FunctionContext& function_context );

	Value BuildNamedOperand( const Synt::NamedOperand& named_operand, NamesScope& names, FunctionContext& function_context );
	Value BuildTernaryOperator( const Synt::TernaryOperator& ternary_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildMoveOpeator( const Synt::MoveOperator& move_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildNumericConstant( const Synt::NumericConstant& numeric_constant, NamesScope& names, FunctionContext& function_context );
	Value BuildStringLiteral( const Synt::StringLiteral& string_literal, NamesScope& names, FunctionContext& function_context );
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
		const std::vector<Variable>& preevaluated_args,
		const std::vector<const Synt::Expression*>& args,
		const bool evaluate_args_in_reverse_order,
		NamesScope& names,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	Variable BuildTempVariableConstruction(
		const Type& type,
		const Synt::CallOperator& call_operator,
		NamesScope& names,
		FunctionContext& function_context );

	Variable ConvertVariable(
		const Variable& variable,
		const Type& dst_type,
		const FunctionVariable& conversion_constructor,
		NamesScope& names,
		FunctionContext& function_context,
		const FilePos& file_pos );

	Value BuildUnaryMinus(
		const Value& value,
		const Synt::UnaryMinus& unary_minus,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildLogicalNot(
		const Value& value,
		const Synt::LogicalNot& logical_not,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildBitwiseNot(
		const Value& value,
		const Synt::BitwiseNot& bitwise_not,
		NamesScope& names,
		FunctionContext& function_context );

	// Typeinfo

	Value BuildTypeinfoOperator( const Synt::TypeInfo& typeinfo_op, NamesScope& names, FunctionContext& function_context );
	Variable BuildTypeInfo( const Type& type, NamesScope& root_namespace );
	ClassProxyPtr CreateTypeinfoClass( NamesScope& root_namespace, const Type& src_type, const ProgramString& name );
	Variable BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace );
	void BuildFullTypeinfo( const Type& type, Variable& typeinfo_variable, NamesScope& root_namespace );
	const Variable& GetTypeinfoListEndNode( NamesScope& root_namespace );
	void FinishTypeinfoClass( Class& class_, const ClassProxyPtr class_proxy, const ClassFieldsVector<llvm::Type*>& fields_llvm_types );
	Variable BuildTypeinfoEnumElementsList( const EnumPtr& enum_type, NamesScope& root_namespace );
	void CreateTypeinfoClassMembersListNodeCommonFields(
		const Class& class_, const ClassProxyPtr& node_class_proxy,
		const ProgramString& member_name,
		ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers );
	Variable BuildTypeinfoClassFieldsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassTypesList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassFunctionsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildeTypeinfoClassParentsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoFunctionArguments( const Function& function_type, NamesScope& root_namespace );
	Variable BuildypeinfoTupleElements( const Tuple& tuple_type, NamesScope& root_namespace );

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
		const Synt::Expression& expression,
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

	void BuildForOperatorCode(
		const Synt::ForOperator& for_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildBreakOperatorCode(
		const Synt::BreakOperator& break_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildContinueOperatorCode(
		const Synt::ContinueOperator& continue_operator,
		NamesScope& names,
		FunctionContext& function_context );

	BlockBuildInfo BuildIfOperatorCode(
		const Synt::IfOperator& if_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context );
	void BuildStaticAssert( const Synt::StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context );

	BlockBuildInfo BuildStaticIfOperatorCode(
		const Synt::StaticIfOperator& static_if_operator,
		NamesScope& names,
		FunctionContext& function_context );

	void BuildHalt( const Synt::Halt& halt, FunctionContext& function_context );
	void BuildHaltIf( const Synt::HaltIf& halt_if, NamesScope& names, FunctionContext& function_context );

	// Name resolving.
	enum class ResolveMode
	{
		Regular,
		ForDeclaration,
		ForTemplateSignatureParameter,
	};
	Value* ResolveValue( const FilePos& file_pos, NamesScope& names_scope, const Synt::ComplexName& complex_name, ResolveMode resolve_mode= ResolveMode::Regular );

	Value* ResolveValue(
		const FilePos& file_pos,
		NamesScope& names_scope,
		const Synt::ComplexName::Component* components,
		size_t component_count,
		ResolveMode resolve_mode );

	// Functions

	FunctionVariable* GetFunctionWithSameType(
		const Function& function_type,
		OverloadedFunctionsSet& functions_set );

	// Returns "false" on error.
	bool ApplyOverloadedFunction(
		OverloadedFunctionsSet& functions_set,
		const FunctionVariable& function,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	const FunctionVariable* GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		const ArgsVector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos,
		bool produce_errors= true,
		bool enable_type_conversions= true);

	const FunctionVariable* GetOverloadedOperator(
		const ArgsVector<Function::Arg>& actual_args,
		OverloadedOperator op,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	const FunctionVariable* GetConversionConstructor(
		const Type& src_type,
		const Type& dst_type,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	const TemplateTypeGenerationResult* SelectTemplateType(
		const std::vector<TemplateTypeGenerationResult>& candidate_templates,
		size_t arg_count );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.

	llvm::Constant* ApplyInitializer(
		const Variable& variable,
		const Synt::Initializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	void ApplyEmptyInitializer(
		const ProgramString& variable_name,
		const FilePos& file_pos,
		const Variable& variable,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyArrayInitializer(
		const Variable& variable,
		const Synt::ArrayInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyStructNamedInitializer(
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

	llvm::Constant* ApplyUninitializedInitializer(
		const Variable& variable,
		const Synt::UninitializedInitializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* InitializeReferenceField(
		const Variable& variable,
		const ClassField& field,
		const Synt::Initializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* InitializeFunctionPointer(
		const Variable& variable,
		const Synt::Expression& initializer_expression,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* InitializeClassFieldWithInClassIninitalizer(
		const Variable& field_variable,
		const ClassField& class_field,
		FunctionContext& function_context );

	llvm::Constant* InitializeReferenceClassFieldWithInClassIninitalizer(
		const Variable& variable,
		const ClassField& class_field,
		FunctionContext& function_context );

	void CheckClassFieldsInitializers( const ClassProxyPtr& class_type );

	// Reference-checking.
	void ProcessFunctionArgReferencesTags(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		Function& function_type,
		const Synt::FunctionArgument& in_arg,
		const Function::Arg& out_arg,
		size_t arg_number );

	void ProcessFunctionReturnValueReferenceTags(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		const Function& function_type );

	void TryGenerateFunctionReturnReferencesMapping(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		Function& function_type );

	void ProcessFunctionReferencesPollution(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::Function& func,
		Function& function_type,
		const ClassProxyPtr& base_class );

	void ProcessFunctionTypeReferencesPollution(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		Function& function_type,
		bool first_arg_is_implicit_this= false );

	void DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos );

	ReferencesGraph MergeVariablesStateAfterIf(
		const std::vector<ReferencesGraph>& bracnhes_variables_state,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

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

	bool IsTypeComplete( const Type& type ) const;
	bool EnsureTypeCompleteness( const Type& type, TypeCompleteness completeness ); // Returns true, if all ok
	bool ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos ); // Returns true of all ok. If types are different can call EnsureTypeCompleteness.

	void GlobalThingBuildNamespace( NamesScope& names_scope );
	void GlobalThingBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, bool build_body );
	void GlobalThingBuildClass( ClassProxyPtr class_type, TypeCompleteness completeness );
	void GlobalThingBuildEnum( const EnumPtr enum_, TypeCompleteness completeness );
	void GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set );
	void GlobalThingBuildTypedef( NamesScope& names_scope, Value& typedef_value );
	void GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value );
	size_t GlobalThingDetectloop( const GlobalThing& global_thing ); // returns loop start index or ~0u
	void GlobalThingReportAboutLoop( size_t loop_start_stack_index, const ProgramString& last_loop_element_name, const FilePos& last_loop_element_file_pos );

	// Other stuff

	U_FundamentalType GetNumericConstantType( const Synt::NumericConstant& number );

	llvm::Type* GetFundamentalLLVMType( U_FundamentalType fundmantal_type );

	// If variable is on stack, creates move to rigister instruction.
	// If variable already in register - does nothing.
	llvm::Value* CreateMoveToLLVMRegisterInstruction( const Variable& variable, FunctionContext& function_context );

	llvm::Constant* GetZeroGEPIndex();
	llvm::Constant* GetFieldGEPIndex( uint64_t field_index );

	llvm::Value* CreateReferenceCast( llvm::Value* ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context );

	llvm::GlobalVariable* CreateGlobalConstantVariable( const Type& type, const std::string& mangled_name, llvm::Constant* initializer= nullptr );

	void SetupGeneratedFunctionLinkageAttributes( llvm::Function& function );

	struct InstructionsState
	{
		ReferencesGraph variables_state;
		size_t current_block_instruction_count;
		size_t alloca_block_instructin_count;
		size_t block_count;
	};

	InstructionsState SaveInstructionsState( FunctionContext& function_context );
	void RestoreInstructionsState(
		FunctionContext& function_context,
		const InstructionsState& state );

private:
	llvm::LLVMContext& llvm_context_;
	const std::string target_triple_str_;
	const llvm::DataLayout data_layout_;

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
		llvm::IntegerType* i128;
		llvm::IntegerType* u128;

		llvm::Type* f32;
		llvm::Type* f64;

		llvm::IntegerType* char8 ;
		llvm::IntegerType* char16;
		llvm::IntegerType* char32;

		llvm::IntegerType* void_;
		llvm::Type* void_for_ret;
		llvm::Type* invalid_type;
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

	FunctionContext* global_function_context_= nullptr;

	std::unique_ptr<llvm::Module> module_;
	std::vector<CodeBuilderError> global_errors_; // Do not use directly. Use NamesScope::GetErrors() instead.

	std::unordered_map< size_t, BuildResultInternal > compiled_sources_cache_;
	ClassTable* current_class_table_= nullptr;

	// Storage for enum types. Do not use shared pointers for enums for loops preventing.
	std::vector< std::unique_ptr<Enum> > enums_table_;

	// Cache needs for generating same classes as template instantiation result in different source files.
	// We can use same classes in different files, because template classes are logically unchangeable after instantiation.
	// Unchangeable they are because incomplete template classes ( or classes inside template classes, etc. ) currently forbidden.
	ProgramStringMap< ClassProxyPtr > template_classes_cache_;

	// We needs to generate same typeinfo classes for same types. Use cache for it.
	// TODO - create hasher for type and use unordered_map.
	std::list< std::pair< Type, Variable > > typeinfo_cache_;
	ClassTable typeinfo_class_table_;

	// Names map for generated template types/functions. We can not insert it in regular namespaces, because we needs insert it, while iterating regular namespaces.
	ProgramStringMap<Value> generated_template_things_storage_;

	std::vector<GlobalThing> global_things_stack_;
};

using MutabilityModifier= Synt::MutabilityModifier;
using ReferenceModifier= Synt::ReferenceModifier;

} // namespace CodeBuilderPrivate

using CodeBuilderPrivate::CodeBuilder;

} // namespace U
