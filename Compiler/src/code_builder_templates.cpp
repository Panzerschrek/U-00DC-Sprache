#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
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
	const ClassTemplate& class_template,
	const std::vector<IExpressionComponentPtr>& template_arguments,
	NamesScope& names_scope )
{
	const FilePos file_pos= FilePos();

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
			Value value= BuildExpressionCode( *template_arguments[i], names_scope, *dummy_function_context_ );
			if( const Type* const type_name= value.GetTypeName() )
			{
				if( dependend_arg_index != ~0u )
				{
					if( class_template.template_parameters[ dependend_arg_index ].type_name != nullptr )
					{
						// Expected variable, but type given.
						error_count_++;
					}
					else if( boost::get<int>( &deduced_template_args[ dependend_arg_index ] ) != nullptr )
					{
						// Set empty arg
						deduced_template_args[ dependend_arg_index ]= *type_name;
					}
					else if( boost::get<Type>( &deduced_template_args[ dependend_arg_index ] ) != nullptr )
					{
						// Conflict.
						error_count_++;
					}
					else if( boost::get<Variable>( &deduced_template_args[ dependend_arg_index ] ) != nullptr )
					{
						// WTF?
						U_ASSERT( false );
					}

				}
			}
			else if( const Variable* const variable= value.GetVariable() )
			{
				const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
				if( fundamental_type == nullptr )
				{
					// SPRACHE_TODO - allow non-fundamental value arguments.
					//TODO - error
					error_count_++;
					return nullptr;
				}
				if( variable->constexpr_value == nullptr )
				{
					errors_.push_back( ReportExpectedConstantExpression( file_pos ) );
					return nullptr;
				}
			}
			else
			{
				// error
				return nullptr;
			}
			// TODO

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
			errors_.push_back( ReportNotImplemented( file_pos, "argumnet not deduced!" ) );
			return nullptr;
		}

		const ProgramString& name= class_template.template_parameters[i].name;
		if( const Type* const  type= boost::get<Type>( &arg ) )
		{
			names_scope.AddName( name, Value(*type) );
		}
		else if( const Variable* const variable= boost::get<Variable>( &arg ) )
		{
			// TODO - set global address for variable.
			// NOW this not works.
			names_scope.AddName( name, Value(*variable) );
		}
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
			// TODO
			U_ASSERT(false);
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

	// TODO - check here class members.

	return names_scope.AddName( name_encoded, Value( the_class ) );
}

} // namespace CodeBuilderPrivate

} // namespace U
