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

void CodeBuilder::PrepareClassTemplate(
	const ClassTemplateDeclaration& class_template_declaration,
	NamesScope& names_scope )
{
	const ClassTemplatePtr class_template( new ClassTemplate );
	const ProgramString& class_template_name= class_template_declaration.class_->name_.components.back().name;

	// SPRACHE_TODO - add class templates overloading.
	if( names_scope.AddName( class_template_name, Value(class_template) ) == nullptr )
	{
		errors_.push_back( ReportRedefinition( class_template_declaration.file_pos_, class_template_name ) );
		return;
	}

	class_template->class_syntax_element= class_template_declaration.class_.get();

	std::vector<ClassTemplate::TemplateParameter>& template_parameters= class_template->template_parameters;
	template_parameters.reserve( class_template_declaration.args_.size() );
	std::vector<bool> template_parameters_usage_flags;

	PushCacheFillResolveHandler( class_template->resolving_cache, names_scope );
	const NamesScopePtr template_parameters_namespace = std::make_shared<NamesScope>( g_template_parameters_namespace_prefix, &names_scope );

	// Check and fill template parameters.
	for( const ClassTemplateDeclaration::Arg& arg : class_template_declaration.args_ )
	{
		// Check redefinition
		for( const auto& prev_arg : template_parameters )
		{
			if( prev_arg.name == arg.name )
			{
				errors_.push_back( ReportRedefinition( class_template_declaration.file_pos_, arg.name ) );
				continue;
			}
		}
		if( NameShadowsTemplateArgument( arg.name, names_scope ) )
			errors_.push_back( ReportDeclarationShadowsTemplateArgument( class_template_declaration.file_pos_, arg.name ) );

		if( !arg.arg_type.components.empty() )
		{
			// If template parameter is value.

			// Resolve from outer space or from this template parameters.
			const NamesScope::InsertedName* const type_name= ResolveName( class_template_declaration.file_pos_, *template_parameters_namespace, arg.arg_type );
			if( type_name == nullptr )
			{
				errors_.push_back( ReportNameNotFound( class_template_declaration.file_pos_, arg.arg_type ) );
				continue;
			}
			const Type* const type= type_name->second.GetTypeName();
			if( type == nullptr )
			{
				errors_.push_back( ReportNameIsNotTypeName( class_template_declaration.file_pos_, type_name->first ) );
				continue;
			}

			if( type->GetTemplateDependentType() == nullptr &&
				!TypeIsValidForTemplateVariableArgument( *type ) )
			{
				errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( class_template_declaration.file_pos_, type->ToString() ) );
				continue;
			}

			// If type is template parameter, set usage flag.
			if( arg.arg_type.components.size() == 1u && !arg.arg_type.components.front().have_template_parameters )
			{
				for( const ClassTemplate::TemplateParameter& template_parameter : template_parameters )
				{
					if( template_parameter.name == arg.arg_type.components.front().name )
					{
						template_parameters_usage_flags[ &template_parameter - template_parameters.data() ]= true;
						break;
					}
				}
			}

			template_parameters.emplace_back();
			template_parameters.back().name= arg.name;
			template_parameters.back().type_name= &arg.arg_type;
			template_parameters_usage_flags.push_back(false);

			// SPRACHE_TODO - support template-dependent template arguments, such template</ type T, Foo<T>::some_constant />

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
						ToStdString( arg.name ) );
			}

			template_parameters_namespace->AddName( arg.name, std::move(variable) );
		}
		else
		{
			// If template parameter is variable.

			template_parameters.emplace_back();
			template_parameters.back().name= arg.name;
			template_parameters_namespace->AddName( arg.name, Type( GetNextTemplateDependentType() ) );
			template_parameters_usage_flags.push_back(false);
		}
	}

	U_ASSERT( template_parameters_usage_flags.size() == class_template->template_parameters.size() );

	// Check and fill signature args.
	class_template->first_optional_signature_argument= 0u;
	for( const ClassTemplateDeclaration::SignatureArg& signature_arg : class_template_declaration.signature_args_ )
	{
		PrepareTemplateSignatureParameter( class_template_declaration.file_pos_, signature_arg.name, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
		class_template->signature_arguments.push_back(&signature_arg.name);

		if( signature_arg.default_value != boost::none )
		{
			PrepareTemplateSignatureParameter( class_template_declaration.file_pos_, *signature_arg.default_value, *template_parameters_namespace, template_parameters, template_parameters_usage_flags );
			class_template->default_signature_arguments.push_back(signature_arg.default_value.get_ptr());
		}
		else
		{
			const size_t index= class_template->signature_arguments.size() - 1u;
			if (index > class_template->first_optional_signature_argument )
				errors_.push_back( ReportMandatoryTemplateSignatureArgumentAfterOptionalArgument( class_template_declaration.file_pos_ ) );

			class_template->default_signature_arguments.push_back(nullptr);
			++class_template->first_optional_signature_argument;
		}
	}
	U_ASSERT( class_template->signature_arguments.size() == class_template->default_signature_arguments.size() );
	U_ASSERT( class_template->first_optional_signature_argument <= class_template->default_signature_arguments.size() );

	for( size_t i= 0u; i < class_template->template_parameters.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			errors_.push_back( ReportTemplateArgumentNotUsedInSignature( class_template_declaration.file_pos_, class_template->template_parameters[i].name ) );

	// SPRACHE_TODO:
	// *) Convert signature and template arguments to "default form" for equality comparison.
	// *) More and more checks.
	// *) Make more and more other stuff.

	// Make first check-pass for template. Resolve all names in this pass.

	ComplexName temp_class_name;
	temp_class_name.components.emplace_back();
	temp_class_name.components.back().name = "_temp"_SpC + class_template_declaration.class_->name_.components.back().name;
	temp_class_name.components.back().is_generated= true;

	const ClassPtr the_class= PrepareClass( *class_template->class_syntax_element, temp_class_name, *template_parameters_namespace );

	PopResolveHandler();

	if( the_class != nullptr )
		RemoveTempClassLLVMValues( *the_class );
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const FilePos& file_pos,
	const ComplexName& signature_parameter,
	NamesScope& names_scope,
	const std::vector<ClassTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	// If signature parameter is template parameter, set usage flag.
	if( signature_parameter.components.size() == 1u && !signature_parameter.components.front().have_template_parameters )
	{
		for( const ClassTemplate::TemplateParameter& template_parameter : template_parameters )
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
	const std::pair< const NamesScope::InsertedName*, NamesScope* > start_name=
		PreResolve( names_scope, signature_parameter.components.data(), signature_parameter.components.size(), skip_components );
	if( start_name.first == nullptr )
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
	const ClassTemplatePtr& class_template_ptr,
	const TemplateParameter& template_parameter,
	const ComplexName& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	const ClassTemplate& class_template= *class_template_ptr;
	const FilePos& template_file_pos= class_template.class_syntax_element->file_pos_;

	// Look if signature argument refers to template argument.
	size_t dependend_arg_index= ~0u;
	if( signature_parameter.components.size() == 1u && !signature_parameter.components.front().have_template_parameters )
	{
		for( const ClassTemplate::TemplateParameter& param : class_template.template_parameters )
		{
			if( param.name == signature_parameter.components.front().name )
			{
				dependend_arg_index= &param - class_template.template_parameters.data();
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
				class_template_ptr,
				variable->type,
				*class_template.template_parameters[ dependend_arg_index ].type_name,
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
				ToStdString( class_template.template_parameters[ dependend_arg_index ].name ) );
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
			if( prev_variable_value->constexpr_value->getUniqueInteger() != variable_for_insertion.constexpr_value->getUniqueInteger() )
				return false;
		}

		return true;
	}

	const Type& given_type= boost::get<Type>(template_parameter);

	// Try deduce simple arg.
	if( dependend_arg_index != ~0u )
	{
		if( class_template.template_parameters[ dependend_arg_index ].type_name != nullptr )
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

	// Try deduce other type kind.
	// Curently, can work only with class types.
	const ClassPtr class_type= given_type.GetClassType();
	if( class_type == nullptr )
		return false;

	typedef boost::variant<NamesScopePtr, ClassPtr> TypePathComponent;
	// Sequence of namespaces/classes, where given type placed.
	// TODO - add root namespace.
	std::vector<TypePathComponent> given_type_predecessors;
	{
		const NamesScope* n= &class_type->members;
		while( n->GetParent() != nullptr )
		{
			const NamesScope* const parent= n->GetParent();
			const NamesScope::InsertedName* const name= parent->GetThisScopeName( n->GetThisNamespaceName() );
			U_ASSERT( name != nullptr );
			if( const NamesScopePtr names_scope= name->second.GetNamespace() )
				given_type_predecessors.insert( given_type_predecessors.begin(), 1u, names_scope );
			else if( const Type* const type= name->second.GetTypeName() )
			{
				if( const ClassPtr class_= type->GetClassType() )
					given_type_predecessors.insert( given_type_predecessors.begin(), 1u,class_ );
				else U_ASSERT(false);

			}
			else U_ASSERT(false);

			n= parent;
		}
	}

	size_t signature_name_components_skip= 0u;
	// TODO - try get template parameters from outer templates here
	const std::pair< const NamesScope::InsertedName*, NamesScope* > start_name=
		PreResolve( names_scope, signature_parameter.components.data(), signature_parameter.components.size(), signature_name_components_skip );
	if( start_name.first == nullptr )
		return false;
	U_ASSERT( signature_name_components_skip > 0u && signature_name_components_skip <= signature_parameter.components.size() );
	std::vector<TypePathComponent> start_name_predecessors;
	{
		const NamesScope* n= start_name.second;
		while( n->GetParent() != nullptr )
		{
			const NamesScope* const parent= n->GetParent();
			const NamesScope::InsertedName* const name= parent->GetThisScopeName( n->GetThisNamespaceName() );
			U_ASSERT( name != nullptr );
			if( const NamesScopePtr names_scope= name->second.GetNamespace() )
				start_name_predecessors.insert( start_name_predecessors.begin(), 1u, names_scope );
			else if( const Type* const type= name->second.GetTypeName() )
			{
				if( const ClassPtr class_= type->GetClassType() )
					start_name_predecessors.insert( start_name_predecessors.begin(), 1u,class_ );
				else U_ASSERT(false);

			}
			else U_ASSERT(false);

			n= parent;
		}
	}

	{
		// Now, we have two sequences - for given type and for start part of complex name in template signature parameter.
		// Check, if template signature parameter predecessors is a subsequence of actual type predecessors.
		if( start_name_predecessors.size() > given_type_predecessors.size() )
			return false;

		const auto it_pair= std::mismatch(
			start_name_predecessors.begin(), start_name_predecessors.end(),
			given_type_predecessors.begin());

		if( it_pair.first != start_name_predecessors.end() ) // Is not subsequence.
			return false;
	}

	const NamesScope::InsertedName* current_name= start_name.first;
	for( size_t n= signature_name_components_skip - 1u, given_type_n_skip= 0u; n < signature_parameter.components.size(); ++n)
	{
		const size_t given_type_n= given_type_n_skip + n + start_name_predecessors.size() - (signature_name_components_skip - 1u); // TODO - check this
		const TypePathComponent& given_type_component= given_type_predecessors[given_type_n];
		const ComplexName::Component& name_component= signature_parameter.components[n];

		if(const NamesScopePtr component_names_scope= current_name->second.GetNamespace() )
		{
			if( name_component.have_template_parameters )
				return false;
			else
			{
				if( const NamesScopePtr* const namespace_ptr= boost::get<NamesScopePtr>(&given_type_component) )
				{
					if( *namespace_ptr == component_names_scope )
					{} // All ok
					else
						return false;
				}
				else
					return false;
			}

			if( n + 1u < signature_parameter.components.size() )
				current_name= component_names_scope->GetThisScopeName(signature_parameter.components[n+1u].name);
		}
		else if( const Type* const type= current_name->second.GetTypeName() )
		{
			if( name_component.have_template_parameters )
				return false;
			if( const ClassPtr given_class_= type->GetClassType() )
			{
				if( const ClassPtr* const class_ptr= boost::get<ClassPtr>(&given_type_component) )
				{
					if( given_class_ == *class_ptr ){} // All ok
					else
						return false; // different classes

					if( n + 1u < signature_parameter.components.size() )
						current_name= given_class_->members.GetThisScopeName( signature_parameter.components[n+1u].name );
				}
			}
			else
				return false;
		}
		else if( const ClassTemplatePtr inner_class_template= current_name->second.GetClassTemplate() )
		{
			if( !name_component.have_template_parameters )
				return false;

			if( const NamesScopePtr* const namespace_= boost::get<NamesScopePtr>(&given_type_component))
			{
				// TODO - know, whics checks we can replace by asserts.
				const NamesScope::InsertedName* const class_name= (*namespace_)->GetThisScopeName( GetNameForGeneratedClass() );
				if( class_name == nullptr )
					return false;
				const Type* const class_type= class_name->second.GetTypeName();
				if( class_type == nullptr )
					return false;
				const ClassPtr given_type_class= class_type->GetClassType();
				if( given_type_class == nullptr )
					return false;

				if( given_type_class->base_template == boost::none || given_type_class->base_template->class_template != inner_class_template )
					return false;

				if( inner_class_template->signature_arguments.size() != name_component.template_parameters.size() )
					return false;

				for( size_t i= 0u; i < name_component.template_parameters.size(); ++i)
				{
					// TODO - Allow expressions as signature arguments - value-signature-arguments.
					if( const NamedOperand* const named_operand= dynamic_cast<const NamedOperand*>( name_component.template_parameters[i].get() ) )
					{
						const bool deduced= DuduceTemplateArguments(
							class_template_ptr,
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

				if( n + 1u < signature_parameter.components.size() )
					current_name= given_type_class->members.GetThisScopeName( signature_parameter.components[n+1u].name );
				++given_type_n_skip;
			}
			else
				return false;
		}
		else
		{
			if( name_component.have_template_parameters )
				return false;

			current_name= nullptr;
		}


		if( n + 1u < signature_parameter.components.size() && current_name == nullptr )
			return false;
	}

	return true;
}

NamesScope::InsertedName* CodeBuilder::GenTemplateClass(
	const FilePos& file_pos,
	const ClassTemplatePtr& class_template_ptr,
	const std::vector<IExpressionComponentPtr>& template_arguments,
	NamesScope& template_names_scope,
	NamesScope& arguments_names_scope )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	const ClassTemplate& class_template= *class_template_ptr;

	if( template_arguments.size() < class_template.first_optional_signature_argument )
	{
		return nullptr;
	}

	DeducibleTemplateParameters deduced_template_args( class_template.template_parameters.size() );

	PushCacheGetResolveHandelr( class_template.resolving_cache );

	const NamesScopePtr template_parameters_namespace = std::make_shared<NamesScope>( ""_SpC, &template_names_scope );
	for( const ClassTemplate::TemplateParameter& param : class_template.template_parameters )
		template_parameters_namespace->AddName( param.name, YetNotDeducedTemplateArg() );

	bool is_template_dependent= false;
	for( size_t i= 0u; i < class_template.signature_arguments.size(); ++i )
	{
		Value value;
		try
		{
			if( i < template_arguments.size() )
				value= BuildExpressionCode( *template_arguments[i], arguments_names_scope, *dummy_function_context_ );
			else
			{
				const NamesScope::InsertedName* const name=
					ResolveName( class_template_ptr->class_syntax_element->file_pos_, *template_parameters_namespace, *class_template.default_signature_arguments[i] );
				if( name == nullptr )
					throw ProgramError();
				value= name->second;
			}
		}
		catch( const ProgramError& )
		{
			continue;
		}

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

		const ComplexName& name= *class_template.signature_arguments[i];

		// TODO - maybe add some errors, if not deduced?
		if( const Type* const type_name= value.GetTypeName() )
		{
			if( !DuduceTemplateArguments( class_template_ptr, *type_name, name, file_pos, deduced_template_args, template_names_scope ) )
				continue;
		}
		else if( const Variable* const variable= value.GetVariable() )
		{
			if( !DuduceTemplateArguments( class_template_ptr, *variable, name, file_pos, deduced_template_args, template_names_scope ) )
				continue;
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
			NamesScope::InsertedName* const name= template_parameters_namespace->GetThisScopeName( class_template.template_parameters[j].name );
			U_ASSERT( name != nullptr );

			if( boost::get<int>( &arg ) != nullptr )
			{} // Not deduced yet.
			else if( const Type* const type= boost::get<Type>( &arg ) )
			{
				if( name->second.GetYetNotDeducedTemplateArg() != nullptr )
					name->second= *type;
			}
			else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			{
				if( name->second.GetYetNotDeducedTemplateArg() != nullptr )
					name->second= *variable;
			}
			else U_ASSERT( false );
		}

	} // for signature arguments

	if( is_template_dependent )
	{
		PopResolveHandler();
		return
			template_names_scope.AddName(
				"_tdv"_SpC + ToProgramString( std::to_string(next_template_dependent_type_index_).c_str() ),
				Type( GetNextTemplateDependentType() ) );
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
	ProgramString name_encoded= g_template_parameters_namespace_prefix + class_template.class_syntax_element->name_.components.back().name;
	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];
		if( const Type* const  type= boost::get<Type>( &arg ) )
		{
			name_encoded+= type->ToString();
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

	// Check, if already class generated.
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
	template_names_scope.AddName( name_encoded, template_parameters_namespace );

	ClassPtr the_class= PrepareClass( *class_template.class_syntax_element, GetComplexNameForGeneratedClass(), *template_parameters_namespace );

	PopResolveHandler();

	if( the_class == nullptr )
		return nullptr;

	// TODO - generate correct mangled name for template.

	// Save in class info about it.
	the_class->base_template.emplace();
	the_class->base_template->class_template= class_template_ptr;
	for( const DeducibleTemplateParameter& arg : deduced_template_args )
	{
		if( const Type* const type= boost::get<Type>( &arg ) )
			the_class->base_template->template_parameters.push_back( *type );
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			the_class->base_template->template_parameters.push_back( *variable );
		else U_ASSERT(false);
	}

	// TODO - check here class members.

	return template_parameters_namespace->GetThisScopeName( GetNameForGeneratedClass() );
}

bool CodeBuilder::NameShadowsTemplateArgument( const ProgramString& name, NamesScope& names_scope )
{
	ComplexName::Component component;
	component.name= name;
	component.is_generated= true;
	const std::pair<const NamesScope::InsertedName*, NamesScope*> name_with_parent_space=
		ResolveNameWithParentSpace( FilePos(), names_scope, &component, 1u );
	if( name_with_parent_space.second == nullptr )
		return false;

	const ProgramString& name_of_namespace= name_with_parent_space.second->GetThisNamespaceName();

	if( name_of_namespace.size() < g_template_parameters_namespace_prefix.size() )
		return false;

	const auto it_pair= std::mismatch(
		g_template_parameters_namespace_prefix.begin(), g_template_parameters_namespace_prefix.end(),
		name_of_namespace.begin());

	if( it_pair.first != g_template_parameters_namespace_prefix.end() ) // Is not subsequence.
		return false;

	if( name == GetNameForGeneratedClass() ) // Name is in template parameters namespace, but it is class template instantiation.
		return false;

	return true;
}

TemplateDependentType CodeBuilder::GetNextTemplateDependentType()
{
	return TemplateDependentType( next_template_dependent_type_index_++, fundamental_llvm_types_.invalid_type_ );
}

bool CodeBuilder::TypeIsValidForTemplateVariableArgument( const Type& type )
{
	if( const FundamentalType* const fundamental= type.GetFundamentalType() )
	{
		// SPRACHE_TODO - allow non-fundamental value arguments.
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
				if( const ClassPtr subclass= type->GetClassType() )
					RemoveTempClassLLVMValues( *subclass );
			}
			else if( const OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : *functions_set )
					function.llvm_function->eraseFromParent();
			}
			else if( name.second.GetClassField() != nullptr )
			{}
			else if( name.second.GetClassTemplate() != nullptr )
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
							const ClassPtr generated_class= generated_class_type->GetClassType();
							U_ASSERT( generated_class != nullptr );
							U_ASSERT( generated_class->base_template != boost::none );
							RemoveTempClassLLVMValues( *generated_class );
						}
					});
			}
			else
			{
				U_ASSERT(false);
			}
		} );
}

} // namespace CodeBuilderPrivate

} // namespace U
