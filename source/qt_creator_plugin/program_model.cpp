#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/program_string.hpp"
#include "../lex_synt_lib/syntax_analyzer.hpp"
#include "strings.hpp"

#include "program_model.hpp"

namespace U
{

namespace QtCreatorPlugin
{


static ProgramString Stringify( const Synt::ComplexName& complex_name );
static ProgramString Stringify( const Synt::TypeName& type_name );

static ProgramString Stringify( const Synt::Expression& expression )
{
	struct Visitor final : public boost::static_visitor<ProgramString>
	{
		ProgramString operator()( const Synt::EmptyVariant& )
		{
			return ProgramString();
		}
		ProgramString operator()( const Synt::BinaryOperator& binary_operator )
		{
			if( binary_operator.left_ == nullptr || binary_operator.right_ == nullptr )
				return ProgramString();
			return Stringify( *binary_operator.left_ ) + " "_SpC + BinaryOperatorToString(binary_operator.operator_type_) + " "_SpC + Stringify( *binary_operator.right_ );
		}
		ProgramString operator()( const Synt::NamedOperand& named_operand )
		{
			return Stringify( named_operand.name_ );
		}
		ProgramString operator()( const Synt::TernaryOperator& ternary_operator )
		{
			if( ternary_operator.condition == nullptr || ternary_operator.true_branch == nullptr || ternary_operator.false_branch == nullptr )
				return ProgramString();
			return "select( "_SpC + Stringify( *ternary_operator.condition ) + " ? "_SpC + Stringify( *ternary_operator.true_branch ) + " : "_SpC + Stringify( *ternary_operator.false_branch ) + " )"_SpC;
		}
		ProgramString operator()( const Synt::NumericConstant& numeric_constant )
		{
			return QStringToProgramString( QString::number(numeric_constant.value_double_) ) + numeric_constant.type_suffix_.data();
		}
		ProgramString operator()( const Synt::StringLiteral& string_literal )
		{
			return "\""_SpC + string_literal.value_ + "\""_SpC + string_literal.type_suffix_.data();
		}
		ProgramString operator()( const Synt::BooleanConstant& boolean_constant )
		{
			return boolean_constant.value_ ? Keyword( Keywords::true_ ) : Keyword( Keywords::false_ );
		}
		ProgramString operator()( const Synt::BracketExpression& bracket_expression )
		{
			if( bracket_expression.expression_ == nullptr )
				return ProgramString();
			return "("_SpC + Stringify( *bracket_expression.expression_ ) + ")"_SpC;
		}
		ProgramString operator()( const Synt::TypeNameInExpression& type_name_in_expression )
		{
			return Stringify( type_name_in_expression.type_name );
		}
		ProgramString operator()( const Synt::MoveOperator& move_operator )
		{
			return Keyword( Keywords::move_ ) + "("_SpC + move_operator.var_name_ + ")"_SpC;
		}
		ProgramString operator()( const Synt::CastRef& cast_ref )
		{
			if( cast_ref.type_ == nullptr || cast_ref.expression_ == nullptr )
				return ProgramString();
			return Keyword( Keywords::cast_ref ) + "</"_SpC + Stringify( *cast_ref.type_ ) + "/>("_SpC + Stringify( *cast_ref.expression_ ) + ")"_SpC;
		}
		ProgramString operator()( const Synt::CastRefUnsafe& cast_ref_unsafe )
		{
			if( cast_ref_unsafe.type_ == nullptr || cast_ref_unsafe.expression_ == nullptr )
				return ProgramString();
			return Keyword( Keywords::cast_ref_unsafe ) + "</"_SpC + Stringify( *cast_ref_unsafe.type_ ) + "/>("_SpC + Stringify( *cast_ref_unsafe.expression_ ) + ")"_SpC;
		}
		ProgramString operator()( const Synt::CastImut& cast_imut )
		{
			if( cast_imut.expression_ == nullptr )
				return ProgramString();
			return Keyword( Keywords::cast_imut ) + "("_SpC + Stringify( *cast_imut.expression_ ) + ")"_SpC;
		}
		ProgramString operator()( const Synt::CastMut& cast_mut )
		{
			if( cast_mut.expression_ == nullptr )
				return ProgramString();
			return Keyword( Keywords::cast_mut ) + "("_SpC + Stringify( *cast_mut.expression_ ) + ")"_SpC;
		}
		ProgramString operator()( const Synt::TypeInfo& typeinfo_ )
		{
			if( typeinfo_.type_ == nullptr )
				return ProgramString();
			return Keyword( Keywords::cast_ref ) + "</"_SpC + Stringify( *typeinfo_.type_ )  + "/>"_SpC;
		}
	};

	Visitor visitor;
	ProgramString result= boost::apply_visitor( visitor, expression );

	struct ExpressionWithUnaryOperatorsVisitor final : public boost::static_visitor<const Synt::ExpressionComponentWithUnaryOperators*>
	{
		const Synt::ExpressionComponentWithUnaryOperators* operator()( const Synt::ExpressionComponentWithUnaryOperators& expression_with_unary_operators ) const
		{
			return &expression_with_unary_operators;
		}
		const Synt::ExpressionComponentWithUnaryOperators* operator()( const Synt::BinaryOperator& ) const
		{
			return nullptr;
		}
		const Synt::ExpressionComponentWithUnaryOperators* operator()( const Synt::EmptyVariant& ) const
		{
			return nullptr;
		}
	};

	if( const auto expression_with_unary_operators= boost::apply_visitor( ExpressionWithUnaryOperatorsVisitor(), expression ) )
	{
		struct PostifxVisitor final : public boost::static_visitor<ProgramString>
		{
			ProgramString operator()( const Synt::IndexationOperator& indexation_operator )
			{
				return "["_SpC + Stringify( indexation_operator.index_ ) + "]"_SpC;
			}
			ProgramString operator()( const Synt::MemberAccessOperator& member_access_operator )
			{
				ProgramString result;
				result+= "."_SpC + member_access_operator.member_name_;
				if( member_access_operator.have_template_parameters )
				{
					result+= "</"_SpC;
					for( const Synt::Expression& template_param : member_access_operator.template_parameters )
					{
						result+= Stringify(template_param);

						if( &template_param != &member_access_operator.template_parameters.back() )
							result+= ", "_SpC;
					}
					result+= "/>"_SpC;
				}
				return result;
			}
			ProgramString operator()( const Synt::CallOperator& call_operator )
			{
				ProgramString result= "("_SpC;
				for( const Synt::Expression& arg : call_operator.arguments_ )
				{
					result+= Stringify(arg);

					if( &arg != &call_operator.arguments_.back() )
						result+= ", "_SpC;
				}
				result+= ")"_SpC;
				return result;
			}
		};

		struct PrefixVisitor final : public boost::static_visitor<ProgramString>
		{
			ProgramString operator()( const Synt::UnaryMinus& )
			{
				return OverloadedOperatorToString( OverloadedOperator::Sub );
			}
			ProgramString operator()( const Synt::UnaryPlus& )
			{
				return OverloadedOperatorToString( OverloadedOperator::Add );
			}
			ProgramString operator()( const Synt::LogicalNot& )
			{
				return OverloadedOperatorToString( OverloadedOperator::LogicalNot );
			}
			ProgramString operator()( const Synt::BitwiseNot& )
			{
				return OverloadedOperatorToString( OverloadedOperator::BitwiseNot );
			}
		};

		PostifxVisitor postfix_visitor;
		for( const Synt::UnaryPostfixOperator& postfix_operator : expression_with_unary_operators->postfix_operators_ )
			result+= boost::apply_visitor( postfix_visitor, postfix_operator );

		PrefixVisitor prefix_visitor;
		for( const Synt::UnaryPrefixOperator& prefix_operator : expression_with_unary_operators->prefix_operators_ )
			result= boost::apply_visitor( prefix_visitor, prefix_operator ) + result;
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
			for( const Synt::Expression& template_param : component.template_parameters )
			{
				result+= Stringify(template_param);

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


static ProgramString Stringify( const Synt::FunctionArgument& arg )
{
	ProgramString result;

	result+= Stringify( arg.type_ );
	switch( arg.reference_modifier_ )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"_SpC; break;
	}

	if( !arg.reference_tag_.empty() )
	{
		result+= "'"_SpC;
		result+= arg.reference_tag_;
	}

	switch( arg.mutability_modifier_ )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ ); break;
	}

	result+= " "_SpC;
	result+= arg.name_;

	if( !arg.inner_arg_reference_tags_.empty() )
	{
		result+= "'"_SpC;
		for( const ProgramString& tag : arg.inner_arg_reference_tags_ )
		{
			if( tag.empty() )
				result+= "..."_SpC;
			else
				result+= tag;
			if( &tag != &arg.inner_arg_reference_tags_.back() && !arg.inner_arg_reference_tags_.back().empty() )
				result+= ", "_SpC;
		}
		result+= "'"_SpC;
	}

	return result;
}

static ProgramString StringifyFunctionTypeEnding( const Synt::FunctionType& function_type )
{
	ProgramString result;
	if( function_type.unsafe_ )
		result+= Keyword( Keywords::unsafe_ );

	// return value
	result+= " : "_SpC;
	if( function_type.return_type_ == nullptr )
		result+= Keyword( Keywords::void_ );
	else
		result+= Stringify( *function_type.return_type_ );

	if( !function_type.return_value_inner_reference_tags_.empty() )
	{
		result+= "'"_SpC;
		for( const ProgramString& tag : function_type.return_value_inner_reference_tags_ )
		{
			if( tag.empty() )
				result+= "..."_SpC;
			else
				result+= tag;
			if( &tag != &function_type.return_value_inner_reference_tags_.back() && !function_type.return_value_inner_reference_tags_.back().empty() )
				result+= ", "_SpC;
		}
		result+= "'"_SpC;
	}

	switch( function_type.return_value_reference_modifier_ )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference:
		result+= "&"_SpC;
		break;
	}

	switch( function_type.return_value_mutability_modifier_ )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

static ProgramString Stringify( const Synt::TypeName& type_name )
{
	struct Visitor final : public boost::static_visitor<ProgramString>
	{
		ProgramString operator()( const Synt::EmptyVariant& )
		{
			U_ASSERT(false);
			return ProgramString();
		}

		ProgramString operator()( const Synt::ArrayTypeName& array_type_name )
		{
			ProgramString result;

			result+= "[ "_SpC;
			if( array_type_name.element_type != nullptr )
				result+= Stringify( *array_type_name.element_type );
			result+= ", "_SpC;
			if( array_type_name.size != nullptr )
				result+= Stringify( *array_type_name.size );
			result+= " ]"_SpC;

			return result;
		}

		ProgramString operator()( const Synt::TupleType& tuple_type_name )
		{
			ProgramString result;

			result+= "tup[ "_SpC;
			for( const Synt::TypeName& element_type : tuple_type_name.element_types_ )
			{
				result+= Stringify( element_type );
				if( &element_type != &tuple_type_name.element_types_.back() )
					result+= ", "_SpC;
			}
			result+= " ]"_SpC;

			return result;
		}

		ProgramString operator()( const Synt::TypeofTypeName& typeof_type_name )
		{
			if( typeof_type_name.expression != nullptr )
				return ProgramString();
			return Keyword( Keywords::typeof_ ) + "("_SpC + Stringify( *typeof_type_name.expression ) + ")"_SpC;
		}

		ProgramString operator()( const Synt::FunctionTypePtr& function_type_name )
		{
			if( function_type_name == nullptr )
				return ""_SpC;

			ProgramString result;
			result+= "fn("_SpC;

			for( const Synt::FunctionArgument& arg : function_type_name->arguments_ )
			{
				result+= Stringify(arg);

				if( &arg != &function_type_name->arguments_.back() )
					result+= ", "_SpC;
			}
			result+=") "_SpC;

			result+= StringifyFunctionTypeEnding( *function_type_name );
			return result;
		}

		ProgramString operator()( const Synt::NamedTypeName& named_type_name )
		{
			return Stringify( named_type_name.name );
		}
	};

	Visitor visitor;
	return boost::apply_visitor( visitor, type_name );
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
	for( const Synt::FunctionArgument& arg : function.type_.arguments_ )
	{
		if( arg.name_ == Keywords::this_ )
		{
			switch( arg.mutability_modifier_ )
			{
			case Synt::MutabilityModifier::None: break;
			case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); result+= " "_SpC; break;
			case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); result+= " "_SpC; break;
			case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ ); result+= " "_SpC; break;
			}
			result+= Keyword( Keywords::this_ );
		}
		else
			result+= Stringify( arg );

		if( &arg != &function.type_.arguments_.back() )
			result+= ", "_SpC;
	}

	result+= ") "_SpC;

	result+= StringifyFunctionTypeEnding( function.type_ );
	return result;
}

static ProgramString Stringify( const Synt::TypeTemplateBase& type_template )
{
	ProgramString result;

	if( const auto class_template= dynamic_cast<const Synt::ClassTemplate*>( &type_template ) )
		result+= class_template->class_->name_;
	else if( const auto typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( &type_template) )
		result+= typedef_template->name_;
	else U_ASSERT(false);

	result+= "</"_SpC;
	if( type_template.is_short_form_ )
	{
		for( const Synt::TypeTemplateBase::Arg& arg : type_template.args_ )
		{
			if( arg.name != nullptr )
				result+= Stringify( *arg.name );
			if( &arg != &type_template.args_.back() )
				result+= ", "_SpC;
		}
	}
	else
	{
		for( const Synt::TypeTemplateBase::SignatureArg& arg : type_template.signature_args_ )
		{
			result+= Stringify( arg.name );
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
		if( arg.name != nullptr )
			result+= Stringify( *arg.name );
		if( &arg != &function_template.args_.back() )
			result+= ", "_SpC;
	}
	result+= "/>"_SpC;

	result+= " "_SpC;
	if( function_template.function_ != nullptr )
		result+= Stringify( *function_template.function_ );
	return result;
}

static ProgramString Stringify( const Synt::ClassField& class_field )
{
	ProgramString result= class_field.name;
	result+= ": "_SpC + Stringify( class_field.type ) + " "_SpC;

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
	struct Visitor final : public boost::static_visitor<>
	{
		std::vector<ProgramModel::ProgramTreeNode> result;
		ProgramModel::Visibility current_visibility= ProgramModel::Visibility::Public;

		void operator()( const Synt::ClassField& class_field_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassFiled;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= class_field_.file_pos_;
			result.push_back(element);
		}

		void operator()( const Synt::FunctionPtr& func )
		{
			if( func == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *func ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= func->file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( func_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= func_template.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassVisibilityLabel& visibility_label )
		{
			current_visibility= visibility_label.visibility_;
		}
		void operator()( const Synt::ClassTemplate& class_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( class_template ) );
			element.kind= ProgramModel::ElementKind::ClassTemplate;
			element.visibility= current_visibility;
			element.childs= BuildProgramModel_r( class_template.class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_template.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::TypedefTemplate& typedef_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( typedef_template ) );
			element.kind= ProgramModel::ElementKind::TypedefTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= typedef_template.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::Enum& enum_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( enum_.name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.childs= BuildProgramModel_r( enum_ );
			element.number_in_parent= result.size();
			element.file_pos= enum_.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::StaticAssert&  )
		{
		}
		void operator()( const Synt::Typedef& typedef_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( typedef_.name );
			element.kind= ProgramModel::ElementKind::Typedef;
			element.number_in_parent= result.size();
			element.file_pos= typedef_.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			const ProgramString type_name= Stringify( variables_declaration.type );
			for( const auto& variable : variables_declaration.variables )
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
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( auto_variable_declaration ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.file_pos= auto_variable_declaration.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassPtr& class_ )
		{
			if( class_ == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.childs= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_->file_pos_;
			result.push_back(element);
		}
	};

	Visitor visitor;
	for( const Synt::ClassElement& class_element : elements )
		boost::apply_visitor( visitor, class_element );

	return std::move(visitor.result);
}

static std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	struct Visitor final : public boost::static_visitor<>
	{
		std::vector<ProgramModel::ProgramTreeNode> result;

		void operator()( const Synt::ClassField& class_field_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassFiled;
			element.number_in_parent= result.size();
			element.file_pos= class_field_.file_pos_;
			result.push_back(element);
		}

		void operator()( const Synt::FunctionPtr& func )
		{
			if( func == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( *func ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.number_in_parent= result.size();
			element.file_pos= func->file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( func_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.number_in_parent= result.size();
			element.file_pos= func_template.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassTemplate& class_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( class_template ) );
			element.kind= ProgramModel::ElementKind::ClassTemplate;
			element.childs= BuildProgramModel_r( class_template.class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_template.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::TypedefTemplate& typedef_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( typedef_template ) );
			element.kind= ProgramModel::ElementKind::TypedefTemplate;
			element.number_in_parent= result.size();
			element.file_pos= typedef_template.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::Enum& enum_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( enum_.name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.childs= BuildProgramModel_r( enum_ );
			element.number_in_parent= result.size();
			element.file_pos= enum_.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::StaticAssert&  )
		{
		}
		void operator()( const Synt::Typedef& typedef_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( typedef_.name );
			element.kind= ProgramModel::ElementKind::Typedef;
			element.number_in_parent= result.size();
			element.file_pos= typedef_.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			const ProgramString type_name= Stringify( variables_declaration.type );
			for( const auto& variable : variables_declaration.variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= ProgramStringToQString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.number_in_parent= result.size();
				element.file_pos= variable.file_pos;
				result.push_back(element);
			}
		}
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( Stringify( auto_variable_declaration ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.number_in_parent= result.size();
			element.file_pos= auto_variable_declaration.file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassPtr& class_ )
		{
			if( class_ == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.childs= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= class_->file_pos_;
			result.push_back(element);
		}
		void operator()( const Synt::NamespacePtr& namespace_ )
		{
			if( namespace_ == nullptr )
				return;

			// TODO - what if there are multiple declarations of same namespace in single source file?
			ProgramModel::ProgramTreeNode element;
			element.name= ProgramStringToQString( namespace_->name_ );
			element.kind= ProgramModel::ElementKind::Namespace;
			element.childs= BuildProgramModel_r( namespace_->elements_ );
			element.number_in_parent= result.size();
			element.file_pos= namespace_->file_pos_;
			result.push_back(element);
		}

	};

	Visitor visitor;
	for( const Synt::ProgramElement& program_element : elements )
		boost::apply_visitor( visitor, program_element );

	return std::move(visitor.result);
}

static std::vector<ProgramModel::ProgramTreeNode> BuildProgramModelMacros( const Synt::MacrosPtr& macros )
{
	std::vector<ProgramModel::ProgramTreeNode> result;
	if( macros == nullptr )
		return result;

	for( const auto& constex_macro_map_pair : *macros )
	{
		for( const auto& name_macro_pair : constex_macro_map_pair.second )
		{
			ProgramModel::ProgramTreeNode node;
			node.kind= ProgramModel::ElementKind::Macro;
			node.name= ProgramStringToQString( name_macro_pair.second.name );
			node.file_pos= name_macro_pair.second.file_pos;
			result.push_back(node);
		}
	}

	std::sort(
		result.begin(),
		result.end(),
		[](const ProgramModel::ProgramTreeNode& l, const ProgramModel::ProgramTreeNode& r ) -> bool
		{
			return l.file_pos < r.file_pos;
		});

	return result;
}

static void SetupParents( ProgramModel::ProgramTreeNode& tree )
{
	size_t i= 0u;
	for( ProgramModel::ProgramTreeNode& child : tree.childs )
	{
		child.parent= &tree;
		child.number_in_parent= i;
		++i;
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

ProgramModelPtr BuildProgramModel( const QString& program_text )
{
	const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text.utf16(), size_t(program_text.size()) );
	if( !lex_result.error_messages.empty() )
		return nullptr;

	U::Synt::SyntaxAnalysisResult synt_result= U::Synt::SyntaxAnalysis( lex_result.lexems, std::make_shared<Synt::MacrosByContextMap>() );
	// Do NOT abort on errors, because in process of source code editing may occurs some errors.

	const auto result= std::make_shared<ProgramModel>();

	result->program_elements= BuildProgramModelMacros( synt_result.macros );

	auto program_elements= BuildProgramModel_r( synt_result.program_elements );
	for( auto& element : program_elements )
		result->program_elements.push_back(std::move(element));

	size_t i= 0u;
	for( ProgramModel::ProgramTreeNode& node : result->program_elements )
	{
		node.number_in_parent= i;
		++i;
		SetupParents( node );
	}

	return result;
}

} // namespace QtCreatorPlugin

} // namespace U
