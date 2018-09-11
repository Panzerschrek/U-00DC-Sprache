#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/program_string.hpp"
#include "../lex_synt_lib/syntax_analyzer.hpp"
#include "strings.h"

#include "program_model.h"

namespace U
{

namespace QtCreatorPlugin
{


static ProgramString Stringify( const Synt::ComplexName& complex_name );
static ProgramString Stringify( const Synt::ITypeName& type_name );

static ProgramString Stringify( const Synt::IExpressionComponent& expression )
{
	ProgramString result;
	if( const auto binary_operator= dynamic_cast<const Synt::BinaryOperator*>(&expression) )
		result= Stringify( *binary_operator->left_ ) + " "_SpC + BinaryOperatorToString(binary_operator->operator_type_) + " "_SpC + Stringify( *binary_operator->right_ );
	else if( const auto named_operand= dynamic_cast<const Synt::NamedOperand*>(&expression) )
		result= Stringify( named_operand->name_ );
	else if( const auto numeric_constant= dynamic_cast<const Synt::NumericConstant*>(&expression) )
		result= QStringToProgramString( QString::number(static_cast<double>(numeric_constant->value_)) ) + numeric_constant->type_suffix_;
	else if( const auto string_literal= dynamic_cast<const Synt::StringLiteral*>(&expression) )
		result= "\""_SpC + string_literal->value_ + "\""_SpC + string_literal->type_suffix_;
	else if( const auto boolean_constant= dynamic_cast<const Synt::BooleanConstant*>(&expression) )
		result= boolean_constant->value_ ? Keyword( Keywords::true_ ) : Keyword( Keywords::false_ );
	else if( const auto bracket_expression= dynamic_cast<const Synt::BracketExpression*>(&expression) )
		result= "("_SpC + Stringify( *bracket_expression->expression_ ) + ")"_SpC;
	else if( const auto type_name_in_expression= dynamic_cast<const Synt::TypeNameInExpression*>(&expression) )
		result= Stringify( *type_name_in_expression->type_name );
	else if( const auto move_operator= dynamic_cast<const Synt::MoveOperator*>(&expression) )
		result= Keyword( Keywords::move_ ) + "("_SpC + move_operator->var_name_ + ")"_SpC;
	else if( const auto cast_ref= dynamic_cast<const Synt::CastRef*>(&expression) )
		result= Keyword( Keywords::cast_ref ) + "</"_SpC + Stringify( *cast_ref->expression_ )  + "/>"_SpC + "("_SpC + Stringify( *cast_ref->expression_ ) + ")"_SpC;
	else if( const auto cast_ref_unsafe= dynamic_cast<const Synt::CastRefUnsafe*>(&expression) )
		result= Keyword( Keywords::cast_ref_unsafe ) + "</"_SpC + Stringify( *cast_ref_unsafe->expression_ )  + "/>"_SpC + "("_SpC + Stringify( *cast_ref_unsafe->expression_ ) + ")"_SpC;
	else if( const auto cast_imut= dynamic_cast<const Synt::CastImut*>(&expression) )
		result= Keyword( Keywords::cast_imut ) + "("_SpC + Stringify( *cast_imut->expression_ ) + ")"_SpC;
	else if( const auto cast_mut= dynamic_cast<const Synt::CastMut*>(&expression) )
		result= Keyword( Keywords::cast_mut ) + "("_SpC + Stringify( *cast_mut->expression_ ) + ")"_SpC;
	else if( const auto typeinfo_= dynamic_cast<const Synt::TypeInfo*>(&expression) )
		result= Keyword( Keywords::cast_ref ) + "</"_SpC + Stringify( *typeinfo_->type_ )  + "/>"_SpC;
	else U_ASSERT(false);

	if( const auto expression_with_unary_operators= dynamic_cast<const Synt::ExpressionComponentWithUnaryOperators*>( &expression ) )
	{
		for( const Synt::IUnaryPostfixOperatorPtr& postfix_operator : expression_with_unary_operators->postfix_operators_ )
		{
			if( const auto indexation_operator= dynamic_cast<const Synt::IndexationOperator*>( postfix_operator.get() ) )
				result+= "["_SpC + Stringify( *indexation_operator->index_ ) + "]"_SpC;
			else if( const auto member_access_operator= dynamic_cast<const Synt::MemberAccessOperator*>( postfix_operator.get() ) )
			{
				result+= "."_SpC + member_access_operator->member_name_;
				if( member_access_operator->have_template_parameters )
				{
					result+= "</"_SpC;
					for( const Synt::IExpressionComponentPtr& template_param : member_access_operator->template_parameters )
					{
						result+= Stringify(*template_param);

						if( &template_param != &member_access_operator->template_parameters.back() )
							result+= ", "_SpC;
					}
					result+= "/>"_SpC;
				}
			}
			else if( const auto call_operator= dynamic_cast<const Synt::CallOperator*>( postfix_operator.get() ) )
			{
				result+= "("_SpC;
				for( const Synt::IExpressionComponentPtr& arg : call_operator->arguments_ )
				{
					result+= Stringify(*arg);

					if( &arg != &call_operator->arguments_.back() )
						result+= ", "_SpC;
				}
				result+= ")"_SpC;
			}
			else U_ASSERT(false);
		} // for unary postfix operators

		for( const Synt::IUnaryPrefixOperatorPtr& prefix_operator : expression_with_unary_operators->prefix_operators_ )
		{
			OverloadedOperator op= OverloadedOperator::None;
			if( dynamic_cast<const Synt::UnaryMinus*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::Sub;
			else if( dynamic_cast<const Synt::UnaryPlus*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::Add;
			else if( dynamic_cast<const Synt::LogicalNot*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::LogicalNot;
			else if( dynamic_cast<const Synt::BitwiseNot*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::BitwiseNot;
			else U_ASSERT( false );

			result= OverloadedOperatorToString(op) + result;
		}
	}
	return result;
}

static ProgramString Stringify( const Synt::ComplexName& complex_name )
{
	ProgramString result;
	for( const Synt::ComplexName::Component& component : complex_name.components )
	{
		result+= component.name;

		if( component.have_template_parameters )
		{
			result+= "</"_SpC;
			for( const Synt::IExpressionComponentPtr& template_param : component.template_parameters )
			{
				result+= Stringify(*template_param);

				if( &template_param != &component.template_parameters.back() )
					result+= ", "_SpC;
			}
			result+= "/>"_SpC;
		}

		if( &component != &complex_name.components.back() )
			result+= "::"_SpC;
	}

	return result;
}

static ProgramString Stringify( const Synt::ITypeName& type_name )
{
	if (const auto named_type_name= dynamic_cast<const Synt::NamedTypeName*>(&type_name) )
	{
		return Stringify( named_type_name->name );
	}
	else if( const auto array_type_name= dynamic_cast<const Synt::ArrayTypeName*>(&type_name) )
	{
		ProgramString result;

		result+= "[ "_SpC;
		result+= Stringify( *array_type_name->element_type );
		result+= ", "_SpC;
		result+= Stringify( *array_type_name->size );
		result+= " ]"_SpC;

		return result;
	}
	else if( const auto function_type_name= dynamic_cast<const Synt::FunctionType*>(&type_name) )
	{
	}
	else U_ASSERT(false);

	return ProgramString();
}

static ProgramString Stringify( const Synt::Function& function )
{
	ProgramString result;
	if( function.overloaded_operator_ != OverloadedOperator::None )
		result+= Keyword( Keywords::op_ ) + OverloadedOperatorToString( function.overloaded_operator_ );
	else
		result+= function.name_.components.back().name;

	result+= "("_SpC;

	// args
	for( const Synt::FunctionArgumentPtr& arg : function.type_.arguments_ )
	{
		if( arg->name_ == Keywords::this_ )
		{
			switch( arg->mutability_modifier_ )
			{
			case Synt::MutabilityModifier::None: break;
			case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); result+= " "_SpC; break;
			case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); result+= " "_SpC; break;
			case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ ); result+= " "_SpC; break;
			}
			result+= Keyword( Keywords::this_ );
		}
		else
		{
			result+= Stringify( *arg->type_ );
			switch( arg->reference_modifier_ )
			{
			case Synt::ReferenceModifier::None: break;
			case Synt::ReferenceModifier::Reference: result+= "&"_SpC; break;
			}

			if( !arg->reference_tag_.empty() )
			{
				result+= "'"_SpC;
				result+= arg->reference_tag_;
			}

			switch( arg->mutability_modifier_ )
			{
			case Synt::MutabilityModifier::None: break;
			case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
			case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
			case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ ); break;
			}

			result+= " "_SpC;
			result+= arg->name_;

			if( !arg->inner_arg_reference_tags_.empty() )
			{
				result+= "'"_SpC;
				for( const ProgramString& tag : arg->inner_arg_reference_tags_ )
				{
					if( tag.empty() )
						result+= "..."_SpC;
					else
						result+= tag;
					if( &tag != &arg->inner_arg_reference_tags_.back() && !arg->inner_arg_reference_tags_.back().empty() )
						result+= ", "_SpC;
				}
				result+= "'"_SpC;
			}
		}

		if( &arg != &function.type_.arguments_.back() )
			result+= ", "_SpC;
	}

	result+= ") "_SpC;

	if( !function.type_.referecnces_pollution_list_.empty() )
	{
		result+= "'"_SpC;
		for( const Synt::FunctionReferencesPollution& pollution : function.type_.referecnces_pollution_list_ )
		{
			result+= pollution.first;
			result+= " <- "_SpC;
			result+= pollution.second.is_mutable ? Keyword( Keywords::mut_ ) : Keyword( Keywords::imut_ );
			result+= " "_SpC;
			result+= pollution.second.name;

			if( &pollution != &function.type_.referecnces_pollution_list_.back() )
				result+= ", "_SpC;
		}
		result+= "' "_SpC;
	}

	if( function.type_.unsafe_ )
		result+= Keyword( Keywords::unsafe_ );

	// return value
	result+= " : "_SpC;
	if( function.type_.return_type_ == nullptr )
		result+= Keyword( Keywords::void_ );
	else
		result+= Stringify( *function.type_.return_type_ );

	if( !function.type_.return_value_inner_reference_tags_.empty() )
	{
		result+= "'"_SpC;
		for( const ProgramString& tag : function.type_.return_value_inner_reference_tags_ )
		{
			if( tag.empty() )
				result+= "..."_SpC;
			else
				result+= tag;
			if( &tag != &function.type_.return_value_inner_reference_tags_.back() && !function.type_.return_value_inner_reference_tags_.back().empty() )
				result+= ", "_SpC;
		}
		result+= "'"_SpC;
	}

	switch( function.type_.return_value_reference_modifier_ )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference:
		result+= "&"_SpC;
		break;
	}

	switch( function.type_.return_value_mutability_modifier_ )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

static ProgramString Stringify( const Synt::TypeTemplateBase& type_template )
{
	ProgramString result;

	if( const auto class_template= dynamic_cast<const Synt::ClassTemplate*>( &type_template ) )
		result+= class_template->class_->name_;
	else if( const auto typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( &type_template ) )
		result+= typedef_template->name_;
	else U_ASSERT(false);

	result+= "</"_SpC;
	if( type_template.is_short_form_ )
	{
		for( const Synt::TypeTemplateBase::Arg& arg : type_template.args_ )
		{
			result+= Stringify( *arg.name );
			if( &arg != &type_template.args_.back() )
				result+= ", "_SpC;
		}
	}
	else
	{
		for( const Synt::TypeTemplateBase::SignatureArg& arg : type_template.signature_args_ )
		{
			result+= Stringify( *arg.name );
			if( &arg != &type_template.signature_args_.back() )
				result+= ", "_SpC;
		}
	}
	result+= "/>"_SpC;

	return result;
}

static ProgramString Stringify( const Synt::FunctionTemplate& function_template )
{
	ProgramString result;

	result+= Keyword( Keywords::template_ );
	result+= "</"_SpC;
	for( const Synt::TypeTemplateBase::Arg& arg : function_template.args_ )
	{
		result+= Stringify( *arg.name );
		if( &arg != &function_template.args_.back() )
			result+= ", "_SpC;
	}
	result+= "/>"_SpC;

	result+= " "_SpC;
	result+= Stringify( *function_template.function_ );
	return result;
}

static ProgramString Stringify( const Synt::ClassField& class_field )
{
	ProgramString result= class_field.name;
	result+= ": "_SpC + Stringify( *class_field.type ) + " "_SpC;

	switch( class_field.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"_SpC; break;
	}

	switch( class_field.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

static ProgramString Stringify( const Synt::VariablesDeclaration::VariableEntry& varaible, const ProgramString& type )
{
	ProgramString result= varaible.name;
	result+= ": "_SpC + type + " "_SpC;

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"_SpC; break;
	}

	switch( varaible.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

static ProgramString Stringify( const Synt::AutoVariableDeclaration& varaible )
{
	ProgramString result= varaible.name;
	result+= ": "_SpC + Keyword( Keywords::auto_ ) + " "_SpC;

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"_SpC; break;
	}

	switch( varaible.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

static std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::Enum& enum_ )
{
	std::vector<ProgramModel::ProgramTreeNode> result;

	for( const Synt::Enum::Member& member : enum_.members )
	{
		ProgramModel::ProgramTreeNode element;
		element.name= ProgramStringToQString( member.name );
		element.kind= ProgramModel::ElementKind::EnumElement;
		element.number_in_parent= result.size();
		element.file_pos= member.file_pos;
		result.push_back(element);
	}

	return result;
}

static std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ClassElements& elements )
{
	std::vector<ProgramModel::ProgramTreeNode> result;

	ProgramModel::Visibility current_visibility= ProgramModel::Visibility::Public;
	for( const Synt::IClassElementPtr& class_element : elements )
	{
		if( const auto class_= dynamic_cast<const Synt::Class*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.childs= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_->file_pos_;
			result.push_back(element);
		}
		else if( const auto enum_= dynamic_cast<const Synt::Enum*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( enum_->name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.childs= BuildProgramModel_r( *enum_ );
			element.number_in_parent= result.size();
			element.file_pos= enum_->file_pos_;
			result.push_back(element);
		}
		else if( const auto typedef_= dynamic_cast<const Synt::Typedef*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( typedef_->name );
			element.kind= ProgramModel::ElementKind::Typedef;
			element.number_in_parent= result.size();
			element.file_pos= typedef_->file_pos_;
			result.push_back(element);
		}
		else if( const auto function_= dynamic_cast<const Synt::Function*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *function_ ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= function_->file_pos_;
			result.push_back(element);
		}
		else if( const auto variables_= dynamic_cast<const Synt::VariablesDeclaration*>( class_element.get() ) )
		{
			const ProgramString type_name= Stringify( *variables_->type );
			for( const auto& variable : variables_->variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= ProgramStringToQString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.visibility= current_visibility;
				element.number_in_parent= result.size();
				element.file_pos= variable.file_pos;
				result.push_back(element);
			}
		}
		else if( const auto auto_variable_= dynamic_cast<const Synt::AutoVariableDeclaration*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *auto_variable_ ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= auto_variable_->file_pos_;
			result.push_back(element);
		}
		else if( const auto class_field_= dynamic_cast<const Synt::ClassField*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassFiled;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= class_field_->file_pos_;
			result.push_back(element);
		}
		else if( const auto class_template= dynamic_cast<const Synt::ClassTemplate*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *class_template ) );
			element.kind= ProgramModel::ElementKind::ClassTemplate;
			element.visibility= current_visibility;
			element.childs= BuildProgramModel_r( class_template->class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_template->file_pos_;
			result.push_back(element);
		}
		else if( const auto typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *typedef_template ) );
			element.kind= ProgramModel::ElementKind::TypedefTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= typedef_template->file_pos_;
			result.push_back(element);
		}
		else if( const auto function_template= dynamic_cast<const Synt::FunctionTemplate*>( class_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *function_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= function_template->file_pos_;
			result.push_back(element);
		}
		else if( const auto visibility_label= dynamic_cast<const Synt::ClassVisibilityLabel*>( class_element.get() ) )
		{
			current_visibility= visibility_label->visibility_;
		}
	}

	return result;
}

static std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	std::vector<ProgramModel::ProgramTreeNode> result;

	for( const Synt::IProgramElementPtr& program_element : elements )
	{
		if( const auto namespace_= dynamic_cast<const Synt::Namespace*>( program_element.get() ) )
		{
			// TODO - what if there are multiple declarations of same namespace in single source file?
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( namespace_->name_ );
			element.kind= ProgramModel::ElementKind::Namespace;
			element.childs= BuildProgramModel_r( namespace_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= namespace_->file_pos_;
			result.push_back(element);
		}
		else if( const auto class_= dynamic_cast<const Synt::Class*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.childs= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_->file_pos_;
			result.push_back(element);
		}
		else if( const auto enum_= dynamic_cast<const Synt::Enum*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( enum_->name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.childs= BuildProgramModel_r( *enum_ );
			element.number_in_parent= result.size();
			element.file_pos= enum_->file_pos_;
			result.push_back(element);
		}
		else if( const auto typedef_= dynamic_cast<const Synt::Typedef*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( typedef_->name );
			element.kind= ProgramModel::ElementKind::Typedef;
			element.number_in_parent= result.size();
			element.file_pos= typedef_->file_pos_;
			result.push_back(element);
		}
		else if( const auto function_= dynamic_cast<const Synt::Function*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *function_ ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.number_in_parent= result.size();
			element.file_pos= function_->file_pos_;
			result.push_back(element);
		}
		else if( const auto variables_= dynamic_cast<const Synt::VariablesDeclaration*>( program_element.get() ) )
		{
			const ProgramString type_name= Stringify( *variables_->type );
			for( const auto& variable : variables_->variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= ProgramStringToQString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.number_in_parent= result.size();
				element.file_pos= variable.file_pos;
				result.push_back(element);
			}
		}
		else if( const auto auto_variable_= dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *auto_variable_ ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.number_in_parent= result.size();
			element.file_pos= auto_variable_->file_pos_;
			result.push_back(element);
		}
		else if( const auto class_template= dynamic_cast<const Synt::ClassTemplate*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *class_template ) );
			element.kind= ProgramModel::ElementKind::ClassTemplate;
			element.childs= BuildProgramModel_r( class_template->class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_template->file_pos_;
			result.push_back(element);
		}
		else if( const auto typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *typedef_template ) );
			element.kind= ProgramModel::ElementKind::TypedefTemplate;
			element.number_in_parent= result.size();
			element.file_pos= typedef_template->file_pos_;
			result.push_back(element);
		}
		else if( const auto function_template= dynamic_cast<const Synt::FunctionTemplate*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *function_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.number_in_parent= result.size();
			element.file_pos= function_template->file_pos_;
			result.push_back(element);
		}
	}

	return result;
}

static void SetupParents( ProgramModel::ProgramTreeNode& tree )
{
	for( ProgramModel::ProgramTreeNode& child : tree.childs )
	{
		child.parent= &tree;
		SetupParents(child);
	}
}

const ProgramModel::ProgramTreeNode* GetNode_r( const std::vector<ProgramModel::ProgramTreeNode>& nodes, const FilePos& file_pos )
{
	// TODO - optimize, make O(log(n)), instead of O(n).
	for( size_t i= 0u; i < nodes.size(); ++i )
	{
		const ProgramModel::ProgramTreeNode& node= nodes[i];

		FilePos next_file_pos;
		if( i + 1u < nodes.size() )
			next_file_pos= nodes[i+1u].file_pos;
		else
			next_file_pos.line= next_file_pos.pos_in_line= next_file_pos.file_index= static_cast<unsigned short>(~0u);

		if( node.file_pos <= file_pos && file_pos < next_file_pos )
		{
			const ProgramModel::ProgramTreeNode* const child_node= GetNode_r( node.childs, file_pos );
			if( child_node != nullptr )
				return child_node;
			return &node;
		}
	}

	return nullptr;
}

const ProgramModel::ProgramTreeNode* ProgramModel::GetNodeForFilePos( const FilePos& file_pos ) const
{
	return GetNode_r( program_elements, file_pos );
}

ProgramModelPtr BuildProgramModel( const ProgramString& program_text )
{
	const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text );
	if( !lex_result.error_messages.empty() )
		return nullptr;

	U::Synt::SyntaxAnalysisResult synt_result= U::Synt::SyntaxAnalysis( lex_result.lexems );
	// Do NOT abort on errors, because in process of source code editing may occurs some errors.

	const auto result= std::make_shared<ProgramModel>();
	result->program_elements= BuildProgramModel_r( synt_result.program_elements );

	for( ProgramModel::ProgramTreeNode& node : result->program_elements )
		SetupParents( node );

	return result;
}

} // namespace QtCreatorPlugin

} // namespace U
