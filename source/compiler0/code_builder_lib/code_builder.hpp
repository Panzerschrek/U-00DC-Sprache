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
#include "enum.hpp"
#include "function_context.hpp"
#include "mangling.hpp"
#include "template_signature_param.hpp"
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

	struct TemplateTypePreparationResult
	{
		TypeTemplatePtr type_template;
		NamesScopePtr template_args_namespace;
		TemplateArgs template_args;
		TemplateArgs signature_args;
	};

	struct TemplateFunctionPreparationResult
	{
		FunctionTemplatePtr function_template;
		NamesScopePtr template_args_namespace;
		TemplateArgs template_args;
	};

	struct GlobalThing // TODO - move struct out of here
	{
		const void* thing_ptr= nullptr;
		std::string name;
		SrcLoc src_loc;

		GlobalThing( const void* const in_thing_ptr, const std::string& in_name, const SrcLoc& in_src_loc )
			: thing_ptr(in_thing_ptr), name(in_name), src_loc(in_src_loc)
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
		const SrcLoc& src_loc, // SrcLoc or original class.
		const ClassProxyPtr& src_class,
		ClassTable& dst_class_table,
		NamesScope& dst_namespace );
	void SetCurrentClassTable( ClassTable& table );

	void FillGlobalNamesScope( NamesScope& global_names_scope );

	// Function context required for accesing local constexpr variables.
	Type PrepareType( const Synt::TypeName& type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::EmptyVariant& type_nam );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ArrayTypeName& array_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionTypePtr& function_type_name_ptr );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TupleType& tuple_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RawPointerType& raw_pointer_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ComplexName& named_type_name );

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
		const SrcLoc& src_loc );

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
		const Synt::TypeTemplate& type_template_declaration,
		TypeTemplatesSet& type_templates_set,
		NamesScope& names_scope );

	void PrepareFunctionTemplate(
		const Synt::FunctionTemplate& function_template_declaration,
		OverloadedFunctionsSet& functions_set,
		NamesScope& names_scope,
		const ClassProxyPtr& base_class );

	void ProcessTemplateParams(
		const std::vector<Synt::TemplateBase::Param>& params,
		NamesScope& names_scope,
		const SrcLoc& src_loc,
		std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
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

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::EmptyVariant& empty_variant,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::ArrayTypeName& array_type_name,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::FunctionTypePtr& function_pointer_type_name_ptr,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::TupleType& tuple_type_name,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		const Synt::RawPointerType& raw_pointer_type_name,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const std::vector<TemplateBase::TemplateParameter>& template_parameters,
		std::vector<bool>& template_parameters_usage_flags );

	TemplateSignatureParam ValueToTemplateParam( const Value& value, NamesScope& names_scope );

	// Resolve as deep, as can, but does not instantiate last component, if it is template.
	Value ResolveForTemplateSignatureParameter(
		const Synt::ComplexName& signature_parameter,
		NamesScope& names_scope );

	// Returns "true" if all ok.
	bool MatchTemplateArg(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::TypeParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::VariableParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::TemplateParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::ArrayParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::TupleParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::RawPointerParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::FunctionParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::SpecializedTemplateParam& template_param );

	// Returns nullptr in case of fail.
	Value* GenTemplateType(
		const SrcLoc& src_loc,
		const TypeTemplatesSet& type_templates_set,
		const std::vector<Synt::Expression>& template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	// Returns nullptr in case of fail.
	TemplateTypePreparationResult PrepareTemplateType(
		const SrcLoc& src_loc,
		const TypeTemplatePtr& type_template_ptr,
		const std::vector<Value>& template_arguments,
		NamesScope& arguments_names_scope );

	Value* FinishTemplateTypeGeneration(
		const SrcLoc& src_loc,
		NamesScope& arguments_names_scope,
		const TemplateTypePreparationResult& template_type_preparation_result );

	const FunctionVariable* GenTemplateFunction(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr,
		const ArgsVector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this );

	TemplateFunctionPreparationResult PrepareTemplateFunction(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr,
		const ArgsVector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this );

	const FunctionVariable* FinishTemplateFunctionParametrization(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr );

	const FunctionVariable* FinishTemplateFunctionGeneration(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const TemplateFunctionPreparationResult& template_function_preparation_result );

	Value* ParametrizeFunctionTemplate(
		const SrcLoc& src_loc,
		const OverloadedFunctionsSet& functions_set,
		const std::vector<Synt::Expression>& template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	bool TypeIsValidForTemplateVariableArgument( const Type& type );

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
		const SrcLoc& src_loc,
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
		const SrcLoc& src_loc );

	void CallDestructors(
		const StackVariablesStorage& stack_variables_storage,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const SrcLoc& src_loc );

	void CallDestructor(
		llvm::Value* ptr,
		const Type& type,
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	void CallDestructorsForLoopInnerVariables( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );
	void CallDestructorsBeforeReturn( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );
	void CallMembersDestructors( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

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
		const SrcLoc& src_loc );

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
	Value BuildExpressionCodeAndDestroyTemporaries(
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Variable BuildExpressionCodeEnsureVariable(
		const Synt::Expression& expression,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildExpressionCode( const Synt::Expression& expression, NamesScope& names, FunctionContext& function_context );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::EmptyVariant& expression );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::CallOperator& call_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::IndexationOperator& indexation_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::MemberAccessOperator& member_access_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::UnaryMinus& unary_minus );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::UnaryPlus& unary_plus );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::LogicalNot& logical_not );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::BitwiseNot& bitwise_not );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::BinaryOperator& binary_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::ComplexName& named_operand );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::TernaryOperator& ternary_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::NumericConstant& numeric_constant );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::BooleanConstant& boolean_constant );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::StringLiteral& string_literal );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::MoveOperator& move_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::TakeOperator& move_operator );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::CastMut& cast_mut );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::CastImut& cast_imut );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::CastRef& cast_ref );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::CastRefUnsafe& cast_ref_unsafe );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::TypeInfo& typeinfo );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::ArrayTypeName& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::FunctionTypePtr& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::TupleType& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::RawPointerType& type_name );

	// Returns Value, if overloaded operator selected or if arguments are template dependent or argumens are error values.
	// Returns std::nullopt, if all ok, but there is no overloaded operator.
	// In success call of overloaded operator arguments evaluated in left to right order.
	std::optional<Value> TryCallOverloadedBinaryOperator(
		OverloadedOperator op,
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		bool evaluate_args_in_reverse_order,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Value CallBinaryOperatorForArrayOrTuple(
		OverloadedOperator op,
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	std::optional<Value> TryCallOverloadedUnaryOperator(
		const Variable& variable,
		OverloadedOperator op,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	std::optional<Value> TryCallOverloadedPostfixOperator(
		const Variable& variable,
		const llvm::ArrayRef<Synt::Expression>& synt_args,
		OverloadedOperator op,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Value BuildBinaryArithmeticOperatorForRawPointers(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );
		
	Value BuildLazyBinaryOperator(
		const Synt::Expression& l_expression,
		const Synt::Expression& r_expression,
		const Synt::BinaryOperator& binary_operator,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Value DoReferenceCast(
		const SrcLoc& src_loc,
		const Synt::TypeName& type_name,
		const Synt::Expression& expression,
		bool enable_unsafe,
		NamesScope& names,
		FunctionContext& function_context );

	Value CallFunction(
		const Value& function_value,
		const std::vector<Synt::Expression>& synt_args,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Value DoCallFunction(
		llvm::Value* function,
		const Function& function_type,
		const SrcLoc& call_src_loc,
		const Variable* this_, // optional
		const llvm::ArrayRef<const Synt::Expression*>& args,
		bool evaluate_args_in_reverse_order,
		NamesScope& names,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	Value DoCallFunction(
		llvm::Value* function,
		const Function& function_type,
		const SrcLoc& call_src_loc,
		const llvm::ArrayRef<Variable>& preevaluated_args,
		const llvm::ArrayRef<const Synt::Expression*>& args,
		bool evaluate_args_in_reverse_order,
		NamesScope& names,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	Variable BuildTempVariableConstruction(
		const Type& type,
		const std::vector<Synt::Expression>& synt_args,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Variable ConvertVariable(
		const Variable& variable,
		const Type& dst_type,
		const FunctionVariable& conversion_constructor,
		NamesScope& names,
		FunctionContext& function_context,
		const SrcLoc& src_loc );

	// Preevaluate expresion to know it's extened type.
	// Call this only inside save/state restore calls.
	Function::Arg PreEvaluateArg( const Synt::Expression& expression, NamesScope& names, FunctionContext& function_context );
	Function::Arg GetArgExtendedType( const Variable& variable );

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
	BlockBuildInfo BuildBlockElement( NamesScope& names, FunctionContext& function_context, const Synt::BlockElement& blocK_element );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::Block& block );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::VariablesDeclaration& variables_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::AutoVariableDeclaration& auto_variable_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::ReturnOperator& return_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::ForOperator& for_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::CStyleForOperator& c_style_for_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::WhileOperator& while_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::BreakOperator& break_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::ContinueOperator& continue_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::WithOperator& with_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::IfOperator& if_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::StaticIfOperator& static_if_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::SingleExpressionOperator& single_expression_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::AssignmentOperator& assignment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::AdditiveAssignmentOperator& additive_assignment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::IncrementOperator& increment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::DecrementOperator& decrement_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::StaticAssert& static_assert_ );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::Halt& halt );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::HaltIf& halt_if );

	BlockBuildInfo BuildBlock( NamesScope& names, FunctionContext& function_context, const Synt::Block& block );

	// ++ and -- operations
	void BuildDeltaOneOperatorCode(
		const Synt::Expression& expression,
		const SrcLoc& src_loc,
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
		const SrcLoc& src_loc );

	const FunctionVariable* GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		const ArgsVector<Function::Arg>& actual_args,
		bool first_actual_arg_is_this,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		bool produce_errors= true,
		bool enable_type_conversions= true);

	const FunctionVariable* GetOverloadedOperator(
		const ArgsVector<Function::Arg>& actual_args,
		OverloadedOperator op,
		NamesScope& names,
		const SrcLoc& src_loc );

	const FunctionVariable* GetConversionConstructor(
		const Type& src_type,
		const Type& dst_type,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	const TemplateTypePreparationResult* SelectTemplateType(
		const std::vector<TemplateTypePreparationResult>& candidate_templates,
		size_t arg_count );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.
	llvm::Constant* ApplyInitializer( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::Initializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::EmptyVariant& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::SequenceInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::StructNamedInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::ConstructorInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::Expression& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::ZeroInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const Variable& variable, NamesScope& names, FunctionContext& function_context, const Synt::UninitializedInitializer& uninitialized_initializer );

	void ApplyEmptyInitializer(
		const std::string& variable_name,
		const SrcLoc& src_loc,
		const Variable& variable,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const Variable& variable,
		const std::vector<Synt::Expression>& synt_args,
		const SrcLoc& src_loc,
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

	void SetupReferencesInCopyOrMove( FunctionContext& function_context, const Variable& dst_variable, const Variable& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	ReferencesGraph MergeVariablesStateAfterIf(
		const std::vector<ReferencesGraph>& bracnhes_variables_state,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	bool IsReferenceAllowedForReturn( FunctionContext& function_context, const ReferencesGraphNodePtr& variable_node );

	void CheckReferencesPollutionBeforeReturn(
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// NamesScope fill

	void NamesScopeFill( const Synt::ProgramElements& namespace_elements, NamesScope& names_scope );
	void NamesScopeFill( const Synt::NamespacePtr& namespace_, NamesScope& names_scope );
	void NamesScopeFill( const Synt::VariablesDeclaration& variables_declaration , NamesScope& names_scope );
	void NamesScopeFill( const Synt::AutoVariableDeclaration& variable_declaration, NamesScope& names_scope );
	void NamesScopeFill( const Synt::FunctionPtr& function_declaration, NamesScope& names_scope, const ClassProxyPtr& base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( const Synt::FunctionTemplate& function_template_declaration, NamesScope& names_scope, const ClassProxyPtr& base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	ClassProxyPtr NamesScopeFill( const Synt::ClassPtr& class_declaration, NamesScope& names_scope, const std::string& override_name= "" );
	void NamesScopeFill( const Synt::TypeTemplate& type_template_declaration, NamesScope& names_scope, const ClassProxyPtr& base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( const Synt::Enum& enum_declaration, NamesScope& names_scope );
	void NamesScopeFill( const Synt::TypeAlias& type_alias_declaration, NamesScope& names_scope );
	void NamesScopeFill( const Synt::StaticAssert& static_assert_, NamesScope& names_scope );
	void NamesScopeFillOutOfLineElements( const Synt::ProgramElements& namespace_elements, NamesScope& names_scope );

	// Global things build

	bool IsTypeComplete( const Type& type ) const;
	bool EnsureTypeComplete( const Type& type ); // Returns true, if complete
	bool ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc ); // Returns true of all ok. If types are different can call EnsureTypeCompleteness.

	void GlobalThingBuildNamespace( NamesScope& names_scope );
	void GlobalThingBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, bool build_body );
	void GlobalThingBuildClass( ClassProxyPtr class_type );
	void GlobalThingBuildEnum( const EnumPtr enum_ );
	void GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set );
	void GlobalThingBuildTypedef( NamesScope& names_scope, Value& typedef_value );
	void GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value );
	size_t GlobalThingDetectloop( const GlobalThing& global_thing ); // returns loop start index or ~0u
	void GlobalThingReportAboutLoop( size_t loop_start_stack_index, const std::string& last_loop_element_name, const SrcLoc& last_loop_element_src_loc );

	// Debug info

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
	llvm::DICompositeType* CreateDIType( const Array& type );
	llvm::DICompositeType* CreateDIType( const Tuple& type );
	llvm::DISubroutineType* CreateDIType( const Function& type );
	llvm::DIDerivedType* CreateDIType( const RawPointer& type );
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

		llvm::StructType* void_;
		llvm::Type* void_for_ret;
		llvm::Type* invalid_type;
		llvm::IntegerType* bool_;

		llvm::IntegerType* int_ptr; // Type with width of pointer.
	} fundamental_llvm_types_;

	llvm::Function* halt_func_= nullptr;

	Type invalid_type_;
	Type void_type_;
	Type bool_type_;
	Type size_type_; // Alias for u32 or u64

	ConstexprFunctionEvaluator constexpr_function_evaluator_;
	Mangler mangler_;

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
