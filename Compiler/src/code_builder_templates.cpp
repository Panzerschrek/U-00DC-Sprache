#include <algorithm>

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

static const ComplexName& GetComplexNameForGeneratedClass()
{
	static ComplexName name;
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

void CodeBuilder::PrepareTypeTemplate(
	const TemplateBase& type_template_declaration,
	NamesScope& names_scope )
{
	/* SPRACHE_TODO:
	 *) Convert signature and template arguments to "default form" for equality comparison.
	 *) Support templates overloading.
	 *) Add "enable_if".
	 *) Support short template form, where template parameters are also signature parameters.
	 *) Support signature parameters with expressions inside template parameters, different, than NamedOperand.
	 *) Support template-dependent types for value parameters, such template</ type T, U</ T /> ut />.
	*/

	const TypeTemplatePtr type_template( new TypeTemplate );
	const ProgramString& type_template_name= type_template_declaration.name_;

	if( names_scope.AddName( type_template_name, Value( type_template, type_template_declaration.file_pos_ ) ) == nullptr )
	{
		errors_.push_back( ReportRedefinition( type_template_declaration.file_pos_, type_template_name ) );
		return;
	}

	type_template->parent_namespace= &names_scope;
	type_template->syntax_element= &type_template_declaration;

	std::vector<TypeTemplate::TemplateParameter>& template_parameters= type_template->template_parameters;
	template_parameters.reserve( type_template_declaration.args_.size() );
	std::vector<bool> template_parameters_usage_flags;

	PushCacheFillResolveHandler( type_template->resolving_cache, names_scope );
	const NamesScopePtr template_parameters_namespace = std::make_shared<NamesScope>( g_template_parameters_namespace_prefix, &names_scope );

	// Check and fill template parameters.
	for( const TemplateBase::Arg& arg : type_template_declaration.args_ )
	{
		U_ASSERT( arg.name.components.size() == 1u );
		const ProgramString& arg_name= arg.name.components.front().name;

		// Check redefinition
		for( const auto& prev_arg : template_parameters )
		{
			if( prev_arg.name == arg_name )
			{
				errors_.push_back( ReportRedefinition( type_template_declaration.file_pos_, arg_name ) );
				continue;
			}
		}
		if( NameShadowsTemplateArgument( arg_name, names_scope ) )
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( type_template_declaration.file_pos_, arg_name ) );

		NamesScope::InsertedName* inserted_template_parameter= nullptr;

		if( !arg.arg_type.components.empty() )
		{
			// If template parameter is variable.

			// Resolve from outer space or from this template parameters.
			const NamesScope::InsertedName* const type_name= ResolveName( type_template_declaration.file_pos_, *template_parameters_namespace, arg.arg_type );
			if( type_name == nullptr )
			{
				errors_.push_back( ReportNameNotFound( type_template_declaration.file_pos_, arg.arg_type ) );
				continue;
			}
			const Type* const type= type_name->second.GetTypeName();
			if( type == nullptr )
			{
				errors_.push_back( ReportNameIsNotTypeName( type_template_declaration.file_pos_, type_name->first ) );
				continue;
			}

			if( type->GetTemplateDependentType() == nullptr &&
				!TypeIsValidForTemplateVariableArgument( *type ) )
			{
				errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( type_template_declaration.file_pos_, type->ToString() ) );
				continue;
			}

			// If type is template parameter, set usage flag.
			if( arg.arg_type.components.size() == 1u && !arg.arg_type.components.front().have_template_parameters )
			{
				for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
				{
					if( template_parameter.name == arg.arg_type.components.front().name )
					{
						template_parameters_usage_flags[ &template_parameter - template_parameters.data() ]= true;
						break;
					}
				}
			}

			template_parameters.emplace_back();
			template_parameters.back().name= arg_name;
			template_parameters.back().type_name= &arg.arg_type;
			template_parameters_usage_flags.push_back(false);

			Variable variable;
			variable.type= *type;
			if( type->GetTemplateDependentType() != nullptr )
			{}
			else
			{
				variable.constexpr_value= llvm::UndefValue::get( type->GetLLVMType() );
				variable.llvm_value=
					new llvm::GlobalVariable(
						*module_,
						type->GetLLVMType(),
						true,
						llvm::GlobalValue::LinkageTypes::InternalLinkage,
						variable.constexpr_value,
						ToStdString( arg_name ) );
			}

			inserted_template_parameter=
				template_parameters_namespace->AddName( arg_name, Value( std::move(variable), type_template_declaration.file_pos_ ) /* TODO - set correct file_pos */ );
		}
		else
		{
			// If template parameter is type.

			template_parameters.emplace_back();
			template_parameters.back().name= arg_name;
			template_parameters_usage_flags.push_back(false);
			inserted_template_parameter=
				template_parameters_namespace->AddName( arg_name, Value( GetNextTemplateDependentType(), type_template_declaration.file_pos_ /* TODO - set correct file_pos */ ) );
		}

		if( inserted_template_parameter != nullptr )
			inserted_template_parameter->second.SetIsTemplateParameter(true);
	}

	U_ASSERT( template_parameters_usage_flags.size() == type_template->template_parameters.size() );

	// Check and fill signature args.
	type_template->first_optional_signature_argument= 0u;
	for( const TemplateBase::SignatureArg& signature_arg : type_template_declaration.signature_args_ )
	{
		PrepareTemplateSignatureParameter( type_template_declaration.file_pos_, signature_arg.name, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
		type_template->signature_arguments.push_back(&signature_arg.name);

		if( signature_arg.default_value != boost::none )
		{
			PrepareTemplateSignatureParameter( type_template_declaration.file_pos_, *signature_arg.default_value, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
			type_template->default_signature_arguments.push_back(signature_arg.default_value.get_ptr());
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
	U_ASSERT( type_template->signature_arguments.size() == type_template->default_signature_arguments.size() );
	U_ASSERT( type_template->first_optional_signature_argument <= type_template->default_signature_arguments.size() );

	for( size_t i= 0u; i < type_template->template_parameters.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			errors_.push_back( ReportTemplateArgumentNotUsedInSignature( type_template_declaration.file_pos_, type_template->template_parameters[i].name ) );

	// Make first check-pass for template. Resolve all names in this pass.

	ComplexName temp_class_name;
	temp_class_name.components.emplace_back();
	temp_class_name.components.back().name = "_temp"_SpC + type_template_name;
	temp_class_name.components.back().is_generated= true;

	if( const ClassTemplateDeclaration* const template_class= dynamic_cast<const ClassTemplateDeclaration*>( &type_template_declaration ) )
	{
		const ClassProxyPtr class_proxy= PrepareClass( *template_class->class_, temp_class_name, *template_parameters_namespace );

		if( class_proxy != nullptr )
		{
			ReportAboutIncompleteMembersOfTemplateClass( type_template_declaration.file_pos_, *class_proxy->class_ );
			RemoveTempClassLLVMValues( *class_proxy->class_ );
		}
	}
	else if( const TypedefTemplate* const typedef_template= dynamic_cast<const TypedefTemplate*>( &type_template_declaration ) )
	{
		PrepareType( typedef_template->typedef_->file_pos_, typedef_template->typedef_->value, *template_parameters_namespace );
	}
	else
		U_ASSERT(false);

	PopResolveHandler();
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const FilePos& file_pos,
	const ComplexName& signature_parameter,
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
	size_t skip_components= 0u;
	const NamesScope::InsertedName* const start_name=
		PreResolve( names_scope, signature_parameter.components.data(), signature_parameter.components.size(), skip_components );
	if( start_name == nullptr )
		errors_.push_back( ReportNameNotFound( file_pos, signature_parameter ) );
	for( const ComplexName::Component& component : signature_parameter.components )
	{
		for( const IExpressionComponentPtr& template_parameter : component.template_parameters )
		{
			if( const NamedOperand* const named_operand= dynamic_cast<const NamedOperand*>(template_parameter.get()))
				PrepareTemplateSignatureParameter( named_operand->file_pos_, named_operand->name_, names_scope, template_parameters, template_parameters_usage_flags );
			else
				errors_.push_back( ReportUnsupportedExpressionTypeForTemplateSignatureArgument( file_pos ) );
			// SPRACHE_TODO - allow value-expressions here
		}
	}
}

bool CodeBuilder::DuduceTemplateArguments(
	const TypeTemplatePtr& type_template_ptr,
	const TemplateParameter& template_parameter,
	const ComplexName& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	const TypeTemplate& type_template= *type_template_ptr;
	const FilePos& template_file_pos= type_template.syntax_element->file_pos_;

	// Look if signature argument refers to template argument.
	size_t dependend_arg_index= ~0u;
	if( signature_parameter.components.size() == 1u && !signature_parameter.components.front().have_template_parameters )
	{
		for( const TypeTemplate::TemplateParameter& param : type_template.template_parameters )
		{
			if( param.name == signature_parameter.components.front().name )
			{
				dependend_arg_index= &param - type_template.template_parameters.data();
				break;
			}
		}
	}

	if( const Variable* const variable= boost::get<Variable>(&template_parameter) )
	{
		// Currently, we can bind variables only to signature arguments, which refers to template arguments.
		// SPRACHE_TODO - maybe allow signatures, like </ A, B, 42 /> ?
		if( dependend_arg_index == ~0u )
			return false;

		if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
		{
			errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( signature_parameter_file_pos, variable->type.ToString() ) );
			return false;
		}

		if( variable->constexpr_value == nullptr )
		{
			errors_.push_back( ReportExpectedConstantExpression( signature_parameter_file_pos ) );
			return false;
		}

		// Check given type and type from signature, deduce also some complex names.
		if( !DuduceTemplateArguments(
				type_template_ptr,
				variable->type,
				*type_template.template_parameters[ dependend_arg_index ].type_name,
				signature_parameter_file_pos,
				deducible_template_parameters,
				names_scope ) )
			return false;

		// Allocate global variable, because we needs address.

		Variable variable_for_insertion;
		variable_for_insertion.type= variable->type;
		variable_for_insertion.location= Variable::Location::Pointer;
		variable_for_insertion.value_type= ValueType::ConstReference;
		llvm::GlobalValue* const global_value=
			new llvm::GlobalVariable(
				*module_,
				variable->type.GetLLVMType(),
				true,
				llvm::GlobalValue::LinkageTypes::InternalLinkage,
				variable->constexpr_value,
				ToStdString( type_template.template_parameters[ dependend_arg_index ].name ) );
		global_value->setUnnamedAddr(true); // We do not require unique address.
		variable_for_insertion.llvm_value= global_value;
		variable_for_insertion.constexpr_value= variable->constexpr_value;

		if( boost::get<int>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
			deducible_template_parameters[ dependend_arg_index ]= std::move( variable_for_insertion ); // Set empty arg.
		else if( boost::get<Type>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
			return false; // WTF?
		else if( const Variable* const prev_variable_value= boost::get<Variable>( &deducible_template_parameters[ dependend_arg_index ] )  )
		{
			// Variable already known, Check conflicts.
			// TODO - do real comparision
			// TODO - maybe generate error in this case?
			if( prev_variable_value->constexpr_value->getUniqueInteger() != variable_for_insertion.constexpr_value->getUniqueInteger() )
				return false;
		}

		return true;
	}

	const Type& given_type= boost::get<Type>(template_parameter);

	// Try deduce simple arg.
	if( dependend_arg_index != ~0u )
	{
		if( type_template.template_parameters[ dependend_arg_index ].type_name != nullptr )
		{
			// Expected variable, but type given.
			return false;
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
				return false;
		}
		else if( boost::get<Variable>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
		{
			// Bind type argument to variable parameter.
			return false;
		}
		return true;
	}

	// Process simple fundamental type.
	if( given_type.GetFundamentalType() != nullptr )
	{
		if( const NamesScope::InsertedName* const inserted_name= ResolveName( template_file_pos, names_scope, signature_parameter ) )
		{
			if( const Type* type= inserted_name->second.GetTypeName() )
			{
				if( *type == given_type )
					return true;
			}
		}
	}

	size_t component_number= 0u;
	const NamesScope::InsertedName* const start_name=
		PreResolve( names_scope, signature_parameter.components.data(), signature_parameter.components.size(), component_number );
	if( start_name == nullptr )
		return false;
	const NamesScope::InsertedName* current_name= start_name;
	while( component_number < signature_parameter.components.size() )
	{
		NamesScope* next_space= nullptr;
		const ComplexName::Component& component= signature_parameter.components[component_number - 1u];
		const bool is_last_component= component_number + 1u == signature_parameter.components.size();

		if( const NamesScopePtr inner_namespace= current_name->second.GetNamespace() )
			next_space= inner_namespace.get();
		else if( const Type* const type= current_name->second.GetTypeName() )
		{
			if( Class* const class_= type->GetClassType() )
			{
				if( !is_last_component && class_->is_incomplete )
				{
					errors_.push_back( ReportUsingIncompleteType( template_file_pos, type->ToString() ) );
					return false;
				}
				next_space= &class_->members;
			}
		}
		else if( const TypeTemplatePtr type_template = current_name->second.GetTypeTemplate() )
		{
			if( component.have_template_parameters && !is_last_component )
			{
				const NamesScope::InsertedName* generated_type=
					GenTemplateType(
						FilePos(),
						type_template,
						component.template_parameters,
						*type_template->parent_namespace,
						names_scope );
				if( generated_type == nullptr )
					return false;
				if( generated_type->second.GetType() == NontypeStub::TemplateDependentValue )
					return true;

				const Type* const type= generated_type->second.GetTypeName();
				U_ASSERT( type != nullptr );
				if( type->GetTemplateDependentType() != nullptr )
					return true;
				if( Class* const class_= type->GetClassType() )
					next_space= &class_->members;
			}
			else if( !is_last_component )
			{
				errors_.push_back( ReportTemplateInstantiationRequired( template_file_pos, type_template->syntax_element->name_ ) );
				return false;
			}

		}
		else
			return false;

		const ComplexName::Component& next_component= signature_parameter.components[component_number];
		if( next_space != nullptr )
			current_name= next_space->GetThisScopeName( next_component.name );
		++component_number;
	}

	if( current_name == nullptr )
		return false;

	if( const Type* const type= current_name->second.GetTypeName() )
	{
		return *type == given_type;
	}
	else if( const TypeTemplatePtr inner_type_template = current_name->second.GetTypeTemplate() )
	{
		const Class* const given_type_class= given_type.GetClassType();
		if( given_type_class == nullptr )
			return false;
		if( given_type_class->base_template == boost::none )
			return false;
		if( given_type_class->base_template->class_template != inner_type_template )
			return false;

		const ComplexName::Component& name_component= signature_parameter.components.back();
		if( !name_component.have_template_parameters )
			return false;
		if( signature_parameter.components.back().template_parameters.size() < inner_type_template->first_optional_signature_argument )
			return false;

		for( size_t i= 0u; i < name_component.template_parameters.size(); ++i)
		{
			// SPRACHE_TODO - Allow expressions as signature arguments - value-signature-arguments.
			if( const NamedOperand* const named_operand= dynamic_cast<const NamedOperand*>( name_component.template_parameters[i].get() ) )
			{
				const bool deduced= DuduceTemplateArguments(
					type_template_ptr,
					given_type_class->base_template->template_parameters[i],
					named_operand->name_,
					template_file_pos,
					deducible_template_parameters,
					names_scope );
				if( !deduced )
					return false;
			}
			else
				return false;
		}
		return true;
	}

	return false;
}

NamesScope::InsertedName* CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatePtr& type_template_ptr,
	const std::vector<IExpressionComponentPtr>& template_arguments,
	NamesScope& template_names_scope,
	NamesScope& arguments_names_scope )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	const TypeTemplate& type_template= *type_template_ptr;

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
	for( size_t i= 0u; i < type_template.signature_arguments.size(); ++i )
	{
		Value value;
		if( i < template_arguments.size() )
			value= BuildExpressionCode( *template_arguments[i], arguments_names_scope, *dummy_function_context_ );
		else
		{
			const NamesScope::InsertedName* const name=
				ResolveName( type_template_ptr->syntax_element->file_pos_, *template_parameters_namespace, *type_template.default_signature_arguments[i] );
			if( name == nullptr )
				continue;
			value= name->second;
		}
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

		const ComplexName& name= *type_template.signature_arguments[i];

		// TODO - maybe add some errors, if not deduced?
		if( const Type* const type_name= value.GetTypeName() )
		{
			if( !DuduceTemplateArguments( type_template_ptr, *type_name, name, file_pos, deduced_template_args, template_names_scope ) )
			{
				deduction_failed= true;
				continue;
			}
		}
		else if( const Variable* const variable= value.GetVariable() )
		{
			if( !DuduceTemplateArguments( type_template_ptr, *variable, name, file_pos, deduced_template_args, template_names_scope ) )
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
			// Currently, can be only integer.
			const llvm::APInt& int_value= variable->constexpr_value->getUniqueInteger();
			if( IsSignedInteger( variable->type.GetFundamentalType()->fundamental_type ) && int_value.isNegative() )
				name_encoded+= ToProgramString( std::to_string(  int64_t(int_value.getLimitedValue()) ).c_str() );
			else
				name_encoded+= ToProgramString( std::to_string( uint64_t(int_value.getLimitedValue()) ).c_str() );
		}
		else U_ASSERT(false);
	}

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

	if( const ClassTemplateDeclaration* const template_class= dynamic_cast<const ClassTemplateDeclaration*>( type_template.syntax_element ) )
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

		template_classes_cache_[class_key]= class_proxy;
		return template_parameters_namespace->GetThisScopeName( GetNameForGeneratedClass() );
	}
	else if( const TypedefTemplate* const typedef_template= dynamic_cast<const TypedefTemplate*>( type_template.syntax_element ) )
	{
		const Type type= PrepareType( typedef_template->typedef_->file_pos_, typedef_template->typedef_->value, *template_parameters_namespace );

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

bool CodeBuilder::NameShadowsTemplateArgument( const ProgramString& name, NamesScope& names_scope )
{
	ComplexName::Component component;
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
		// SPRACHE_TODO - allow non-fundamental value arguments, such enums, for example.
		if( IsInteger( fundamental->fundamental_type ) || fundamental->fundamental_type == U_FundamentalType::Bool )
			return true;
	}

	return false;
}

void CodeBuilder::RemoveTempClassLLVMValues( Class& class_ )
{
	class_.members.ForEachInThisScope(
		[this]( const NamesScope::InsertedName& name )
		{
			if( const Type* const type= name.second.GetTypeName() )
			{
				if( Class* const subclass= type->GetClassType() )
					RemoveTempClassLLVMValues( *subclass );
			}
			else if( const OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : *functions_set )
					function.llvm_function->eraseFromParent();
			}
			else if( name.second.GetClassField() != nullptr )
			{}
			else if( name.second.GetTypeTemplate() != nullptr )
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
							RemoveTempClassLLVMValues( *generated_class );
						}
					});
			}
			else if( const Variable* const variable= name.second.GetVariable() )
			{
				U_UNUSED(variable);
				// TODO - maybe we can delete global variable without breaking llvm code structure?
			}
			else
				U_ASSERT(false);
		} );
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
					if( subclass->is_incomplete )
						errors_.push_back( ReportIncompleteMemberOfClassTemplate( file_pos, name.first ) );
					else
						ReportAboutIncompleteMembersOfTemplateClass( file_pos, *subclass );
				}
			}
			else if( const OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : *functions_set )
				{
					if( !function.have_body )
						errors_.push_back( ReportIncompleteMemberOfClassTemplate( file_pos, name.first ) );
				}
			}
			else if( name.second.GetClassField() != nullptr )
			{}
			else if( name.second.GetTypeTemplate() != nullptr )
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
			else
				U_ASSERT(false);
		} );
}

} // namespace CodeBuilderPrivate

} // namespace U
