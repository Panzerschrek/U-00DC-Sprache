#include "assert.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static const auto g_name_stub= YetNotDeducedTemplateArg();

void CodeBuilder::PreResolveName( const Synt::ComplexName& name, NamesScope& names )
{
	size_t skip_components= 0u;
	PreResolve( names, name.components.data(), name.components.size(), skip_components );
	U_ASSERT( skip_components <= name.components.size() );

	for( size_t i= skip_components; i < name.components.size(); ++i )
	{
		for( const auto& template_arg : name.components[i].template_parameters )
			PreResolveExpression( *template_arg, names );
	}
}

void CodeBuilder::PreResolveEnum( const Synt::Enum& enum_, NamesScope& names )
{
	ResolveName( enum_.file_pos_, names, enum_.underlaying_type_name );
	names.AddName( enum_.name, g_name_stub );
}

void CodeBuilder::PreResolveTypedef( const Synt::Typedef& typedef_, NamesScope& names )
{
	PreResolveType( *typedef_.value, names );
	names.AddName( typedef_.name, g_name_stub );
}

void CodeBuilder::PreResolveTypeTemplate( const Synt::TypeTemplateBase& type_template, NamesScope& names )
{
	NamesScope parameters_scope( ""_SpC, &names );
	for( const Synt::TemplateBase::Arg& arg : type_template.args_ )
		parameters_scope.AddName( arg.name->components.front().name, g_name_stub );

	if( const auto template_class= dynamic_cast<const Synt::ClassTemplate*>( &type_template ) )
		PreResovleClass( *template_class->class_, parameters_scope, false );
	else if( const auto typedef_template= dynamic_cast<const Synt::TypedefTemplate*>( &type_template ) )
		PreResolveTypedef( *typedef_template->typedef_, parameters_scope );
	else U_ASSERT(false);
}

void CodeBuilder::PreResolveFunctionTemplate( const Synt::FunctionTemplate& function_template, NamesScope& names )
{
	NamesScope parameters_scope( ""_SpC, &names );
	for( const Synt::TemplateBase::Arg& arg : function_template.args_ )
		parameters_scope.AddName( arg.name->components.front().name, g_name_stub );

	PreResolveFunctionPrototype( *function_template.function_, parameters_scope );
	PreResolveFunctionBody( *function_template.function_, parameters_scope );
}

void CodeBuilder::PreResovleClass( const Synt::Class& class_declaration, NamesScope& names, const bool only_prototype )
{
	const ProgramString& class_name= class_declaration.name_.components.back().name;

	const NamesScope::InsertedName* previous_declaration= nullptr;
	if( class_declaration.name_.components.size() == 1u )
	{
		// Simple name - look only in current namespace.
		previous_declaration= names.GetThisScopeName( class_name );
	}
	else
	{
		// Complex name - make full name resolving.
		previous_declaration= ResolveName( class_declaration.file_pos_, names, class_declaration.name_, true );
		if( previous_declaration == nullptr )
			return;
	}

	ClassProxyPtr the_class;
	if( previous_declaration != nullptr )
	{
		if( const Type* const previous_type= previous_declaration->second.GetTypeName() )
			if( const ClassProxyPtr previous_class= previous_type->GetClassTypeProxy() )
				the_class= previous_class;
	}
	else
	{
		the_class= std::make_shared<ClassProxy>( new Class( class_name, &names ) );
		names.AddName( class_declaration.name_.components.back().name, Value( Type(the_class), class_declaration.file_pos_ ) );
	}

	if( the_class == nullptr )
		return;

	if( only_prototype || class_declaration.is_forward_declaration_ )
		return;

	for( const Synt::ComplexName& parent : class_declaration.parents_ )
		PreResolveName( parent, names );

	NamesScope& class_namespace= the_class->class_->members;

	std::vector<const Synt::Class*> inner_classes;
	for( const Synt::IClassElementPtr& member : class_declaration.elements_ )
	{
		if( const auto field= dynamic_cast<const Synt::ClassField*>( member.get() ) )
		{
			PreResolveType( *field->type, names );
			class_namespace.AddName( field->name, g_name_stub );
		}
		else if( dynamic_cast<const Synt::Function*>( member.get() ) != nullptr ) {}
		else if( const auto inner_class= dynamic_cast<const Synt::Class*>( member.get() ) )
		{
			inner_classes.push_back( inner_class );
			PreResovleClass( *inner_class, class_namespace, true );
		}
		else if( const auto variables_declaration= dynamic_cast<const Synt::VariablesDeclaration*>( member.get() ) )
			PreResolveVariablesDeclaration( *variables_declaration, names );
		else if( const auto auto_variable_declaration= dynamic_cast<const Synt::AutoVariableDeclaration*>( member.get() ) )
			PreResolveAutoVariableDeclaration( *auto_variable_declaration, class_namespace );
		else if( const auto static_assert_= dynamic_cast<const Synt::StaticAssert*>( member.get() ) )
			PreResolveStaticAssert( *static_assert_, class_namespace );
		else if( const auto enum_= dynamic_cast<const Synt::Enum*>( member.get() ) )
			PreResolveEnum( *enum_, class_namespace );
		else if( const auto typedef_= dynamic_cast<const Synt::Typedef*>( member.get() ) )
			PreResolveTypedef( *typedef_, class_namespace );
		else if( const auto type_template= dynamic_cast<const Synt::TypeTemplateBase*>( member.get() ) )
			PreResolveTypeTemplate( *type_template, class_namespace );
		else if( dynamic_cast<const Synt::FunctionTemplate*>( member.get() ) != nullptr ) {}
		else if( dynamic_cast<const Synt::ClassVisibilityLabel*>( member.get() ) != nullptr ) {}
		else U_ASSERT(false);
	}

	std::vector<const Synt::Function*> functions;
	std::vector<const Synt::FunctionTemplate*> function_templates;

	for( const Synt::IClassElementPtr& member : class_declaration.elements_ )
	{
		if( const auto function_declaration= dynamic_cast<const Synt::Function*>( member.get() ) )
		{
			functions.push_back( function_declaration );
			PreResolveFunctionPrototype( *function_declaration, class_namespace );
		}
		else if( const auto function_template= dynamic_cast<const Synt::FunctionTemplate*>( member.get() ) )
			function_templates.push_back( function_template );
	}

	for( const Synt::FunctionTemplate* function_template : function_templates )
		PreResolveFunctionTemplate( *function_template, class_namespace );

	for( const Synt::Class* const inner_class : inner_classes )
		if( !inner_class->is_forward_declaration_ )
			PreResovleClass( *inner_class, class_namespace, false );

	for( const Synt::Function* const function : functions )
	{
		if( function->block_ != nullptr )
			PreResolveFunctionBody( *function, class_namespace );
	}
}

void CodeBuilder::PreResolveType( const Synt::ITypeName& type_name, NamesScope& names )
{
	if( const auto array_type_name= dynamic_cast<const Synt::ArrayTypeName*>(&type_name) )
	{
		PreResolveType( *array_type_name->element_type, names );
		PreResolveExpression( *array_type_name->size, names );
	}
	else if( const auto function_type_name= dynamic_cast<const Synt::FunctionType*>(&type_name) )
	{
		if( function_type_name->return_type_ != nullptr )
			PreResolveType( *function_type_name->return_type_, names );
		for( const Synt::FunctionArgumentPtr& arg : function_type_name->arguments_ )
			PreResolveType( *arg->type_, names );
	}
	else if( const auto named_type_name= dynamic_cast<const Synt::NamedTypeName*>(&type_name) )
		PreResolveName( named_type_name->name, names );
	else U_ASSERT(false);
}

void CodeBuilder::PreResolveFunctionPrototype( const Synt::Function& function, NamesScope& names )
{
	names.AddName( function.name_.components.back().name, g_name_stub );

	for( const Synt::FunctionArgumentPtr& arg : function.type_.arguments_ )
	{
		if( arg->type_ != nullptr ) // May be null for "this"
			PreResolveType( *arg->type_, names );
	}

	if( function.type_.return_type_ != nullptr )
		PreResolveType( *function.type_.return_type_, names );
}

void CodeBuilder::PreResolveFunctionBody( const Synt::Function& function, NamesScope& names )
{
	U_ASSERT( function.block_ != nullptr );

	NamesScope parameters_scope( ""_SpC, &names );
	for( const Synt::FunctionArgumentPtr& arg : function.type_.arguments_ )
		parameters_scope.AddName( arg->name_, g_name_stub );

	PreResolveBlock( *function.block_, parameters_scope );
}

void CodeBuilder::PreResolveBlock( const Synt::Block& block, NamesScope& names )
{
	NamesScope block_names( ""_SpC, &names );

	for( const Synt::IBlockElementPtr& block_element : block.elements_ )
	{
		const Synt::IBlockElement* const block_element_ptr= block_element.get();
		if( const auto variables_declaration= dynamic_cast<const Synt::VariablesDeclaration*>( block_element_ptr ) )
			PreResolveVariablesDeclaration( *variables_declaration, block_names );
		else if( const auto auto_variable_declaration= dynamic_cast<const Synt::AutoVariableDeclaration*>( block_element_ptr ) )
			PreResolveAutoVariableDeclaration( *auto_variable_declaration, block_names );
		else if( const auto expression= dynamic_cast<const Synt::SingleExpressionOperator*>( block_element_ptr ) )
			PreResolveExpression( *expression->expression_, block_names );
		else if( const auto assignment_operator= dynamic_cast<const Synt::AssignmentOperator*>( block_element_ptr ) )
		{
			PreResolveExpression( *assignment_operator->l_value_, block_names );
			PreResolveExpression( *assignment_operator->r_value_, block_names );
		}
		else if( const auto additive_assignment_operator= dynamic_cast<const Synt::AdditiveAssignmentOperator*>( block_element_ptr ) )
		{
			PreResolveExpression( *additive_assignment_operator->l_value_, block_names );
			PreResolveExpression( *additive_assignment_operator->r_value_, block_names );
		}
		else if( const auto increment_operator= dynamic_cast<const Synt::IncrementOperator*>( block_element_ptr ) )
			PreResolveExpression( *increment_operator->expression, block_names );
		else if( const auto decrement_operator= dynamic_cast<const Synt::DecrementOperator*>( block_element_ptr ) )
			PreResolveExpression( *decrement_operator->expression, block_names );
		else if( const auto return_operator= dynamic_cast<const Synt::ReturnOperator*>( block_element_ptr ) )
		{
			if( return_operator->expression_ != nullptr )
				PreResolveExpression( *return_operator->expression_, block_names );
		}
		else if( const auto while_operator= dynamic_cast<const Synt::WhileOperator*>( block_element_ptr ) )
			PreResolveBlock( *while_operator->block_, block_names );
		else if( dynamic_cast<const Synt::BreakOperator*>( block_element_ptr ) != nullptr ){}
		else if( dynamic_cast<const Synt::ContinueOperator*>( block_element_ptr ) != nullptr ){}
		else if( const auto if_operator= dynamic_cast<const Synt::IfOperator*>( block_element_ptr ) )
		{
			for( const auto& branch : if_operator->branches_ )
			{
				if( branch.condition != nullptr )
					PreResolveExpression( *branch.condition, block_names );
				PreResolveBlock( *branch.block, block_names );
			}
		}
		else if( const auto static_if_operator= dynamic_cast<const Synt::StaticIfOperator*>( block_element_ptr ) )
		{
			for( const auto& branch : static_if_operator->if_operator_->branches_ )
			{
				if( branch.condition != nullptr )
					PreResolveExpression( *branch.condition, block_names );
				PreResolveBlock( *branch.block, block_names );
			}
		}
		else if( const auto static_assert_= dynamic_cast<const Synt::StaticAssert*>( block_element_ptr ) )
			PreResolveStaticAssert( *static_assert_, block_names );
		else if( dynamic_cast<const Synt::Halt*>( block_element_ptr ) != nullptr ) {}
		else if( const auto halt_if= dynamic_cast<const Synt::HaltIf*>( block_element_ptr ) )
			PreResolveExpression( *halt_if->condition, block_names );
		else if( const auto inner_block= dynamic_cast<const Synt::Block*>( block_element_ptr ) )
			PreResolveBlock( *inner_block, block_names );
		else if( const auto unsafe_block= dynamic_cast<const Synt::UnsafeBlock*>( block_element_ptr ) )
			PreResolveBlock( *unsafe_block->block_, block_names );
		else U_ASSERT(false);
	} // for block elements
}

void CodeBuilder::PreResolveVariablesDeclaration( const Synt::VariablesDeclaration& variables_declaration, NamesScope& names )
{
	PreResolveType( *variables_declaration.type, names );
	for( const auto& var : variables_declaration.variables )
		names.AddName( var.name, g_name_stub );
}

void CodeBuilder::PreResolveAutoVariableDeclaration( const Synt::AutoVariableDeclaration& auto_variable_declaration, NamesScope& names )
{
	PreResolveExpression( *auto_variable_declaration.initializer_expression, names );
	names.AddName( auto_variable_declaration.name, g_name_stub );
}

void CodeBuilder::PreResolveStaticAssert( const Synt::StaticAssert& static_assert_, NamesScope& names )
{
	PreResolveExpression( *static_assert_.expression, names );
}

void CodeBuilder::PreResolveExpression( const Synt::IExpressionComponent& expression, NamesScope& names )
{
	if( const auto binary_operator= dynamic_cast<const Synt::BinaryOperator*>(&expression) )
	{
		PreResolveExpression( *binary_operator->left_ , names );
		PreResolveExpression( *binary_operator->right_, names );
	}
	else if( const auto named_operand= dynamic_cast<const Synt::NamedOperand*>(&expression) )
		PreResolveName( named_operand->name_, names );
	else if( dynamic_cast<const Synt::NumericConstant*>(&expression) != nullptr ) {}
	else if( dynamic_cast<const Synt::BooleanConstant*>(&expression) != nullptr ) {}
	else if( const auto bracket_expression= dynamic_cast<const Synt::BracketExpression*>(&expression) )
		PreResolveExpression( *bracket_expression->expression_, names );
	else if( const auto type_name_in_expression= dynamic_cast<const Synt::TypeNameInExpression*>(&expression) )
		PreResolveType( *type_name_in_expression->type_name, names );
	else if( dynamic_cast<const Synt::MoveOperator*>(&expression) != nullptr ) {}
	else if( const auto cast_ref= dynamic_cast<const Synt::CastRef*>(&expression) )
		PreResolveExpression( *cast_ref->expression_, names );
	else if( const auto cast_ref_unsafe= dynamic_cast<const Synt::CastRefUnsafe*>(&expression) )
		PreResolveExpression( *cast_ref_unsafe->expression_, names );
	else if( const auto cast_imut= dynamic_cast<const Synt::CastImut*>(&expression) )
		PreResolveExpression( *cast_imut->expression_, names );
	else if( const auto cast_mut= dynamic_cast<const Synt::CastMut*>(&expression) )
		PreResolveExpression( *cast_mut->expression_, names );
	else if( const auto typeinfo_= dynamic_cast<const Synt::TypeInfo*>(&expression) )
		PreResolveType( *typeinfo_->type_, names );
	else U_ASSERT(false);

	if( const auto expression_with_unary_operators= dynamic_cast<const Synt::ExpressionComponentWithUnaryOperators*>( &expression ) )
	{
		for( const Synt::IUnaryPostfixOperatorPtr& postfix_operator : expression_with_unary_operators->postfix_operators_ )
		{
			if( const auto indexation_operator= dynamic_cast<const Synt::IndexationOperator*>( postfix_operator.get() ) )
				PreResolveExpression( *indexation_operator->index_, names );
			else if( const auto member_access_operator= dynamic_cast<const Synt::MemberAccessOperator*>( postfix_operator.get() ) )
			{
				for( const auto& arg : member_access_operator->template_parameters )
					PreResolveExpression( *arg, names );
			}
			else if( const auto call_operator= dynamic_cast<const Synt::CallOperator*>( postfix_operator.get() ) )
			{
				for( const auto& arg : call_operator->arguments_ )
					PreResolveExpression( *arg, names );
			}
			else U_ASSERT(false);
		} // for unary postfix operators
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
