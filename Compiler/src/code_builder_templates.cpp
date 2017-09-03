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
}

NamesScope::InsertedName* CodeBuilder::GenTemplateClass(
	const ClassTemplatePtr& class_template_ptr,
	const std::vector<IExpressionComponentPtr>& template_arguments,
	NamesScope& names_scope )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	const ClassTemplate& class_template= *class_template_ptr;

	const FilePos file_pos= FilePos(); // TODO - set file_pos

	if( class_template.signature_arguments.size() != template_arguments.size() )
	{
		return nullptr;
	}

	typedef boost::variant< int, Type, Variable > TemplateArg;
	std::vector<TemplateArg> deduced_template_args( class_template.template_parameters.size() );

	for( size_t i= 0u; i < class_template.signature_arguments.size(); ++i )
	{
		size_t dependend_arg_index= ~0u;
		const ComplexName& name= *class_template.signature_arguments[i];
		if( name.components.size() == 1u && name.components.front().template_parameters.empty() )
		{
			for( const ClassTemplate::TemplateParameter& param : class_template.template_parameters )
			{
				if( param.name == name.components.front().name )
				{
					dependend_arg_index= &param - class_template.template_parameters.data();
					break;
				}
			}
		}

		try
		{
			Value value= BuildExpressionCode( *template_arguments[i], names_scope, *dummy_function_context_ ); // TODO - use here correct names_scope
			if( const Type* const type_name= value.GetTypeName() )
			{
				if( dependend_arg_index != ~0u )
				{
					if( class_template.template_parameters[ dependend_arg_index ].type_name != nullptr )
					{
						// Expected variable, but type given.
						return nullptr;
					}
					else if( boost::get<int>( &deduced_template_args[ dependend_arg_index ] ) != nullptr )
					{
						// Set empty arg.
						deduced_template_args[ dependend_arg_index ]= *type_name;
					}
					else if( const Type* const prev_type= boost::get<Type>( &deduced_template_args[ dependend_arg_index ] ) )
					{
						// Type already known. Check conflicts.
						if( *prev_type != *type_name )
							return nullptr;
					}
					else if( boost::get<Variable>( &deduced_template_args[ dependend_arg_index ] ) != nullptr )
					{
						// Bind type argument to variable parameter.
						return nullptr;
					}
				}
			}
			else if( const Variable* const variable= value.GetVariable() )
			{
				const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
				if( fundamental_type == nullptr || !IsInteger( fundamental_type->fundamental_type ) )
				{
					// SPRACHE_TODO - allow non-fundamental value arguments.
					errors_.push_back( ReportInvalidTypeOfTemplateVariableArgument( file_pos, variable->type.ToString() ) );
					return nullptr;
				}
				if( variable->constexpr_value == nullptr )
				{
					errors_.push_back( ReportExpectedConstantExpression( file_pos ) );
					return nullptr;
				}
				if( dependend_arg_index != ~0u )
				{
					// TODO - perform type checks earlier, when template defined.
					const ComplexName* const type_complex_name= class_template.template_parameters[ dependend_arg_index ].type_name;
					if( type_complex_name == nullptr )
					{
						// Expected variable, but type given.
						return nullptr;
					}
					const NamesScope::InsertedName* const type_name=
						ResolveName( names_scope, *type_complex_name );
					if( type_name == nullptr )
					{
						ReportNameNotFound( file_pos, *type_complex_name );
						return nullptr;
					}
					const Type* const type= type_name->second.GetTypeName();
					if( type_name == nullptr )
					{
						errors_.push_back( ReportNameIsNotTypeName( file_pos, type_complex_name->components.back().name ) );
						return nullptr;
					}
					if( *type != variable->type )
					{
						errors_.push_back( ReportTypesMismatch( file_pos, type->ToString(), variable->type.ToString() ) );
						return nullptr;
					}

					// Allocate global variable, because we needs address.

					Variable variable_for_insertion;
					variable_for_insertion.type= *type;
					variable_for_insertion.location= Variable::Location::Pointer;
					variable_for_insertion.value_type= ValueType::ConstReference;
					variable_for_insertion.llvm_value=
						new llvm::GlobalVariable(
							*module_,
							variable->type.GetLLVMType(),
							true,
							llvm::GlobalValue::LinkageTypes::InternalLinkage,
							variable->constexpr_value,
							ToStdString( class_template.template_parameters[ dependend_arg_index ].name ) );
					variable_for_insertion.constexpr_value= variable->constexpr_value;

					deduced_template_args[dependend_arg_index]= std::move( variable_for_insertion );
				}
			}
			else
			{
				errors_.push_back( ReportInvalidValueAsTemplateArgument( file_pos, value.GetType().ToString() ) );
				return nullptr;
			}
		}
		catch( const ProgramError& )
		{
			return nullptr;
		}
	}

	NamesScope template_parameters_names_scope( ""_SpC, &names_scope );

	//for( const auto& arg : deduced_template_args )
	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];

		if( boost::get<int>( &arg ) != nullptr )
		{
			errors_.push_back( ReportTemplateParametersDeductionFailed( file_pos ) );
			return nullptr;
		}

		const ProgramString& name= class_template.template_parameters[i].name;
		if( const Type* const type= boost::get<Type>( &arg ) )
			names_scope.AddName( name, Value(*type) );
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			names_scope.AddName( name, Value(*variable) );
		else U_ASSERT(false);
	}

	// Encode name.
	ProgramString name_encoded= class_template.class_syntax_element->name_.components.back().name;
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
	if( NamesScope::InsertedName* const inserted_name= names_scope.GetThisScopeName( name_encoded ) )
	{
		// Already generated.
		return inserted_name;
	}

	ClassPtr the_class= PrepareClass( *class_template.class_syntax_element, template_parameters_names_scope );
	if( the_class == nullptr )
		return nullptr;

	// TODO - generate correct mangled name for template.
	the_class->llvm_type->setName( MangleClass( names_scope, name_encoded ) );

	// Set correct scope, not fake temporary names scope for template parameters.
	the_class->members.SetParent( &names_scope );

	// Save in class info about it.
	the_class->base_template.emplace();
	the_class->base_template->class_template= class_template_ptr;
	for( const TemplateArg& arg : deduced_template_args )
	{
		if( const Type* const type= boost::get<Type>( &arg ) )
			the_class->base_template->template_parameters.push_back( *type );
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
			the_class->base_template->template_parameters.push_back( *variable );
		else U_ASSERT(false);
	}

	// TODO - check here class members.

	return names_scope.AddName( name_encoded, Value( the_class ) );
}

} // namespace CodeBuilderPrivate

} // namespace U
