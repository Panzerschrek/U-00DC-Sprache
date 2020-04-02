#include <algorithm>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/GlobalVariable.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "error_reporting.hpp"
#include "mangling.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

namespace
{

const std::string g_template_parameters_namespace_prefix= "_tp_ns-";

template< class TemplateParam >
std::string EncodeTemplateParameters( std::vector<TemplateParam>& deduced_template_args )
{
	std::string r;
	for(const auto& arg : deduced_template_args )
	{
		if( const Type* const type= std::get_if<Type>( &arg ) )
		{
			// We needs full mangled name of template parameter here, because short type names from different spaces may coincide.
			r+= MangleType( *type );
		}
		else if( const Variable* const variable= std::get_if<Variable>( &arg ) )
		{
			// Currently, can be only integer, char or enum type.
			FundamentalType raw_type;
			if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType () )
				raw_type= *fundamental_type;
			else if( const Enum* const enum_type= variable->type.GetEnumType () )
				raw_type= enum_type->underlaying_type;
			else U_ASSERT( false );

			r+= "_val_of_t_" + GetFundamentalTypeName( raw_type.fundamental_type ) + "_";

			const llvm::APInt& int_value= variable->constexpr_value->getUniqueInteger();
			if( IsSignedInteger( raw_type.fundamental_type ) && int_value.isNegative() )
				r+= std::to_string(  int64_t(int_value.getLimitedValue()) );
			else
				r+= std::to_string( uint64_t(int_value.getLimitedValue()) );
		}
		else U_ASSERT(false);
		r += "_";
	}
	return r;
}

void CreateTemplateErrorsContext(
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos,
	const NamesScopePtr& template_parameters_namespace,
	const TemplateBase& template_,
	const std::string& template_name,
	const DeducibleTemplateParameters& template_args,
	const std::vector< std::pair< std::string, Value > >& known_template_args= {} )
{
	REPORT_ERROR( TemplateContext, errors_container, file_pos );
	const auto template_error_context= std::make_shared<TemplateErrorsContext>();
	template_error_context->template_declaration_file_pos= template_.file_pos;
	errors_container.back().template_context= template_error_context;
	template_parameters_namespace->SetErrors( template_error_context->errors );

	{
		std::string args_description;
		args_description+= "[ with ";

		size_t total_args= known_template_args.size() + template_args.size();
		size_t args_processed= 0u;
		for( const auto& known_arg : known_template_args )
		{
			args_description+= known_arg.first + " = ";
			if( const Type* const type= known_arg.second.GetTypeName() )
				args_description+= type->ToString();
			else if( const Variable* const variable= known_arg.second.GetVariable() )
				args_description+= std::to_string( int64_t(variable->constexpr_value->getUniqueInteger().getLimitedValue()) );
			else U_ASSERT(false);

			++args_processed;
			if( args_processed < total_args )
				args_description+= ", ";
		}

		U_ASSERT( template_.template_parameters.size() == template_args.size() );
		for( size_t i= 0u; i < template_args.size() ; ++i )
		{
			const DeducibleTemplateParameter& arg= template_args[i];

			args_description+= template_.template_parameters[i].name + " = ";
			if( const Type* const type= std::get_if<Type>( &arg ) )
				args_description+= type->ToString();
			else if( const Variable* const variable= std::get_if<Variable>( &arg ) )
				args_description+= std::to_string( int64_t(variable->constexpr_value->getUniqueInteger().getLimitedValue()) );
			else U_ASSERT(false);

			++args_processed;
			if( args_processed < total_args )
				args_description+= ", ";
		}

		args_description+= " ]";
		template_error_context->parameters_description= std::move(args_description);
	}
	{
		std::string name= template_.parent_namespace->ToString();
		if( !name.empty() )
			name+= "::";
		name+= template_name;

		template_error_context->template_name= std::move(name);
	}
}

} // namesapce

void CodeBuilder::PrepareTypeTemplate(
	const Synt::TypeTemplateBase& type_template_declaration,
	TypeTemplatesSet& type_templates_set,
	NamesScope& names_scope )
{
	/* SPRACHE_TODO:
	 *) Support default template arguments for short form.
	 *) Convert signature and template arguments to "default form" for equality comparison.
	 *) Add "enable_if".
	 *) Support template-dependent types for value parameters, such template</ type T, U</ T /> ut />.
	*/

	const auto type_template= std::make_shared<TypeTemplate>();
	type_templates_set.type_templates.push_back( type_template );

	type_template->parent_namespace= &names_scope;
	type_template->syntax_element= &type_template_declaration;
	type_template->file_pos= type_template_declaration.file_pos_;

	std::vector<TypeTemplate::TemplateParameter>& template_parameters= type_template->template_parameters;
	template_parameters.reserve( type_template_declaration.args_.size() );
	std::vector<bool> template_parameters_usage_flags;

	NamesScope template_parameters_namespace( g_template_parameters_namespace_prefix, &names_scope );

	ProcessTemplateArgs(
		type_template_declaration.args_,
		names_scope,
		type_template_declaration.file_pos_,
		template_parameters,
		template_parameters_namespace,
		template_parameters_usage_flags );

	if( type_template_declaration.is_short_form_ )
	{
		U_ASSERT( type_template_declaration.signature_args_.empty() );
		// Assign template arguments to signature arguments.
		for( const Synt::TypeTemplateBase::Arg& arg : type_template_declaration.args_ )
		{
			PrepareTemplateSignatureParameter( type_template_declaration.file_pos_, *arg.name, template_parameters_namespace, template_parameters, template_parameters_usage_flags );
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
			PrepareTemplateSignatureParameter( signature_arg.name, template_parameters_namespace, template_parameters, template_parameters_usage_flags );
			type_template->signature_arguments.push_back(&signature_arg.name);

			if( std::get_if<Synt::EmptyVariant>( &signature_arg.default_value ) == nullptr )
			{
				PrepareTemplateSignatureParameter( signature_arg.default_value, template_parameters_namespace, template_parameters, template_parameters_usage_flags );
				type_template->default_signature_arguments.push_back(&signature_arg.default_value);
			}
			else
			{
				const size_t index= type_template->signature_arguments.size() - 1u;
				if (index > type_template->first_optional_signature_argument )
					REPORT_ERROR( MandatoryTemplateSignatureArgumentAfterOptionalArgument, names_scope.GetErrors(), type_template_declaration.file_pos_ );

				type_template->default_signature_arguments.push_back(nullptr);
				++type_template->first_optional_signature_argument;
			}
		}
	}
	U_ASSERT( type_template->signature_arguments.size() == type_template->default_signature_arguments.size() );
	U_ASSERT( type_template->first_optional_signature_argument <= type_template->default_signature_arguments.size() );

	for( size_t i= 0u; i < type_template->template_parameters.size(); ++i )
		if( !template_parameters_usage_flags[i] )
			REPORT_ERROR( TemplateArgumentNotUsedInSignature, names_scope.GetErrors(), type_template_declaration.file_pos_, type_template->template_parameters[i].name );
}

void CodeBuilder::PrepareFunctionTemplate(
	const Synt::FunctionTemplate& function_template_declaration,
	OverloadedFunctionsSet& functions_set,
	NamesScope& names_scope,
	const ClassProxyPtr& base_class )
{
	const Synt::ComplexName& complex_name = function_template_declaration.function_->name_;
	const std::string& function_template_name= complex_name.components.front().name;

	if( complex_name.components.size() > 1u )
		REPORT_ERROR( FunctionDeclarationOutsideItsScope, names_scope.GetErrors(), function_template_declaration.file_pos_ );
	if( complex_name.components.front().have_template_parameters )
		REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), function_template_declaration.file_pos_ );

	if( function_template_declaration.function_->block_ == nullptr )
		REPORT_ERROR( IncompleteMemberOfClassTemplate, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );
	if( function_template_declaration.function_->virtual_function_kind_ != Synt::VirtualFunctionKind::None )
		REPORT_ERROR( VirtualForFunctionTemplate, names_scope.GetErrors(), function_template_declaration.file_pos_, function_template_name );

	const auto function_template= std::make_shared<FunctionTemplate>();
	function_template->syntax_element= &function_template_declaration;
	function_template->file_pos= function_template_declaration.file_pos_;
	function_template->parent_namespace= &names_scope;
	function_template->base_class= base_class;

	std::vector<bool> template_parameters_usage_flags; // Currently unused, because function template have no signature.

	NamesScope template_parameters_namespace( g_template_parameters_namespace_prefix, &names_scope );

	ProcessTemplateArgs(
		function_template_declaration.args_,
		names_scope,
		function_template_declaration.file_pos_,
		function_template->template_parameters,
		template_parameters_namespace,
		template_parameters_usage_flags );

	// TODO - check duplicates and function templates with same signature.
	functions_set.template_functions.push_back( function_template );
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
		const std::string& arg_name= arg.name->components.front().name;

		// Check redefinition
		for( const auto& prev_arg : template_parameters )
		{
			if( prev_arg.name == arg_name )
			{
				REPORT_ERROR( Redefinition, names_scope.GetErrors(), file_pos, arg_name );
				continue;
			}
		}
		if( NameShadowsTemplateArgument( arg_name, names_scope ) )
			REPORT_ERROR( DeclarationShadowsTemplateArgument, names_scope.GetErrors(), file_pos, arg_name );

		Value* inserted_template_parameter= nullptr;

		if( arg.arg_type != nullptr )
		{
			// If template parameter is variable.

			// Resolve from outer space or from this template parameters.
			const Value* const type_value= ResolveValue( file_pos, template_parameters_namespace, *arg.arg_type );
			if( type_value == nullptr )
			{
				REPORT_ERROR( NameNotFound, names_scope.GetErrors(), file_pos, *arg.arg_type );
				continue;
			}
			const Type* const type= type_value->GetTypeName();
			if( type == nullptr )
			{
				REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), file_pos, arg.arg_type->components.back().name );
				continue;
			}

			if( !TypeIsValidForTemplateVariableArgument( *type ) )
			{
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), file_pos, type );
				continue;
			}

			// If type is template parameter, set usage flag.
			if( arg.arg_type->components.size() == 1u && !arg.arg_type->components.front().have_template_parameters )
			{
				for( const TypeTemplate::TemplateParameter& template_parameter : template_parameters )
				{
					if( template_parameter.name == arg.arg_type->components.front().name )
					{
						template_parameters_usage_flags[ size_t(&template_parameter - template_parameters.data()) ]= true;
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
			variable.constexpr_value= llvm::UndefValue::get( type->GetLLVMType() );
			variable.llvm_value= CreateGlobalConstantVariable( *type, arg_name, variable.constexpr_value );

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
				template_parameters_namespace.AddName( arg_name, Value( Type( FundamentalType( U_FundamentalType::i32, fundamental_llvm_types_.i32 ) ), file_pos ) ); // TODO - is this correct, use conncrete type?
		}

		if( inserted_template_parameter != nullptr )
			inserted_template_parameter->SetIsTemplateParameter(true);
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
				template_parameters_usage_flags[ size_t(&template_parameter - template_parameters.data()) ]= true;
				break;
			}
		}
	}

	// Do recursive preresolve for subsequent deduction.
	const Value* const start_value=
		ResolveForTemplateSignatureParameter( file_pos, signature_parameter, names_scope );
	if( start_value == nullptr )
	{
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), file_pos, signature_parameter );
		return;
	}
	if( start_value->GetTypeTemplatesSet() != nullptr )
	{
		for( const Synt::Expression& template_parameter : signature_parameter.components.back().template_parameters )
			PrepareTemplateSignatureParameter( template_parameter, names_scope, template_parameters, template_parameters_usage_flags );
	}
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const Synt::Expression& template_parameter,
	NamesScope& names_scope,
	const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	bool special_expr_type= true;

	if( const auto named_operand= std::get_if<Synt::NamedOperand>( &template_parameter ) )
	{
		if( named_operand->postfix_operators_.empty() && named_operand->prefix_operators_.empty() )
			PrepareTemplateSignatureParameter( named_operand->file_pos_, named_operand->name_, names_scope, template_parameters, template_parameters_usage_flags );
		else
			special_expr_type= false;
	}
	else if( const auto type_name= std::get_if<Synt::TypeNameInExpression>( &template_parameter ) )
		PrepareTemplateSignatureParameter( type_name->type_name, names_scope, template_parameters, template_parameters_usage_flags );
	else if( const auto bracket_expression= std::get_if<Synt::BracketExpression>( &template_parameter ) )
	{
		if( bracket_expression->postfix_operators_.empty() && bracket_expression->prefix_operators_.empty() )
			PrepareTemplateSignatureParameter( *bracket_expression->expression_, names_scope, template_parameters, template_parameters_usage_flags );
		else
			special_expr_type= false;
	}
	else
		special_expr_type= false;

	if( special_expr_type )
		return;

	// If this is not special expression - assume that this is variable-expression.

	const Variable var= BuildExpressionCodeEnsureVariable( template_parameter, names_scope, *global_function_context_ );

	if( !TypeIsValidForTemplateVariableArgument( var.type ) )
	{
		REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), Synt::GetExpressionFilePos( template_parameter ), var.type );
		return;
	}
	if( var.constexpr_value == nullptr )
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), Synt::GetExpressionFilePos( template_parameter ) );
}

void CodeBuilder::PrepareTemplateSignatureParameter(
	const Synt::TypeName& type_name_template_parameter,
	NamesScope& names_scope,
	const std::vector<TypeTemplate::TemplateParameter>& template_parameters,
	std::vector<bool>& template_parameters_usage_flags )
{
	if( const auto named_type_name= std::get_if<Synt::NamedTypeName>(&type_name_template_parameter) )
		PrepareTemplateSignatureParameter( named_type_name->file_pos_, named_type_name->name, names_scope, template_parameters, template_parameters_usage_flags );
	else if( const auto array_type_name= std::get_if<Synt::ArrayTypeName>(&type_name_template_parameter) )
	{
		PrepareTemplateSignatureParameter( *array_type_name->size, names_scope, template_parameters, template_parameters_usage_flags );
		PrepareTemplateSignatureParameter( *array_type_name->element_type, names_scope, template_parameters, template_parameters_usage_flags );
	}
	else if( const auto tuple_type_name= std::get_if<Synt::TupleType>(&type_name_template_parameter) )
	{
		for( const auto& element_type : tuple_type_name->element_types_ )
			PrepareTemplateSignatureParameter( element_type, names_scope, template_parameters, template_parameters_usage_flags );
	}
	else if( const auto function_pointer_type_name_ptr= std::get_if<Synt::FunctionTypePtr>(&type_name_template_parameter) )
	{
		const Synt::FunctionType& function_pointer_type_name= **function_pointer_type_name_ptr;
		// TODO - maybe check also reference tags?
		if( function_pointer_type_name.return_type_ != nullptr )
			PrepareTemplateSignatureParameter(*function_pointer_type_name.return_type_, names_scope, template_parameters, template_parameters_usage_flags );
		for( const Synt::FunctionArgument& arg : function_pointer_type_name.arguments_ )
			PrepareTemplateSignatureParameter( arg.type_, names_scope, template_parameters, template_parameters_usage_flags );
	}
	else U_ASSERT(false);
}

Value* CodeBuilder::ResolveForTemplateSignatureParameter(
	const FilePos& file_pos,
	const Synt::ComplexName& signature_parameter,
	NamesScope& names_scope )
{
	return ResolveValue( file_pos, names_scope, signature_parameter, ResolveMode::ForTemplateSignatureParameter );
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
				dependend_arg_index= size_t(&param - template_.template_parameters.data());
				break;
			}
		}
	}

	if( const Variable* const variable= std::get_if<Variable>(&template_parameter) )
	{
		if( dependend_arg_index == ~0u )
		{
			const Value* const signature_parameter_value=
				ResolveForTemplateSignatureParameter( signature_parameter_file_pos, signature_parameter, names_scope );
			if( signature_parameter_value == nullptr )
				return DeducedTemplateParameter::Invalid();

			if( const Variable* const named_variable= signature_parameter_value->GetVariable() )
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
			REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, names_scope.GetErrors(), signature_parameter_file_pos, variable->type );
			return DeducedTemplateParameter::Invalid();
		}

		if( variable->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), signature_parameter_file_pos );
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
				template_.template_parameters[ dependend_arg_index ].name,
				variable->constexpr_value );
		variable_for_insertion.constexpr_value= variable->constexpr_value;

		if( std::get_if<int>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
			deducible_template_parameters[ dependend_arg_index ]= std::move( variable_for_insertion ); // Set empty arg.
		else if( std::get_if<Type>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
			return DeducedTemplateParameter::Invalid(); // WTF?
		else if( const Variable* const prev_variable_value= std::get_if<Variable>( &deducible_template_parameters[ dependend_arg_index ] )  )
		{
			// Variable already known, Check conflicts.
			// TODO - do real comparision
			// TODO - maybe generate error in this case?
			if( prev_variable_value->constexpr_value->getUniqueInteger() != variable_for_insertion.constexpr_value->getUniqueInteger() )
				return DeducedTemplateParameter::Invalid();
		}

		return DeducedTemplateParameter::TemplateParameter();
	}

	const Type& given_type= std::get<Type>(template_parameter);

	// Try deduce simple arg.
	if( dependend_arg_index != ~0u )
	{
		if( template_.template_parameters[ dependend_arg_index ].type_name != nullptr )
		{
			// Expected variable, but type given.
			return DeducedTemplateParameter::Invalid();
		}
		else if( std::get_if<int>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
		{
			// Set empty arg.
			deducible_template_parameters[ dependend_arg_index ]= given_type;
		}
		else if( const Type* const prev_type= std::get_if<Type>( &deducible_template_parameters[ dependend_arg_index ] ) )
		{
			// Type already known. Check conflicts.
			if( *prev_type != given_type )
				return DeducedTemplateParameter::Invalid();
		}
		else if( std::get_if<Variable>( &deducible_template_parameters[ dependend_arg_index ] ) != nullptr )
		{
			// Bind type argument to variable parameter.
			return DeducedTemplateParameter::Invalid();
		}
		return DeducedTemplateParameter::TemplateParameter();
	}

	const Value* const signature_parameter_value=
		ResolveForTemplateSignatureParameter( signature_parameter_file_pos, signature_parameter, names_scope );
	if( signature_parameter_value == nullptr )
		return DeducedTemplateParameter::Invalid();

	if( const Type* const type= signature_parameter_value->GetTypeName() )
	{
		if( *type == given_type )
			return DeducedTemplateParameter::Type();
		return DeducedTemplateParameter::Invalid();
	}
	else if( const TypeTemplatesSet* const inner_type_templates_set= signature_parameter_value->GetTypeTemplatesSet() )
	{
		const Class* const given_type_class= given_type.GetClassType();
		if( given_type_class == nullptr )
			return DeducedTemplateParameter::Invalid();
		if( given_type_class->base_template == std::nullopt )
			return DeducedTemplateParameter::Invalid();

		// TODO - build type templates set here
		const TypeTemplate* inner_type_template= nullptr;
		for( const TypeTemplatePtr& candidate_template : inner_type_templates_set->type_templates )
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
					name_component.template_parameters[i],
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
	const Synt::Expression& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	if( const auto named_operand= std::get_if<Synt::NamedOperand>(&signature_parameter) )
	{
		if( named_operand->postfix_operators_.empty() && named_operand->prefix_operators_.empty() )
			return DeduceTemplateArguments( template_, template_parameter, named_operand->name_, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	}
	else if( const auto type_name= std::get_if<Synt::TypeNameInExpression>(&signature_parameter) )
		return DeduceTemplateArguments( template_, template_parameter, type_name->type_name, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	else if( const auto bracket_expression= std::get_if<Synt::BracketExpression>(&signature_parameter) )
	{
		if( bracket_expression->postfix_operators_.empty() && bracket_expression->prefix_operators_.empty() )
			return DeduceTemplateArguments( template_, template_parameter, *bracket_expression->expression_, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	}

	// This is not special kind of template signature argument. So, process it as variable-expression.

	const Variable* const param_var= std::get_if<Variable>( &template_parameter );
	if( param_var == nullptr )
		return DeducedTemplateParameter::Invalid();
	if( !TypeIsValidForTemplateVariableArgument( param_var->type ) )
		return DeducedTemplateParameter::Invalid();

	const Value val= BuildExpressionCode( signature_parameter, names_scope, *global_function_context_ );
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
	const Synt::TypeName& signature_parameter,
	const FilePos& signature_parameter_file_pos,
	DeducibleTemplateParameters& deducible_template_parameters,
	NamesScope& names_scope )
{
	if( const auto named_type= std::get_if<Synt::NamedTypeName>(&signature_parameter) )
		return DeduceTemplateArguments( template_, template_parameter, named_type->name, signature_parameter_file_pos, deducible_template_parameters, names_scope );
	else if( const auto array_type= std::get_if<Synt::ArrayTypeName>(&signature_parameter) )
	{
		const Type* const param_type= std::get_if<Type>( &template_parameter );
		if( param_type == nullptr )
			return DeducedTemplateParameter::Invalid();
		const Array* const param_array_type= param_type->GetArrayType();
		if( param_array_type == nullptr )
			return DeducedTemplateParameter::Invalid();

		Variable size_var;
		size_var.type= size_type_;
		size_var.value_type= ValueType::Value;
		size_var.llvm_value= size_var.constexpr_value=
			llvm::Constant::getIntegerValue( size_var.type.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), param_array_type->size ) );

		DeducedTemplateParameter::Array result;
		result.type=
			std::make_unique<DeducedTemplateParameter>(
				DeduceTemplateArguments( template_, param_array_type->type, *array_type->element_type, signature_parameter_file_pos, deducible_template_parameters, names_scope ) );
		result.size=
			std::make_unique<DeducedTemplateParameter>(
				DeduceTemplateArguments( template_, size_var, *array_type->size, signature_parameter_file_pos, deducible_template_parameters, names_scope ) );
		if( result.type->IsInvalid() || result.size->IsInvalid() ) // TODO - what is size is not variable, but type name? Check this.
			return DeducedTemplateParameter::Invalid();

		// All array parameters is known, so, this is concrete type.
		if( result.type->IsType() && result.size->IsVariable() )
			return DeducedTemplateParameter::Type();

		return std::move(result);
	}
	else if( const auto tuple_type_ptr= std::get_if<Synt::TupleType>(&signature_parameter) )
	{
		const Type* const param_type= std::get_if<Type>( &template_parameter );
		if( param_type == nullptr )
			return DeducedTemplateParameter::Invalid();
		const Tuple* const param_tuple_type= param_type->GetTupleType();
		if( param_tuple_type == nullptr )
			return DeducedTemplateParameter::Invalid();
		if( tuple_type_ptr->element_types_.size() != param_tuple_type->elements.size() )
			return DeducedTemplateParameter::Invalid();

		DeducedTemplateParameter::Tuple result;
		result.element_types.reserve( param_tuple_type->elements.size() );
		bool all_types_are_known= true;
		for( size_t i= 0u; i < param_tuple_type->elements.size(); ++i )
		{
			result.element_types.push_back( DeduceTemplateArguments( template_, param_tuple_type->elements[i], tuple_type_ptr->element_types_[i], signature_parameter_file_pos, deducible_template_parameters, names_scope ) );
			if( result.element_types.back().IsInvalid() )
				return DeducedTemplateParameter::Invalid();
			all_types_are_known= all_types_are_known && result.element_types.back().IsType();
		}

		if( all_types_are_known )
			return DeducedTemplateParameter::Type();
		else
			return std::move(result);
	}
	else if( const auto function_pointer_type_ptr= std::get_if<Synt::FunctionTypePtr>(&signature_parameter) )
	{
		const Synt::FunctionType* const function_pointer_type= function_pointer_type_ptr->get();
		const auto param_type= std::get_if<Type>( &template_parameter );
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
			result.return_type= std::make_unique<DeducedTemplateParameter>( std::move(ret_type_result) );
		}
		else
		{
			if( param_function_pointer_type->function.return_type != void_type_ )
				return DeducedTemplateParameter::Invalid();
			result.return_type= std::make_unique<DeducedTemplateParameter>( DeducedTemplateParameter::Type() );
		}

		if( !function_pointer_type->return_value_inner_reference_tags_.empty() ||
			!function_pointer_type->return_value_reference_tag_.empty() )
			REPORT_ERROR( NotImplemented, names_scope.GetErrors(), function_pointer_type->file_pos_, "reference tags for template signature parameters" );

		// Process args.
		if( param_function_pointer_type->function.args.size() != function_pointer_type->arguments_.size() )
			return DeducedTemplateParameter::Invalid();
		for( size_t i= 0u; i < function_pointer_type->arguments_.size(); ++i)
		{
			const Synt::FunctionArgument& expected_arg= function_pointer_type->arguments_[i];
			const Function::Arg& given_arg= param_function_pointer_type->function.args[i];

			const bool expected_mutable= expected_arg.mutability_modifier_ == MutabilityModifier::Mutable;
			const bool expected_reference= expected_arg.reference_modifier_ == ReferenceModifier::Reference;

			if( expected_mutable != given_arg.is_mutable || expected_reference != given_arg.is_reference )
				return DeducedTemplateParameter::Invalid();

			result.argument_types.push_back(
				DeduceTemplateArguments( template_, given_arg.type, expected_arg.type_, signature_parameter_file_pos, deducible_template_parameters, names_scope ));

			if( result.argument_types.back().IsInvalid() )
				return DeducedTemplateParameter::Invalid();

			if( !expected_arg.inner_arg_reference_tags_.empty() || !expected_arg.reference_tag_.empty() )
				REPORT_ERROR( NotImplemented, names_scope.GetErrors(), function_pointer_type->file_pos_, "reference tags for template signature parameters" );
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

Value* CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatesSet& type_templates_set,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope )
{
	if( type_templates_set.type_templates.size() == 1u )
		return GenTemplateType( file_pos, type_templates_set.type_templates.front(), template_arguments, arguments_names_scope, false ).type;

	std::vector<TemplateTypeGenerationResult> generated_types;
	for( const TypeTemplatePtr& type_template : type_templates_set.type_templates )
	{
		TemplateTypeGenerationResult generated_type=
			GenTemplateType(
				file_pos,
				type_template,
				template_arguments,
				arguments_names_scope,
				true );
		if( generated_type.type_template != nullptr )
		{
			generated_types.push_back( generated_type );
			U_ASSERT(generated_type.deduced_template_parameters.size() >= template_arguments.size());
		}
	}

	if( const TemplateTypeGenerationResult* const selected_template= SelectTemplateType( generated_types, template_arguments.size() ) )
		return GenTemplateType( file_pos, selected_template->type_template, template_arguments, arguments_names_scope, false ).type;
	else
	{
		REPORT_ERROR( CouldNotSelectMoreSpicializedTypeTemplate, arguments_names_scope.GetErrors(), file_pos );
		return nullptr;
	}
}

CodeBuilder::TemplateTypeGenerationResult CodeBuilder::GenTemplateType(
	const FilePos& file_pos,
	const TypeTemplatePtr& type_template_ptr,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope,
	const bool skip_type_generation )
{
	// This method does not generate some errors, because instantiation may fail
	// for one class template, but success for other.

	TemplateTypeGenerationResult result;

	const TypeTemplate& type_template= *type_template_ptr;
	NamesScope& template_names_scope= *type_template.parent_namespace;

	const std::string& type_template_name= type_template.syntax_element->name_;

	if( template_arguments.size() < type_template.first_optional_signature_argument )
		return result;

	DeducibleTemplateParameters deduced_template_args( type_template.template_parameters.size() );

	const NamesScopePtr template_parameters_namespace= std::make_shared<NamesScope>( g_template_parameters_namespace_prefix, &template_names_scope );
	for( const TypeTemplate::TemplateParameter& param : type_template.template_parameters )
		template_parameters_namespace->AddName( param.name, YetNotDeducedTemplateArg() );

	bool deduction_failed= false;
	std::vector<TemplateParameter> result_signature_parameters(type_template.signature_arguments.size());
	result.deduced_template_parameters.resize(type_template.signature_arguments.size());
	for( size_t i= 0u; i < type_template.signature_arguments.size(); ++i )
	{
		Value value;
		if( i < template_arguments.size() )
			value= BuildExpressionCode( template_arguments[i], arguments_names_scope, *global_function_context_ );
		else
			value= BuildExpressionCode( *type_template.default_signature_arguments[i], *template_parameters_namespace, *global_function_context_ );

		if( value.GetErrorValue() != nullptr )
			continue;

		const Synt::Expression& expr= *type_template.signature_arguments[i];
		// TODO - maybe add some errors, if not deduced?
		if( const Type* const type_name= value.GetTypeName() )
		{
			const DeducedTemplateParameter deduced= DeduceTemplateArguments( type_template, *type_name, expr, file_pos, deduced_template_args, template_names_scope );
			if( deduced.IsInvalid() )
			{
				deduction_failed= true;
				continue;
			}
			result.deduced_template_parameters[i]= deduced;
		}
		else if( const Variable* const variable= value.GetVariable() )
		{
			const DeducedTemplateParameter deduced= DeduceTemplateArguments( type_template, *variable, expr, file_pos, deduced_template_args, template_names_scope );
			if( deduced.IsInvalid() )
			{
				deduction_failed= true;
				continue;
			}
			result.deduced_template_parameters[i]= deduced;
		}
		else
		{
			REPORT_ERROR( InvalidValueAsTemplateArgument, arguments_names_scope.GetErrors(), file_pos, value.GetKindName() );
			continue;
		}

		// Update known arguments.
		for( size_t j= 0u; j < deduced_template_args.size(); ++j )
		{
			const DeducibleTemplateParameter& arg= deduced_template_args[j];
			Value* const value= template_parameters_namespace->GetThisScopeValue( type_template.template_parameters[j].name );
			U_ASSERT( value != nullptr );

			if( std::get_if<int>( &arg ) != nullptr )
			{} // Not deduced yet.
			else if( const Type* const type= std::get_if<Type>( &arg ) )
			{
				if( value->GetYetNotDeducedTemplateArg() != nullptr )
					*value= Value( *type, type_template.syntax_element->file_pos_ /*TODO - set correctfile_pos */ );
			}
			else if( const Variable* const variable= std::get_if<Variable>( &arg ) )
			{
				if( value->GetYetNotDeducedTemplateArg() != nullptr )
					*value= Value( *variable, type_template.syntax_element->file_pos_ /*TODO - set correctfile_pos */ );
			}
			else U_ASSERT( false );
		}

		if( !deduction_failed )
		{
			const Value result_singature_parameter= BuildExpressionCode( expr, *template_parameters_namespace, *global_function_context_ );
			if( const Type* const type_name= result_singature_parameter.GetTypeName() )
				result_signature_parameters[i]= *type_name;
			else if( const Variable* const variable= result_singature_parameter.GetVariable() )
				result_signature_parameters[i]= *variable;
		}

	} // for signature arguments

	if( deduction_failed )
	{
		return result;
	}
	result.type_template= type_template_ptr;

	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];

		if( std::get_if<int>( &arg ) != nullptr )
		{
			// SPRACHE_TODO - maybe not generate this error?
			// Other function templates, for example, can match given aruments.
			REPORT_ERROR( TemplateParametersDeductionFailed, arguments_names_scope.GetErrors(), file_pos );
			return result;
		}
	}

	if( skip_type_generation )
	{
		return result;
	}

	// Encode name.
	std::string name_encoded= g_template_parameters_namespace_prefix + type_template.syntax_element->name_;
	name_encoded+= EncodeTemplateParameters( deduced_template_args );
	name_encoded+= std::to_string( reinterpret_cast<uintptr_t>(&type_template) ); // Encode also template itself, because we can have multiple templates with same name.

	{ // Check, if already type generated.
		const auto it= generated_template_things_storage_.find( name_encoded );
		if( it != generated_template_things_storage_.end() )
		{
			const NamesScopePtr template_parameters_space= it->second.GetNamespace();
			U_ASSERT( template_parameters_space != nullptr );
			result.type= template_parameters_space->GetThisScopeValue( Class::c_template_class_name );
			return result;
		}
	}

	// Encode signature parameters for namespace. Each class template have different signature (parameters itslef may be same).
	template_parameters_namespace->SetThisNamespaceName( g_template_parameters_namespace_prefix + type_template_name + EncodeTemplateParameters( result_signature_parameters ) );

	generated_template_things_storage_.insert( std::make_pair( name_encoded, Value( template_parameters_namespace, type_template.syntax_element->file_pos_ ) ) );

	CreateTemplateErrorsContext( arguments_names_scope.GetErrors(), file_pos, template_parameters_namespace, type_template, type_template.syntax_element->name_, deduced_template_args );

	if( type_template.syntax_element->kind_ == Synt::TypeTemplateBase::Kind::Class )
	{
		const auto cache_class_it= template_classes_cache_.find( name_encoded );
		if( cache_class_it != template_classes_cache_.end() )
		{
			result.type=
				template_parameters_namespace->AddName(
					Class::c_template_class_name,
					Value(
						cache_class_it->second,
						type_template.syntax_element->file_pos_ /* TODO - check file_pos */ ) );
			return result;
		}

		const ClassProxyPtr class_proxy= NamesScopeFill( static_cast<const Synt::ClassTemplate*>( type_template.syntax_element )->class_, *template_parameters_namespace, Class::c_template_class_name );
		if( class_proxy == nullptr )
			return result;

		Class& the_class= *class_proxy->class_;
		// Save in class info about it`s base template.
		the_class.base_template.emplace();
		the_class.base_template->class_template= type_template_ptr;
		for( const DeducibleTemplateParameter& arg : deduced_template_args )
		{
			if( const Type* const type= std::get_if<Type>( &arg ) )
				the_class.base_template->template_parameters.push_back( *type );
			else if( const Variable* const variable= std::get_if<Variable>( &arg ) )
				the_class.base_template->template_parameters.push_back( *variable );
			else U_ASSERT(false);
		}
		the_class.base_template->signature_parameters= std::move(result_signature_parameters);

		template_classes_cache_[name_encoded]= class_proxy;
		result.type= template_parameters_namespace->GetThisScopeValue( Class::c_template_class_name );

		class_proxy->class_->llvm_type->setName( MangleType( class_proxy ) ); // Update llvm type name after setting base template.

		GlobalThingBuildClass( class_proxy, TypeCompleteness::Complete );

		return result;
	}
	else if( type_template.syntax_element->kind_ == Synt::TypeTemplateBase::Kind::Typedef )
	{
		const Type type= PrepareType( static_cast<const Synt::TypedefTemplate*>( type_template.syntax_element )->typedef_->value, *template_parameters_namespace, *global_function_context_ );

		if( type == invalid_type_ )
			return result;

		result.type= template_parameters_namespace->AddName( Class::c_template_class_name, Value( type, file_pos /* TODO - check file_pos */ ) );
		return result;
	}
	else U_ASSERT(false);

	return result;
}

const FunctionVariable* CodeBuilder::GenTemplateFunction(
	CodeBuilderErrorsContainer& errors_container,
	const FilePos& file_pos,
	const FunctionTemplatePtr& function_template_ptr,
	const ArgsVector<Function::Arg>& actual_args,
	const bool first_actual_arg_is_this,
	bool skip_arguments )
{
	const FunctionTemplate& function_template= *function_template_ptr;
	const Synt::Function& function_declaration= *function_template.syntax_element->function_;
	const std::string& func_name= function_declaration.name_.components.back().name;

	NamesScope& template_names_scope= *function_template.parent_namespace;

	const Function::Arg* given_args= actual_args.data();
	size_t given_arg_count= actual_args.size();

	if( first_actual_arg_is_this &&
		!function_declaration.type_.arguments_.empty() && function_declaration.type_.arguments_.front().name_ != Keywords::this_ )
	{
		++given_args;
		--given_arg_count;
	}

	if( !skip_arguments && given_arg_count != function_declaration.type_.arguments_.size() )
		return nullptr;

	DeducibleTemplateParameters deduced_template_args( function_template.template_parameters.size() );

	const auto template_parameters_namespace= std::make_shared<NamesScope>( g_template_parameters_namespace_prefix, &template_names_scope );
	for( size_t i= 0u; i < function_template.template_parameters.size(); ++i )
		template_parameters_namespace->AddName( function_template.template_parameters[i].name, YetNotDeducedTemplateArg() );
	for( size_t i= 0u; i < function_template.known_template_parameters.size(); ++i )
		template_parameters_namespace->AddName( function_template.known_template_parameters[i].first, function_template.known_template_parameters[i].second );

	bool deduction_failed= false;
	std::vector<DeducedTemplateParameter> deduced_temlpate_parameters( function_declaration.type_.arguments_.size() );
	for( size_t i= 0u; i < function_declaration.type_.arguments_.size() && !skip_arguments; ++i )
	{
		const Synt::FunctionArgument& function_argument= function_declaration.type_.arguments_[i];

		const bool expected_arg_is_mutalbe_reference=
			function_argument.mutability_modifier_ == Synt::MutabilityModifier::Mutable &&
			( function_argument.reference_modifier_ == Synt::ReferenceModifier::Reference || function_argument.name_ == Keywords::this_ );

		// Functin arg declared as "mut&", but given something immutable.
		if( expected_arg_is_mutalbe_reference && !given_args[i].is_mutable )
		{
			deduction_failed= true;
			continue;
		}

		if( i == 0u && function_argument.name_ == Keywords::this_ )
		{
			if( function_template.base_class != nullptr &&
				!( given_args[i].type == function_template.base_class || ReferenceIsConvertible( given_args[i].type, function_template.base_class, errors_container, file_pos ) ) )
			{
				// Givent type and type of "this" are different.
				deduction_failed= true;
				continue;
			}
			deduced_temlpate_parameters[i]= DeducedTemplateParameter::Type();
		}
		else
		{
			// For named types we check, if reference or type conversion is possible, and if not, do arguments deduction.
			bool deduced_specially= false;
			if( const auto named_type_name= std::get_if<Synt::NamedTypeName>( &function_argument.type_ ) )
			{
				size_t dependend_arg_index= ~0u;
				if( named_type_name->name.components.size() == 1u && !named_type_name->name.components.front().have_template_parameters )
				{
					for( const TypeTemplate::TemplateParameter& param : function_template.template_parameters )
					{
						if( param.name == named_type_name->name.components.front().name )
						{
							dependend_arg_index= size_t(&param - function_template.template_parameters.data());
							break;
						}
					}
				}

				if( dependend_arg_index == ~0u )
				{
					// Not template parameter, must be type name or template.
					const Value* const signature_parameter_value=
						ResolveForTemplateSignatureParameter( named_type_name->file_pos_, named_type_name->name, *template_parameters_namespace /*TODO - is this correct namespace? */ );
					if( signature_parameter_value == nullptr )
					{
						deduction_failed= true;
						continue;
					}
					if( const Type* const type= signature_parameter_value->GetTypeName() )
					{
						if( *type == given_args[i].type || ReferenceIsConvertible( given_args[i].type, *type, errors_container, file_pos ) ||
							( !expected_arg_is_mutalbe_reference && GetConversionConstructor( given_args[i].type, *type, errors_container, file_pos ) != nullptr ) )
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
						function_argument.type_,
						function_argument.file_pos_,
						deduced_template_args,
						*template_parameters_namespace /*TODO - is this correct namespace? */ );
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
			Value* const value= template_parameters_namespace->GetThisScopeValue( function_template.template_parameters[j].name );
			U_ASSERT( value != nullptr );

			if( std::get_if<int>( &arg ) != nullptr )
			{} // Not deduced yet.
			else if( const Type* const type= std::get_if<Type>( &arg ) )
			{
				if( value->GetYetNotDeducedTemplateArg() != nullptr )
					*value= Value( *type, function_template.file_pos /*TODO - set correctfile_pos */ );
			}
			else if( const Variable* const variable= std::get_if<Variable>( &arg ) )
			{
				if( value->GetYetNotDeducedTemplateArg() != nullptr )
					*value= Value( *variable, function_template.file_pos /*TODO - set correctfile_pos */ );
			}
			else U_ASSERT( false );
		}

	} // for template function arguments

	if( deduction_failed )
		return nullptr;

	for( size_t i = 0u; i < deduced_template_args.size() ; ++i )
	{
		const auto& arg = deduced_template_args[i];

		if( std::get_if<int>( &arg ) != nullptr )
		{
			REPORT_ERROR( TemplateParametersDeductionFailed, errors_container, file_pos );
			return nullptr;
		}
	}

	// Encode name.
	// Use encoded name only for cache search.
	// For function template namespace use only default namespace name.
	// Template namespace encoding does not needed, because in normal program each function (and template function) have different parameters and mangled name.
	std::string name_encoded= g_template_parameters_namespace_prefix + func_name;
	name_encoded+= EncodeTemplateParameters( deduced_template_args );
	name_encoded+= std::to_string( reinterpret_cast<uintptr_t>(&function_template) ); // HACK! use address of template object, because we can have multiple templates with same name.

	{
		const auto it= generated_template_things_storage_.find( name_encoded );
		if( it != generated_template_things_storage_.end() )
		{
			//Function for this template arguments already generated.
			const NamesScopePtr template_parameters_space= it->second.GetNamespace();
			U_ASSERT( template_parameters_space != nullptr );
			OverloadedFunctionsSet& result_functions_set= *template_parameters_space->GetThisScopeValue( func_name )->GetFunctionsSet();
			U_ASSERT( result_functions_set.functions.size() == 1u );
			return &result_functions_set.functions.front();
		}
	}
	generated_template_things_storage_.insert( std::make_pair( name_encoded, Value( template_parameters_namespace, function_declaration.file_pos_ ) ) );

	CreateTemplateErrorsContext( errors_container, file_pos, template_parameters_namespace, function_template, func_name, deduced_template_args, function_template.known_template_parameters );

	// First, prepare only as prototype.
	NamesScopeFill( function_template.syntax_element->function_, *template_parameters_namespace, function_template.base_class );
	OverloadedFunctionsSet& result_functions_set= *template_parameters_namespace->GetThisScopeValue( func_name )->GetFunctionsSet();
	GlobalThingBuildFunctionsSet( *template_parameters_namespace, result_functions_set, false );

	if( result_functions_set.functions.empty() )
		return nullptr; // Function prepare failed

	FunctionVariable& function_variable= result_functions_set.functions.front();
	function_variable.deduced_temlpate_parameters= std::move(deduced_temlpate_parameters);

	// And generate function body after insertion of prototype.
	if( !function_variable.have_body ) // if function is constexpr, body may be already generated.
		BuildFuncCode(
			function_variable,
			function_template.base_class,
			*template_parameters_namespace,
			function_template.syntax_element->function_->name_.components.back().name,
			function_template.syntax_element->function_->type_.arguments_,
			function_template.syntax_element->function_->block_.get(),
			function_template.syntax_element->function_->constructor_initialization_list_.get() );

	// Set correct mangled name
	if( function_variable.llvm_function != nullptr )
	{
		std::vector<TemplateParameter> params_for_mangle;
		for( const auto& known_param : function_template.known_template_parameters )
		{
			if( const auto type= known_param.second.GetTypeName() )
				params_for_mangle.emplace_back( *type );
			else if( const auto variable= known_param.second.GetVariable() )
				params_for_mangle.emplace_back( *variable );
			else U_ASSERT(false);
		}

		for(const auto& param : deduced_template_args)
		{
			if( const auto type= std::get_if<Type>( &param ) )
				params_for_mangle.emplace_back( *type );
			else if( const auto variable= std::get_if<Variable>( &param ) )
				params_for_mangle.emplace_back( *variable );
			else U_ASSERT(false);
		}

		const std::string mangled_name =
			MangleFunction(
				template_names_scope,
				func_name,
				*function_variable.type.GetFunctionType(),
				&params_for_mangle );
		function_variable.llvm_function->setName( mangled_name );
		if( llvm::Comdat* const comdat= function_variable.llvm_function->getComdat() )
		{
			llvm::Comdat* const new_comdat= module_->getOrInsertComdat( mangled_name );
			new_comdat->setSelectionKind( comdat->getSelectionKind() );
			function_variable.llvm_function->setComdat( new_comdat );
		}
	}

	// Two-step preparation needs for recursive function template call.

	return &function_variable;
}

// TODO - pass function context.
Value* CodeBuilder::GenTemplateFunctionsUsingTemplateParameters(
	const FilePos& file_pos,
	const std::vector<FunctionTemplatePtr>& function_templates,
	const std::vector<Synt::Expression>& template_arguments,
	NamesScope& arguments_names_scope )
{
	U_ASSERT( !function_templates.empty() );

	DeducibleTemplateParameters template_parameters;
	bool something_is_wrong= false;
	for( const Synt::Expression& expr : template_arguments )
	{
		const Value value= BuildExpressionCode( expr, arguments_names_scope, *global_function_context_ );
		if( const auto type_name= value.GetTypeName() )
			template_parameters.push_back( *type_name );
		else if( const auto variable= value.GetVariable() )
		{
			if( !TypeIsValidForTemplateVariableArgument( variable->type ) )
				REPORT_ERROR( InvalidTypeOfTemplateVariableArgument, arguments_names_scope.GetErrors(), Synt::GetExpressionFilePos(expr), variable->type );
			else if( variable->constexpr_value == nullptr )
				REPORT_ERROR( ExpectedConstantExpression, arguments_names_scope.GetErrors(), Synt::GetExpressionFilePos(expr) );
			else
				template_parameters.push_back( *variable );
		}
		else
		{
			REPORT_ERROR( InvalidValueAsTemplateArgument, arguments_names_scope.GetErrors(), file_pos, value.GetKindName() );
			something_is_wrong= true;
		}

	} // for given template arguments.

	if( something_is_wrong )
		return nullptr;

	// Encode name, based on set of function templates and given tempate parameters.
	std::string name_encoded= g_template_parameters_namespace_prefix + function_templates.front()->syntax_element->function_->name_.components.front().name;
	name_encoded+= EncodeTemplateParameters( template_parameters );

	{
		const auto it= generated_template_things_storage_.find( name_encoded );
		if( it != generated_template_things_storage_.end() )
			return &it->second; // Already generated.
	}

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
			if( std::get_if<Type>( &template_parameters[i] ) != nullptr )
			{
				if( function_template_parameter.type_name != nullptr )
				{
					ok= false; // Error, type parameter given, but value parameter expected.
					break;
				}
			}
			else if( const Variable* const given_variable= std::get_if<Variable>( &template_parameters[i] ) )
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
					if( const Type* const expected_type= std::get_if<Type>( &template_parameters[type_parameter_index] ) )
						ok= *expected_type == given_variable->type;
					else
						ok= false;
				}
				else
				{
					if( const Value* const type_value=
							ResolveValue( function_template.file_pos, *function_template.parent_namespace, *function_template_parameter.type_name ) )
					{
						if( const Type* const expected_type= type_value->GetTypeName() )
							ok= *expected_type == given_variable->type;
						else
							ok= false;
					}
					else
						ok= false;
				}

				if( !ok )
					break;
			}
			else U_ASSERT(false);

		} // for given template parameters

		if( !ok )
			continue;

		const auto new_template= std::make_shared<FunctionTemplate>();
		// Reduce count of template arguments in new function template.
		new_template->template_parameters.insert(
			new_template->template_parameters.end(),
			function_template.template_parameters.begin() + std::ptrdiff_t(template_parameters.size()), function_template.template_parameters.end() );

		new_template->parent_namespace= function_template.parent_namespace;
		new_template->file_pos= function_template.file_pos;
		new_template->syntax_element= function_template.syntax_element;
		new_template->base_class= function_template.base_class;

		// Fill set of known parameters.
		for( size_t i= 0u; i < template_parameters.size(); ++i )
		{
			const std::string& name= function_template.template_parameters[i].name;
			if( const Type* const type= std::get_if<Type>( &template_parameters[i] ) )
				new_template->known_template_parameters.emplace_back( name, Value( *type, file_pos ) );
			else if( const Variable* const variable= std::get_if<Variable>( &template_parameters[i] ) )
				new_template->known_template_parameters.emplace_back( name, Value( *variable, file_pos ) );
			else U_ASSERT(false);
		}

		result.template_functions.push_back( new_template );
	} // for function templates

	if( result.template_functions.empty() )
	{
		REPORT_ERROR( TemplateFunctionGenerationFailed, arguments_names_scope.GetErrors(), file_pos, function_templates.front()->syntax_element->function_->name_.components.back().name );
		return nullptr;
	}

	return & generated_template_things_storage_.insert( std::make_pair( name_encoded, result ) ).first->second;
}

bool CodeBuilder::NameShadowsTemplateArgument( const std::string& name, NamesScope& names_scope )
{
	// Not implemented correctly yet.
	U_UNUSED(name);
	U_UNUSED(names_scope);
	return false;
}

bool CodeBuilder::TypeIsValidForTemplateVariableArgument( const Type& type )
{
	if( const FundamentalType* const fundamental= type.GetFundamentalType() )
	{
		if( IsInteger( fundamental->fundamental_type ) ||
			IsChar( fundamental->fundamental_type ) ||
			fundamental->fundamental_type == U_FundamentalType::Bool )
			return true;
	}
	if( type.GetEnumType() != nullptr )
	{
		U_ASSERT( TypeIsValidForTemplateVariableArgument( type.GetEnumType()->underlaying_type ) );
		return true;
	}

	return false;
}

void CodeBuilder::ReportAboutIncompleteMembersOfTemplateClass( const FilePos& file_pos, Class& class_ )
{
	class_.members.ForEachInThisScope(
		[this, file_pos, &class_]( const std::string& name, const Value& value )
		{
			if( const Type* const type= value.GetTypeName() )
			{
				if( Class* const subclass= type->GetClassType() )
				{
					if( subclass->completeness != TypeCompleteness::Complete )
						REPORT_ERROR( IncompleteMemberOfClassTemplate, class_.members.GetErrors(), file_pos, name );
					else
						ReportAboutIncompleteMembersOfTemplateClass( file_pos, *subclass );
				}
			}
			else if( const OverloadedFunctionsSet* const functions_set= value.GetFunctionsSet() )
			{
				for( const FunctionVariable& function : functions_set->functions )
				{
					if( !function.have_body )
						REPORT_ERROR( IncompleteMemberOfClassTemplate, class_.members.GetErrors(), file_pos, name );
				}
			}
			else if( value.GetClassField() != nullptr )
			{}
			else if( value.GetTypeTemplatesSet() != nullptr )
			{}
			else if( const NamesScopePtr inner_namespace= value.GetNamespace() )
			{
				const std::string& generated_class_name= Class::c_template_class_name;

				// This must be only namespace for class template instantiation.
				inner_namespace->ForEachInThisScope(
					[&]( const std::string& name, const Value& inner_namespace_value )
					{
						if( name == generated_class_name )
						{
							const Type* const generated_class_type= inner_namespace_value.GetTypeName();
							U_ASSERT( generated_class_type != nullptr );
							Class* const generated_class= generated_class_type->GetClassType();
							U_ASSERT( generated_class != nullptr );
							U_ASSERT( generated_class->base_template != std::nullopt );
							ReportAboutIncompleteMembersOfTemplateClass( file_pos, *generated_class );
						}
					});
			}
			else if( value.GetVariable() != nullptr )
			{}
			else U_ASSERT(false);
		} );
}

} // namespace CodeBuilderPrivate

} // namespace U
