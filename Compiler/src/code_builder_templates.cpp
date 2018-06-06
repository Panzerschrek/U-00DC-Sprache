#include <algorithm>

#include <boost/range/adaptor/reversed.hpp>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/GlobalVariable.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"
#include "mangling.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static const Synt::ComplexName& GetComplexNameForGeneratedClass()
{
	static Synt::ComplexName name;
	static bool init= false;
	if( !init )
	{
		name.components.emplace_back();
		name.components.back().name= "_"_SpC;
		name.components.back().is_generated= true;

		init= true;
	}
	return name;
}

static const ProgramString& GetNameForGeneratedClass()
{
	return GetComplexNameForGeneratedClass().components.front().name;
}

static const ProgramString g_template_parameters_namespace_prefix= "_tp_ns-"_SpC;

ProgramString CodeBuilder::PrepareTypeTemplate(
	const Synt::TypeTemplateBase& type_template_declaration,
	NamesScope& names_scope )
{
	/* SPRACHE_TODO:
	 *) Support default template arguments for short form.
	 *) Convert signature and template arguments to "default form" for equality comparison.
	 *) Support templates overloading.
	 *) Add "enable_if".
	 *) Support template-dependent types for value parameters, such template</ type T, U</ T /> ut />.
	*/

	const TypeTemplatePtr type_template( new TypeTemplate );
	const ProgramString& type_template_name= type_template_declaration.name_;

	if( NamesScope::InsertedName* const prev_name= names_scope.GetThisScopeName( type_template_name ) )
	{
		if( TypeTemplatesSet* const type_templates_set= prev_name->second.GetTypeTemplatesSet() )
			type_templates_set->push_back( type_template ); // TODO - check typpe template signature equality.
		else
		{
			errors_.push_back( ReportRedefinition( type_template_declaration.file_pos_, type_template_name ) );
			return type_template_name;
		}
	}
	else
		names_scope.AddName( type_template_name, Value( TypeTemplatesSet{type_template}, type_template_declaration.file_pos_ ) );

	type_template->parent_namespace= &names_scope;
	type_template->syntax_element= &type_template_declaration;
	type_template->file_pos= type_template_declaration.file_pos_;

	std::vector<TypeTemplate::TemplateParameter>& template_parameters= type_template->template_parameters;
	template_parameters.reserve( type_template_declaration.args_.size() );
	std::vector<bool> template_parameters_usage_flags;

	PushCacheFillResolveHandler( type_template->resolving_cache, names_scope );
	const NamesScopePtr template_parameters_namespace = std::make_shared<NamesScope>( g_template_parameters_namespace_prefix, &names_scope );

	ProcessTemplateArgs(
		type_template_declaration.args_,
		names_scope,
		type_template_declaration.file_pos_,
		template_parameters,
		*template_parameters_namespace,
		template_parameters_usage_flags );

	if( type_template_declaration.is_short_form_ )
	{
		U_ASSERT( type_template_declaration.signature_args_.empty() );
		// Assign template arguments to signature arguments.
		for( const Synt::TypeTemplateBase::Arg& arg : type_template_declaration.args_ )
		{
			PrepareTemplateSignatureParameter( type_template_declaration.file_pos_, *arg.name, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
			type_template->signature_arguments.push_back(arg.name_expr.get());
			type_template->default_signature_arguments.push_back(nullptr);
		}
		type_template->first_optional_signature_argument= type_template->signature_arguments.size();
	}
	else
	{
		// Check and fill signature args.
		type_template->first_optional_signature_argument= 0u;
		for( const Synt::TypeTemplateBase::SignatureArg& signature_arg : type_template_declaration.signature_args_ )
		{
			PrepareTemplateSignatureParameter( signature_arg.name, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
			type_template->signature_arguments.push_back(signature_arg.name.get());

			if( signature_arg.default_value != nullptr )
			{
				PrepareTemplateSignatureParameter( signature_arg.default_value, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
				type_template->default_signature_arguments.push_back(signature_arg.default_value.get());
			}
			else
			{
				const size_t index= type_template->signature_arguments.size() - 1u;
				if (index > type_template->first_optional_signature_argument )
					errors_.push_back( ReportMandatoryTemplateSignatureArgumentAfterOptionalArgument( type_template_declaration.file_pos_ ) );

				type_template->default_signature_arguments.push_back(nullptr);
				++type_template->first_optional_signature_argument;
			}
		}
	}
	U_ASSERT( type_template->signature_arguments.size() == type_template->default_signature_arguments.size() );
	U_ASSERT( type_template->first_optional_signature_argument <= type_template->default_signature_arguments.size() );

	for( size_t i= 0u; i < type_template->template_parameters.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			errors_.push_back( ReportTemplateArgumentNotUsedInSignature( type_template_declaration.file_pos_, type_template->template_parameters[i].name ) );

	// Make first check-pass for template. Resolve all names in this pass.

	Synt::ComplexName temp_class_name;
	temp_class_name.components.emplace_back();
	temp_class_name.components.back().name = "_temp"_SpC + type_template_name;
	temp_class_name.components.back().is_generated= true;

	if( const Synt::ClassTemplate* const template_class= dynamic_cast<const Synt::ClassTemplate*>( &type_template_declaration ) )
	{
		const ClassProxyPtr class_proxy= PrepareClass( *template_class->class_, temp_class_name, *template_parameters_namespace );

		if( class_proxy != nullptr )
		{
			ReportAboutIncompleteMembersOfTemplateClass( type_template_declaration.file_pos_, *class_proxy->class_ );

			// Remove llvm functions and variables of temp class.
			// Clear dummy function before it, because dummy function can contain references to removed functions.
			CleareDummyFunction();
			RemoveTempClassLLVMValues( *class_proxy->class_ );
		}
	}
	else if( const Synt::TypedefTemplate* const typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( &type_template_declaration ) )
	{
		PrepareType( typedef_template->typedef_->value, *template_parameters_namespace );
	}
	else
		U_ASSERT(false);

	PopResolveHandler();

	return type_template_name;
}

void CodeBuilder::PrepareFunctionTemplate(
	const Synt::FunctionTemplate& function_template_declaration,
	NamesScope& names_scope,
	const ClassProxyPtr& base_class,
	const ClassMemberVisibility visibility )
{
	const Synt::ComplexName& complex_name = function_template_declaration.function_->name_;
	const ProgramString& function_template_name= complex_name.components.front().name;

	if( complex_name.components.size() > 1u )
		errors_.push_back( ReportFunctionDeclarationOutsideItsScope( function_template_declaration.file_pos_ ) );
	if( complex_name.components.front().have_template_parameters )
		errors_.push_back( ReportValueIsNotTemplate( function_template_declaration.file_pos_ ) );

	if( function_template_declaration.function_->block_ == nullptr )
		errors_.push_back( ReportIncompleteMemberOfClassTemplate( function_template_declaration.file_pos_, function_template_name ) );
	if( function_template_declaration.function_->virtual_function_kind_ != Synt::VirtualFunctionKind::None )
		errors_.push_back( ReportVirtualForFunctionTemplate( function_template_declaration.file_pos_, function_template_name ) );

	const FunctionTemplatePtr function_template( new FunctionTemplate );
	function_template->syntax_element= &function_template_declaration;
	function_template->file_pos= function_template_declaration.file_pos_;
	function_template->parent_namespace= &names_scope;
	function_template->base_class= base_class;

	std::vector<bool> template_parameters_usage_flags; // Currently unused, because function template have no signature.

	PushCacheFillResolveHandler( function_template->resolving_cache, names_scope );
	const NamesScopePtr template_parameters_namespace = std::make_shared<NamesScope>( g_template_parameters_namespace_prefix, &names_scope );

	ProcessTemplateArgs(
		function_template_declaration.args_,
		names_scope,
		function_template_declaration.file_pos_,
		function_template->template_parameters,
		*template_parameters_namespace,
		template_parameters_usage_flags );

	// Make first check-pass for template. Resolve all names in this pass.
	const PrepareFunctionResult prepare_result=
		PrepareFunction( *function_template_declaration.function_, false, base_class, *template_parameters_namespace );

	PopResolveHandler();

	// Insert function template
	if( NamesScope::InsertedName* const same_name= names_scope.GetThisScopeName( function_template_name ) )
	{
		if( OverloadedFunctionsSet* const functions_set= same_name->second.GetFunctionsSet() )
		{
			if( base_class != nullptr && base_class->class_->GetMemberVisibility( function_template_name ) != visibility )
				errors_.push_back( ReportFunctionsVisibilityMismatch( function_template_declaration.file_pos_, function_template_name ) ); // All functions with same name must have same visibility.

			// SPRACHE_TODO - check equality of different template functions.
			functions_set->template_functions.push_back( function_template );
		}
		else
			errors_.push_back( ReportRedefinition( function_template_declaration.file_pos_, function_template_name ) );
	}
	else
	{
		OverloadedFunctionsSet functions_set;
		functions_set.template_functions.push_back( function_template );
		names_scope.AddName( function_template_name, std::move(functions_set) );

		if( base_class != nullptr )
			base_class->class_->SetMemberVisibility( function_template_name, visibility );
	}

	// Remove temp llvm function.
	if( prepare_result.functions_set != nullptr )
	{
		llvm::Function* const llvm_function= prepare_result.functions_set->functions[prepare_result.function_index].llvm_function;
		if( llvm_function != nullptr )
		{
			// Clear dummy function before it, because dummy function can contain references to removed functions.
			CleareDummyFunction();
			llvm_function->eraseFromParent();
		}
	}
}

void CodeBuilder::ProcessTemplateArgs(
	const std::vector<Synt::TemplateBase::Arg>& args,
	NamesScope& names_scope,
	const FilePos& file_pos,
	std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	NamesScope& template_parameters_namespace,
	std::vector<bool>& template_parameters_usage_flags )
{
	U_ASSERT( template_parameters.empty() );
	U_ASSERT( template_parameters_usage_flags.empty() );

	// Check and fill template parameters.
	for( const Synt::TemplateBase::Arg& arg : args )
	{
		U_ASSERT( arg.name->components.size() == 1u );
		const ProgramString& arg_name= arg.name->components.front().name;

		// Check redefinition
		for( const auto& prev_arg : template_parameters )
		{
			if( prev_arg.name == arg_name )
			{
				errors_.push_back( ReportRedefinition( file_pos, arg_name ) );
				continue;
			}
		}
		if( NameShadowsTemplateArgument( arg_name, names_scope ) )
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( file_pos, arg_name ) );

		NamesScope::InsertedName* inserted_template_parameter= nullptr;

		if( arg.arg_type != nullptr )
		{
			// If template parameter is variable.

			// Resolve from outer space or from this template parameters.
			const NamesScope::InsertedName* const type_name= ResolveName( file_pos, template_parameters_namespace, *arg.arg_type );
			if( type_name == nullptr )
			{
				errors_.push_back( ReportNameNotFound( file_pos, *arg.arg_type ) );
				continue;
			}
			const Type* const type= type_name->second.GetTypeName();
			if( type == nullptr )
			{
				errors_.push_back( ReportNameIsNotTypeName( file_pos, type_name->first ) );
				continue;
			}

			if( type->GetTemplateDependentType() == nullptr &&
				!TypeIsValidForTemplateVariableArgument( *type ) )
			{
				errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( file_pos, type->ToString() ) );
				continue;
			}

			// If type is template parameter, set usage flag.
			if( arg.arg_type->components.size() == 1u && !arg.arg_type->components.front().have_template_parameters )
			{
				for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
				{
					if( template_parameter.name == arg.arg_type->components.front().name )
					{
						template_parameters_usage_flags[ &template_parameter - template_parameters.data() ]= true;
						break;
					}
				}
			}

			template_parameters.emplace_back();
			template_parameters.back().name= arg_name;
			template_parameters.back().type_name= arg.arg_type;
			template_parameters_usage_flags.push_back(false);

			Variable variable;
			variable.type= *type;
			if( type->GetTemplateDependentType() != nullptr )
			{}
			else
			{
				variable.constexpr_value= llvm::UndefValue::get( type->GetLLVMType() );
				variable.llvm_value=
					CreateGlobalConstantVariable( *type, ToStdString( arg_name ), variable.constexpr_value );
			}

			inserted_template_parameter=
				template_parameters_namespace.AddName( arg_name, Value( std::move(variable), file_pos ) /* TODO - set correct file_pos */ );
		}
		else
		{
			// If template parameter is type.

			template_parameters.emplace_back();
			template_parameters.back().name= arg_name;
			template_parameters_usage_flags.push_back(false);
			inserted_template_parameter=
				template_parameters_namespace.AddName( arg_name, Value( GetNextTemplateDependentType(), file_pos /* TODO - set correct file_pos */ ) );
		}

		if( inserted_template_parameter != nullptr )
			inserted_template_parameter->second.SetIsTemplateParameter(true);
	}

	U_ASSERT( template_parameters_usage_flags.size() == template_parameters.size() );
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const FilePos& file_pos,
	const Synt::ComplexName& signature_parameter,
	NamesScope& names_scope,
	const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	// If signature parameter is template parameter, set usage flag.
	if( signature_parameter.components.size() == 1u && !signature_parameter.components.front().have_template_parameters )
	{
		for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
		{
			if( template_parameter.name == signature_parameter.components.front().name )
			{
				template_parameters_usage_flags[ &template_parameter - template_parameters.data() ]= true;
				break;
			}
		}
	}

	// Do recursive preresolve for subsequent deduction.
	const NamesScope::InsertedName* const start_name=
		ResolveForTemplateSignatureParameter( file_pos, signature_parameter, names_scope );
	if( start_name == nullptr )
	{
		errors_.push_back( ReportNameNotFound( file_pos, signature_parameter ) );
		return;
	}
	if( start_name->second.GetTypeTemplatesSet() != nullptr )
	{
		for( const Synt::IExpressionComponentPtr& template_parameter : signature_parameter.components.back().template_parameters )
			PrepareTemplateSignatureParameter( template_parameter, names_scope, template_parameters, template_parameters_usage_flags );
	}
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const Synt::IExpressionComponentPtr& template_parameter,
	NamesScope& names_scope,
	const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	bool special_expr_type= true;

	if( const auto named_operand= dynamic_cast<const Synt::NamedOperand*>(template_parameter.get()) )
	{
		if( named_operand->postfix_operators_.empty() && named_operand->prefix_operators_.empty() )
			PrepareTemplateSignatureParameter( named_operand->file_pos_, named_operand->name_, names_scope, template_parameters, template_parameters_usage_flags );
		else
			special_expr_type= false;
	}
	else if( const auto type_name= dynamic_cast<const Synt::TypeNameInExpression*>(template_parameter.get()) )
		PrepareTemplateSignatureParameter( *type_name->type_name, names_scope, template_parameters, template_parameters_usage_flags );
	else if( const auto bracket_expression= dynamic_cast<const Synt::BracketExpression*>(template_parameter.get()) )
	{
		if( bracket_expression->postfix_operators_.empty() && bracket_expression->prefix_operators_.empty() )
			PrepareTemplateSignatureParameter( bracket_expression->expression_, names_scope, template_parameters, template_parameters_usage_flags );
		else
			special_expr_type= false;
	}
	else
		special_expr_type= false;

	if( special_expr_type )
		return;

	// If this is not special expression - assume that this is variable-expression.

	const Value val= BuildExpressionCode( *template_parameter, names_scope, *dummy_function_context_ );
	if( val.GetErrorValue() != nullptr || val.GetTemplateDependentValue() != nullptr ||
		val.GetType().GetTemplateDependentType() != nullptr )
		return;

	const Variable* const var= val.GetVariable();
	if( var == nullptr )
	{
		errors_.push_back( ReportExpectedVariable( template_parameter->GetFilePos(), val.GetType().ToString() ) );
		return;
	}
	if( var->type.GetTemplateDependentType() == nullptr && !TypeIsValidForTemplateVariableArgument( var->type ) )
	{
		errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( template_parameter->GetFilePos(), var->type.ToString() ) );
		return;
	}
	if( var->constexpr_value == nullptr )
		errors_.push_back( ReportExpectedConstantExpression( template_parameter->GetFilePos() ) );
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const Synt::ITypeName& type_name_template_parameter,
	NamesScope& names_scope,
	const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	if( const auto named_type_name= dynamic_cast<const Synt::NamedTypeName*>(&type_name_template_parameter) )
		PrepareTemplateSignatureParameter( named_type_name->file_pos_, named_type_name->name, names_scope, template_parameters, template_parameters_usage_flags );
	else if( const auto array_type_name= dynamic_cast<const Synt::ArrayTypeName*>(&type_name_template_parameter) )
	{
		PrepareTemplateSignatureParameter( array_type_name->size, names_scope, template_parameters, template_parameters_usage_flags );
		PrepareTemplateSignatureParameter( *array_type_name->element_type, names_scope, template_parameters, template_parameters_usage_flags );
	}
	else if( const auto function_pointer_type_name= dynamic_cast<const Synt::FunctionType*>(&type_name_template_parameter) )
	{
		// TODO - maybe check also reference tags?
		if( function_pointer_type_name->return_type_ != nullptr )
			PrepareTemplateSignatureParameter(*function_pointer_type_name->return_type_, names_scope, template_parameters, template_parameters_usage_flags );
		for( const Synt::FunctionArgumentPtr& arg : function_pointer_type_name->arguments_ )
			PrepareTemplateSignatureParameter( *arg->type_, names_scope, template_parameters, template_parameters_usage_flags );
	}
	else U_ASSERT(false);
}

const NamesScope::InsertedName* CodeBuilder::ResolveForTemplateSignatureParameter(
	const FilePos& file_pos,
	const Synt::ComplexName& signature_parameter,
	NamesScope& names_scope )
{
	size_t component_number= 0u;
	const NamesScope::InsertedName* const start_name=
		PreResolve( names_scope, signature_parameter.components.data(), signature_parameter.components.size(), component_number );
	if( start_name == nullptr )
		return nullptr;
	const NamesScope::InsertedName* current_name= start_name;
	while( component_number < signature_parameter.components.size() )
	{
		NamesScope* next_space= nullptr;
		ClassProxyPtr next_space_class= nullptr;
		const Synt::ComplexName::Component& component= signature_parameter.components[component_number - 1u];
		const bool is_last_component= component_number + 1u == signature_parameter.components.size();

		if( const NamesScopePtr inner_namespace= current_name->second.GetNamespace() )
			next_space= inner_namespace.get();
		else if( const Type* const type= current_name->second.GetTypeName() )
		{
			if( Class* const class_= type->GetClassType() )
			{
				if( !is_last_component && class_->completeness != Class::Completeness::Complete )
				{
					errors_.push_back( ReportUsingIncompleteType( file_pos, type->ToString() ) );
					return nullptr;
				}
				next_space= &class_->members;
				next_space_class= type->GetClassTypeProxy();
			}
		}
		else if( const TypeTemplatesSet* const type_templates_set = current_name->second.GetTypeTemplatesSet() )
		{
			if( component.have_template_parameters && !is_last_component )
			{
				const NamesScope::InsertedName* generated_type=
					GenTemplateType(
						file_pos,
						*type_templates_set,
						component.template_parameters,
						names_scope );
				if( generated_type == nullptr )
					return nullptr;
				if( generated_type->second.GetType() == NontypeStub::TemplateDependentValue )
					return generated_type;

				const Type* const type= generated_type->second.GetTypeName();
				U_ASSERT( type != nullptr );
				if( type->GetTemplateDependentType() != nullptr )
					return &names_scope.GetTemplateDependentValue();
				if( Class* const class_= type->GetClassType() )
					next_space= &class_->members;
				next_space_class= type->GetClassTypeProxy();
			}
			else if( !is_last_component )
			{
				errors_.push_back( ReportTemplateInstantiationRequired( file_pos, type_templates_set->front()->syntax_element->name_ ) );
				return nullptr;
			}

		}
		else
			return nullptr;

		const Synt::ComplexName::Component& next_component= signature_parameter.components[component_number];
		if( next_space != nullptr )
		{
			current_name= next_space->GetThisScopeName( next_component.name );

			if( next_space_class != nullptr &&
				names_scope.GetAccessFor( next_space_class ) < next_space_class->class_->GetMemberVisibility( next_component.name ) )
				errors_.push_back( ReportAccessingNonpublicClassMember( file_pos, next_space_class->class_->members.GetThisNamespaceName(), next_component.name ) );

		}
		else if( !is_last_component )
			return nullptr;

		++component_number;
	}

	return current_name;
}

DeducedTemplateParameter CodeBuilder::DeduceTemplateArguments(
	const TemplateBase& template_,
	const TemplateParameter& template_parameter,
	const Synt::ComplexName& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	const FilePos& template_file_pos= template_.file_pos;

	// Look if signature argument refers to template argument.
	size_t dependend_arg_index= ~0u;
	if( signature_parameter.components.size() == 1u && !signature_parameter.components.front().have_template_parameters )
	{
		for( const TypeTemplate::TemplateParameter& param : template_.template_parameters )
		{
			if( param.name == signature_parameter.components.front().name )
			{
				dependend_arg_index= &param - template_.template_parameters.data();
				break;
			}
		}
	}

	if( const Variable* const variable= boost::get<Variable>(&template_parameter) )
	{
		if( dependend_arg_index == ~0u )
		{
			const NamesScope::InsertedName* const signature_parameter_name=
				ResolveForTemplateSignatureParameter( signature_parameter_file_pos, signature_parameter, names_scope );
			if( signature_parameter_name == nullptr )
				return DeducedTemplateParameter::Invalid();

			const Variable* named_variable = signature_parameter_name->second.GetVariable();
			if( named_variable == nullptr )
			{
				if( const StoredVariablePtr stored_variable= signature_parameter_name->second.GetStoredVariable() )
					named_variable= &stored_variable->content;
			}
			if( named_variable != nullptr )
			{
				if( named_variable->type == variable->type &&
					TypeIsValidForTemplateVariableArgument( named_variable->type ) &&
					named_variable->constexpr_value != nullptr && variable->constexpr_value != nullptr &&
					named_variable->constexpr_value->getUniqueInteger() == variable->constexpr_value->getUniqueInteger() )
					return DeducedTemplateParameter::Variable();
			}
			return DeducedTemplateParameter::Invalid();
		}

		if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
		{
			errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( signature_parameter_file_pos, variable->type.ToString() ) );
			return DeducedTemplateParameter::Invalid();
		}

		if( variable->constexpr_value == nullptr )
		{
			errors_.push_back( ReportExpectedConstantExpression( signature_parameter_file_pos ) );
			return DeducedTemplateParameter::Invalid();
		}

		// Check given type and type from signature, deduce also some complex names.
		const DeducedTemplateParameter deduced_value_type=
			DeduceTemplateArguments(
				template_,
				variable->type,
				*template_.template_parameters[ dependend_arg_index ].type_name,
				signature_parameter_file_pos,
				deducible_template_parameters,
				names_scope );
		if( deduced_value_type.IsInvalid() )
			return DeducedTemplateParameter::Invalid();

		// Allocate global variable, because we needs address.

		Variable variable_for_insertion;
		variable_for_insertion.type= variable->type;
		variable_for_insertion.location= Variable::Location::Pointer;
		variable_for_insertion.value_type= ValueType::ConstReference;
		variable_for_insertion.llvm_value=
			CreateGlobalConstantVariable(
				variable->type,
				ToStdString( template_.template_parameters[ dependend_arg_index ].name ),
				variable->constexpr_value );
		variable_for_insertion.constexpr_value= variable->constexpr_value;

		if( boost::get<int>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
			deducible_template_parameters[ dependend_arg_index ]= std::move( variable_for_insertion ); // Set empty arg.
		else if( boost::get<Type>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
			return DeducedTemplateParameter::Invalid(); // WTF?
		else if( const Variable* const prev_variable_value= boost::get<Variable>( &deducible_template_parameters[ dependend_arg_index ] )  )
		{
			// Variable already known, Check conflicts.
			// TODO - do real comparision
			// TODO - maybe generate error in this case?
			if( prev_variable_value->constexpr_value->getUniqueInteger() != variable_for_insertion.constexpr_value->getUniqueInteger() )
				return DeducedTemplateParameter::Invalid();
		}

		return DeducedTemplateParameter::TemplateParameter();
	}

	const Type& given_type= boost::get<Type>(template_parameter);

	// Try deduce simple arg.
	if( dependend_arg_index != ~0u )
	{
		if( template_.template_parameters[ dependend_arg_index ].type_name != nullptr )
		{
			// Expected variable, but type given.
			return DeducedTemplateParameter::Invalid();
		}
		else if( boost::get<int>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
		{
			// Set empty arg.
			deducible_template_parameters[ dependend_arg_index ]= given_type;
		}
		else if( const Type* const prev_type= boost::get<Type>( &deducible_template_parameters[ dependend_arg_index ] ) )
		{
			// Type already known. Check conflicts.
			if( *prev_type != given_type )
				return DeducedTemplateParameter::Invalid();
		}
		else if( boost::get<Variable>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
		{
			// Bind type argument to variable parameter.
			return DeducedTemplateParameter::Invalid();
		}
		return DeducedTemplateParameter::TemplateParameter();
	}

	const NamesScope::InsertedName* const signature_parameter_name=
		ResolveForTemplateSignatureParameter( signature_parameter_file_pos, signature_parameter, names_scope );
	if( signature_parameter_name == nullptr )
		return DeducedTemplateParameter::Invalid();

	if( const Type* const type= signature_parameter_name->second.GetTypeName() )
	{
		if( *type == given_type )
			return DeducedTemplateParameter::Type();
		return DeducedTemplateParameter::Invalid();
	}
	else if( const TypeTemplatesSet* const inner_type_templates_set= signature_parameter_name->second.GetTypeTemplatesSet() )
	{
		const Class* const given_type_class= given_type.GetClassType();
		if( given_type_class == nullptr )
			return DeducedTemplateParameter::Invalid();
		if( given_type_class->base_template == boost::none )
			return DeducedTemplateParameter::Invalid();

		const TypeTemplate* inner_type_template= nullptr;
		for( const TypeTemplatePtr& candidate_template : *inner_type_templates_set)
		{
			if( candidate_template == given_type_class->base_template->class_template )
			{
				inner_type_template= candidate_template.get();
				break;
			}
		}
		if( inner_type_template == nullptr )
			return DeducedTemplateParameter::Invalid();

		const Synt::ComplexName::Component& name_component= signature_parameter.components.back();
		if( !name_component.have_template_parameters )
			return DeducedTemplateParameter::Invalid();
		if( name_component.template_parameters.size() < inner_type_template->first_optional_signature_argument )
			return DeducedTemplateParameter::Invalid();

		DeducedTemplateParameter::Template result;
		for( size_t i= 0u; i < name_component.template_parameters.size(); ++i)
		{
			DeducedTemplateParameter deduced=
				DeduceTemplateArguments(
					template_,
					given_type_class->base_template->signature_parameters[i],
					*name_component.template_parameters[i],
					template_file_pos,
					deducible_template_parameters,
					names_scope );
			if( deduced.IsInvalid() )
				return DeducedTemplateParameter::Invalid();
			result.args.push_back(std::move(deduced));
		}

		// Check, if given something, like std::tuple</ i32, std::vector</float/>, [ bool, 4 ] />.
		bool all_template_parameters_is_concrete= true;
		for( const DeducedTemplateParameter& param : result.args )
		{
			if( !( param.IsType() || param.IsVariable() ) )
			{
				all_template_parameters_is_concrete= false;
				break;
			}
		}
		if( all_template_parameters_is_concrete )
			return DeducedTemplateParameter::Type();

		return result;
	}

	return DeducedTemplateParameter::Invalid();
}

DeducedTemplateParameter CodeBuilder::DeduceTemplateArguments(
	const TemplateBase& template_,
	const TemplateParameter& template_parameter,
	const Synt::IExpressionComponent& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	if( const auto named_operand= dynamic_cast<const Synt::NamedOperand*>(&signature_parameter) )
	{
		if( named_operand->postfix_operators_.empty() && named_operand->prefix_operators_.empty() )
			return DeduceTemplateArguments( template_, template_parameter, named_operand->name_, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	}
	else if( const auto type_name= dynamic_cast<const Synt::TypeNameInExpression*>(&signature_parameter) )
		return DeduceTemplateArguments( template_, template_parameter, *type_name->type_name, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	else if( const auto bracket_expression= dynamic_cast<const Synt::BracketExpression*>(&signature_parameter) )
	{
		if( bracket_expression->postfix_operators_.empty() && bracket_expression->prefix_operators_.empty() )
			return DeduceTemplateArguments( template_, template_parameter, *bracket_expression->expression_, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	}

	// This is not special kind of template signature argument. So, process it as variable-expression.

	const Variable* const param_var= boost::get<const Variable>( &template_parameter );
	if( param_var == nullptr )
		return DeducedTemplateParameter::Invalid();
	if( !TypeIsValidForTemplateVariableArgument( param_var->type ) )
		return DeducedTemplateParameter::Invalid();

	const Value val= BuildExpressionCode( signature_parameter, names_scope, *dummy_function_context_ );
	if( val.GetVariable() == nullptr )
		return DeducedTemplateParameter::Invalid();
	const Variable& var= *val.GetVariable();
	if( !TypeIsValidForTemplateVariableArgument( var.type ) )
		return DeducedTemplateParameter::Invalid();

	// SPRACHE_TODO - maybe try compare integers without type?
	if( param_var->type != var.type )
		return DeducedTemplateParameter::Invalid();
	if( param_var->constexpr_value->getUniqueInteger() != var.constexpr_value->getUniqueInteger() )
		return DeducedTemplateParameter::Invalid();

	return DeducedTemplateParameter::Variable();
}

DeducedTemplateParameter CodeBuilder::DeduceTemplateArguments(
	const TemplateBase& template_,
	const TemplateParameter& template_parameter,
	const Synt::ITypeName& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	if( const auto named_type= dynamic_cast<const Synt::NamedTypeName*>(&signature_parameter) )
		return DeduceTemplateArguments( template_, template_parameter, named_type->name, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	else if( const auto array_type= dynamic_cast<const Synt::ArrayTypeName*>(&signature_parameter) )
	{
		const Type* const param_type= boost::get<const Type>( &template_parameter );
		if( param_type == nullptr )
			return DeducedTemplateParameter::Invalid();
		const Array* const param_array_type= param_type->GetArrayType();
		if( param_array_type == nullptr )
			return DeducedTemplateParameter::Invalid();

		Variable size_var;
		size_var.type= FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 ); // TODO - maybe selet native size type?
		size_var.value_type= ValueType::Value;
		size_var.llvm_value= size_var.constexpr_value=
			llvm::Constant::getIntegerValue( size_var.type.GetLLVMType(), llvm::APInt( size_var.type.SizeOf() * 8u, param_array_type->size ) );

		DeducedTemplateParameter::Array result;
		result.type.reset(
			new DeducedTemplateParameter(
				DeduceTemplateArguments( template_, param_array_type->type, *array_type->element_type, signature_parameter_file_pos, deducible_template_parameters, names_scope ) ) );
		result.size.reset(
			new DeducedTemplateParameter(
				DeduceTemplateArguments( template_, size_var, *array_type->size, signature_parameter_file_pos, deducible_template_parameters, names_scope ) ) );
		if( result.type->IsInvalid() || result.size->IsInvalid() ) // TODO - what is size is not variable, but type name? Check this.
			return DeducedTemplateParameter::Invalid();

		// All array parameters is known, so, this is concrete type.
		if( result.type->IsType() && result.size->IsVariable() )
			return DeducedTemplateParameter::Type();

		return std::move(result);
	}
	else if( const auto function_pointer_type= dynamic_cast<const Synt::FunctionType*>(&signature_parameter) )
	{
		const Type* const param_type= boost::get<const Type>( &template_parameter );
		if( param_type == nullptr )
			return DeducedTemplateParameter::Invalid();
		const FunctionPointer* const param_function_pointer_type= param_type->GetFunctionPointerType();
		if( param_function_pointer_type == nullptr )
			return DeducedTemplateParameter::Invalid();

		DeducedTemplateParameter::Function result;

		// Process return value.
		const bool expected_ret_mutable= function_pointer_type->return_value_mutability_modifier_ == MutabilityModifier::Mutable;
		const bool expected_ret_reference= function_pointer_type->return_value_reference_modifier_ == ReferenceModifier::Reference;
		if( expected_ret_mutable != param_function_pointer_type->function.return_value_is_mutable ||
			expected_ret_reference != param_function_pointer_type->function.return_value_is_reference )
			return DeducedTemplateParameter::Invalid();

		if( function_pointer_type->return_type_ != nullptr )
		{
			DeducedTemplateParameter ret_type_result=
				DeduceTemplateArguments(
					template_,
					param_function_pointer_type->function.return_type,
					*function_pointer_type->return_type_,
					signature_parameter_file_pos,
					deducible_template_parameters,
					names_scope );
			if( ret_type_result.IsInvalid() )
				return DeducedTemplateParameter::Invalid();
			result.return_type.reset( new DeducedTemplateParameter( std::move(ret_type_result) ) );
		}
		else
		{
			if( param_function_pointer_type->function.return_type != void_type_ )
				return DeducedTemplateParameter::Invalid();
			result.return_type.reset( new DeducedTemplateParameter( DeducedTemplateParameter::Type() ) );
		}

		if( !function_pointer_type->return_value_inner_reference_tags_.empty() ||
			!function_pointer_type->return_value_reference_tag_.empty() )
			errors_.push_back( ReportNotImplemented( function_pointer_type->file_pos_, "reference tags for template signature parameters" ) );

		// Process args.
		if( param_function_pointer_type->function.args.size() != function_pointer_type->arguments_.size() )
			return DeducedTemplateParameter::Invalid();
		for( size_t i= 0u; i < function_pointer_type->arguments_.size(); ++i)
		{
			const Synt::FunctionArgument& expected_arg= *function_pointer_type->arguments_[i];
			const Function::Arg& given_arg= param_function_pointer_type->function.args[i];

			const bool expected_mutable= expected_arg.mutability_modifier_ == MutabilityModifier::Mutable;
			const bool expected_reference= expected_arg.reference_modifier_ == ReferenceModifier::Reference;

			if( expected_mutable != given_arg.is_mutable || expected_reference != given_arg.is_reference )
				return DeducedTemplateParameter::Invalid();

			result.argument_types.push_back(
				DeduceTemplateArguments( template_, given_arg.type, *expected_arg.type_, signature_parameter_file_pos, deducible_template_parameters, names_scope ));

			if( result.argument_types.back().IsInvalid() )
				return DeducedTemplateParameter::Invalid();

			if( !expected_arg.inner_arg_reference_tags_.empty() || !expected_arg.reference_tag_.empty() )
				errors_.push_back( ReportNotImplemented( function_pointer_type->file_pos_, "reference tags for template signature parameters" ) );
		}

		if( param_function_pointer_type->function.unsafe != function_pointer_type->unsafe_ )
			return DeducedTemplateParameter::Invalid();

		bool all_types_are_known= true;
		if( !result.return_type->IsType() )
			all_types_are_known= false;
		for( const DeducedTemplateParameter& arg : result.argument_types )
		{
			if( !arg.IsType() )
				all_types_are_known= false;
			if( arg.IsVariable() )
				return DeducedTemplateParameter::Invalid();
		}

		if( result.return_type->IsVariable() )
			return DeducedTemplateParameter::Invalid();

		if( all_types_are_known )
			return DeducedTemplateParameter::Type();

		return result;
	}

	else U_ASSERT(false);

	return DeducedTemplateParameter::Invalid();
}

NamesScope::InsertedName* CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatesSet& type_templates_set,
	const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
	NamesScope& arguments_names_scope )
{
	std::vector<NamesScope::InsertedName*> generated_types;
	for( const TypeTemplatePtr& type_template : type_templates_set )
	{
		NamesScope::InsertedName* const generated_type=
			GenTemplateType(
				file_pos,
				type_template,
				template_arguments,
				arguments_names_scope );
		if( generated_type != nullptr )
			generated_types.push_back( generated_type );
	}

	if( generated_types.empty() )
		return nullptr;

	if( generated_types.size() == 1u )
		return generated_types.front();

	// TODO - generate separate error
	errors_.push_back( ReportNotImplemented( file_pos, "selection over multiple valid type templates" ) );
	return nullptr;
}

NamesScope::InsertedName* CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatePtr& type_template_ptr,
	const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
	NamesScope& arguments_names_scope )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	const TypeTemplate& type_template= *type_template_ptr;
	NamesScope& template_names_scope= *type_template.parent_namespace;

	if( template_arguments.size() < type_template.first_optional_signature_argument )
	{
		return nullptr;
	}

	DeducibleTemplateParameters deduced_template_args( type_template.template_parameters.size() );

	PushCacheGetResolveHandelr( type_template.resolving_cache );

	const NamesScopePtr template_parameters_namespace = std::make_shared<NamesScope>( ""_SpC, &template_names_scope );
	for( const TypeTemplate::TemplateParameter& param : type_template.template_parameters )
		template_parameters_namespace->AddName( param.name, YetNotDeducedTemplateArg() );

	bool is_template_dependent= false;
	bool deduction_failed= false;
	std::vector<TemplateParameter> result_signature_parameters(type_template.signature_arguments.size());
	for( size_t i= 0u; i < type_template.signature_arguments.size(); ++i )
	{
		Value value;
		if( i < template_arguments.size() )
			value= BuildExpressionCode( *template_arguments[i], arguments_names_scope, *dummy_function_context_ );
		else
			value= BuildExpressionCode( *type_template.default_signature_arguments[i], *template_parameters_namespace, *dummy_function_context_ );

		if( value.GetErrorValue() != nullptr )
			continue;

		// Each template with template-dependent signature arguments is template-dependent values.
		if( value.GetType() == NontypeStub::TemplateDependentValue ||
			value.GetType().GetTemplateDependentType() != nullptr )
		{
			is_template_dependent= true;
			continue;
		}
		if( const Type* const type_name= value.GetTypeName() )
		{
			if( type_name->GetTemplateDependentType() != nullptr )
			{
				is_template_dependent= true;
				continue;
			}
		}

		const Synt::IExpressionComponent& expr= *type_template.signature_arguments[i];
		// TODO - maybe add some errors, if not deduced?
		if( const Type* const type_name= value.GetTypeName() )
		{
			const DeducedTemplateParameter deduced= DeduceTemplateArguments( type_template, *type_name, expr, file_pos, deduced_template_args, template_names_scope );
			if( deduced.IsInvalid() )
			{
				deduction_failed= true;
				continue;
			}
		}
		else if( const Variable* const variable= value.GetVariable() )
		{
			const DeducedTemplateParameter deduced= DeduceTemplateArguments( type_template, *variable, expr, file_pos, deduced_template_args, template_names_scope );
			if( deduced.IsInvalid() )
			{
				deduction_failed= true;
				continue;
			}
		}
		else
		{
			errors_.push_back( ReportInvalidValueAsTemplateArgument( file_pos, value.GetType().ToString() ) );
			continue;
		}

		// Update known arguments.
		for( size_t j= 0u; j < deduced_template_args.size(); ++j )
		{
			const DeducibleTemplateParameter& arg= deduced_template_args[j];
			NamesScope::InsertedName* const name= template_parameters_namespace->GetThisScopeName( type_template.template_parameters[j].name );
			U_ASSERT( name != nullptr );

			if( boost::get<int>( &arg ) != nullptr )
			{} // Not deduced yet.
			else if( const Type* const type= boost::get<Type>( &arg ) )
			{
				if( name->second.GetYetNotDeducedTemplateArg() != nullptr )
					name->second= Value( *type, type_template_ptr->syntax_element->file_pos_ /*TODO - set correctfile_pos */ );
			}
			else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			{
				if( name->second.GetYetNotDeducedTemplateArg() != nullptr )
					name->second= Value( *variable, type_template_ptr->syntax_element->file_pos_ /*TODO - set correctfile_pos */ );
			}
			else U_ASSERT( false );
		}

		if( !deduction_failed )
		{
			const Value result_singature_parameter= BuildExpressionCode( expr, *template_parameters_namespace, *dummy_function_context_ );
			if( const Type* const type_name= result_singature_parameter.GetTypeName() )
				result_signature_parameters[i]= *type_name;
			else if( const Variable* const variable= result_singature_parameter.GetVariable() )
				result_signature_parameters[i]= *variable;
		}

	} // for signature arguments

	if( deduction_failed )
	{
		PopResolveHandler();
		return nullptr;
	}
	if( is_template_dependent )
	{
		PopResolveHandler();
		return
			template_names_scope.AddName(
				"_tdv"_SpC + ToProgramString( std::to_string(next_template_dependent_type_index_).c_str() ),
				Value( GetNextTemplateDependentType(), type_template_ptr->syntax_element->file_pos_ /*TODO - set correctfile_pos */ ) );
	}

	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];

		if( boost::get<int>( &arg ) != nullptr )
		{
			// SPRACHE_TODO - maybe not generate this error?
			// Other function templates, for example, can match given aruments.
			errors_.push_back( ReportTemplateParametersDeductionFailed( file_pos ) );
			PopResolveHandler();
			return nullptr;
		}
	}

	// Encode name.
	// TODO - maybe generate correct mangled name for template?
	ProgramString name_encoded= g_template_parameters_namespace_prefix + type_template.syntax_element->name_;
	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];
		if( const Type* const type= boost::get<Type>( &arg ) )
		{
			// We needs full mangled name of template parameter here, because short type names from different spaces may coincide.
			name_encoded+= ToProgramString( MangleType( *type ).c_str() );
		}
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
		{
			// Currently, can be only integer or enum type.
			FundamentalType raw_type;
			if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType () )
				raw_type= *fundamental_type;
			else if( const Enum* const enum_type= variable->type.GetEnumType () )
				raw_type= enum_type->underlaying_type;
			else
				U_ASSERT( false );

			const llvm::APInt& int_value= variable->constexpr_value->getUniqueInteger();
			if( IsSignedInteger( raw_type.fundamental_type ) && int_value.isNegative() )
				name_encoded+= ToProgramString( std::to_string(  int64_t(int_value.getLimitedValue()) ).c_str() );
			else
				name_encoded+= ToProgramString( std::to_string( uint64_t(int_value.getLimitedValue()) ).c_str() );
		}
		else U_ASSERT(false);
	}
	name_encoded+= ToProgramString( std::to_string( reinterpret_cast<uintptr_t>(&type_template) ).c_str() ); // HACK. encode also template itself, because we can have multiple templates with same name.

	// Check, if already type generated.
	// In recursive instantiation this also works.
	if( NamesScope::InsertedName* const inserted_name= template_names_scope.GetThisScopeName( name_encoded ) )
	{
		// Already generated.
		PopResolveHandler();

		const NamesScopePtr template_parameters_space= inserted_name->second.GetNamespace();
		U_ASSERT( template_parameters_space != nullptr );
		return template_parameters_space->GetThisScopeName( GetNameForGeneratedClass() );
	}

	template_parameters_namespace->SetThisNamespaceName( name_encoded );
	template_names_scope.AddName( name_encoded, Value( template_parameters_namespace, type_template_ptr->syntax_element->file_pos_ /* TODO - check file_pos */ ) );

	if( const Synt::ClassTemplate* const template_class= dynamic_cast<const Synt::ClassTemplate*>( type_template.syntax_element ) )
	{
		const TemplateClassKey class_key{ type_template_ptr, name_encoded };
		const auto cache_class_it= template_classes_cache_.find( class_key );
		if( cache_class_it != template_classes_cache_.end() )
		{
			PopResolveHandler();

			return
				template_parameters_namespace->AddName(
					GetNameForGeneratedClass(),
					Value(
						cache_class_it->second,
						type_template_ptr->syntax_element->file_pos_ /* TODO - check file_pos */ ) );
		}

		const ClassProxyPtr class_proxy= PrepareClass( *template_class->class_, GetComplexNameForGeneratedClass(), *template_parameters_namespace );

		PopResolveHandler();

		if( class_proxy == nullptr )
			return nullptr;

		Class& the_class= *class_proxy->class_;
		// Save in class info about it`s base template.
		the_class.base_template.emplace();
		the_class.base_template->class_template= type_template_ptr;
		for( const DeducibleTemplateParameter& arg : deduced_template_args )
		{
			if( const Type* const type= boost::get<Type>( &arg ) )
				the_class.base_template->template_parameters.push_back( *type );
			else if( const Variable* const variable= boost::get<Variable>( &arg ) )
				the_class.base_template->template_parameters.push_back( *variable );
			else U_ASSERT(false);
		}
		the_class.base_template->signature_parameters= std::move(result_signature_parameters);

		template_classes_cache_[class_key]= class_proxy;
		return template_parameters_namespace->GetThisScopeName( GetNameForGeneratedClass() );
	}
	else if( const Synt::TypedefTemplate* const typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( type_template.syntax_element ) )
	{
		const Type type= PrepareType( typedef_template->typedef_->value, *template_parameters_namespace );

		PopResolveHandler();

		if( type == invalid_type_ )
			return nullptr;

		// HACK - add name to map for correct result returning.
		return template_parameters_namespace->AddName( GetNameForGeneratedClass(), Value( type, file_pos /* TODO - check file_pos */ ) );
	}
	else
		U_ASSERT(false);

	return nullptr;
}

const FunctionVariable* CodeBuilder::GenTemplateFunction(
	const FilePos& file_pos,
	const FunctionTemplatePtr& function_template_ptr,
	NamesScope& template_names_scope,
	const std::vector<Function::Arg>& actual_args,
	const bool first_actual_arg_is_this,
	bool skip_arguments )
{
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function_;

	const Function::Arg* given_args= actual_args.data();
	size_t given_arg_count= actual_args.size();

	if( first_actual_arg_is_this &&
		!function_declaration.type_.arguments_.empty() && function_declaration.type_.arguments_.front()->name_ != Keywords::this_ )
	{
		++given_args;
		--given_arg_count;
	}

	if( !skip_arguments && given_arg_count != function_declaration.type_.arguments_.size() )
		return nullptr;

	DeducibleTemplateParameters deduced_template_args( function_template.template_parameters.size() );

	NamesScope template_parameters_namespace( ""_SpC, &template_names_scope );
	for( size_t i= 0u; i < function_template.template_parameters.size(); ++i )
		template_parameters_namespace.AddName( function_template.template_parameters[i].name, YetNotDeducedTemplateArg() );
	for( size_t i= 0u; i < function_template.known_template_parameters.size(); ++i )
		template_parameters_namespace.AddName( function_template.known_template_parameters[i].first, function_template.known_template_parameters[i].second );

	PushCacheGetResolveHandelr( function_template.resolving_cache );

	bool deduction_failed= false;
	std::vector<DeducedTemplateParameter> deduced_temlpate_parameters( function_declaration.type_.arguments_.size() );
	for( size_t i= 0u; i < function_declaration.type_.arguments_.size() && !skip_arguments; ++i )
	{
		const Synt::FunctionArgument& function_argument= *function_declaration.type_.arguments_[i];

		// Functin arg declared as "mut&", but given something immutable.
		if( function_argument.mutability_modifier_ == Synt::MutabilityModifier::Mutable &&
			( function_argument.reference_modifier_ == Synt::ReferenceModifier::Reference || function_argument.name_ == Keywords::this_ ) &&
			!given_args[i].is_mutable )
		{
			deduction_failed= true;
			continue;
		}

		if( i == 0u && function_argument.name_ == Keywords::this_ )
		{
			if( function_template.base_class == nullptr || // Can be in case of error.
				given_args[i].type != function_template.base_class )
			{
				// Givent type and type of "this" are different.
				deduction_failed= true;
				continue;
			}
			deduced_temlpate_parameters[i]= DeducedTemplateParameter::Type();
		}
		else
		{
			// For named types we check, if reference (and type in future) conversion is possible, and if not, do arguments deduction.
			bool deduced_specially= false;
			if( const Synt::NamedTypeName* const named_type_name= dynamic_cast<Synt::NamedTypeName*>( function_argument.type_.get() ) )
			{
				size_t dependend_arg_index= ~0u;
				if( named_type_name->name.components.size() == 1u && !named_type_name->name.components.front().have_template_parameters )
				{
					for( const TypeTemplate::TemplateParameter& param : function_template.template_parameters )
					{
						if( param.name == named_type_name->name.components.front().name )
						{
							dependend_arg_index= &param - function_template.template_parameters.data();
							break;
						}
					}
				}

				if( dependend_arg_index == ~0u )
				{
					// Not template parameter, must be type name or template.
					const NamesScope::InsertedName* const signature_parameter_name=
						ResolveForTemplateSignatureParameter( named_type_name->file_pos_, named_type_name->name, template_parameters_namespace /*TODO - is this correct namespace? */ );
					if( signature_parameter_name == nullptr )
					{
						deduction_failed= true;
						continue;
					}
					if( const Type* const type= signature_parameter_name->second.GetTypeName() )
					{
						if( *type == given_args[i].type || given_args[i].type.ReferenceIsConvertibleTo( *type ) )
						{
							deduced_temlpate_parameters[i]= DeducedTemplateParameter::Type();
							deduced_specially= true;
						}
						else
						{
							deduction_failed= true;
							continue;
						}
					}
				}
			}

			if( !deduced_specially )
			{
				DeducedTemplateParameter deduced=
					DeduceTemplateArguments(
						function_template,
						given_args[i].type,
						*function_argument.type_,
						function_argument.file_pos_,
						deduced_template_args,
						template_parameters_namespace /*TODO - is this correct namespace? */ );
				if( deduced.IsInvalid() )
				{
					deduction_failed= true;
					continue;
				}
				deduced_temlpate_parameters[i]= std::move(deduced);
			}
		}

		// Update known arguments in names scope.
		for( size_t j= 0u; j < deduced_template_args.size(); ++j )
		{
			const DeducibleTemplateParameter& arg= deduced_template_args[j];
			NamesScope::InsertedName* const name= template_parameters_namespace.GetThisScopeName( function_template.template_parameters[j].name );
			U_ASSERT( name != nullptr );

			if( boost::get<int>( &arg ) != nullptr )
			{} // Not deduced yet.
			else if( const Type* const type= boost::get<Type>( &arg ) )
			{
				if( name->second.GetYetNotDeducedTemplateArg() != nullptr )
					name->second= Value( *type, function_template.file_pos /*TODO - set correctfile_pos */ );
			}
			else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			{
				if( name->second.GetYetNotDeducedTemplateArg() != nullptr )
					name->second= Value( *variable, function_template.file_pos /*TODO - set correctfile_pos */ );
			}
			else U_ASSERT( false );
		}

	} // for template function arguments

	if( deduction_failed )
	{
		PopResolveHandler();
		return nullptr;
	}

	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];

		if( boost::get<int>( &arg ) != nullptr )
		{
			errors_.push_back( ReportTemplateParametersDeductionFailed( file_pos ) );
			PopResolveHandler();
			return nullptr;
		}
	}

	// TODO - move to function, with same code in CodeBuilder::GenTemplateType
	// Encode name.
	// TODO - maybe generate correct mangled name for template?
	ProgramString name_encoded= g_template_parameters_namespace_prefix + function_template.syntax_element->function_->name_.components.front().name;
	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];
		if( const Type* const type= boost::get<Type>( &arg ) )
		{
			// We needs full mangled name of template parameter here, because short type names from different spaces may coincide.
			name_encoded+= ToProgramString( MangleType( *type ).c_str() );
		}
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
		{
			// Currently, can be only integer or enum type.
			FundamentalType raw_type;
			if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType () )
				raw_type= *fundamental_type;
			else if( const Enum* const enum_type= variable->type.GetEnumType () )
				raw_type= enum_type->underlaying_type;
			else
				U_ASSERT( false );

			const llvm::APInt& int_value= variable->constexpr_value->getUniqueInteger();
			if( IsSignedInteger( raw_type.fundamental_type ) && int_value.isNegative() )
				name_encoded+= ToProgramString( std::to_string(  int64_t(int_value.getLimitedValue()) ).c_str() );
			else
				name_encoded+= ToProgramString( std::to_string( uint64_t(int_value.getLimitedValue()) ).c_str() );
		}
		else U_ASSERT(false);
	}
	name_encoded += ToProgramString( std::to_string( reinterpret_cast<uintptr_t>(&function_template) ).c_str() ); // HACK! use address of template object, because we can have multiple templates with same name.

	if( const NamesScope::InsertedName* const inserted_name= function_template.parent_namespace->GetThisScopeName( name_encoded ) )
	{
		//Function for this template arguments already generated.
		PopResolveHandler();
		return inserted_name->second.GetFunctionVariable();
	}

	// First, prepare only as prototype.
	const PrepareFunctionResult prepare_result=
		PrepareFunction( function_declaration, true, function_template.base_class, template_parameters_namespace );

	if( prepare_result.functions_set == nullptr ||
		prepare_result.function_index >= prepare_result.functions_set->functions.size() )
	{
		PopResolveHandler();
		return nullptr; // Function prepare failed
	}

	// Insert generated function
	FunctionVariable function_variable= prepare_result.functions_set->functions[prepare_result.function_index];
	function_variable.deduced_temlpate_parameters= std::move(deduced_temlpate_parameters);
	const NamesScope::InsertedName* const inserted_function_name= function_template.parent_namespace->AddName( name_encoded, function_variable );
	U_ASSERT( inserted_function_name != nullptr );

	// And generate function body after insertion of prototype.
	PrepareFunction( function_declaration, false, function_template.base_class, template_parameters_namespace );
	PopResolveHandler();

	// Two-step preparation needs for recursive function template call.

	return inserted_function_name->second.GetFunctionVariable();
}

const NamesScope::InsertedName* CodeBuilder::GenTemplateFunctionsUsingTemplateParameters(
	const FilePos& file_pos,
	const std::vector<FunctionTemplatePtr>& function_templates,
	const std::vector<Synt::IExpressionComponentPtr>& template_arguments,
	NamesScope& template_names_scope,
	NamesScope& arguments_names_scope )
{
	U_ASSERT( !function_templates.empty() );

	DeducibleTemplateParameters template_parameters;
	bool is_template_dependent= false;
	bool something_is_wrong= false;
	for( const Synt::IExpressionComponentPtr& expr : template_arguments )
	{
		const Value value= BuildExpressionCode( *expr, arguments_names_scope, *dummy_function_context_ );
		if( value.GetType() == NontypeStub::TemplateDependentValue ||
			value.GetType().GetTemplateDependentType() != nullptr )
		{
			is_template_dependent= true;
			continue;
		}
		else if( const auto type_name= value.GetTypeName() )
		{
			if( type_name->GetTemplateDependentType() != nullptr )
			{
				is_template_dependent= true;
				continue;
			}
			template_parameters.push_back( *type_name );
		}
		else if( const auto variable= value.GetVariable() )
		{
			if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
				errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( expr->GetFilePos(), variable->type.ToString() ) );
			else if( variable->constexpr_value == nullptr )
				errors_.push_back( ReportExpectedConstantExpression( expr->GetFilePos() ) );
			else
				template_parameters.push_back( *variable );
		}
		else
		{
			errors_.push_back( ReportInvalidValueAsTemplateArgument( file_pos, value.GetType().ToString() ) );
			something_is_wrong= true;
		}

	} // for given template arguments.

	if( something_is_wrong )
		return nullptr;
	if( is_template_dependent )
		return &template_names_scope.GetTemplateDependentValue();

	// Encode name, based on set of function templates and given tempate parameters.
	ProgramString name_encoded= g_template_parameters_namespace_prefix;
	for( const FunctionTemplatePtr& function_template_ptr : function_templates )
		name_encoded += ToProgramString( std::to_string( reinterpret_cast<uintptr_t>(function_template_ptr.get()) ).c_str() );
	for( size_t i = 0u; i < template_parameters.size() ; ++i )
	{
		const auto& arg = template_parameters[i];
		if( const Type* const type= boost::get<Type>( &arg ) )
		{
			// We needs full mangled name of template parameter here, because short type names from different spaces may coincide.
			name_encoded+= ToProgramString( MangleType( *type ).c_str() );
		}
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
		{
			// Currently, can be only integer or enum type.
			FundamentalType raw_type;
			if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType () )
				raw_type= *fundamental_type;
			else if( const Enum* const enum_type= variable->type.GetEnumType () )
				raw_type= enum_type->underlaying_type;
			else
				U_ASSERT( false );

			const llvm::APInt& int_value= variable->constexpr_value->getUniqueInteger();
			if( IsSignedInteger( raw_type.fundamental_type ) && int_value.isNegative() )
				name_encoded+= ToProgramString( std::to_string(  int64_t(int_value.getLimitedValue()) ).c_str() );
			else
				name_encoded+= ToProgramString( std::to_string( uint64_t(int_value.getLimitedValue()) ).c_str() );
		}
		else U_ASSERT(false);
	}

	if( const auto prev_name= template_names_scope.GetThisScopeName( name_encoded ) )
		return prev_name; // Already generated.

	OverloadedFunctionsSet result;
	for( const FunctionTemplatePtr& function_template_ptr : function_templates )
	{
		const FunctionTemplate& function_template= *function_template_ptr;
		if( template_parameters.size() > function_template.template_parameters.size() )
			continue;

		// Check given arguments and template parameters.
		bool ok= true;
		for( size_t i= 0u; i < template_parameters.size(); ++i )
		{
			const TemplateBase::TemplateParameter& function_template_parameter= function_template.template_parameters[i];
			if( boost::get<Type>( &template_parameters[i] ) != nullptr )
			{
				if( function_template_parameter.type_name != nullptr )
				{
					ok= false; // Error, type parameter given, but value parameter expected.
					break;
				}
			}
			else if( const Variable* const given_variable= boost::get<Variable>( &template_parameters[i] ) )
			{
				if( function_template_parameter.type_name == nullptr )
				{
					ok= false; // Error, value parameter given, but type parameter expected.
					break;
				}

				// Check type.
				size_t type_parameter_index= ~0u;
				if( function_template_parameter.type_name->components.size() == 1 && !function_template_parameter.type_name->components.front().have_template_parameters )
				{
					for( size_t j= 0u; j < i; ++j )
					{
						if( function_template_parameter.type_name->components.front().name == function_template.template_parameters[j].name )
						{
							type_parameter_index= j;
							break;
						}
					}
				}
				if( type_parameter_index != ~0u )
				{
					if( const Type* const expected_type= boost::get<Type>( &template_parameters[type_parameter_index] ) )
						ok= *expected_type == given_variable->type;
					else
						ok= false;
				}
				else
				{
					PushCacheGetResolveHandelr( function_template.resolving_cache );
					if( const NamesScope::InsertedName* const type_name=
							ResolveName( function_template.file_pos, *function_template.parent_namespace, *function_template_parameter.type_name ) )
					{
						if( const Type* const expected_type= type_name->second.GetTypeName() )
							ok= *expected_type == given_variable->type;
						else
							ok= false;
					}
					else
						ok= false;
					PopResolveHandler();
				}

				if( !ok )
					break;
			}
			else U_ASSERT(false);

		} // for given template parameters

		if( !ok )
			continue;

		FunctionTemplatePtr new_template( new FunctionTemplate );
		// Reduce count of template arguments in new function template.
		new_template->template_parameters.insert(
			new_template->template_parameters.end(),
			function_template.template_parameters.begin() + template_parameters.size(), function_template.template_parameters.end() );

		new_template->resolving_cache= function_template.resolving_cache;
		new_template->parent_namespace= function_template.parent_namespace;
		new_template->file_pos= function_template.file_pos;
		new_template->syntax_element= function_template.syntax_element;
		new_template->base_class= function_template.base_class;

		// Fill set of known parameters.
		for( size_t i= 0u; i < template_parameters.size(); ++i )
		{
			const ProgramString& name= function_template.template_parameters[i].name;
			if( const Type* const type= boost::get<Type>( &template_parameters[i] ) )
				new_template->known_template_parameters.emplace_back( name, Value( *type, file_pos ) );
			else if( const Variable* const variable= boost::get<Variable>( &template_parameters[i] ) )
				new_template->known_template_parameters.emplace_back( name, Value( *variable, file_pos ) );
			else U_ASSERT(false);
		}

		result.template_functions.push_back( new_template );
	} // for function templates

	if( result.template_functions.empty() )
	{
		errors_.push_back( ReportTemplateFunctionGenerationFailed( file_pos, function_templates.front()->syntax_element->function_->name_.components.back().name ) );
		return nullptr;
	}

	return template_names_scope.AddName( name_encoded, result );
}

bool CodeBuilder::NameShadowsTemplateArgument( const ProgramString& name, NamesScope& names_scope )
{
	Synt::ComplexName::Component component;
	component.name= name;
	component.is_generated= true;
	const NamesScope::InsertedName* const name_resolved= ResolveName( FilePos(), names_scope, &component, 1u );
	if( name_resolved == nullptr )
		return false;

	return name_resolved->second.IsTemplateParameter();
}

TemplateDependentType CodeBuilder::GetNextTemplateDependentType()
{
	return TemplateDependentType( next_template_dependent_type_index_++, fundamental_llvm_types_.invalid_type_ );
}

bool CodeBuilder::TypeIsValidForTemplateVariableArgument( const Type& type )
{
	if( const FundamentalType* const fundamental= type.GetFundamentalType() )
	{
		if( IsInteger( fundamental->fundamental_type ) || fundamental->fundamental_type == U_FundamentalType::Bool )
			return true;
	}
	if( type.GetEnumType() != nullptr )
	{
		U_ASSERT( TypeIsValidForTemplateVariableArgument( type.GetEnumType()->underlaying_type ) );
		return true;
	}
	if( type.GetFunctionPointerType() != nullptr )
		return true;

	return false;
}

void CodeBuilder::RemoveTempClassLLVMValues( Class& class_ )
{
	RemoveTempClassLLVMValues_impl( class_, false );
	RemoveTempClassLLVMValues_impl( class_, true );
}

void CodeBuilder::RemoveTempClassLLVMValues_impl( Class& class_, const bool is_delete_pass )
{
	class_.members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& name )
		{
			if( const Type* const type= name.second.GetTypeName() )
			{
				if( Class* const subclass= type->GetClassType() )
					RemoveTempClassLLVMValues_impl( *subclass, is_delete_pass );
			}
			else if( const OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : functions_set->functions )
				{
					if( is_delete_pass )
						function.llvm_function->eraseFromParent();
					else
					{
						function.llvm_function->dropAllReferences();
					}
				}
			}
			else if( name.second.GetClassField() != nullptr )
			{}
			else if( name.second.GetTypeTemplatesSet() != nullptr )
			{}
			else if( const NamesScopePtr inner_namespace= name.second.GetNamespace() )
			{
				const ProgramString& generated_class_name= GetNameForGeneratedClass();

				// This must be only namespace for class template instantiation.
				inner_namespace->ForEachInThisScope(
					[&]( const NamesScope::InsertedName& inner_namespace_name )
					{
						if( inner_namespace_name.first == generated_class_name )
						{
							const Type* const generated_class_type= inner_namespace_name.second.GetTypeName();
							U_ASSERT( generated_class_type != nullptr );
							Class* const generated_class= generated_class_type->GetClassType();
							U_ASSERT( generated_class != nullptr );
							U_ASSERT( generated_class->base_template != boost::none );
							RemoveTempClassLLVMValues_impl( *generated_class, is_delete_pass );
						}
					});
			}
			else if( const Variable* const variable= name.second.GetVariable() )
			{
				U_UNUSED(variable);
				// TODO - maybe we can delete global variable without breaking llvm code structure?
			}
			else if( const StoredVariablePtr stored_variable= name.second.GetStoredVariable() )
			{
				U_UNUSED(stored_variable);
				// TODO - maybe we can delete global variable without breaking llvm code structure?
			}
			else if( name.second.GetTemplateDependentValue() != nullptr )
			{}
			else U_ASSERT(false);
		});

	if( is_delete_pass )
	{
		if( class_.this_class_virtual_table != nullptr )
			class_.this_class_virtual_table->dropAllReferences();
		for( const auto& vt : class_.ancestors_virtual_tables )
			vt.second->dropAllReferences();
	}
	else
	{
		if( class_.this_class_virtual_table != nullptr )
			class_.this_class_virtual_table->eraseFromParent();
		for( const auto& vt : class_.ancestors_virtual_tables )
			vt.second->eraseFromParent();
	}
}

void CodeBuilder::CleareDummyFunction()
{
	llvm::Function::BasicBlockListType& bb_list= dummy_function_context_->function->getBasicBlockList();

	// Clear blocks in reverse order, because newer blocks can depend on elder blocks.
	for (llvm::BasicBlock& block : boost::adaptors::reverse(bb_list))
	{
		// Destroy instructions in reverse order.
		while( ! block.getInstList().empty() )
			block.getInstList().pop_back();
		//block.getInstList().clear();
	}
}

void CodeBuilder::ReportAboutIncompleteMembersOfTemplateClass( const FilePos& file_pos, Class& class_ )
{
	class_.members.ForEachInThisScope(
		[this, file_pos]( const NamesScope::InsertedName& name )
		{
			if( const Type* const type= name.second.GetTypeName() )
			{
				if( Class* const subclass= type->GetClassType() )
				{
					if( subclass->completeness != Class::Completeness::Complete )
						errors_.push_back( ReportIncompleteMemberOfClassTemplate( file_pos, name.first ) );
					else
						ReportAboutIncompleteMembersOfTemplateClass( file_pos, *subclass );
				}
			}
			else if( const OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : functions_set->functions )
				{
					if( !function.have_body )
						errors_.push_back( ReportIncompleteMemberOfClassTemplate( file_pos, name.first ) );
				}
			}
			else if( name.second.GetClassField() != nullptr )
			{}
			else if( name.second.GetTypeTemplatesSet() != nullptr )
			{}
			else if( const NamesScopePtr inner_namespace= name.second.GetNamespace() )
			{
				const ProgramString& generated_class_name= GetNameForGeneratedClass();

				// This must be only namespace for class template instantiation.
				inner_namespace->ForEachInThisScope(
					[&]( const NamesScope::InsertedName& inner_namespace_name )
					{
						if( inner_namespace_name.first == generated_class_name )
						{
							const Type* const generated_class_type= inner_namespace_name.second.GetTypeName();
							U_ASSERT( generated_class_type != nullptr );
							Class* const generated_class= generated_class_type->GetClassType();
							U_ASSERT( generated_class != nullptr );
							U_ASSERT( generated_class->base_template != boost::none );
							ReportAboutIncompleteMembersOfTemplateClass( file_pos, *generated_class );
						}
					});
			}
			else if( name.second.GetVariable() != nullptr )
			{}
			else if( name.second.GetStoredVariable() != nullptr )
			{}
			else if( name.second.GetTemplateDependentValue() != nullptr )
			{}
			else
				U_ASSERT(false);
		} );
}

} // namespace CodeBuilderPrivate

} // namespace U
