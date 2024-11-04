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
#include "lambdas.hpp"
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
	bool collect_definition_points= false;
	// Skip building generated methods, functions inside templates.
	// Such option produces incomplete module, but for some cases (testing, language server) it is enough.
	// The main reason for this option to exist is to speed-up code for such purposes.
	bool skip_building_generated_functions= false;
	ManglingScheme mangling_scheme= ManglingScheme::ItaniumABI;
};

class CodeBuilder
{
public:
	struct BuildResult
	{
		std::vector<CodeBuilderError> errors;
		std::unique_ptr<llvm::Module> module;
		std::vector<IVfs::Path> embedded_files;
	};

	using CompletionRequestPrefixComponent= std::variant<
		const Synt::Namespace*,
		const Synt::Class*,
		const Synt::TypeTemplate*>;

	enum class CompletionItemKind : uint8_t
	{
		Variable,
		FunctionsSet,
		Type,
		ClassField,
		NamesScope,
		TypeTemplatesSet,
	};

	struct CompletionItem
	{
		std::string name;
		std::string sort_text;
		std::string detail;
		CompletionItemKind kind= CompletionItemKind::Variable;
	};

	struct SignatureHelpItem
	{
		std::string label;
		// TODO - fill parameters range.
	};

	using SourceGraphPtr= std::shared_ptr<const SourceGraph>;

public:
	// Use static creation methods for building of code, since it is unsafe to reuse internal data structures after building single source graph.

	// Build program and return result. Instance of this class is created and destroyed inside.
	// Use this method for normal compilation.
	static BuildResult BuildProgram(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		const llvm::Triple& target_triple,
		const CodeBuilderOptions& options,
		const SourceGraphPtr& source_graph,
		IVfsSharedPtr vfs );

	// Build program, but leave internal state and LLVM module.
	// Use this for expecting program after its building (in IDE language server, for example).
	static std::unique_ptr<CodeBuilder> BuildProgramAndLeaveInternalState(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		const llvm::Triple& target_triple,
		const CodeBuilderOptions& options,
		const SourceGraphPtr& source_graph,
		IVfsSharedPtr vfs );

public: // IDE helpers.
	CodeBuilderErrorsContainer TakeErrors();

	// Get definition for given location (of some name lexem ).
	// Works only if definition collection is enabled in options.
	std::optional<SrcLoc> GetDefinition( const SrcLoc& src_loc );

	// Get all usage points for specified symbol.
	// For both symbol usage and symvol definition it returns definition point and all unsage points.
	// Result lost is sorted and contains unique entrires.
	std::vector<SrcLoc> GetAllOccurrences( const SrcLoc& src_loc );

	// Try to compile given program element, including internal completion syntax element.
	// Return completion result.
	// Prefix is used to find proper namespace/class (name lookups are used).
	template<typename T>
	std::vector<CompletionItem> Complete( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix, const T& el )
	{
		++completion_request_index_;

		const NamesScopePtr names_scope= GetNamesScopeForCompletion( prefix );
		if( names_scope == nullptr )
			return {};

		BuildElementForCompletionImpl( *names_scope, el );
		return CompletionResultFinalize();
	}

	template<typename T>
	std::vector<SignatureHelpItem> GetSignatureHelp( const llvm::ArrayRef<CompletionRequestPrefixComponent> prefix, const T& el )
	{
		// Use same routines for completion and signature help.
		++completion_request_index_;

		const NamesScopePtr names_scope= GetNamesScopeForCompletion( prefix );
		if( names_scope == nullptr )
			return {};

		BuildElementForCompletionImpl( *names_scope, el );
		return SignatureHelpResultFinalize();
	}

	// Delete bodies of functions (excepth constexpr ones).
	// This breaks result module and should not be used for a program compilation (with result object file).
	// But this is usable for ide helpers in order to reduce memory usage.
	void DeleteFunctionsBodies();

private:
	CodeBuilder(
		llvm::LLVMContext& llvm_context,
		const llvm::DataLayout& data_layout,
		const llvm::Triple& target_triple,
		const CodeBuilderOptions& options,
		IVfsSharedPtr vfs );

	// This function may be called exactly once.
	void BuildProgramInternal( const SourceGraphPtr& source_graph );

	// Run code, necessary for result LLVM module finalization, but not (strictly) necessary for other purposes.
	void FinalizeProgram();

private:
	using ClassesMembersNamespacesTable= std::unordered_map<ClassPtr, std::shared_ptr<const NamesScope>>;
	struct SourceBuildResult
	{
		NamesScopePtr names_map;
		ClassesMembersNamespacesTable classes_members_namespaces_table;
	};

	struct BlockBuildInfo
	{
		bool has_terminal_instruction_inside= false;
	};

	struct TemplateTypePreparationResult
	{
		TypeTemplatePtr type_template;
		NamesScopePtr template_args_namespace;
		TemplateArgs signature_args;
	};

	struct TemplateFunctionPreparationResult
	{
		FunctionTemplatePtr function_template;
		NamesScopePtr template_args_namespace;
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
	void CollectDefinition( const NamesScopeValue& value, const SrcLoc& src_loc );
	void CollectFunctionDefinition( const FunctionVariable& function_variable, const SrcLoc& src_loc );

	NamesScopePtr GetNamesScopeForCompletion( llvm::ArrayRef<CompletionRequestPrefixComponent> prefix );
	NamesScopePtr EvaluateCompletionRequestPrefix_r( const NamesScopePtr& start_scope, llvm::ArrayRef<CompletionRequestPrefixComponent> prefix );
	std::vector<CompletionItem> CompletionResultFinalize();
	std::vector<SignatureHelpItem> SignatureHelpResultFinalize();

	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::AutoVariableDeclaration& auto_variable_declaration );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::StaticAssert& static_assert_ );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeAlias& type_alias );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Enum& enum_ );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Function& function );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Class& class_ptr );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::TypeTemplate& type_template );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::FunctionTemplate& function_template );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Namespace& namespace_ );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassField& class_field );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::ClassVisibilityLabel& class_visibility_label );
	void BuildElementForCompletionImpl( NamesScope& names_scope, const Synt::Mixin& mixin );

	// Performs template instantiation with dummy args and returns names scope, if it is a class template.
	// May reuse value, created during previous dummy-instantiation.
	NamesScopePtr InstantiateTypeTemplateWithDummyArgs( const TypeTemplatePtr& type_template );

	void InstantiateFunctionTemplateWithDummyArgs( const FunctionTemplatePtr& function_template );

	// This function is basically reverse of "MatchTemplateArg".
	TemplateArg CreateDummyTemplateSignatureArg( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam& signature_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::Type& type_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::Variable& variable_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::TypeTemplate& type_template_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::TemplateParam& template_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::Array& array_type_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::Tuple& tuple_type_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::RawPointer& raw_pointer_type_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::Function& function_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::Coroutine& coroutine_param );
	TemplateArg CreateDummyTemplateSignatureArgImpl( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateSignatureParam::SpecializedTemplate& specialized_template_param );

	TemplateArg CreateDummyTemplateSignatureArgForTemplateParam( llvm::ArrayRef<TemplateParameter> template_params, NamesScope& args_names_scope, const TemplateParameter& param );

	Type GetStubTemplateArgType();

	NamesScopePtr EnsureDummyTemplateInstantiationArgsScopeCreated();
	void DummyInstantiateTemplates();
	void DummyInstantiateTemplates_r( NamesScope& names_scope );

	void RootNamespaseLookupCompleteImpl( const NamesScope& names_scope, std::string_view name );
	void NameLookupCompleteImpl( const NamesScope& names_scope, std::string_view name );
	void NamesScopeFetchComleteImpl( const Value& base, std::string_view name );
	void MemberAccessCompleteImpl( const VariablePtr& variable, std::string_view name );
	void NamesScopeFetchComleteForNamesScope( const NamesScope& names_scope, std::string_view name );
	void NamesScopeFetchComleteForClass( const Class* class_, std::string_view name );
	void ComleteClassOwnFields( const Class* class_, std::string_view name );
	void CompleteProcessValue( std::string_view completion_name, std::string_view value_name, const NamesScopeValue& names_scope_value );

	void PerformSignatureHelp( const Value& value );

	void DeleteFunctionsBodies_r( NamesScope& names_scope );

private:
	void BuildSourceGraphNode( const SourceGraph& source_graph, size_t node_index );

	void MergeNameScopes(
		NamesScope& dst,
		const NamesScope& src,
		const ClassesMembersNamespacesTable& src_classes_members_namespaces_table );

	void FillGlobalNamesScope( NamesScope& global_names_scope );

	// Returns true if this is definition in the main file and not in an imported file.
	bool IsSrcLocFromMainFile( const SrcLoc& src_loc );

	// Function context required for accesing local constexpr variables.
	Type PrepareType( const Synt::TypeName& type_name, NamesScope& names_scope, FunctionContext& function_context );

	template<typename T>
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const std::unique_ptr<T>& el )
	{
		return PrepareTypeImpl( names_scope, function_context, *el );
	}

	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::EmptyVariant& type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup& root_namespace_lookup );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookupCompletion& root_namespace_lookup_completion );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup& name_lookup );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookupCompletion& name_lookup_completion );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch& names_scope_name_fetch );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetchCompletion& names_scope_name_fetch_completion );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TemplateParameterization& template_parameterization );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ArrayTypeName& array_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionType& function_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TupleType& tuple_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RawPointerType& raw_pointer_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CoroutineType& coroutine_type_name );
	Type PrepareTypeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin_type_name );
	Type ValueToType( NamesScope& names_scope, const Value& value, const SrcLoc& src_loc );

	FunctionType PrepareFunctionType( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionType& function_type_name, ClassPtr class_= nullptr );
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
	bool ImmediateEvaluateNonSyncTag( NamesScope& names_scope, FunctionContext& function_context, const Synt::NonSyncTag& non_sync_tag );
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
		llvm::ArrayRef<Synt::TemplateParam> params,
		NamesScope& names_scope,
		std::vector<TemplateParameter>& template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags );

	// Handler for ComplexName/TypeName/Expression.
	template< typename ... VariantArgs >
	TemplateSignatureParam CreateTemplateSignatureParameter(
		NamesScope& names_scope,
		FunctionContext& function_context,
		const llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const std::variant< VariantArgs ... >& template_parameter )
	{
		return
			std::visit(
				[&]( const auto& t ) { return CreateTemplateSignatureParameterImpl( names_scope, function_context, template_parameters, template_parameters_usage_flags, t ); },
				template_parameter );
	}

	// Handler for unique_ptr unwrapping.
	template<typename T>
	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const std::unique_ptr<T>& el )
	{
		return CreateTemplateSignatureParameterImpl( names_scope, function_context, template_parameters, template_parameters_usage_flags, *el );
	}

	// Handler for non-important for template args creation nodes - just evaluate expression.
	template<typename T>
	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const T& el )
	{
		(void)template_parameters;
		(void)template_parameters_usage_flags;
		return ValueToTemplateParam( BuildExpressionCodeImpl( names_scope, function_context, el ), names_scope, el.src_loc );
	}

	// Template signature parameter handlers for different expression node kinds.

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::EmptyVariant& empty_variant );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::ArrayTypeName& array_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::FunctionType& function_pointer_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::TupleType& tuple_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::RawPointerType& raw_pointer_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::CoroutineType& coroutine_type_name );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::NameLookup& name_lookup );

	TemplateSignatureParam CreateTemplateSignatureParameterImpl(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::ArrayRef<TemplateParameter> template_parameters,
		llvm::SmallVectorImpl<bool>& template_parameters_usage_flags,
		const Synt::TemplateParameterization& template_parameterization );

	TemplateSignatureParam ValueToTemplateParam( const Value& value, NamesScope& names_scope, const SrcLoc& src_loc );

	// Returns "true" if all ok.
	bool MatchTemplateArg(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam& template_signature_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::Type& type_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::Variable& variable_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::TypeTemplate& type_template_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::TemplateParam& template_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::Array& array_type_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::Tuple& tuple_type_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::RawPointer& raw_pointer_type_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::Function& function_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::Coroutine& coroutine_param );

	bool MatchTemplateArgImpl(
		llvm::ArrayRef<TemplateParameter> template_params,
		NamesScope& args_names_scope,
		const TemplateArg& template_arg,
		const TemplateSignatureParam::SpecializedTemplate& specialized_template_param );

	// Returns none in case of fail.
	std::optional<Type> GenTemplateType(
		const SrcLoc& src_loc,
		const TypeTemplatesSet& type_templates_set,
		llvm::ArrayRef<Synt::Expression> template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	// Returns nullptr in case of fail.
	TemplateTypePreparationResult PrepareTemplateType(
		const TypeTemplatePtr& type_template_ptr,
		llvm::ArrayRef<TemplateArg> template_arguments );

	std::optional<Type> FinishTemplateTypeGeneration(
		const SrcLoc& src_loc,
		NamesScope& arguments_names_scope,
		const TemplateTypePreparationResult& template_type_preparation_result );

	TemplateFunctionPreparationResult PrepareTemplateFunction(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr,
		llvm::ArrayRef<FunctionType::Param> actual_args,
		bool first_actual_arg_is_this );

	const FunctionVariable* FinishTemplateFunctionParameterization(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const FunctionTemplatePtr& function_template_ptr );

	const FunctionVariable* FinishTemplateFunctionGeneration(
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		const TemplateFunctionPreparationResult& template_function_preparation_result );

	// Returns nullptr on fail.
	OverloadedFunctionsSetPtr ParameterizeFunctionTemplate(
		const SrcLoc& src_loc,
		const OverloadedFunctionsSetConstPtr& functions_set_ptr,
		llvm::ArrayRef<Synt::Expression> template_arguments,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context );

	void EvaluateTemplateArgs(
		llvm::ArrayRef<Synt::Expression> template_arguments,
		const SrcLoc& src_loc,
		NamesScope& arguments_names_scope,
		FunctionContext& function_context,
		llvm::SmallVectorImpl<TemplateArg>& out_args );

	std::optional<TemplateArg> ValueToTemplateArg( const Value& value, CodeBuilderErrorsContainer& errors, const SrcLoc& src_loc );

	void FillKnownFunctionTemplateArgsIntoNamespace(
		const FunctionTemplate& function_template,
		NamesScope& target_namespace );

	void AddNewTemplateThing( TemplateKey key, NamesScopePtr thing );

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
		NamesScope& names_scope,
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

	// Unused name error generation stuff.
	void CheckForUnusedGlobalNames( const NamesScope& names_scope );
	void CheckForUnusedGlobalNamesImpl( const NamesScope& names_scope );
	void CheckForUnusedLocalNames( const NamesScope& names_scope );
	bool VariableExistanceMayHaveSideEffects( const Type& variable_type );

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

	void BuildFuncCode(
		FunctionVariable& func,
		ClassPtr base_class,
		NamesScope& parent_names_scope,
		std::string_view func_name,
		// Contexts for different kinds of preprocessing.
		ReturnTypeDeductionContext* return_type_deduction_context= nullptr,
		ReferenceNotationDeductionContext* reference_notation_deduction_context= nullptr,
		LambdaPreprocessingContext* lambda_preprocessing_context= nullptr );

	// Expressions.
	VariablePtr BuildExpressionCodeEnsureVariable(
		const Synt::Expression& expression,
		NamesScope& names_scope,
		FunctionContext& function_context );

	// May move local variables.
	VariablePtr BuildExpressionCodeForValueReturn(
		const Synt::Expression& expression,
		NamesScope& names_scope,
		FunctionContext& function_context );

	Value BuildExpressionCode( const Synt::Expression& expression, NamesScope& names_scope, FunctionContext& function_context );

	template<typename T>
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const std::unique_ptr<T>& el )
	{
		return BuildExpressionCodeImpl( names_scope, function_context, *el );
	}

	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::EmptyVariant& expression );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CallOperator& call_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CallOperatorSignatureHelp& call_operator_signature_help );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::IndexationOperator& indexation_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::MemberAccessOperator& member_access_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::MemberAccessOperatorCompletion& member_access_operator_completion );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::VariableInitialization& variable_initialization );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::AwaitOperator& await_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::UnaryMinus& unary_minus );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::LogicalNot& logical_not );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::BitwiseNot& bitwise_not );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::BinaryOperator& binary_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TernaryOperator& ternary_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NumericConstant& numeric_constant );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::BooleanConstant& boolean_constant );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::StringLiteral& string_literal );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::MoveOperator& move_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::MoveOperatorCompletion& move_operator_completion );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TakeOperator& move_operator );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Lambda& lambda );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CastMut& cast_mut );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CastImut& cast_imut );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CastRef& cast_ref );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CastRefUnsafe& cast_ref_unsafe );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Embed& embed );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeInfo& typeinfo );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::SameType& same_type );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NonSyncExpression& non_sync_expression );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::SafeExpression& safe_expression );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::UnsafeExpression& unsafe_expression );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup& root_namespace_lookup );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookupCompletion& root_namespace_lookup_completion );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup& name_lookup );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookupCompletion& name_lookup_completion );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch& names_scope_fetch );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetchCompletion& names_scope_fetch_completion );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TemplateParameterization& template_parameterization );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ArrayTypeName& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::FunctionType& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TupleType& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RawPointerType& type_name );
	Value BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CoroutineType& type_name );

	VariablePtr AccessClassBase( const VariablePtr& variable, FunctionContext& function_context );
	Value AccessClassField(
		NamesScope& names_scope,
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
		NamesScope& names_scope,
		FunctionContext& function_context );

	Value CallBinaryOperatorForArrayOrTuple(
		OverloadedOperator op,
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	Value ConcatenateCharArrays(
		const Synt::Expression&  left_expr,
		const Synt::Expression& right_expr,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	std::optional<Value> TryCallOverloadedUnaryOperator(
		const VariablePtr& variable,
		OverloadedOperator op,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	std::optional<Value> TryCallOverloadedPostfixOperator(
		const VariablePtr& variable,
		llvm::ArrayRef<Synt::Expression> synt_args,
		OverloadedOperator op,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	Value BuildBinaryOperator(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	Value BuildBinaryArithmeticOperatorForRawPointers(
		const Variable& l_var,
		const Variable& r_var,
		BinaryOperatorType binary_operator,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );
		
	Value BuildLazyBinaryOperator(
		const Synt::Expression& l_expression,
		const Synt::Expression& r_expression,
		const Synt::BinaryOperator& binary_operator,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	Value DoReferenceCast(
		const SrcLoc& src_loc,
		const Synt::TypeName& type_name,
		const Synt::Expression& expression,
		bool enable_unsafe,
		NamesScope& names_scope,
		FunctionContext& function_context );

	// Call something (function, pointer) or even perform temp variable construction.
	Value CallFunctionValue(
		const Value& function_value,
		llvm::ArrayRef<Synt::Expression> synt_args,
		const SrcLoc& call_src_loc,
		const std::optional<SrcLoc>& function_value_src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	// Perform specific call.
	Value DoCallFunction(
		llvm::Value* function,
		const FunctionType& function_type,
		const SrcLoc& call_src_loc,
		const VariablePtr& this_, // optional
		llvm::ArrayRef<const Synt::Expression*> args,
		bool evaluate_args_in_reverse_order,
		NamesScope& names_scope,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	Value DoCallFunction(
		llvm::Value* function,
		const FunctionType& function_type,
		const SrcLoc& call_src_loc,
		llvm::ArrayRef<VariablePtr> preevaluated_args,
		llvm::ArrayRef<const Synt::Expression*> args,
		bool evaluate_args_in_reverse_order,
		NamesScope& names_scope,
		FunctionContext& function_context,
		bool func_is_constexpr= false );

	VariablePtr BuildTempVariableConstruction(
		const Type& type,
		llvm::ArrayRef<Synt::Expression> synt_args,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
		FunctionContext& function_context );

	VariablePtr ConvertVariable(
		VariablePtr variable,
		const Type& dst_type,
		const FunctionVariable& conversion_constructor,
		NamesScope& names_scope,
		FunctionContext& function_context,
		const SrcLoc& src_loc );

	bool EvaluateBoolConstantExpression( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression );

	// Preevaluate expresion to know it's extened type.
	// Call this only inside save/state restore calls.
	FunctionType::Param PreEvaluateArg( const Synt::Expression& expression, NamesScope& names_scope, FunctionContext& function_context );
	FunctionType::Param GetArgExtendedType( const Variable& variable );

	// Typeinfo

	VariablePtr BuildTypeInfo( const Type& type, NamesScope& root_namespace );
	ClassPtr CreateTypeinfoClass( NamesScope& root_namespace, const Type& src_type, std::string name );
	VariableMutPtr BuildTypeinfoPrototype( const Type& type, NamesScope& root_namespace );
	void BuildFullTypeinfo( const Type& type, const VariableMutPtr& typeinfo_variable, NamesScope& root_namespace );
	void FinishTypeinfoClass( ClassPtr class_type, const ClassFieldsVector<llvm::Type*>& fields_llvm_types );

	VariablePtr TryFetchTypeinfoClassLazyField( const Type& typeinfo_type, std::string_view name ); // Returns nullptr if can't fetch.

	VariablePtr CreateTypeinfoListVariable( llvm::SmallVectorImpl<TypeinfoListElement>& list );

	VariablePtr BuildTypeinfoEnumElementsList( EnumPtr enum_type, NamesScope& root_namespace );
	void CreateTypeinfoClassMembersListNodeCommonFields(
		const Class& class_, ClassPtr node_class_type,
		std::string_view member_name,
		ClassFieldsVector<llvm::Type*>& fields_llvm_types, ClassFieldsVector<llvm::Constant*>& fields_initializers );
	VariablePtr BuildTypeinfoClassFieldsList( ClassPtr class_type, NamesScope& root_namespace );
	VariablePtr BuildTypeinfoClassTypesList( ClassPtr class_type, NamesScope& root_namespace );
	VariablePtr BuildTypeinfoClassFunctionsList( ClassPtr class_type, NamesScope& root_namespace );
	VariablePtr BuildTypeinfoClassParentsList( ClassPtr class_type, NamesScope& root_namespace );
	VariablePtr BuildTypeinfoFunctionParams( const FunctionType& function_type, NamesScope& root_namespace );
	VariablePtr BuildTypeinfoTupleElements( const TupleType& tuple_type, NamesScope& root_namespace );

	// Block elements
	BlockBuildInfo BuildIfAlternative( NamesScope& names_scope, FunctionContext& function_context, const Synt::IfAlternative& if_alterntative );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Block& block );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ScopeBlock& block );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::VariablesDeclaration& variables_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::AutoVariableDeclaration& auto_variable_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::AllocaDeclaration& alloca_declaration );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ReturnOperator& return_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::YieldOperator& yield_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RangeForOperator& range_for_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CStyleForOperator& c_style_for_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::WhileOperator& while_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::LoopOperator& loop_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::BreakOperator& break_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::ContinueOperator& continue_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::WithOperator& with_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::IfOperator& if_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::StaticIfOperator& static_if_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::IfCoroAdvanceOperator& if_coro_advance );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::SwitchOperator& switch_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::SingleExpressionOperator& single_expression_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::AssignmentOperator& assignment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::CompoundAssignmentOperator& compound_assignment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::IncrementOperator& increment_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::DecrementOperator& decrement_operator );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::StaticAssert& static_assert_ );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeAlias& type_alias );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Halt& halt );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::HaltIf& halt_if );
	BlockBuildInfo BuildBlockElementImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin );

	BlockBuildInfo BuildBlock( NamesScope& names_scope, FunctionContext& function_context, const Synt::Block& block );
	// Build elements, withut creating separate names scope.
	BlockBuildInfo BuildBlockElements( NamesScope& names_scope, FunctionContext& function_context, const Synt::BlockElementsList& block_elements );

	void BuildEmptyReturn( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );

	void AddLoopFrame(
		NamesScope& names_scope,
		FunctionContext& function_context,
		llvm::BasicBlock* break_block,
		llvm::BasicBlock* continue_block,
		const std::optional<Synt::Label>& label );

	// Returns nullptr if not found.
	LoopFrame* FetchLoopFrame( NamesScope& names_scope, FunctionContext& function_context, const std::optional<Synt::Label>& label );

	// ++ and -- operations
	void BuildDeltaOneOperatorCode(
		const Synt::Expression& expression,
		const SrcLoc& src_loc,
		bool positive, // true - increment, false - decrement
		NamesScope& names_scope,
		FunctionContext& function_context );

	void BuildStaticAssert( StaticAssert& static_assert_, NamesScope& names_scope, FunctionContext& function_context );

	//
	// Name resolving.
	//

	Value ResolveValue( NamesScope& names_scope, FunctionContext& function_context, const Synt::ComplexName& complex_name );

	template<typename T>
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const std::unique_ptr<T>& el )
	{
		return ResolveValueImpl( names_scope, function_context, *el );
	}

	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup& root_namespace_lookup );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookupCompletion& root_namespace_lookup_completion );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup& name_lookup );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookupCompletion& name_lookup_completion );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch& names_scope_fetch );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetchCompletion& names_scope_fetch_completion );
	Value ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TemplateParameterization& template_parameterization );

	void BuildGlobalThingDuringResolveIfNecessary( NamesScope& names_scope, NamesScopeValue* value );
	Value ContextualizeValueInResolve( NamesScope& names_scope, FunctionContext& function_context, const Value& value, const SrcLoc& src_loc );

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

	using OverloadingResolutionItem= std::variant<const FunctionVariable*, TemplateFunctionPreparationResult>;

	FunctionType::Param OverloadingResolutionItemGetParamExtendedType( const OverloadingResolutionItem& item, size_t param_index );
	const TemplateSignatureParam& OverloadingResolutionItemGetTemplateSignatureParam( const OverloadingResolutionItem& item, size_t param_index );
	bool OverloadingResolutionItemIsThisCall( const OverloadingResolutionItem& item );
	bool OverloadingResolutionItemIsConversionConstructor( const OverloadingResolutionItem& item );

	// This call may trigger template function building.
	const FunctionVariable* FinalizeSelectedFunction(
		const OverloadingResolutionItem& item,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// Fetch all functions (including instantiations of function template), that match given args.
	// Adds functions into output container (but does not clear it).
	void FetchMatchedOverloadedFunctions(
		const OverloadedFunctionsSet& functions_set,
		llvm::ArrayRef<FunctionType::Param> actual_args,
		bool first_actual_arg_is_this,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		bool enable_type_conversions,
		llvm::SmallVectorImpl<OverloadingResolutionItem>& out_match_functions );

	// Select single (best) matched overloaded function.
	// Returns pointer to input array if single function is selected.
	// Returns nullptr and produced an error if can't properly select.
	const OverloadingResolutionItem* SelectOverloadedFunction(
		llvm::ArrayRef<FunctionType::Param> actual_args,
		bool first_actual_arg_is_this,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc,
		llvm::ArrayRef<OverloadingResolutionItem> matched_functions );

	// Fetch and select overloaded function.
	const FunctionVariable* GetOverloadedFunction(
		const OverloadedFunctionsSet& functions_set,
		llvm::ArrayRef<FunctionType::Param> actual_args,
		bool first_actual_arg_is_this,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// Fetch and select overloaded operator.
	const FunctionVariable* GetOverloadedOperator(
		llvm::ArrayRef<FunctionType::Param> actual_args,
		OverloadedOperator op,
		NamesScope& names_scope,
		const SrcLoc& src_loc );

	// Returns non-null, if type is class and has constructors.
	OverloadedFunctionsSetPtr GetConstructors(
		const Type& type,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// Fetch and select overloaded conversion constructor.
	const FunctionVariable* GetConversionConstructor(
		FunctionType::Param src_type_extended,
		const Type& dst_type,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// Check existance of conversion constuctor, but do not trigger its building.
	bool HasConversionConstructor(
		FunctionType::Param src_type_extended,
		const Type& dst_type,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	const TemplateTypePreparationResult* SelectTemplateType(
		llvm::ArrayRef<TemplateTypePreparationResult> candidate_templates,
		size_t arg_count );

	// Initializers.
	// Some initializers returns nonnul constant, if initializer is constant.
	llvm::Constant* ApplyInitializer( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::Initializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::EmptyVariant& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::SequenceInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::StructNamedInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::ConstructorInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::ConstructorInitializerSignatureHelp& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::ZeroInitializer& initializer );
	llvm::Constant* ApplyInitializerImpl( const VariablePtr& variable, NamesScope& names_scope, FunctionContext& function_context, const Synt::UninitializedInitializer& uninitialized_initializer );

	llvm::Constant* ApplyEmptyInitializer(
		std::string_view variable_name,
		const SrcLoc& src_loc,
		VariablePtr variable,
		NamesScope& names_scope,
		FunctionContext& function_context );

	llvm::Constant* ApplyConstructorInitializer(
		const VariablePtr& variable,
		llvm::ArrayRef<Synt::Expression> synt_args,
		const SrcLoc& src_loc,
		NamesScope& names_scope,
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
		NamesScope& names_scope,
		FunctionContext& function_context );

	llvm::Constant* InitializeFunctionPointer(
		const VariablePtr& variable,
		const Synt::Expression& initializer_expression,
		NamesScope& names_scope,
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

	void ProcessFunctionReferencesPollution(
		CodeBuilderErrorsContainer& errors_container,
		const Synt::Function& func,
		FunctionType& function_type,
		ClassPtr base_class );

	// Call this only when types of params and return value are complete.
	void CheckCompleteFunctionReferenceNotation( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void CheckFunctionReferencesNotationInnerReferences( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void CheckFunctionReferencesNotationMutabilityCorrectness( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );
	void CheckReferenceNotationMutabilityViolationForReturnReferences( const FunctionType& function_type, const FunctionType::ReturnReferences& return_references, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );
	void CheckReferenceNotationMutabilityViolationForMutableReference( const FunctionType& function_type, const FunctionType::ParamReference& param_reference, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void SetupReferencesInCopyOrMove( FunctionContext& function_context, const VariablePtr& dst_variable, const VariablePtr& src_variable, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	void RegisterTemporaryVariable( FunctionContext& function_context, VariablePtr variable );
	void DestroyUnusedTemporaryVariables( FunctionContext& function_context, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	ReferencesGraph MergeVariablesStateAfterIf(
		llvm::ArrayRef<ReferencesGraph> bracnhes_variables_state,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	void CheckReturnedReferenceIsAllowed( NamesScope& names_scope, FunctionContext& function_context, const VariablePtr& return_reference_node, const SrcLoc& src_loc );
	bool IsReferenceAllowedForReturn( FunctionContext& function_context, const VariablePtr& variable_node );

	void CheckReturnedInnerReferenceIsAllowed( NamesScope& names_scope, FunctionContext& function_context, const VariablePtr& return_reference_node, const SrcLoc& src_loc );
	bool IsReferenceAllowedForInnerReturn( FunctionContext& function_context, const VariablePtr& variable_node, size_t index );

	void CheckAsyncReturnReferenceIsAllowed(
		NamesScope& names_scope,
		FunctionContext& function_context,
		const CoroutineTypeDescription& coroutine_type_description,
		const VariablePtr& node,
		const SrcLoc& src_loc );

	void CheckAsyncReturnInnerReferencesAreAllowed(
		NamesScope& names_scope,
		FunctionContext& function_context,
		const CoroutineTypeDescription& coroutine_type_description,
		const VariablePtr& node,
		const SrcLoc& src_loc );

	std::optional<FunctionType::ParamReference> GetCoroutineInnerReferenceForParamNode( FunctionContext& function_context, const VariablePtr& node );

	void CheckReferencesPollutionBeforeReturn(
		FunctionContext& function_context,
		CodeBuilderErrorsContainer& errors_container,
		const SrcLoc& src_loc );

	// If this is preprocessing with reference notation deduction - collect actual reference effects.
	void CollectReturnReferences( FunctionContext& function_context, const VariablePtr& return_node );
	void CollectReturnInnerReferences( FunctionContext& function_context, const VariablePtr& return_node );
	void CollectReferencePollution( FunctionContext& function_context );

	// Reference notation.

	std::optional<uint8_t> EvaluateReferenceFieldTag( NamesScope& names_scope, const Synt::Expression& expression );
	std::optional< llvm::SmallVector<uint8_t, 4> > EvaluateReferenceFieldInnerTags( NamesScope& names_scope, const Synt::Expression& expression );
	FunctionType::ReferencesPollution EvaluateFunctionReferencePollution( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, size_t num_params );
	FunctionType::ReturnReferences EvaluateFunctionReturnReferences( NamesScope& names_scope,FunctionContext& function_context,  const Synt::Expression& expression, size_t num_params );
	FunctionType::ReturnInnerReferences EvaluateFunctionReturnInnerReferences( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, size_t num_params );
	VariablePtr EvaluateReferenceNotationExpression( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression );

	using ReferenceNotationConstant= std::pair<Type, llvm::Constant*>;
	ReferenceNotationConstant GetReturnReferencesConstant( const FunctionType::ReturnReferences& return_references );
	ReferenceNotationConstant GetReturnInnerReferencesConstant( const FunctionType::ReturnInnerReferences& return_inner_references );
	ReferenceNotationConstant GetReferencesPollutionConstant( const FunctionType::ReferencesPollution& references_pollution );
	llvm::Constant* GetParamReferenceConstant( const FunctionType::ParamReference& param_reference ); // Type is reference_notation_param_reference_description_type_

	// Coroutines

	// Call this before transforming function type.
	void PerformCoroutineFunctionReferenceNotationChecks( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc );

	// Make return type - coroutine type and prepare it properly. Modifies given function type.
	void TransformCoroutineFunctionType( FunctionType& coroutine_function_type, FunctionVariable::Kind kind, NamesScope& names_scope, const SrcLoc& src_loc );

	ClassPtr GetCoroutineType( NamesScope& root_namespace, const CoroutineTypeDescription& coroutine_type_description );

	// This function should be called for coroutine function just after aruments preparation.
	void PrepareCoroutineBlocks( FunctionContext& function_context );

	void CoroutineYield( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc );
	void AsyncReturn( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc );
	Value BuildAwait( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc );
	void CoroutineSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );
	void CoroutineFinalSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc );

	// Lambdas

	Value BuildLambda( NamesScope& names_scope, FunctionContext& function_context, const Synt::Lambda& lambda );
	std::pair<llvm::Value*, llvm::Constant*> InitializeLambdaField( NamesScope& names_scope, FunctionContext& function_context, const ClassField& field, const VariablePtr& variable, const VariablePtr& result, const SrcLoc& src_loc );
	ClassPtr PrepareLambdaClass( NamesScope& names_scope, FunctionContext& function_context, const Synt::Lambda& lambda );
	ClassPtr GetLambdaPreprocessingDummyClass( NamesScope& names );
	std::string GetLambdaBaseName( const Synt::Lambda& lambda, llvm::ArrayRef<uint32_t> tuple_for_indices );
	std::unordered_set<VariablePtr> CollectCurrentFunctionVariables( FunctionContext& function_context );
	void LambdaPreprocessingCheckVariableUsage( NamesScope& names_scope, FunctionContext& function_context, const VariablePtr& variable, const std::string& name, const SrcLoc& src_loc );
	VariablePtr LambdaPreprocessingAccessExternalVariable( FunctionContext& function_context, const VariablePtr& variable, const std::string& name );
	void LambdaPreprocessingEnsureCapturedVariableRegistered( FunctionContext& function_context, const LambdaPreprocessingContext::CapturedVariableData& captured_variable );
	Value LambdaPreprocessingHandleCapturedVariableMove( NamesScope& names_scope, FunctionContext& function_context, const VariablePtr& variable,  const std::string& name, const SrcLoc& src_loc );

	// NamesScope fill

	void NamesScopeFill( NamesScope& names_scope, const Synt::ProgramElementsList& namespace_elements );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Namespace& namespace_ );
	void NamesScopeFill( NamesScope& names_scope, const Synt::VariablesDeclaration& variables_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::AutoVariableDeclaration& variable_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Function& function_declaration, ClassPtr base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFillFunction( NamesScope& names_scope, const Synt::Function& function_declaration, std::string_view func_name, ClassPtr base_class, ClassMemberVisibility visibility );
	void NamesScopeFill( NamesScope& names_scope, const Synt::FunctionTemplate& function_template_declaration, ClassPtr base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	ClassPtr NamesScopeFill( NamesScope& names_scope, const Synt::Class& class_declaration, std::optional<Class::BaseTemplate> base_template= std::nullopt );
	void FillClassNamesScope( ClassPtr class_type, std::string_view class_name, Synt::ClassKindAttribute class_kind, const Synt::ClassElementsList& class_elements, ClassMemberVisibility initial_visibility );
	void NamesScopeFill( NamesScope& names_scope, const Synt::TypeTemplate& type_template_declaration, ClassPtr base_class= nullptr, ClassMemberVisibility visibility= ClassMemberVisibility::Public );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Enum& enum_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::TypeAlias& type_alias_declaration );
	void NamesScopeFill( NamesScope& names_scope, const Synt::StaticAssert& static_assert_ );
	void NamesScopeFill( NamesScope& names_scope, const Synt::Mixin& mixin, ClassMemberVisibility visibility= ClassMemberVisibility::Public );

	void NamesScopeFillOutOfLineElements( NamesScope& names_scope, const Synt::ProgramElementsList& namespace_elements );
	template<typename T> void NamesScopeFillOutOfLineElement( NamesScope&, const T& ) {} // Ignore almost all nodes in out of line filling, except a couple of special nodes.
	void NamesScopeFillOutOfLineElement( NamesScope& names_scope, const Synt::Function& function );
	void NamesScopeFillOutOfLineElement( NamesScope& names_scope, const Synt::Namespace& namespace_ );

	// Mixins
	void ProcessMixins( NamesScope& names_scope );
	// Returns total number of mixins.
	uint32_t EvaluateMixinsExpressions_r( NamesScope& names_scope );
	void ExpandNamespaceMixins_r( NamesScope& names_scope );
	void ProcessClassMixins( ClassPtr class_type );
	void ExpandClassMixins_r( ClassPtr class_type );
	void ExpandNamespaceMixin( NamesScope& names_scope, Mixin& mixin );
	void ExpandClassMixin( ClassPtr class_type, Mixin& mixin );
	const Synt::BlockElementsList* ExpandBlockMixin( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin );
	const Synt::TypeName* ExpandTypeNameMixin( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin );
	const Synt::Expression* ExpandExpressionMixin( NamesScope& names_scope, FunctionContext& function_context, const Synt::Mixin& mixin );
	void EvaluateMixinExpressionInGlobalContext( NamesScope& names_scope, Mixin& mixin );
	void EvaluateMixinExpression( NamesScope& names_scope, FunctionContext& function_context, Mixin& mixin );

	std::optional<Lexems> PrepareMixinLexems( NamesScope& names_scope, const SrcLoc& src_loc, std::string_view mixin_text );

	// Global things build

	bool IsTypeComplete( const Type& type ) const;
	bool EnsureTypeComplete( const Type& type ); // Returns true, if complete
	bool ReferenceIsConvertible( const Type& from, const Type& to, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc ); // Returns true of all ok. If types are different can call EnsureTypeCompleteness.

	void GlobalThingBuildNamespace( NamesScope& names_scope );
	void PrepareFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set );
	void BuildFunctionsSetBodies( NamesScope& names_scope, OverloadedFunctionsSet& functions_set );
	void PrepareFunctionsSetAndBuildConstexprBodies( NamesScope& names_scope, OverloadedFunctionsSet& functions_set );
	void GlobalThingPrepareClassParentsList( ClassPtr class_type );
	void GlobalThingBuildClass( ClassPtr class_type );
	void GlobalThingBuildEnum( EnumPtr enum_ );
	void GlobalThingBuildTypeTemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set );
	void GlobalThingBuildTypeAlias( NamesScope& names_scope, Value& type_alias_value );
	void GlobalThingBuildVariable( NamesScope& names_scope, Value& global_variable_value );
	size_t GlobalThingDetectloop( const GlobalThing& global_thing ); // returns loop start index or ~0u
	void GlobalThingReportAboutLoop( size_t loop_start_stack_index, std::string_view last_loop_element_name, const SrcLoc& last_loop_element_src_loc );

	// Other stuff

	llvm::Type* GetFundamentalLLVMType( U_FundamentalType fundmantal_type );
	uint64_t GetFundamentalTypeSize( U_FundamentalType fundamental_type );

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
	const bool collect_definition_points_;
	bool skip_building_generated_functions_;

	const IVfsSharedPtr vfs_;

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

		// Types with width of pointer.
		llvm::IntegerType* ssize_type_;
		llvm::IntegerType* size_type_ ;

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
	Type ssize_type_;
	Type size_type_ ;
	Type reference_notation_param_reference_description_type_;
	Type reference_notation_pollution_element_type_;
	llvm::PointerType* virtual_function_pointer_type_= nullptr; // Use common type for all function pointers in virtual table - for simplicity.
	llvm::StructType* polymorph_type_id_table_element_type_= nullptr;

	Interpreter constexpr_function_evaluator_;
	const std::shared_ptr<IMangler> mangler_;
	TBAAMetadataBuilder tbaa_metadata_builder_;

	std::unique_ptr<FunctionContext> global_function_context_;
	std::unique_ptr<StackVariablesStorage> global_function_context_variables_storage_;

	std::unique_ptr<llvm::Module> module_;
	const std::shared_ptr<CodeBuilderErrorsContainer> global_errors_= std::make_shared<CodeBuilderErrorsContainer>();

	// Current source graph.
	// Store shared_ptr because we need to keep it alive, because some internal structures contain raw pointers to its contents.
	SourceGraphPtr source_graph_;

	Synt::MacroExpansionContextsPtr macro_expansion_contexts_; // Macro expansion contexts of currently compiled source graph.
	std::vector<SourceBuildResult> compiled_sources_;

	// Storage for class types. Do not use shared pointers for classes for loops preventing.
	std::vector< std::unique_ptr<Class> > classes_table_;

	// Storage for enum types. Do not use shared pointers for enums for loops preventing.
	std::vector< std::unique_ptr<Enum> > enums_table_;

	// We needs to generate same typeinfo classes for same types. Use cache for it.
	std::unordered_map< Type, TypeinfoCacheElement, TypeHasher > typeinfo_cache_;
	std::vector<std::unique_ptr<Class>> typeinfo_class_table_;

	std::vector<Type> non_sync_expression_stack_;

	// Names map for generated template types/functions. We can not insert it in regular namespaces, because we needs insert it, while iterating regular namespaces.
	std::unordered_map<TemplateKey, NamesScopePtr, TemplateKeyHasher> generated_template_things_storage_;
	// Template things for current source graph node added sequentialy into this vector too.
	std::vector<TemplateKey> generated_template_things_sequence_;

	// Cache results of template functions parameterization.
	std::unordered_map<ParameterizedFunctionTemplateKey, OverloadedFunctionsSetPtr, ParameterizedFunctionTemplateKeyHasher> parameterized_template_functions_cache_;

	std::vector<GlobalThing> global_things_stack_;

	std::optional<DebugInfoBuilder> debug_info_builder_;

	std::unordered_map<CoroutineTypeDescription, std::unique_ptr<Class>, CoroutineTypeDescriptionHasher> coroutine_classes_table_;

	std::unordered_map<LambdaKey, std::unique_ptr<Class>, LambdaKeyHasher> lambda_classes_table_;
	std::unique_ptr<Class> lambda_preprocessing_dummy_class_; // Lazily created.

	// Store here mixin expansion results, because we need syntax elements to be alive, because they may be accessed during code building via raw pointers.
	// Also it's useful to reuse expansions of same mixins in different templates if result text is identical.
	// Important note: unordered_map doesn't invalidate pointers/references to elements after new elements are inserted.
	// This makes it safe to store somewhere pointers to elements of this container, assuming no elements are deleted.
	template<typename T> using MixinExpansionsMap= std::unordered_map<MixinExpansionKey, T, MixinExpansionKeyHasher>;
	MixinExpansionsMap<Synt::ProgramElementsList> namespace_mixin_expansions_;
	MixinExpansionsMap<Synt::ClassElementsList> class_mixin_expansions_;
	MixinExpansionsMap<Synt::BlockElementsList> block_mixin_expansions_;
	MixinExpansionsMap<Synt::TypeName> type_name_mixin_expansions_;
	MixinExpansionsMap<Synt::Expression> expression_mixin_expansions_;

	// Full file path to file contents map.
	std::unordered_map<IVfs::Path, std::optional<IVfs::FileContent>> embed_files_cache_;

	// Definition points. Collected during code building (if it is required).
	// Only single result is stored, that affects template stuff and other places in source code with multiple building passes.
	struct DefinitionPoint
	{
		SrcLoc src_loc;
	};
	// Map usage point to definition point.
	std::unordered_map<SrcLoc, DefinitionPoint, SrcLocHasher> definition_points_;

	std::optional<Type> stub_template_param_type_;

	// Use dummy namespace as source point for dummy instantiations of templates.
	NamesScopePtr dummy_template_instantiation_args_scope_;

	uint32_t completion_request_index_= 0;
	// Output container for completion result items.
	std::vector<CompletionItem> completion_items_;
	// Output container for signature help result items.
	std::vector<SignatureHelpItem> signature_help_items_;
};

using MutabilityModifier= Synt::MutabilityModifier;
using ReferenceModifier= Synt::ReferenceModifier;

} // namespace U
