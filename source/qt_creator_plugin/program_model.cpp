#include "../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "../compiler0/lex_synt_lib/program_string.hpp"
#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"

#include "program_model.hpp"

namespace U
{

namespace QtCreatorPlugin
{

namespace
{

std::string Stringify( const Synt::ComplexName& complex_name );
std::string Stringify( const Synt::TypeName& type_name );
std::string Stringify( const Synt::Expression& expression );

std::string Stringify( const Synt::EmptyVariant& )
{
	return std::string();
}

std::string Stringify( const Synt::BinaryOperator& binary_operator )
{
	if( binary_operator.left_ == nullptr || binary_operator.right_ == nullptr )
		return std::string();
	return Stringify( *binary_operator.left_ ) + " " + BinaryOperatorToString(binary_operator.operator_type_) + " " + Stringify( *binary_operator.right_ );
}

std::string Stringify( const Synt::TernaryOperator& ternary_operator )
{
	if( ternary_operator.condition == nullptr || ternary_operator.true_branch == nullptr || ternary_operator.false_branch == nullptr )
		return std::string();
	return "select( " + Stringify( *ternary_operator.condition ) + " ? " + Stringify( *ternary_operator.true_branch ) + " : " + Stringify( *ternary_operator.false_branch ) + " )";
}

std::string Stringify( const Synt::NumericConstant& numeric_constant )
{
	return std::to_string(numeric_constant.value_double) + numeric_constant.type_suffix.data();
}

std::string Stringify( const Synt::StringLiteral& string_literal )
{
	return "\"" + string_literal.value_ + "\"" + string_literal.type_suffix_.data();
}

std::string Stringify( const Synt::BooleanConstant& boolean_constant )
{
	return boolean_constant.value_ ? Keyword( Keywords::true_ ) : Keyword( Keywords::false_ );
}

std::string Stringify( const Synt::MoveOperator& move_operator )
{
	return Keyword( Keywords::move_ ) + "(" + move_operator.var_name_ + ")";
}

std::string Stringify( const Synt::TakeOperator& take_operator )
{
	return Keyword( Keywords::take_ ) + "(" + Stringify(*take_operator.expression_) + ")";
}

std::string Stringify( const Synt::CastRef& cast_ref )
{
	if( cast_ref.type_ == nullptr || cast_ref.expression_ == nullptr )
		return std::string();
	return Keyword( Keywords::cast_ref_ ) + "</" + Stringify( *cast_ref.type_ ) + "/>(" + Stringify( *cast_ref.expression_ ) + ")";
}

std::string Stringify( const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	if( cast_ref_unsafe.type_ == nullptr || cast_ref_unsafe.expression_ == nullptr )
		return std::string();
	return Keyword( Keywords::cast_ref_unsafe_ ) + "</" + Stringify( *cast_ref_unsafe.type_ ) + "/>(" + Stringify( *cast_ref_unsafe.expression_ ) + ")";
}

std::string Stringify( const Synt::CastImut& cast_imut )
{
	if( cast_imut.expression_ == nullptr )
		return std::string();
	return Keyword( Keywords::cast_imut_ ) + "(" + Stringify( *cast_imut.expression_ ) + ")";
}

std::string Stringify( const Synt::CastMut& cast_mut )
{
	if( cast_mut.expression_ == nullptr )
		return std::string();
	return Keyword( Keywords::cast_mut_ ) + "(" + Stringify( *cast_mut.expression_ ) + ")";
}

std::string Stringify( const Synt::TypeInfo& typeinfo_ )
{
	if( typeinfo_.type_ == nullptr )
		return std::string();
	return Keyword( Keywords::cast_ref_ ) + "</" + Stringify( *typeinfo_.type_ )  + "/>";
}

std::string Stringify( const Synt::IndexationOperator& indexation_operator )
{
	return "[" + Stringify( *indexation_operator.index_ ) + "]";
}

std::string Stringify( const Synt::MemberAccessOperator& member_access_operator )
{
	std::string result;
	result+= "." + member_access_operator.member_name_;
	if( member_access_operator.template_parameters != std::nullopt )
	{
		result+= "</";
		for( const Synt::Expression& template_param : *member_access_operator.template_parameters )
		{
			result+= Stringify(template_param);

			if( &template_param != &member_access_operator.template_parameters->back() )
				result+= ", ";
		}
		result+= "/>";
	}
	return result;
}

std::string Stringify( const Synt::CallOperator& call_operator )
{
	std::string result= "(";
	for( const Synt::Expression& arg : call_operator.arguments_ )
	{
		result+= Stringify(arg);

		if( &arg != &call_operator.arguments_.back() )
			result+= ", ";
	}
	result+= ")";
	return result;
}

std::string Stringify( const Synt::UnaryMinus& )
{
	return OverloadedOperatorToString( OverloadedOperator::Sub );
}
std::string Stringify( const Synt::UnaryPlus& )
{
	return OverloadedOperatorToString( OverloadedOperator::Add );
}
std::string Stringify( const Synt::LogicalNot& )
{
	return OverloadedOperatorToString( OverloadedOperator::LogicalNot );
}
std::string Stringify( const Synt::BitwiseNot& )
{
	return OverloadedOperatorToString( OverloadedOperator::BitwiseNot );
}

std::string Stringify( const Synt::Expression& expression )
{
	std::string result=
		std::visit(
			[]( const auto& t ){ return Stringify(t); },
			expression );

	struct ExpressionWithUnaryOperatorsVisitor final
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

	if( const auto expression_with_unary_operators= std::visit( ExpressionWithUnaryOperatorsVisitor(), expression ) )
	{
		for( const Synt::UnaryPostfixOperator& postfix_operator : expression_with_unary_operators->postfix_operators_ )
			result+=
				std::visit(
					[]( const auto& t ){ return Stringify(t); },
					postfix_operator );
		
		for( const Synt::UnaryPrefixOperator& prefix_operator : expression_with_unary_operators->prefix_operators_ )
			result=
				std::visit(
					[]( const auto& t ){ return Stringify(t); },
					prefix_operator ) +
				result;
		
	}

	return result;
}

std::string Stringify( const Synt::ComplexName& complex_name )
{
	std::string result;

	if( std::get_if<Synt::EmptyVariant>(&complex_name.start_value) != nullptr )
	{}
	else if( const auto typeof_type_name = std::get_if<Synt::TypeofTypeName>(&complex_name.start_value) )
	{
		if( typeof_type_name->expression != nullptr )
			result= Keyword( Keywords::typeof_ ) + "(" + Stringify( *typeof_type_name->expression ) + ")";
	}
	else if(const auto simple_name= std::get_if<std::string>(&complex_name.start_value) )
		result= *simple_name;

	auto tail= complex_name.tail.get();
	while(tail != nullptr)
	{
		if( const auto name= std::get_if<std::string>( &tail->name_or_template_paramenters ) )
		{
			result+= "::";
			result+= *name;
		}
		else if( const auto template_prameters= std::get_if< std::vector<Synt::Expression> >( &tail->name_or_template_paramenters ) )
		{
			result+= "</ ";
			for( const Synt::Expression& expr : *template_prameters )
			{
				result+= Stringify( expr );
				if( &expr != &template_prameters->back() )
					result+= ", ";
			}
			result+= " />";
		}
		else
			U_ASSERT( false );

		tail= tail->next.get();
	}

	return result;
}

std::string Stringify( const Synt::FunctionArgument& arg )
{
	std::string result;

	result+= Stringify( arg.type_ );
	switch( arg.reference_modifier_ )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
	}

	if( !arg.reference_tag_.empty() )
	{
		result+= "'";
		result+= arg.reference_tag_;
	}

	switch( arg.mutability_modifier_ )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ ); break;
	}

	result+= " ";
	result+= arg.name_;

	if( !arg.inner_arg_reference_tag_.empty() )
	{
		result+= "'";
		result+= arg.inner_arg_reference_tag_;
		result+= "'";
	}

	return result;
}

std::string StringifyFunctionTypeEnding( const Synt::FunctionType& function_type )
{
	std::string result;
	if( function_type.unsafe_ )
		result+= Keyword( Keywords::unsafe_ );

	// return value
	result+= " : ";
	if( function_type.return_type_ == nullptr )
		result+= Keyword( Keywords::void_ );
	else
		result+= Stringify( *function_type.return_type_ );

	if( !function_type.return_value_inner_reference_tag_.empty() )
	{
		result+= "'";
		result+= function_type.return_value_inner_reference_tag_;
		result+= "'";
	}

	switch( function_type.return_value_reference_modifier_ )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference:
		result+= "&";
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

std::string Stringify( const Synt::ArrayTypeName& array_type_name )
{
	std::string result;

	result+= "[ ";
	if( array_type_name.element_type != nullptr )
		result+= Stringify( *array_type_name.element_type );
	result+= ", ";
	if( array_type_name.size != nullptr )
		result+= Stringify( *array_type_name.size );
	result+= " ]";

	return result;
}

std::string Stringify( const Synt::TupleType& tuple_type_name )
{
	std::string result;

	result+= "tup[ ";
	for( const Synt::TypeName& element_type : tuple_type_name.element_types_ )
	{
		result+= Stringify( element_type );
		if( &element_type != &tuple_type_name.element_types_.back() )
			result+= ", ";
	}
	result+= " ]";

	return result;
}

std::string Stringify( const Synt::FunctionTypePtr& function_type_name )
{
	if( function_type_name == nullptr )
		return "";

	std::string result;
	result+= "fn(";

	for( const Synt::FunctionArgument& arg : function_type_name->arguments_ )
	{
		result+= Stringify(arg);

		if( &arg != &function_type_name->arguments_.back() )
			result+= ", ";
	}
	result+=") ";

	result+= StringifyFunctionTypeEnding( *function_type_name );
	return result;
}

std::string Stringify( const Synt::NamedTypeName& named_type_name )
{
	return Stringify( named_type_name.name );
}

std::string Stringify( const Synt::TypeName& type_name )
{
	return 
		std::visit(
			[]( const auto& t ){ return Stringify(t); },
			type_name );
}

std::string Stringify( const Synt::Function& function )
{
	std::string result;
	if( function.overloaded_operator_ != OverloadedOperator::None )
		result+= Keyword( Keywords::op_ ) + OverloadedOperatorToString( function.overloaded_operator_ );
	else
		result+= function.name_.back();

	result+= "(";

	// args
	for( const Synt::FunctionParam& param : function.type_.params_ )
	{
		if( param.name_ == Keywords::this_ )
		{
			switch( param.mutability_modifier_ )
			{
			case Synt::MutabilityModifier::None: break;
			case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); result+= " "; break;
			case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); result+= " "; break;
			case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ ); result+= " "; break;
			}
			result+= Keyword( Keywords::this_ );
		}
		else
			result+= Stringify( param );

		if( &param != &function.type_.arguments_.back() )
			result+= ", ";
	}

	result+= ") ";

	result+= StringifyFunctionTypeEnding( function.type_ );
	return result;
}

std::string Stringify( const Synt::TypeTemplate& type_template )
{
	std::string result= type_template.name_;

	result+= "</";
	if( type_template.is_short_form_ )
	{
		for( const Synt::TypeTemplateBase::Arg& arg : type_template.args_ )
		{
			if( arg.name != nullptr )
				result+= Stringify( *arg.name );
			if( &arg != &type_template.args_.back() )
				result+= ", ";
		}
	}
	else
	{
		for( const Synt::TypeTemplateBase::SignatureArg& arg : type_template.signature_args_ )
		{
			result+= Stringify( arg.name );
			if( &arg != &type_template.signature_args_.back() )
				result+= ", ";
		}
	}
	result+= "/>";

	return result;
}

std::string Stringify( const Synt::FunctionTemplate& function_template )
{
	std::string result;

	result+= Keyword( Keywords::template_ );
	result+= "</";
	for( const Synt::TemplateBase::Param& param : function_template.params_ )
	{
		if( param.name != nullptr )
			result+= Stringify( *param.name );
		if( &param != &function_template.args_.back() )
			result+= ", ";
	}
	result+= "/>";

	result+= " ";
	if( function_template.function_ != nullptr )
		result+= Stringify( *function_template.function_ );
	return result;
}

std::string Stringify( const Synt::ClassField& class_field )
{
	std::string result= class_field.name;
	result+= ": " + Stringify( class_field.type ) + " ";

	switch( class_field.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
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

std::string Stringify( const Synt::VariablesDeclaration::VariableEntry& varaible, const std::string& type )
{
	std::string result= varaible.name;
	result+= ": " + type + " ";

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
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

std::string Stringify( const Synt::AutoVariableDeclaration& varaible )
{
	std::string result= varaible.name;
	result+= ": " + Keyword( Keywords::auto_ ) + " ";

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
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

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::Enum& enum_ )
{
	std::vector<ProgramModel::ProgramTreeNode> result;

	for( const Synt::Enum::Member& member : enum_.members )
	{
		ProgramModel::ProgramTreeNode element;
		element.name= QString::fromStdString( member.name );
		element.kind= ProgramModel::ElementKind::EnumElement;
		element.number_in_parent= result.size();
		element.src_loc= member.src_loc;
		result.push_back(element);
	}

	return result;
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ClassElements& elements )
{
	struct Visitor final
	{
		std::vector<ProgramModel::ProgramTreeNode> result;
		ProgramModel::Visibility current_visibility= ProgramModel::Visibility::Public;

		void operator()( const Synt::ClassField& class_field_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassField;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= class_field_.src_loc_;
			result.push_back(element);
		}

		void operator()( const Synt::FunctionPtr& func )
		{
			if( func == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( *func ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= func->src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( func_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= func_template.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassVisibilityLabel& visibility_label )
		{
			current_visibility= visibility_label.visibility_;
		}
		void operator()( const Synt::TypeTemplate& class_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( class_template ) );
			element.kind= ProgramModel::ElementKind::ClassTemplate;
			element.visibility= current_visibility;
			element.childs= BuildProgramModel_r( class_template.class_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= class_template.src_loc_;
			result.push_back(element);

			/*
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( typedef_template ) );
			element.kind= ProgramModel::ElementKind::TypedefTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= typedef_template.src_loc_;
			result.push_back(element);
			*/
		}
		void operator()( const Synt::Enum& enum_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( enum_.name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.childs= BuildProgramModel_r( enum_ );
			element.number_in_parent= result.size();
			element.src_loc= enum_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::StaticAssert&  )
		{
		}
		void operator()( const Synt::TypeAlias& typedef_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( typedef_.name );
			element.kind= ProgramModel::ElementKind::Typedef;
			element.number_in_parent= result.size();
			element.src_loc= typedef_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			const std::string type_name= Stringify( variables_declaration.type );
			for( const auto& variable : variables_declaration.variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= QString::fromStdString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.visibility= current_visibility;
				element.number_in_parent= result.size();
				element.src_loc= variable.src_loc;
				result.push_back(element);
			}
		}
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( auto_variable_declaration ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= auto_variable_declaration.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassPtr& class_ )
		{
			if( class_ == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.childs= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= class_->src_loc_;
			result.push_back(element);
		}
	};

	Visitor visitor;
	for( const Synt::ClassElement& class_element : elements )
		std::visit( visitor, class_element );

	return std::move(visitor.result);
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	struct Visitor final
	{
		std::vector<ProgramModel::ProgramTreeNode> result;

		void operator()( const Synt::ClassField& class_field_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassField;
			element.number_in_parent= result.size();
			element.src_loc= class_field_.src_loc_;
			result.push_back(element);
		}

		void operator()( const Synt::FunctionPtr& func )
		{
			if( func == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( *func ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.number_in_parent= result.size();
			element.src_loc= func->src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( func_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.number_in_parent= result.size();
			element.src_loc= func_template.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::TypeTemplate& class_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( class_template ) );
			element.kind= ProgramModel::ElementKind::ClassTemplate;
			element.childs= BuildProgramModel_r( class_template.class_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= class_template.src_loc_;
			result.push_back(element);
			/*
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( typedef_template ) );
			element.kind= ProgramModel::ElementKind::TypedefTemplate;
			element.number_in_parent= result.size();
			element.src_loc= typedef_template.src_loc_;
			result.push_back(element);
			*/
		}
		void operator()( const Synt::Enum& enum_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( enum_.name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.childs= BuildProgramModel_r( enum_ );
			element.number_in_parent= result.size();
			element.src_loc= enum_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::StaticAssert&  )
		{
		}
		void operator()( const Synt::TypeAlias& typedef_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( typedef_.name );
			element.kind= ProgramModel::ElementKind::Typedef;
			element.number_in_parent= result.size();
			element.src_loc= typedef_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			const std::string type_name= Stringify( variables_declaration.type );
			for( const auto& variable : variables_declaration.variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= QString::fromStdString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.number_in_parent= result.size();
				element.src_loc= variable.src_loc;
				result.push_back(element);
			}
		}
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( auto_variable_declaration ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.number_in_parent= result.size();
			element.src_loc= auto_variable_declaration.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassPtr& class_ )
		{
			if( class_ == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.childs= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= class_->src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::NamespacePtr& namespace_ )
		{
			if( namespace_ == nullptr )
				return;

			// TODO - what if there are multiple declarations of same namespace in single source file?
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( namespace_->name_ );
			element.kind= ProgramModel::ElementKind::Namespace;
			element.childs= BuildProgramModel_r( namespace_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= namespace_->src_loc_;
			result.push_back(element);
		}

	};

	Visitor visitor;
	for( const Synt::ProgramElement& program_element : elements )
		std::visit( visitor, program_element );

	return std::move(visitor.result);
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModelMacros( const Synt::MacrosPtr& macros )
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
			node.name= QString::fromStdString( name_macro_pair.second.name );
			node.src_loc= name_macro_pair.second.src_loc;
			result.push_back(node);
		}
	}

	std::sort(
		result.begin(),
		result.end(),
		[](const ProgramModel::ProgramTreeNode& l, const ProgramModel::ProgramTreeNode& r ) -> bool
		{
			return l.src_loc < r.src_loc;
		});

	return result;
}

void SetupParents( ProgramModel::ProgramTreeNode& tree )
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

const ProgramModel::ProgramTreeNode* GetNode_r( const std::vector<ProgramModel::ProgramTreeNode>& nodes, const SrcLoc& src_loc )
{
	// TODO - optimize, make O(log(n)), instead of O(n).
	for( size_t i= 0u; i < nodes.size(); ++i )
	{
		const ProgramModel::ProgramTreeNode& node= nodes[i];

		SrcLoc next_src_loc;
		if( i + 1u < nodes.size() )
			next_src_loc= nodes[i+1u].src_loc;
		else
			next_src_loc= SrcLoc( 0u, SrcLoc::c_max_line, SrcLoc::c_max_column );

		if( node.src_loc <= src_loc && src_loc < next_src_loc )
		{
			const ProgramModel::ProgramTreeNode* const child_node= GetNode_r( node.childs, src_loc );
			if( child_node != nullptr )
				return child_node;
			return &node;
		}
	}

	return nullptr;
}

} // namespace

const ProgramModel::ProgramTreeNode* ProgramModel::GetNodeForSrcLoc( const SrcLoc& src_loc ) const
{
	return GetNode_r( program_elements, src_loc );
}

ProgramModelPtr BuildProgramModel( const QString& program_text )
{
	const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text.toStdString() );
	if( !lex_result.errors.empty() )
		return nullptr;

	const Synt::SyntaxAnalysisResult synt_result=
		Synt::SyntaxAnalysis(
			lex_result.lexems,
			Synt::MacrosByContextMap(),
			std::make_shared<Synt::MacroExpansionContexts>(), "" );
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
