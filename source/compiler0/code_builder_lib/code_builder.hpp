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
#include "../../code_builder_lib_common/interpreter.hpp"
#include "../../code_builder_lib_common/mangling.hpp"
#include "class.hpp"
#include "debug_info_builder.hpp"
#include "enum.hpp"
#include "function_context.hpp"
#include "mangling.hpp"
#include "tbaa_metadata_builder.hpp"
#include "template_signature_param.hpp"
#include "template_types.hpp"

namespace U
{

struct CodeBuilderOptions
{
	bool build_debug_info= false;
	bool create_lifetimes= true;
	bool generate_lifetime_start_end_debug_calls= false;
	bool generate_tbaa_metadata= false;
	bool report_about_unused_names= true;
	ManglingScheme mangling_scheme= ManglingScheme::ItaniumABI;
};

struct TypeinfoPartVariable
{
	Type type;
	llvm::Constant* constexpr_value= nullptr;
};

class CodeBuilder
{
public:
	CodeBuilder(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		const llvm::Triple& target_triple,
		const CodeBuilderOptions& options= CodeBuilderOptions() );

	struct BuildResult
	{
		std::vector<CodeBuilderError> errors;
		std::unique_ptr<llvm::Module> module;
	};
	BuildResult BuildProgram( const SourceGraph& source_graph );

private:
	using ClassesMembersNamespacesTable= std::unordered_map<ClassPtr, std::shared_ptr<const NamesScope>>;
	struct SourceBuildResult
	{
		std::unique_ptr<NamesScope> names_map;
		ClassesMembersNamespacesTable classes_members_namespaces_table;
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

		GlobalThing( const void* const in_thing_ptr, std::string in_name, const SrcLoc& in_src_loc )
			: thing_ptr(in_thing_ptr), name(std::move(in_name)), src_loc(in_src_loc)
		{}
	};

private:
	void BuildSourceGraphNode( const SourceGraph& source_graph, size_t node_index );

	void MergeNameScopes(
		NamesScope& dst,
		const NamesScope& src,
		const ClassesMembersNamespacesTable& src_classes_members_namespaces_table );

	void FillGlobalNamesScope( NamesScope& global_names_scope );

	// Function context required for accesing local constexpr variables.
	Type PrepareType( const Synt::TypeName& type_name, NamesScope& names_scope, FunctionContext& function_context );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::EmptyVariant& type_nam );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ArrayTypeName& array_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionTypePtr& function_type_name_ptr );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TupleType& tuple_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RawPointerType& raw_pointer_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::GeneratorTypePtr& generator_type_name_ptr );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ComplexName& named_type_name );

	FunctionPointerType FunctionTypeToPointer( FunctionType function_type );

	// Getting LLVM function type may require building complete types for arguments/return value.
	llvm::FunctionType* GetLLVMFunctionType( const FunctionType& function_type );
	llvm::CallingConv::ID GetLLVMCallingConvention(
		const std::optional<std::string>& calling_convention_name,
		const SrcLoc& src_loc,
		CodeBuilderErrorsContainer& errors );

	// Requires return type to be complete.
	static bool FunctionTypeIsSRet( const FunctionType& function_type );

	// Returns scalar type, if this is a scalar type of a composite type, containing (recursively) such type.
	// Returns null otherwise.
	// Requires type to be complete.
	static llvm::Type* GetSingleScalarType( llvm::Type* type );

	// Virtual stuff
	void CheckvirtualFunctionOverridingReferenceNotation(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionVariable& src_function,
		const FunctionVariable& new_function );
	void PrepareClassVirtualTable( Class& the_class );
	void PrepareClassVirtualTableType( ClassPtr class_type );

	void BuildPolymorphClassTypeId( ClassPtr class_type );

	llvm::Constant* BuildClassVirtualTable_r( const Class& ancestor_class, const Class& dst_class, uint64_t offset );
	void BuildClassVirtualTable( ClassPtr class_type );

	std::pair<VariablePtr, llvm::Value*> TryFetchVirtualFunction(
		const VariablePtr& this_,
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

	// NonSync stuff

	bool GetTypeNonSync( const Type& type, NamesScope& names_scope, const SrcLoc& src_loc );
	bool GetTypeNonSyncImpl( llvm::SmallVectorImpl<Type>& prev_types_stack, const Type& type, NamesScope& names_scope, const SrcLoc& src_loc );
	bool ImmediateEvaluateNonSyncTag( NamesScope& names, FunctionContext& function_context, const Synt::NonSyncTag& non_sync_tag );
	void CheckClassNonSyncTagExpression( ClassPtr class_type );
	void CheckClassNonSyncTagInheritance( ClassPtr class_type );

	// Templates
	void PrepareTypeTemplate(
		const Synt::TypeTemplate& type_template_declaration,
		TypeTemplatesSet& type_templates_set,
		NamesScope& names_scope );

	void PrepareFunctionTemplate(
		const Synt::FunctionTemplate& function_template_declaration,
		OverloadedFunctionsSet& functions_set,
		NamesScope& names_scope,
		ClassPtr base_class );

	void ProcessTemplateParams(
		llvm::ArrayRef<Synt::TemplateBase::Param> params,
		NamesScope& names_scope,
		const SrcLoc& src_loc,
		std::vector<TypeTemplate::TemplateParameter>& template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::ComplexName& signature_parameter );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::Expression& template_parameter );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::TypeName& template_parameter );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::EmptyVariant& empty_variant );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::ArrayTypeName& array_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::FunctionTypePtr& function_pointer_type_name_ptr );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::TupleType& tuple_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::RawPointerType& raw_pointer_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateBase::TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::GeneratorTypePtr& generator_type_name_ptr );

	TemplateSignatureParam ValueToTemplateParam( const Value& value, NamesScope& names_scope, const SrcLoc& src_loc );

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
		const TemplateSignatureParam::CoroutineParam& template_param );

	bool MatchTemplateArgImpl(
		const TemplateBase& template_,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const SrcLoc& src_loc,
		const TemplateSignatureParam::SpecializedTemplateParam& template_param );

	// Returns nullptr in case of fail.
	NamesScopeValue* GenTemplateType(
		const SrcLoc& src_loc,
		const TypeTemplatesSet& type_templates_set,
		llvm::ArrayRef<Synt::Expression> template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	// Returns nullptr in case of fail.
	TemplateTypePreparationResult PrepareTemplateType(
		const SrcLoc& src_loc,
		const TypeTemplatePtr& type_template_ptr,
		llvm::ArrayRef<Value> template_arguments,
		NamesScope& arguments_names_scope );

	NamesScopeValue* FinishTemplateTypeGeneration(
		const SrcLoc& src_loc,
		NamesScope& arguments_names_scope,
		const TemplateTypePreparationResult& template_type_preparation_result );

	const FunctionVariable* GenTemplateFunction(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr,
		const ArgsVector<FunctionType::Param>& actual_args,
		bool first_actual_arg_is_this );

	TemplateFunctionPreparationResult PrepareTemplateFunction(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr,
		const ArgsVector<FunctionType::Param>& actual_args,
		bool first_actual_arg_is_this );

	const FunctionVariable* FinishTemplateFunctionParametrization(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr );

	const FunctionVariable* FinishTemplateFunctionGeneration(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const TemplateFunctionPreparationResult& template_function_preparation_result );

	NamesScopeValue* ParametrizeFunctionTemplate(
		const SrcLoc& src_loc,
		const OverloadedFunctionsSet& functions_set,
		llvm::ArrayRef<Synt::Expression> template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	bool TypeIsValidForTemplateVariableArgument( const Type& type );

	void FillKnownFunctionTemplateArgsIntoNamespace(
		const FunctionTemplate& function_template,
		NamesScope& target_namespace,
		const SrcLoc& src_loc );

	NamesScopeValue* AddNewTemplateThing( std::string key, NamesScopeValue thing );

	// Constructors/destructors
	void TryGenerateDefaultConstructor( ClassPtr class_type );
	void TryGenerateCopyConstructor( ClassPtr class_type );
	FunctionVariable GenerateDestructorPrototype( ClassPtr class_type );
	void GenerateDestructorBody( ClassPtr class_type, FunctionVariable& destructor_function );
	void TryGenerateDestructor( ClassPtr class_type );
	void TryGenerateCopyAssignmentOperator( ClassPtr class_type );
	void TryGenerateEqualityCompareOperator( ClassPtr class_type );

	// Sets "constexpr" flag for method and checks for errors.
	void ProcessGeneratedMethodConstexprFlag( ClassPtr class_type, FunctionContext& function_context_after_body_generation, FunctionVariable& method );

	void BuildCopyConstructorPart(
		llvm::Value* dst, llvm::Value* src,
		const Type& type,
		FunctionContext& function_context );

	void BuildCopyAssignmentOperatorPart(
		llvm::Value* dst, llvm::Value* src,
		const Type& type,
		FunctionContext& function_context );

	void BuildEqualityCompareOperatorPart(
		llvm::Value* l_address, llvm::Value* r_address,
		const Type& type,
		llvm::BasicBlock* false_basic_block,
		FunctionContext& function_context );

	void CopyBytes(
		llvm::Value* dst, llvm::Value* src,
		const Type& type,
		FunctionContext& function_context );

	llvm::Constant* ConstexprCompareEqual(
		llvm::Constant* l,
		llvm::Constant* r,
		const Type& type,
		NamesScope& names,
		const SrcLoc& src_loc );

	void MoveConstantToMemory(
		const Type& type,
		llvm::Value* ptr, llvm::Constant* constant,
		FunctionContext& function_context );

	static llvm::Constant* WrapRawScalarConstant( llvm::Constant* constant, llvm::Type* dst_type );
	static llvm::Constant* UnwrapRawScalarConstant( llvm::Constant* constant );

	void TryCallCopyConstructor(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		llvm::Value* this_, llvm::Value* src,
		ClassPtr class_type,
		FunctionContext& function_context );

	bool IsDefaultConstructor( const FunctionType& function_type, const Type& base_class );
	bool IsCopyConstructor( const FunctionType& function_type, const Type& base_class );
	bool IsCopyAssignmentOperator( const FunctionType& function_type, const Type& base_class );
	bool IsEqualityCompareOperator( const FunctionType& function_type, const Type& base_class );

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

	// TODO - avoid passing errors container?
	void CallDestructor(
		llvm::Value* ptr,
		const Type& type,
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	void CallDestructorsForLoopInnerVariables( NamesScope& names_scope, FunctionContext& function_context, size_t stack_variables_stack_size, const SrcLoc& src_loc );
	void CallDestructorsBeforeReturn( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );
	void CallMembersDestructors( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void CheckForUnusedNames( const NamesScope& names_scope );

	// Returns index of function in set, if function successfuly prepared and inserted. Returns ~0 on fail.
	size_t PrepareFunction(
		NamesScope& names_scope,
		ClassPtr base_class,
		OverloadedFunctionsSet& functions_set,
		const Synt::Function& function_declaration,
		bool is_out_of_line_function );

	void CheckOverloadedOperator(
		ClassPtr base_class,
		const FunctionType& func_type,
		OverloadedOperator overloaded_operator,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// Returns type of return value.
	Type BuildFuncCode(
		FunctionVariable& func,
		ClassPtr base_class,
		NamesScope& parent_names_scope,
		std::string_view func_name,
		const Synt::FunctionParams& params,
		const Synt::Block& block,
		const Synt::StructNamedInitializer* constructor_initialization_list );

	// Expressions.
	VariablePtr BuildExpressionCodeEnsureVariable(
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
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::NonSyncExpression& non_sync_expression );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::SafeExpression& safe_expression );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::UnsafeExpression& unsafe_expression );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::ArrayTypeName& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::FunctionTypePtr& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::TupleType& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::RawPointerType& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names, FunctionContext& function_context, const Synt::GeneratorTypePtr& type_name );

	VariablePtr AccessClassBase( const VariablePtr& variable, FunctionContext& function_context );
	Value AccessClassField(
		NamesScope& names,
		FunctionContext& function_context,
		const VariablePtr& variable,
		const ClassField& field,
		const std::string& field_name,
		const SrcLoc& src_loc );

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
		const VariablePtr& variable,
		OverloadedOperator op,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	std::optional<Value> TryCallOverloadedPostfixOperator(
		const VariablePtr& variable,
		llvm::ArrayRef<Synt::Expression> synt_args,
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
		llvm::ArrayRef<Synt::Expression> synt_args,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	Value DoCallFunction(
		llvm::Value* function,
		const FunctionType& function_type,
		const SrcLoc& call_src_loc,
		const VariablePtr& this_, // optional
		llvm::ArrayRef<const Synt::Expression*> args,
		bool evaluate_args_in_reverse_order,
		NamesScope& names,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	Value DoCallFunction(
		llvm::Value* function,
		const FunctionType& function_type,
		const SrcLoc& call_src_loc,
		llvm::ArrayRef<VariablePtr> preevaluated_args,
		llvm::ArrayRef<const Synt::Expression*> args,
		bool evaluate_args_in_reverse_order,
		NamesScope& names,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	VariablePtr BuildTempVariableConstruction(
		const Type& type,
		llvm::ArrayRef<Synt::Expression> synt_args,
		const SrcLoc& src_loc,
		NamesScope& names,
		FunctionContext& function_context );

	VariablePtr ConvertVariable(
		VariablePtr variable,
		const Type& dst_type,
		const FunctionVariable& conversion_constructor,
		NamesScope& names,
		FunctionContext& function_context,
		const SrcLoc& src_loc );

	bool EvaluateBoolConstantExpression( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression );

	// Preevaluate expresion to know it's extened type.
	// Call this only inside save/state restore calls.
	FunctionType::Param PreEvaluateArg( const Synt::Expression& expression, NamesScope& names, FunctionContext& function_context );
	FunctionType::Param GetArgExtendedType( const Variable& variable );

	// Typeinfo

	VariablePtr BuildTypeInfo( const Type& type, NamesScope& root_namespace );
	ClassPtr CreateTypeinfoClass( NamesScope& root_namespace, const Type& src_type, std::string name );
	VariableMutPtr BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace );
	void BuildFullTypeinfo( const Type& type, const VariableMutPtr& typeinfo_variable, NamesScope& root_namespace );
	const Variable& GetTypeinfoListEndNode( NamesScope& root_namespace );
	void FinishTypeinfoClass( ClassPtr class_type, const ClassFieldsVector<llvm::Type*>& fields_llvm_types );
	TypeinfoPartVariable BuildTypeinfoEnumElementsList( EnumPtr enum_type, NamesScope& root_namespace );
	void CreateTypeinfoClassMembersListNodeCommonFields(
		const Class& class_, ClassPtr node_class_type,
		std::string_view member_name,
		ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers );
	TypeinfoPartVariable BuildTypeinfoClassFieldsList( ClassPtr class_type, NamesScope& root_namespace );
	TypeinfoPartVariable BuildTypeinfoClassTypesList( ClassPtr class_type, NamesScope& root_namespace );
	TypeinfoPartVariable BuildTypeinfoClassFunctionsList( ClassPtr class_type, NamesScope& root_namespace );
	TypeinfoPartVariable BuildTypeinfoClassParentsList( ClassPtr class_type, NamesScope& root_namespace );
	TypeinfoPartVariable BuildTypeinfoFunctionArguments( const FunctionType& function_type, NamesScope& root_namespace );
	TypeinfoPartVariable BuildTypeinfoTupleElements( const TupleType& tuple_type, NamesScope& root_namespace );

	// Block elements
	BlockBuildInfo BuildIfAlternative( NamesScope& names, FunctionContext& function_context, const Synt::IfAlternative& if_alterntative );
	BlockBuildInfo BuildBlockElement( NamesScope& names, FunctionContext& function_context, const Synt::BlockElement& block_element );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::Block& block );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::ScopeBlock& block );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::VariablesDeclaration& variables_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::AutoVariableDeclaration& auto_variable_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::ReturnOperator& return_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::YieldOperator& yield_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::RangeForOperator& range_for_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::CStyleForOperator& c_style_for_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::WhileOperator& while_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::LoopOperator& loop_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::BreakOperator& break_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::ContinueOperator& continue_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::WithOperator& with_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::IfOperator& if_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::StaticIfOperator& static_if_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::IfCoroAdvanceOperator& if_coro_advance );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::SwitchOperator& switch_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::SingleExpressionOperator& single_expression_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::AssignmentOperator& assignment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::AdditiveAssignmentOperator& additive_assignment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::IncrementOperator& increment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::DecrementOperator& decrement_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::StaticAssert& static_assert_ );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::TypeAlias& type_alias );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::Halt& halt );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names, FunctionContext& function_context, const Synt::HaltIf& halt_if );

	BlockBuildInfo BuildBlock( NamesScope& names, FunctionContext& function_context, const Synt::Block& block );
	// Build elements, withut creating separate names scope.
	BlockBuildInfo BuildBlockElements( NamesScope& names, FunctionContext& function_context, const Synt::BlockElements& block_elements );

	void BuildEmptyReturn( NamesScope& names, FunctionContext& function_context, const SrcLoc& src_loc );

	void AddLoopFrame(
		NamesScope& names,
		FunctionContext& function_context,
		llvm::BasicBlock* break_block,
		llvm::BasicBlock* continue_block,
		const std::optional<Synt::Label>& label );

	// Returns nullptr if not found.
	LoopFrame* FetchLoopFrame( NamesScope& names, FunctionContext& function_context, const std::optional<Synt::Label>& label );

	// ++ and -- operations
	void BuildDeltaOneOperatorCode(
		const Synt::Expression& expression,
		const SrcLoc& src_loc,
		bool positive, // true - increment, false - decrement
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names, FunctionContext& function_context );

	//
	// Name resolving.
	//

	Value ResolveValue( NamesScope& names_scope, FunctionContext& function_context, const Synt::ComplexName& complex_name );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup& root_namespace_lookup );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup& name_lookup );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch& names_scope_fetch );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TemplateParametrization& template_parametrization );

	void BuildGlobalThingDuringResolveIfNecessary( NamesScope& names_scope, NamesScopeValue* value );

	struct NameLookupResult
	{
		// Namespace where this value is located. Needed in order to build some values (like template sets).
		// May be empty.
		NamesScope* space= nullptr;
		// Value pointer itself. Should be stable pointer (inside some namespace, usually).
		// Empty if not found.
		NamesScopeValue* value= nullptr;
	};

	// Try to lookup value from names scope. If it is not found - try to lookup it from parent scope, than from parent of parent, etc.
	// Do not perform name build.
	NameLookupResult LookupName( NamesScope& names_scope, std::string_view name, const SrcLoc& src_loc );

	std::pair<NamesScopeValue*, ClassMemberVisibility> ResolveClassValue( ClassPtr class_type, std::string_view name );
	std::pair<NamesScopeValue*, ClassMemberVisibility> ResolveClassValueImpl( ClassPtr class_type, std::string_view name, bool recursive_call= false );

	// Functions

	FunctionVariable* GetFunctionWithSameType(
		const FunctionType& function_type,
		OverloadedFunctionsSet& functions_set );

	// Returns "false" on error.
	bool ApplyOverloadedFunction(
		OverloadedFunctionsSet& functions_set,
		const FunctionVariable& function,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	const FunctionVariable* GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		const ArgsVector<FunctionType::Param>& actual_args,
		bool first_actual_arg_is_this,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		bool produce_errors= true,
		bool enable_type_conversions= true);

	const FunctionVariable* GetOverloadedOperator(
		const ArgsVector<FunctionType::Param>& actual_args,
		OverloadedOperator op,
		NamesScope& names,
		const SrcLoc& src_loc );

	const FunctionVariable* GetConversionConstructor(
		const Type& src_type,
		const Type& dst_type,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	const TemplateTypePreparationResult* SelectTemplateType(
		llvm::ArrayRef<TemplateTypePreparationResult> candidate_templates,
		size_t arg_count );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.
	llvm::Constant* ApplyInitializer( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::Initializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::EmptyVariant& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::SequenceInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::StructNamedInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::ConstructorInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::Expression& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::ZeroInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names, FunctionContext& function_context, const Synt::UninitializedInitializer& uninitialized_initializer );

	llvm::Constant* ApplyEmptyInitializer(
		std::string_view variable_name,
		const SrcLoc& src_loc,
		VariablePtr variable,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const VariablePtr& variable,
		llvm::ArrayRef<Synt::Expression> synt_args,
		const SrcLoc& src_loc,
		NamesScope& block_names,
		FunctionContext& function_context );

	void BuildConstructorInitialization(
		const VariablePtr& this_,
		const Class& base_class,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const Synt::StructNamedInitializer& constructor_initialization_list );

	llvm::Constant* InitializeReferenceField(
		const VariablePtr& variable,
		const ClassField& field,
		const Synt::Initializer& initializer,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* InitializeFunctionPointer(
		const VariablePtr& variable,
		const Synt::Expression& initializer_expression,
		NamesScope& block_names,
		FunctionContext& function_context );

	llvm::Constant* InitializeClassFieldWithInClassIninitalizer(
		const VariablePtr& field_variable,
		const ClassField& class_field,
		FunctionContext& function_context );

	llvm::Constant* InitializeReferenceClassFieldWithInClassIninitalizer(
		VariablePtr variable,
		const ClassField& class_field,
		FunctionContext& function_context );

	void CheckClassFieldsInitializers( ClassPtr class_type );

	// Reference-checking.
	void ProcessFunctionParamReferencesTags(
		const Synt::FunctionType& func,
		FunctionType& function_type,
		const Synt::FunctionParam& in_arg,
		const FunctionType::Param& out_arg,
		size_t arg_number );

	void ProcessFunctionReturnValueReferenceTags(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		const FunctionType& function_type );

	void TryGenerateFunctionReturnReferencesMapping(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		FunctionType& function_type );

	void ProcessFunctionReferencesPollution(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::Function& func,
		FunctionType& function_type,
		ClassPtr base_class );

	void ProcessFunctionTypeReferencesPollution(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::FunctionType& func,
		FunctionType& function_type );

	void SetupReferencesInCopyOrMove( FunctionContext& function_context, const VariablePtr& dst_variable, const VariablePtr& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void RegisterTemporaryVariable( FunctionContext& function_context, VariablePtr variable );
	void DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	ReferencesGraph MergeVariablesStateAfterIf(
		llvm::ArrayRef<ReferencesGraph> bracnhes_variables_state,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	bool IsReferenceAllowedForReturn( FunctionContext& function_context, const VariablePtr& variable_node );

	void CheckReferencesPollutionBeforeReturn(
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// Coroutines

	ClassPtr GetGeneratorFunctionReturnType( NamesScope& root_namespace, const FunctionType& generator_function_type, bool non_sync );
	std::set<FunctionType::ParamReference> GetGeneratorFunctionReturnReferences( const FunctionType& generator_function_type );

	ClassPtr GetCoroutineType( NamesScope& root_namespace, const CoroutineTypeDescription& coroutine_type_description );

	// This function should be called for generator function just after aruments preparation.
	void PrepareGeneratorBlocks( FunctionContext& function_context );
	void GeneratorYield( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc );
	void GeneratorSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );
	void GeneratorFinalSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );

	// NamesScope fill

	void NamesScopeFill( NamesScope& names_scope, const Synt::ProgramElements& namespace_elements );
	void NamesScopeFill( NamesScope& names_scope, const Synt::NamespacePtr& namespace_ );
	void NamesScopeFill( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::AutoVariableDeclaration& variable_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::FunctionPtr& function_declaration, ClassPtr base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_declaration, ClassPtr base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	ClassPtr NamesScopeFill( NamesScope& names_scope, const Synt::ClassPtr& class_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::TypeTemplate& type_template_declaration, ClassPtr base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Enum& enum_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::TypeAlias& type_alias_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::StaticAssert& static_assert_ );
	void NamesScopeFillOutOfLineElements( NamesScope& names_scope, const Synt::ProgramElements& namespace_elements );

	// Global things build

	bool IsTypeComplete( const Type& type ) const;
	bool EnsureTypeComplete( const Type& type ); // Returns true, if complete
	bool ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc ); // Returns true of all ok. If types are different can call EnsureTypeCompleteness.

	void GlobalThingBuildNamespace( NamesScope& names_scope );
	void GlobalThingBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, bool build_body );
	void GlobalThingPrepareClassParentsList( ClassPtr class_type );
	void GlobalThingBuildClass( ClassPtr class_type );
	void GlobalThingBuildEnum( EnumPtr enum_ );
	void GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set );
	void GlobalThingBuildTypedef( NamesScope& names_scope, Value& typedef_value );
	void GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value );
	size_t GlobalThingDetectloop( const GlobalThing& global_thing ); // returns loop start index or ~0u
	void GlobalThingReportAboutLoop( size_t loop_start_stack_index, std::string_view last_loop_element_name, const SrcLoc& last_loop_element_src_loc );

	// Other stuff

	llvm::Type* GetFundamentalLLVMType( U_FundamentalType fundmantal_type );

	llvm::Value* CreateTypedLoad( FunctionContext& function_context, const Type& type, llvm::Value* address );
	llvm::LoadInst* CreateTypedReferenceLoad( FunctionContext& function_context, const Type& type, llvm::Value* address );
	void CreateTypedStore( FunctionContext& function_context, const Type& type, llvm::Value* value_to_store, llvm::Value* address );
	void CreateTypedReferenceStore( FunctionContext& function_context, const Type& type, llvm::Value* value_to_store, llvm::Value* address );

	// If variable is on stack, creates move to rigister instruction.
	// If variable already in register - does nothing.
	llvm::Value* CreateMoveToLLVMRegisterInstruction( const Variable& variable, FunctionContext& function_context );

	llvm::Constant* GetZeroGEPIndex();
	llvm::Constant* GetFieldGEPIndex( uint64_t field_index );

	llvm::Value* CreateBaseClassGEP( FunctionContext& function_context, const Class& class_type, llvm::Value* class_ptr );
	llvm::Value* CreateClassFieldGEP( FunctionContext& function_context, const Variable& class_variable, uint64_t field_index );
	llvm::Value* CreateClassFieldGEP( FunctionContext& function_context, const Class& class_type, llvm::Value* class_ptr, uint64_t field_index );
	llvm::Value* CreateTupleElementGEP( FunctionContext& function_context, const Variable& tuple_variable, uint64_t element_index );
	llvm::Value* CreateTupleElementGEP( FunctionContext& function_context, const TupleType& tuple_type, llvm::Value* tuple_ptr, uint64_t element_index );
	llvm::Value* CreateArrayElementGEP( FunctionContext& function_context, const Variable& array_variable, uint64_t element_index );
	llvm::Value* CreateArrayElementGEP( FunctionContext& function_context, const Variable& array_variable, llvm::Value* index );
	llvm::Value* CreateArrayElementGEP( FunctionContext& function_context, const ArrayType& array_type, llvm::Value* array_ptr, uint64_t element_index );
	llvm::Value* CreateArrayElementGEP( FunctionContext& function_context, const ArrayType& array_type, llvm::Value* array_ptr, llvm::Value* index );
	llvm::Value* CreateCompositeElementGEP( FunctionContext& function_context, llvm::Type* type, llvm::Value* value, llvm::Value* index );

	// Create GEP instruction even in functionless context.
	llvm::Value* ForceCreateConstantIndexGEP( FunctionContext& function_context, llvm::Type* type, llvm::Value* value, uint32_t index );

	llvm::Value* CreateReferenceCast( llvm::Value* ref, const Type& src_type, const Type& dst_type, FunctionContext& function_context );

	llvm::GlobalVariable* CreateGlobalConstantVariable( const Type& type, std::string_view mangled_name, llvm::Constant* initializer= nullptr );
	llvm::GlobalVariable* CreateGlobalMutableVariable( const Type& type, std::string_view mangled_name, bool externally_available );

	bool IsGlobalVariable( const VariablePtr& variable );

	// Creates LLVM function and its LLVM type lazily. This call may trigger types competion.
	llvm::Function* EnsureLLVMFunctionCreated( const FunctionVariable& function_variable );

	// Requires complete types
	void SetupDereferenceableFunctionParamsAndRetAttributes( FunctionVariable& function_variable );
	void SetupDereferenceableFunctionParamsAndRetAttributes_r( NamesScope& names_scope );

	void CreateLifetimeStart( FunctionContext& function_context, llvm::Value* address );
	void CreateLifetimeEnd( FunctionContext& function_context, llvm::Value* address );

	struct FunctionContextState
	{
		ReferencesGraph variables_state;
		size_t block_count= 0;
	};

	FunctionContextState SaveFunctionContextState( FunctionContext& function_context );
	void RestoreFunctionContextState( FunctionContext& function_context, const FunctionContextState& state );

private:
	llvm::LLVMContext& llvm_context_;
	const llvm::DataLayout data_layout_;
	const llvm::Triple target_triple_;
	const bool build_debug_info_;
	const bool create_lifetimes_;
	const bool generate_lifetime_start_end_debug_calls_;
	const bool generate_tbaa_metadata_;
	const bool report_about_unused_names_;

	struct
	{
		llvm::Type* invalid_type_;

		llvm::StructType* void_;
		llvm::IntegerType* bool_;

		llvm::IntegerType* i8_  ;
		llvm::IntegerType* u8_  ;
		llvm::IntegerType* i16_ ;
		llvm::IntegerType* u16_ ;
		llvm::IntegerType* i32_ ;
		llvm::IntegerType* u32_ ;
		llvm::IntegerType* i64_ ;
		llvm::IntegerType* u64_ ;
		llvm::IntegerType* i128_;
		llvm::IntegerType* u128_;

		llvm::Type* f32_;
		llvm::Type* f64_;

		llvm::IntegerType* char8_ ;
		llvm::IntegerType* char16_;
		llvm::IntegerType* char32_;

		llvm::IntegerType* byte8_  ;
		llvm::IntegerType* byte16_ ;
		llvm::IntegerType* byte32_ ;
		llvm::IntegerType* byte64_ ;
		llvm::IntegerType* byte128_;

		llvm::Type* void_for_ret_;

		llvm::IntegerType* int_ptr; // Type with width of pointer.
	} fundamental_llvm_types_{};

	llvm::Function* halt_func_= nullptr;

	llvm::Function* lifetime_start_debug_func_= nullptr;
	llvm::Function* lifetime_end_debug_func_= nullptr;

	// Allocate/deallocate heap memory in some places (for now in coroutines).
	llvm::Function* malloc_func_= nullptr;
	llvm::Function* free_func_= nullptr;

	Type invalid_type_;
	Type void_type_;
	Type bool_type_;
	Type size_type_; // Alias for u32 or u64
	llvm::PointerType* virtual_function_pointer_type_= nullptr; // Use common type for all function pointers in virtual table - for simplicity.
	llvm::StructType* polymorph_type_id_table_element_type_= nullptr;

	Interpreter constexpr_function_evaluator_;
	const std::shared_ptr<IMangler> mangler_;
	TBAAMetadataBuilder tbaa_metadata_builder_;

	FunctionContext* global_function_context_= nullptr;

	std::unique_ptr<llvm::Module> module_;
	std::vector<CodeBuilderError> global_errors_; // Do not use directly. Use NamesScope::GetErrors() instead.

	std::vector<SourceBuildResult> compiled_sources_;

	// Storage for class types. Do not use shared pointers for classes for loops preventing.
	std::vector< std::unique_ptr<Class> > classes_table_;

	// Storage for enum types. Do not use shared pointers for enums for loops preventing.
	std::vector< std::unique_ptr<Enum> > enums_table_;

	// Cache needs for generating same classes as template instantiation result in different source files.
	// We can use same classes in different files, because template classes are logically unchangeable after instantiation.
	// Unchangeable they are because incomplete template classes ( or classes inside template classes, etc. ) currently forbidden.
	ProgramStringMap< ClassPtr > template_classes_cache_;

	// We needs to generate same typeinfo classes for same types. Use cache for it.
	std::unordered_map< Type, VariableMutPtr, TypeHasher > typeinfo_cache_;
	std::vector<std::unique_ptr<Class>> typeinfo_class_table_;

	std::vector<Type> non_sync_expression_stack_;

	// Names map for generated template types/functions. We can not insert it in regular namespaces, because we needs insert it, while iterating regular namespaces.
	ProgramStringMap<NamesScopeValue> generated_template_things_storage_;
	// Template things for current source graph node added sequentialy into this vector too.
	std::vector<std::string> generated_template_things_sequence_;

	std::vector<GlobalThing> global_things_stack_;

	std::optional<DebugInfoBuilder> debug_info_builder_;

	std::unordered_map<CoroutineTypeDescription, std::unique_ptr<Class>, CoroutineTypeDescriptionHasher> coroutine_classes_table_;
};

using MutabilityModifier= Synt::MutabilityModifier;
using ReferenceModifier= Synt::ReferenceModifier;

} // namespace U
