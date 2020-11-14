#pragma once
#include <list>
#include <set>
#include <vector>
#include <unordered_map>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/source_graph_loader.hpp"
#include "../../code_builder_lib_common/constexpr_function_evaluator.hpp"
#include "class.hpp"
#include "template_signature_param.hpp"
#include "enum.hpp"
#include "function_context.hpp"
#include "template_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

class CodeBuilder
{
public:
	CodeBuilder(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		bool build_debug_info );

	struct BuildResult
	{
		std::vector<CodeBuilderError> errors;
		std::unique_ptr<llvm::Module> module;
	};
	BuildResult BuildProgram( const SourceGraph& source_graph );

private:
	using ClassTable= std::unordered_map< ClassProxyPtr, std::unique_ptr<Class> >;
	struct BuildResultInternal
	{
		std::unique_ptr<NamesScope> names_map;
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
	};

	struct GlobalThing // TODO - move struct out of here
	{
		const void* thing_ptr= nullptr;
		std::string name;
		FilePos file_pos;

		GlobalThing( const void* const in_thing_ptr, const std::string& in_name, const FilePos& in_file_pos )
			: thing_ptr(in_thing_ptr), name(in_name), file_pos(in_file_pos)
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
	Type PrepareType( const Synt::EmptyVariant& type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareType( const Synt::ArrayTypeName& array_type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareType( const Synt::TypeofTypeName& typeof_type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareType( const Synt::FunctionTypePtr& function_type_name_ptr, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareType( const Synt::FunctionType& function_type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareType( const Synt::TupleType& tuple_type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareType( const Synt::NamedTypeName& named_type_name, NamesScope& names_scope, FunctionContext& function_context );

	llvm::FunctionType* GetLLVMFunctionType( const Function& function_type );

	// Virtual stuff
	void PrepareClassVirtualTable( Class& the_class );
	void PrepareClassVirtualTableType( const ClassProxyPtr& class_type );

	void BuildPolymorphClassTypeId( Class& the_class, const Type& class_type );

	llvm::Constant* BuildClassVirtualTable_r( const Class& ancestor_class, const Class& dst_class, llvm::Value* dst_class_ptr_null_based );
	void BuildClassVirtualTable( Class& the_class, const Type& class_type ); // Returns type of vtable pointer or nullptr.

	std::pair<Variable, llvm::Value*> TryFetchVirtualFunction(
		const Variable& this_,
		const FunctionVariable& function,
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	void SetupVirtualTablePointers_r(
		llvm::Value* this_,
		llvm::Value* ptr_to_vtable_ptr,
		const Class& the_class,
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
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const FilePos& file_pos,
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::Expression& template_parameter,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::TypeName& template_parameter,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam ValueToTemplateParam( const Value& value, NamesScope& names_scope );

	// Resolve as deep, as can, but does not instantiate last component, if it is template.
	Value ResolveForTemplateSignatureParameter(
		const FilePos& file_pos,
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope );

	// Returns "true" if all ok.
	bool MatchTemplateArg(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::TypeParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::VariableParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::TemplateParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::ArrayParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::TupleParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::FunctionParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const FilePos& file_pos,
		const TemplateSignatureParam::SpecializedTemplateParam& template_param );

	// Returns nullptr in case of fail.
	Value* GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatesSet& type_templates_set,
		const std::vector<Synt::Expression>& template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	// Returns nullptr in case of fail.
	TemplateTypeGenerationResult GenTemplateType(
		const FilePos& file_pos,
		const TypeTemplatePtr& type_template_ptr,
		const std::vector<Value>& template_arguments,
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
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	bool NameShadowsTemplateArgument( const std::string& name, NamesScope& names_scope );

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

	void CopyBytes(
		llvm::Value* src, llvm::Value* dst, // TODO - swap dst and src.
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
		const std::string& func_name,
		const Synt::FunctionArgumentsDeclaration& args,
		const Synt::Block* block, // null for prototypes.
		const Synt::StructNamedInitializer* constructor_initialization_list );

	void BuildConstructorInitialization(
		const Variable& this_,
		const Class& base_class,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const Synt::StructNamedInitializer& constructor_initialization_list );

	// Expressions.
	Value BuildExpressionCode( const Synt::Expression& expression, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::EmptyVariant& expression, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::BinaryOperator& binary_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::NamedOperand& named_operand, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::TernaryOperator& ternary_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::TypeNameInExpression& type_name_in_expression, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::NumericConstant& numeric_constant, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::BracketExpression& bracket_expression, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::BooleanConstant& boolean_constant, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::StringLiteral& string_literal, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::MoveOperator& move_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::TakeOperator& move_operator, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::CastMut& cast_mut, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::CastImut& cast_imut, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::CastRef& cast_ref, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::CastRefUnsafe& cast_ref_unsafe, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCode( const Synt::TypeInfo& typeinfo, NamesScope& names, FunctionContext& function_context );

	Value BuildExpressionCodeAndDestroyTemporaries(
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildExpressionCodeEnsureVariable(
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	// Returns Value, if overloaded operator selected or if arguments are template dependent or argumens are error values.
	// Returns std::nullopt, if all ok, but there is no overloaded operator.
	// In success call of overloaded operator arguments evaluated in left to right order.
	std::optional<Value> TryCallOverloadedBinaryOperator(
		OverloadedOperator op,
		const Synt::SyntaxElementBase& op_syntax_element,
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		bool evaluate_args_in_reverse_order,
		const FilePos& file_pos,
		NamesScope& names,
		FunctionContext& function_context );

	Value CallBinaryOperatorForArrayOrTuple(
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

	Value DoReferenceCast(
		const FilePos& file_pos,
		const Synt::TypeName& type_name,
		const Synt::Expression& expression,
		bool enable_unsafe,
		NamesScope& names,
		FunctionContext& function_context );

	// Postfix operators
	Value BuildPostfixOperator( const Synt::CallOperator& call_operator, const Value& value, NamesScope& names, FunctionContext& function_context );
	Value BuildPostfixOperator( const Synt::IndexationOperator& indexation_operator, const Value& value, NamesScope& names, FunctionContext& function_context );
	Value BuildPostfixOperator( const Synt::MemberAccessOperator& member_access_operator, const Value& value, NamesScope& names, FunctionContext& function_context );

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

	// Prefix operators
	Value BuildPrefixOperator( const Synt::UnaryMinus& unary_minus, const Value& value, NamesScope& names, FunctionContext& function_context );
	Value BuildPrefixOperator( const Synt::UnaryPlus& unary_plus, const Value& value, NamesScope& names, FunctionContext& function_context );
	Value BuildPrefixOperator( const Synt::LogicalNot& logical_not, const Value& value, NamesScope& names, FunctionContext& function_context );
	Value BuildPrefixOperator( const Synt::BitwiseNot& bitwise_not, 	const Value& value, NamesScope& names, FunctionContext& function_context );

	// Typeinfo

	Variable BuildTypeInfo( const Type& type, NamesScope& root_namespace );
	ClassProxyPtr CreateTypeinfoClass( NamesScope& root_namespace, const Type& src_type, std::string name );
	Variable BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace );
	void BuildFullTypeinfo( const Type& type, Variable& typeinfo_variable, NamesScope& root_namespace );
	const Variable& GetTypeinfoListEndNode( NamesScope& root_namespace );
	void FinishTypeinfoClass( Class& class_, const ClassProxyPtr class_proxy, const ClassFieldsVector<llvm::Type*>& fields_llvm_types );
	Variable BuildTypeinfoEnumElementsList( const EnumPtr& enum_type, NamesScope& root_namespace );
	void CreateTypeinfoClassMembersListNodeCommonFields(
		const Class& class_, const ClassProxyPtr& node_class_proxy,
		const std::string& member_name,
		ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers );
	Variable BuildTypeinfoClassFieldsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassTypesList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassFunctionsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoClassParentsList( const ClassProxyPtr& class_type, NamesScope& root_namespace );
	Variable BuildTypeinfoFunctionArguments( const Function& function_type, NamesScope& root_namespace );
	Variable BuildTypeinfoTupleElements( const Tuple& tuple_type, NamesScope& root_namespace );

	// Block elements
	BlockBuildInfo BuildBlockElement( const Synt::Block& block, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::VariablesDeclaration& variables_declaration, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::AutoVariableDeclaration& auto_variable_declaration, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::ReturnOperator& return_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::ForOperator& for_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::CStyleForOperator& c_style_for_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::WhileOperator& while_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::BreakOperator& break_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::ContinueOperator& continue_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::WithOperator& with_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::IfOperator& if_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::StaticIfOperator& static_if_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::SingleExpressionOperator& single_expression_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::AssignmentOperator& assignment_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::AdditiveAssignmentOperator& additive_assignment_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::IncrementOperator& increment_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::DecrementOperator& decrement_operator, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::Halt& halt, NamesScope& names, FunctionContext& function_context );
	BlockBuildInfo BuildBlockElement( const Synt::HaltIf& halt_if, NamesScope& names, FunctionContext& function_context );

	// ++ and -- operations
	void BuildDeltaOneOperatorCode(
		const Synt::Expression& expression,
		const FilePos& file_pos,
		bool positive, // true - increment, false - decrement
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context );

	// Name resolving.
	enum class ResolveMode
	{
		Regular,
		ForTemplateSignatureParameter,
	};

	Value ResolveValue(
		const FilePos& file_pos,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const Synt::ComplexName& complex_name,
		ResolveMode resolve_mode= ResolveMode::Regular );

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
	
	llvm::Constant* ApplyInitializer( const Synt::Initializer& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::EmptyVariant& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::ArrayInitializer& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::StructNamedInitializer& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::ConstructorInitializer& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::ExpressionInitializer& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::ZeroInitializer& initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );
	llvm::Constant* ApplyInitializer( const Synt::UninitializedInitializer& uninitialized_initializer, const Variable& variable, NamesScope& names, FunctionContext& function_context );

	void ApplyEmptyInitializer(
		const std::string& variable_name,
		const FilePos& file_pos,
		const Variable& variable,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const Synt::CallOperator& call_operator,
		const Variable& variable,
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
		Function& function_type );

	void SetupReferencesInCopyOrMove( FunctionContext& function_context, const Variable& dst_variable, const Variable& src_variable, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos );

	void DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos );

	ReferencesGraph MergeVariablesStateAfterIf(
		const std::vector<ReferencesGraph>& bracnhes_variables_state,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	bool IsReferenceAllowedForReturn( FunctionContext& function_context, const ReferencesGraphNodePtr& variable_node );

	void CheckReferencesPollutionBeforeReturn(
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const FilePos& file_pos );

	// NamesScope fill

	void NamesScopeFill( const Synt::ProgramElements& namespace_elements, NamesScope& names_scope );
	void NamesScopeFill( const Synt::NamespacePtr& namespace_, NamesScope& names_scope );
	void NamesScopeFill( const Synt::VariablesDeclaration& variables_declaration , NamesScope& names_scope );
	void NamesScopeFill( const Synt::AutoVariableDeclaration& variable_declaration, NamesScope& names_scope );
	void NamesScopeFill( const Synt::FunctionPtr& function_declaration, NamesScope& names_scope, const ClassProxyPtr& base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( const Synt::FunctionTemplate& function_template_declaration, NamesScope& names_scope, const ClassProxyPtr& base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	ClassProxyPtr NamesScopeFill( const Synt::ClassPtr& class_declaration, NamesScope& names_scope, const std::string& override_name= "" );
	void NamesScopeFill( const Synt::TypeTemplateBase& type_template_declaration, NamesScope& names_scope, const ClassProxyPtr& base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( const Synt::Enum& enum_declaration, NamesScope& names_scope );
	void NamesScopeFill( const Synt::Typedef& typedef_declaration, NamesScope& names_scope );
	void NamesScopeFill( const Synt::StaticAssert& static_assert_, NamesScope& names_scope );
	void NamesScopeFillOutOfLineElements( const Synt::ProgramElements& namespace_elements, NamesScope& names_scope );

	// Global things build

	bool IsTypeComplete( const Type& type ) const;
	bool EnsureTypeComplete( const Type& type ); // Returns true, if complete
	bool ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const FilePos& file_pos ); // Returns true of all ok. If types are different can call EnsureTypeCompleteness.

	void GlobalThingBuildNamespace( NamesScope& names_scope );
	void GlobalThingBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, bool build_body );
	void GlobalThingBuildClass( ClassProxyPtr class_type );
	void GlobalThingBuildEnum( const EnumPtr enum_ );
	void GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set );
	void GlobalThingBuildTypedef( NamesScope& names_scope, Value& typedef_value );
	void GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value );
	size_t GlobalThingDetectloop( const GlobalThing& global_thing ); // returns loop start index or ~0u
	void GlobalThingReportAboutLoop( size_t loop_start_stack_index, const std::string& last_loop_element_name, const FilePos& last_loop_element_file_pos );

	// Debug info

	llvm::DIFile* GetDIFile(size_t file_index);

	void CreateVariableDebugInfo(
		const Variable& variable,
		const std::string& variable_name,
		const FilePos& file_pos,
		FunctionContext& function_context );

	void CreateReferenceVariableDebugInfo(
		const Variable& variable,
		const std::string& variable_name,
		const FilePos& file_pos,
		FunctionContext& function_context );

	void CreateFunctionDebugInfo(
		const FunctionVariable& func_variable,
		const std::string& function_name );

	void SetCurrentDebugLocation(
		const FilePos& file_pos,
		FunctionContext& function_context );

	void DebugInfoStartBlock( const FilePos& file_pos, FunctionContext& function_context );
	void DebugInfoEndBlock( FunctionContext& function_context );

	llvm::DIType* CreateDIType( const Type& type );
	llvm::DIBasicType* CreateDIType( const FundamentalType& type );
	llvm::DICompositeType* CreateDIType( const Array& type );
	llvm::DICompositeType* CreateDIType( const Tuple& type );
	llvm::DISubroutineType* CreateDIType( const Function& type );
	llvm::DIDerivedType* CreateDIType( const FunctionPointer& type );
	llvm::DICompositeType* CreateDIType( const ClassProxyPtr& type );
	llvm::DICompositeType* CreateDIType( const EnumPtr& type );

	// Other stuff

	llvm::Type* GetFundamentalLLVMType( U_FundamentalType fundmantal_type );

	// If variable is on stack, creates move to rigister instruction.
	// If variable already in register - does nothing.
	llvm::Value* CreateMoveToLLVMRegisterInstruction( const Variable& variable, FunctionContext& function_context );

	llvm::Constant* GetZeroGEPIndex();
	llvm::Constant* GetFieldGEPIndex( uint64_t field_index );

	llvm::Value* CreateReferenceCast( llvm::Value* ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context );

	llvm::GlobalVariable* CreateGlobalConstantVariable( const Type& type, const std::string& mangled_name, llvm::Constant* initializer= nullptr );

	void SetupGeneratedFunctionAttributes( llvm::Function& function );

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
	const llvm::DataLayout data_layout_;
	const bool build_debug_info_;

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
	std::unordered_map< Type, Variable, TypeHasher > typeinfo_cache_;
	ClassTable typeinfo_class_table_;

	// Names map for generated template types/functions. We can not insert it in regular namespaces, because we needs insert it, while iterating regular namespaces.
	ProgramStringMap<Value> generated_template_things_storage_;

	std::vector<GlobalThing> global_things_stack_;

	// Debug info.
	struct
	{
		std::vector<llvm::DIFile*> source_file_entries; // Entry for each file in sources graph.

		// Debug info builder, compile unit, types cache - unique only for current file.
		std::unique_ptr<llvm::DIBuilder> builder;
		llvm::DICompileUnit* compile_unit= nullptr;

		// Build debug info for classes and enums once and put it to cache.
		std::unordered_map<ClassProxyPtr, llvm::DICompositeType*> classes_di_cache;
		std::unordered_map<EnumPtr, llvm::DICompositeType*> enums_di_cache;
	} debug_info_;
};

using MutabilityModifier= Synt::MutabilityModifier;
using ReferenceModifier= Synt::ReferenceModifier;

} // namespace CodeBuilderPrivate

using CodeBuilderPrivate::CodeBuilder;

} // namespace U
