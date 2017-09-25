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

	std::vector<ClassTemplate::TemplateParameter> template_parameters;
	template_parameters.reserve( class_template_declaration.args_.size() );

	// Check and fill template parameters.
	for( const ClassTemplateDeclaration::Arg& arg : class_template_declaration.args_ )
	{
		// Check redefinition
		for( const auto& prev_arg : template_parameters )
		{
			if( prev_arg.name == arg.name )
			{
				errors_.push_back( ReportRedefinition( class_template_declaration.file_pos_, arg.name ) );
				return;
			}
		}

		template_parameters.emplace_back();
		template_parameters.back().name= arg.name;
		if( !arg.arg_type.components.empty() )
		{
			// If template parameter is value.

			// Search type in template type-arguments.
			const ClassTemplate::TemplateParameter* template_arg= nullptr;
			for( const auto& prev_arg : template_parameters )
			{
				if( prev_arg.type_name == nullptr &&
					prev_arg.name == arg.name )
				{
					template_arg= &prev_arg;
					break;
				}
			}

			if( template_arg == nullptr )
			{
				// Search for external type name.
				const NamesScope::InsertedName* const name= ResolveName( names_scope, arg.arg_type );
				if( name == nullptr )
				{
					errors_.push_back( ReportNameNotFound( class_template_declaration.file_pos_, arg.arg_type ) );
					return;
				}
				const Type* const type= name->second.GetTypeName();
				if( type == nullptr )
				{
					errors_.push_back( ReportNameIsNotTypeName( class_template_declaration.file_pos_, name->first ) );
					return;
				}
			}
			template_parameters.back().type_name= &arg.arg_type;
		}
	}

	// Check and fill signature args.
	for( const ClassTemplateDeclaration::SignatureArg& signature_arg : class_template_declaration.signature_args_ )
	{
		ClassTemplate::TemplateParameter* existent_template_param= nullptr;
		if( signature_arg.name.components.size() == 1u )
		{
			for( auto& template_param : template_parameters )
			{
				if( template_param.name == signature_arg.name.components.front().name )
				{
					existent_template_param= &template_param;
					break;
				}
			}
		}

		if( existent_template_param != nullptr )
		{
		}
		else
		{
			/* // TODO
			const NamesScope::InsertedName* const name= ResolveName( names_scope, signature_arg.name );
			if( name == nullptr )
			{
				errors_.push_back( ReportNameNotFound( class_template_declaration.file_pos_, signature_arg.name ) );
				return;
			}
			const Type* const type= name->second.GetTypeName();
			if( type == nullptr )
			{
				errors_.push_back( ReportNameIsNotTypeName( class_template_declaration.file_pos_, name->first ) );
				return;
			}
			*/
		}
		class_template->signature_arguments.push_back(&signature_arg.name);
	}

	class_template->template_parameters= std::move(template_parameters);

	// SPRACHE_TODO:
	// *) Check signature arguments for template arguments.
	// *) Convert signature and template arguments to "default form" for equality comparison.
	// *) More and more checks.
	// *) Resolve all class template names at template definition point.
	// *) Make more and more other stuff.

	// Make first check-pass for template. Resolve all names in this pass.


	NamesScope& template_arguments_space= PushTemplateArgumentsSpace();
	for( const ClassTemplate::TemplateParameter& param : class_template->template_parameters )
		template_arguments_space.AddName( param.name, TemplateDependentValue() );

	PushCacheFillResolveHandler( class_template->resolving_cache, names_scope );

	// SPRACHE_TODO - pre-resolve signature paramters here too.

	ComplexName temp_class_name;
	temp_class_name.components.emplace_back();
	temp_class_name.components.back().name = "_temp"_SpC + class_template_declaration.class_->name_.components.back().name;
	temp_class_name.components.back().is_generated= true;

	PrepareClass( *class_template->class_syntax_element, temp_class_name, names_scope );

	PopResolveHandler();

	PopTemplateArgumentsSpace();
}

bool CodeBuilder::DuduceTemplateArguments(
	const ClassTemplatePtr& class_template_ptr,
	const TemplateParameter& template_parameter,
	const ComplexName& signature_parameter,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	const FilePos file_pos= FilePos(); // TODO
	const ClassTemplate& class_template= *class_template_ptr;

	if( const Variable* const variable= boost::get<Variable>(&template_parameter) )
	{
		size_t dependend_arg_index= ~0u;
		if( signature_parameter.components.size() == 1u &&
			signature_parameter.components.front().template_parameters.empty() )
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

		const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
		if( fundamental_type == nullptr || !IsInteger( fundamental_type->fundamental_type ) )
		{
			// SPRACHE_TODO - allow non-fundamental value arguments.
			errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( file_pos, variable->type.ToString() ) );
			return false;
		}
		if( variable->constexpr_value == nullptr )
		{
			errors_.push_back( ReportExpectedConstantExpression( file_pos ) );
			return false;
		}
		if( dependend_arg_index != ~0u )
		{
			// TODO - perform type checks earlier, when template defined.
			const ComplexName* const type_complex_name= class_template.template_parameters[ dependend_arg_index ].type_name;
			if( type_complex_name == nullptr )
			{
				// Expected variable, but type given.
				return false;
			}
			const NamesScope::InsertedName* const type_name=
				ResolveName( names_scope, *type_complex_name );
			if( type_name == nullptr )
			{
				ReportNameNotFound( file_pos, *type_complex_name );
				return false;
			}
			const Type* const type= type_name->second.GetTypeName();
			if( type_name == nullptr )
			{
				errors_.push_back( ReportNameIsNotTypeName( file_pos, type_complex_name->components.back().name ) );
				return false;
			}
			if( *type != variable->type )
			{
				// Given type does not match actual type.
				return false;
			}

			// Allocate global variable, because we needs address.

			Variable variable_for_insertion;
			variable_for_insertion.type= *type;
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

			deducible_template_parameters[dependend_arg_index]= std::move( variable_for_insertion );

			return true;
		}
		else
			return false;
	}

	const Type& given_type= boost::get<Type>(template_parameter);

	// Try deduce simple arg.
	if( signature_parameter.components.size() == 1u &&
		signature_parameter.components.front().template_parameters.empty() )
	{
		size_t dependend_arg_index= ~0u;
		for( const ClassTemplate::TemplateParameter& param : class_template.template_parameters )
		{
			if( param.name == signature_parameter.components.front().name )
			{
				dependend_arg_index= &param - class_template.template_parameters.data();
				break;
			}
		}

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
	}

	// Process simple fundamental type.
	if( given_type.GetFundamentalType() != nullptr )
	{
		if( const NamesScope::InsertedName* const inserted_name= ResolveName( names_scope, signature_parameter ) )
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

	// Resolve first component without template parameters.
	const std::pair< const NamesScope::InsertedName*, NamesScope* > start_name=
		ResolveNameWithParentSpace( names_scope, signature_parameter.components.data(), 1u, true );
	if( start_name.first == nullptr )
		return false;
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
		// Now, we have two squaences - for given type and for start part of complex name in template signature parameter.
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
	for( size_t n= 0u; n < signature_parameter.components.size(); ++n)
	{
		const size_t given_type_n= n + start_name_predecessors.size(); // TODO - check this
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
					else return false;
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
					else return false; // different classes

					if( n + 1u < signature_parameter.components.size() )
						current_name= given_class_->members.GetThisScopeName( signature_parameter.components[n+1u].name );
				}
			}
			else
				return false;
		}
		else if( const ClassTemplatePtr class_template= current_name->second.GetClassTemplate() )
		{
			if( !name_component.have_template_parameters )
				return false;

			if( const ClassPtr* const given_type_class_ptr= boost::get<ClassPtr>(&given_type_component) )
			{
				const Class& class_= **given_type_class_ptr;
				if( class_.base_template == boost::none )
					return false;
				if( class_.base_template->class_template == class_template ) // Ak, same template
				{
					if( class_template->signature_arguments.size() != name_component.template_parameters.size() )
						return false;

					for( size_t i= 0u; i < name_component.template_parameters.size(); ++i)
					{
						// TODO - process signature_args as expression.
						const IExpressionComponent* const expr= name_component.template_parameters[i].get();
						if( const NamedOperand* const named_operand= dynamic_cast<const NamedOperand*>(expr) )
						{
							const bool deduced= DuduceTemplateArguments(
								class_template,
								class_.base_template->template_parameters[i],
								named_operand->name_,
								deducible_template_parameters,
								names_scope );
							if( !deduced )
								return false;
						}
						else
							return false;
					}

					if( n + 1u < signature_parameter.components.size() )
						current_name= class_.members.GetThisScopeName( signature_parameter.components[n+1u].name );
				}
				else
					return false;
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
	const ClassTemplatePtr& class_template_ptr,
	const std::vector<IExpressionComponentPtr>& template_arguments,
	NamesScope& template_names_scope,
	NamesScope& arguments_names_scope )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	const ClassTemplate& class_template= *class_template_ptr;

	const FilePos file_pos= FilePos(); // TODO - set file_pos

	if( class_template.signature_arguments.size() != template_arguments.size() )
	{
		return nullptr;
	}

	DeducibleTemplateParameters deduced_template_args( class_template.template_parameters.size() );

	for( size_t i= 0u; i < class_template.signature_arguments.size(); ++i )
	{
		Value value;
		try
		{
			value= BuildExpressionCode( *template_arguments[i], arguments_names_scope, *dummy_function_context_ );
		}
		catch( const ProgramError& )
		{
			return nullptr;
		}

		// Each template with template-dependent signature arguments is template-dependent values.
		if( value.GetType() == NontypeStub::TemplateDependentValue )
			return &template_names_scope.GetTemplateDependentValue();

		const ComplexName& name= *class_template.signature_arguments[i];

		if( const Type* const type_name= value.GetTypeName() )
		{
			if( !DuduceTemplateArguments( class_template_ptr, *type_name, name, deduced_template_args, template_names_scope ) )
				return nullptr;
		}
		else if( const Variable* const variable= value.GetVariable() )
		{
			if( !DuduceTemplateArguments( class_template_ptr, *variable, name, deduced_template_args, template_names_scope ) )
				return nullptr;
		}
		else
		{
			errors_.push_back( ReportInvalidValueAsTemplateArgument( file_pos, value.GetType().ToString() ) );
			return nullptr;
		}
	} // for arguments

	NamesScope& template_arguments_space= PushTemplateArgumentsSpace();

	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];

		if( boost::get<int>( &arg ) != nullptr )
		{
			errors_.push_back( ReportTemplateParametersDeductionFailed( file_pos ) );
			PopTemplateArgumentsSpace();
			return nullptr;
		}

		const ProgramString& name= class_template.template_parameters[i].name;
		if( const Type* const type= boost::get<Type>( &arg ) )
			template_arguments_space.AddName( name, Value(*type) );
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			template_arguments_space.AddName( name, Value(*variable) );
		else U_ASSERT(false);
	}

	// Encode name.
	ProgramString name_encoded= "_template"_SpC + class_template.class_syntax_element->name_.components.back().name;
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
		PopTemplateArgumentsSpace();
		return inserted_name;
	}

	PushCacheGetResolveHandelr( class_template.resolving_cache );

	ComplexName generated_class_complex_name;
	generated_class_complex_name.components.emplace_back();
	generated_class_complex_name.components.back().name= name_encoded;
	generated_class_complex_name.components.back().is_generated= true;

	ClassPtr the_class= PrepareClass( *class_template.class_syntax_element, generated_class_complex_name, template_names_scope );

	PopResolveHandler();
	PopTemplateArgumentsSpace();

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

	return template_names_scope.GetThisScopeName( name_encoded );
}

std::pair<const NamesScope::InsertedName*, NamesScope*> CodeBuilder::ResolveTemplateArgument(
	const ComplexName::Component* components,
	const size_t component_count )
{
	// TODO - also, check shadowing of template parameters.

	U_ASSERT( component_count >= 1u );

	if( component_count != 1u || components[0].name.empty() || components[0].have_template_parameters )
		return std::make_pair( nullptr, nullptr );

	for( auto it= template_arguments_stack_.rbegin(); it != template_arguments_stack_.rend(); ++it )
	{
		NamesScope& names_scope= **it;
		if( const NamesScope::InsertedName* const name= names_scope.GetThisScopeName( components[0].name ) )
			return std::make_pair( name, &names_scope );
	}

	return std::make_pair( nullptr, nullptr );
}

NamesScope& CodeBuilder::PushTemplateArgumentsSpace()
{
	template_arguments_stack_.emplace_back( new NamesScope( ProgramString(), nullptr ) );
	return *template_arguments_stack_.back();
}

void CodeBuilder::PopTemplateArgumentsSpace()
{
	template_arguments_stack_.pop_back();
}

} // namespace CodeBuilderPrivate

} // namespace U
