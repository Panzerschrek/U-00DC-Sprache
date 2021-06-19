#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MD5.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BlockElement& block_element )
{
	return
		std::visit(
			[&]( const auto& t )
			{
				SetCurrentDebugLocation( t.src_loc_, function_context );
				return BuildBlockElementImpl( names, function_context, t );
			},
			block_element );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::Block& block )
{
	return BuildBlock( names, function_context, block );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::VariablesDeclaration& variables_declaration )
{
	const Type type= PrepareType( variables_declaration.type, names, function_context );

	for( const Synt::VariablesDeclaration::VariableEntry& variable_declaration : variables_declaration.variables )
	{
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference ||
			variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
		{
			// Full completeness required for value-variables and any constexpr variable.
			if( !EnsureTypeComplete( type ) )
			{
				REPORT_ERROR( UsingIncompleteType, names.GetErrors(), variables_declaration.src_loc_, type );
				continue;
			}
		}
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference && type.IsAbstract() )
			REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), variables_declaration.src_loc_, type );

		if( variable_declaration.reference_modifier != ReferenceModifier::Reference && !type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		if( IsKeyword( variable_declaration.name ) )
		{
			REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), variables_declaration.src_loc_ );
			continue;
		}

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !type.CanBeConstexpr() )
		{
			REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names.GetErrors(), variables_declaration.src_loc_ );
			continue;
		}

		// Destruction frame for temporary variables of initializer expression.
		StackVariablesStorage& prev_variables_storage= *function_context.stack_variables_stack.back();
		const StackVariablesStorage temp_variables_storage( function_context );

		Variable variable;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		ReferencesGraphNode::Kind node_kind;
		if( variable_declaration.reference_modifier != ReferenceModifier::Reference )
			node_kind= ReferencesGraphNode::Kind::Variable;
		else if( variable_declaration.mutability_modifier == MutabilityModifier::Mutable )
			node_kind= ReferencesGraphNode::Kind::ReferenceMut;
		else
			node_kind= ReferencesGraphNode::Kind::ReferenceImut;
		const auto var_node= std::make_shared<ReferencesGraphNode>( variable_declaration.name, node_kind );

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType() );
			variable.llvm_value->setName( variable_declaration.name );

			CreateVariableDebugInfo( variable, variable_declaration.name, variable_declaration.src_loc, function_context );

			prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
			variable.node= var_node;

			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value=
					ApplyInitializer( variable, names, function_context, *variable_declaration.initializer );
			else
				ApplyEmptyInitializer( variable_declaration.name, variable_declaration.src_loc, variable, names, function_context );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			if( variable_declaration.mutability_modifier != MutabilityModifier::Mutable )
				variable.value_type= ValueType::ConstReference;
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			// Mark references immutable before initialization.
			if( variable_declaration.mutability_modifier != MutabilityModifier::Mutable )
				variable.value_type= ValueType::ConstReference;

			if( variable_declaration.initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names.GetErrors(), variables_declaration.src_loc_, variable_declaration.name );
				continue;
			}

			const Synt::Expression* initializer_expression= nullptr;
			if( const auto expression_initializer= std::get_if<Synt::Expression>( variable_declaration.initializer.get() ) )
				initializer_expression= expression_initializer;
			else if( const auto constructor_initializer= std::get_if<Synt::ConstructorInitializer>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->arguments.size() != 1u )
				{
					REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, names.GetErrors(), constructor_initializer->src_loc_ );
					continue;
				}
				initializer_expression= &constructor_initializer->arguments.front();
			}
			else
			{
				REPORT_ERROR( UnsupportedInitializerForReference, names.GetErrors(), variable_declaration.src_loc );
				continue;
			}

			const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, names, function_context );
			if( !ReferenceIsConvertible( expression_result.type, variable.type, names.GetErrors(), variables_declaration.src_loc_ ) )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), variables_declaration.src_loc_, variable.type, expression_result.type );
				continue;
			}

			if( expression_result.value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), variables_declaration.src_loc_ );
				continue;
			}
			if( expression_result.value_type == ValueType::ConstReference && variable.value_type == ValueType::Reference )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), variable_declaration.src_loc );
				continue;
			}

			// TODO - maybe make copy of varaible address in new llvm register?
			llvm::Value* result_ref= expression_result.llvm_value;
			if( variable.type != expression_result.type )
				result_ref= CreateReferenceCast( result_ref, expression_result.type, variable.type, function_context );
			variable.llvm_value= result_ref;
			variable.constexpr_value= expression_result.constexpr_value;

			CreateReferenceVariableDebugInfo( variable, variable_declaration.name, variable_declaration.src_loc, function_context );

			prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
			variable.node= var_node;

			const bool is_mutable= variable.value_type == ValueType::Reference;
			if( expression_result.node != nullptr )
			{
				if( is_mutable )
				{
					if( function_context.variables_state.HaveOutgoingLinks( expression_result.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), variable_declaration.src_loc, expression_result.node->name );
				}
				else if( function_context.variables_state.HaveOutgoingMutableNodes( expression_result.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), variable_declaration.src_loc, expression_result.node->name );
				function_context.variables_state.AddLink( expression_result.node, var_node );
			}
		}
		else U_ASSERT(false);

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			variable.constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names.GetErrors(), variable_declaration.src_loc );
			continue;
		}

		// Reset constexpr initial value for mutable variables.
		if( variable.value_type != ValueType::ConstReference )
			variable.constexpr_value= nullptr;

		const Value* const inserted_value=
			names.AddName( variable_declaration.name, Value( variable, variable_declaration.src_loc ) );
		if( inserted_value == nullptr )
		{
			REPORT_ERROR( Redefinition, names.GetErrors(), variables_declaration.src_loc_, variable_declaration.name );
			continue;
		}

		// After lock of references we can call destructors.
		CallDestructors( temp_variables_storage, names, function_context, variable_declaration.src_loc );
	} // for variables

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::AutoVariableDeclaration& auto_variable_declaration )
{
	if( IsKeyword( auto_variable_declaration.name ) )
		REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), auto_variable_declaration.src_loc_ );

	// Destruction frame for temporary variables of initializer expression.
	StackVariablesStorage& prev_variables_storage= *function_context.stack_variables_stack.back();
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable initializer_experrsion= BuildExpressionCodeEnsureVariable( auto_variable_declaration.initializer_expression, names, function_context );

	if( initializer_experrsion.type == invalid_type_ )
	{
		REPORT_ERROR( InvalidTypeForAutoVariable, names.GetErrors(), auto_variable_declaration.src_loc_, initializer_experrsion.type );
		return BlockBuildInfo();
	}

	Variable variable;
	variable.type= initializer_experrsion.type;
	variable.value_type= auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable ? ValueType::Reference : ValueType::ConstReference;
	variable.location= Variable::Location::Pointer;

	ReferencesGraphNode::Kind node_kind;
	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference )
		node_kind= ReferencesGraphNode::Kind::Variable;
	else if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable )
		node_kind= ReferencesGraphNode::Kind::ReferenceMut;
	else
		node_kind= ReferencesGraphNode::Kind::ReferenceImut;
	const auto var_node= std::make_shared<ReferencesGraphNode>( auto_variable_declaration.name, node_kind );

	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference ||
		auto_variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
	{
		// Full completeness required for value-variables and any constexpr variable.
		if( !EnsureTypeComplete( variable.type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), auto_variable_declaration.src_loc_, variable.type );
			return BlockBuildInfo();
		}
	}
	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference && variable.type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), auto_variable_declaration.src_loc_, variable.type );

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !variable.type.CanBeConstexpr() )
	{
		REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names.GetErrors(), auto_variable_declaration.src_loc_ );
		return BlockBuildInfo();
	}

	if( auto_variable_declaration.reference_modifier == ReferenceModifier::Reference )
	{
		if( initializer_experrsion.value_type == ValueType::ConstReference && variable.value_type != ValueType::ConstReference )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), auto_variable_declaration.src_loc_ );
			return BlockBuildInfo();
		}

		variable.llvm_value= initializer_experrsion.llvm_value;
		variable.constexpr_value= initializer_experrsion.constexpr_value;

		if( initializer_experrsion.value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), auto_variable_declaration.src_loc_ );
			return BlockBuildInfo();
		}

		CreateVariableDebugInfo( variable, auto_variable_declaration.name, auto_variable_declaration.src_loc_, function_context );

		prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
		variable.node= var_node;

		const bool is_mutable= variable.value_type == ValueType::Reference;
		if( initializer_experrsion.node != nullptr )
		{
			if( is_mutable )
			{
				if( function_context.variables_state.HaveOutgoingLinks( initializer_experrsion.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), auto_variable_declaration.src_loc_, initializer_experrsion.node->name );
			}
			else if( function_context.variables_state.HaveOutgoingMutableNodes( initializer_experrsion.node ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), auto_variable_declaration.src_loc_, initializer_experrsion.node->name );
			function_context.variables_state.AddLink( initializer_experrsion.node, var_node );
		}
	}
	else if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
	{
		if( !variable.type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType(), nullptr, auto_variable_declaration.name );

		CreateVariableDebugInfo( variable, auto_variable_declaration.name, auto_variable_declaration.src_loc_, function_context );

		prev_variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
		variable.node= var_node;

		SetupReferencesInCopyOrMove( function_context, variable, initializer_experrsion, names.GetErrors(), auto_variable_declaration.src_loc_ );

		if( initializer_experrsion.value_type == ValueType::Value )
		{
			const ReferencesGraphNodePtr& variable_for_move= initializer_experrsion.node;
			if( variable_for_move != nullptr )
			{
				U_ASSERT(variable_for_move->kind == ReferencesGraphNode::Kind::Variable );
				function_context.variables_state.MoveNode( variable_for_move );
			}

			CopyBytes( initializer_experrsion.llvm_value, variable.llvm_value, variable.type, function_context );
		}
		else
		{
			if( !variable.type.IsCopyConstructible() )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), auto_variable_declaration.src_loc_, variable.type );
				return BlockBuildInfo();
			}
			BuildCopyConstructorPart(
				variable.llvm_value, initializer_experrsion.llvm_value,
				variable.type,
				function_context );
		}
		// constexpr preserved for move/copy.
		variable.constexpr_value= initializer_experrsion.constexpr_value;
	}
	else U_ASSERT(false);

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && variable.constexpr_value == nullptr )
	{
		REPORT_ERROR( VariableInitializerIsNotConstantExpression, names.GetErrors(), auto_variable_declaration.src_loc_ );
		return BlockBuildInfo();
	}

	// Reset constexpr initial value for mutable variables.
	if( variable.value_type != ValueType::ConstReference )
		variable.constexpr_value= nullptr;

	const Value* const inserted_value=
		names.AddName( auto_variable_declaration.name, Value( variable, auto_variable_declaration.src_loc_ ) );
	if( inserted_value == nullptr )
		REPORT_ERROR( Redefinition, names.GetErrors(), auto_variable_declaration.src_loc_, auto_variable_declaration.name );

	// After lock of references we can call destructors.
	CallDestructors( temp_variables_storage, names, function_context, auto_variable_declaration.src_loc_ );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ReturnOperator& return_operator )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;

	if( std::get_if<Synt::EmptyVariant>(&return_operator.expression_) != nullptr )
	{
		if( function_context.return_type == std::nullopt )
		{
			if( function_context.function_type.return_value_is_reference )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), return_operator.src_loc_ );
				return block_info;
			}

			if( function_context.deduced_return_type == std::nullopt )
				function_context.deduced_return_type = void_type_;
			else if( *function_context.deduced_return_type != void_type_ )
				REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.deduced_return_type, void_type_ );
			return block_info;
		}

		if( !( function_context.return_type == void_type_ && !function_context.function_type.return_value_is_reference ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, void_type_, *function_context.return_type );
			return block_info;
		}

		CallDestructorsBeforeReturn( names, function_context, return_operator.src_loc_ );
		CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), return_operator.src_loc_ );

		if( function_context.destructor_end_block == nullptr )
			function_context.llvm_ir_builder.CreateRetVoid();
		else
		{
			// In explicit destructor, break to block with destructor calls for class members.
			function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
		}

		return block_info;
	}

	// Destruction frame for temporary variables of result expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	Variable expression_result= BuildExpressionCodeEnsureVariable( return_operator.expression_, names, function_context );
	if( expression_result.type == invalid_type_ )
	{
		// Add "ret void", because we do not need to break llvm basic blocks structure.
		function_context.llvm_ir_builder.CreateRetVoid();
		return block_info;
	}

	// For functions with "auto" on return type use type of first return expression.
	if( function_context.return_type == std::nullopt )
	{
		if( function_context.deduced_return_type == std::nullopt )
			function_context.deduced_return_type = expression_result.type;
		else if( *function_context.deduced_return_type != expression_result.type )
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.deduced_return_type, expression_result.type );
		return block_info;
	}

	if( function_context.function_type.return_value_is_reference )
	{
		if( !ReferenceIsConvertible( expression_result.type, *function_context.return_type, names.GetErrors(), return_operator.src_loc_ ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.return_type, expression_result.type );
			return block_info;
		}

		if( expression_result.value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), return_operator.src_loc_ );
			return block_info;
		}
		if( expression_result.value_type == ValueType::ConstReference && function_context.function_type.return_value_is_mutable )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), return_operator.src_loc_ );
			return block_info;
		}

		// Check correctness of returning reference.
		if( expression_result.node != nullptr )
		{
			for( const ReferencesGraphNodePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( expression_result.node ) )
			{
				if( !IsReferenceAllowedForReturn( function_context, var_node ) )
					REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), return_operator.src_loc_ );
			}
		}

		{ // Lock references to return value variables.
			ReferencesGraphNodeHolder return_value_lock(
				std::make_shared<ReferencesGraphNode>(
					"ret result",
					function_context.function_type.return_value_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut ),
				function_context );
			if( expression_result.node != nullptr )
				function_context.variables_state.AddLink( expression_result.node, return_value_lock.Node() );

			CallDestructorsBeforeReturn( names, function_context, return_operator.src_loc_ );
		} // Reset locks AFTER destructors call. We must get error in case of returning of reference to stack variable or value-argument.

		CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), return_operator.src_loc_ );

		llvm::Value* ret_value= expression_result.llvm_value;
		if( expression_result.type != function_context.return_type )
			ret_value= CreateReferenceCast( ret_value, expression_result.type, *function_context.return_type, function_context );
		function_context.llvm_ir_builder.CreateRet( ret_value );
	}
	else
	{
		if( expression_result.type != function_context.return_type )
		{
			if( const auto conversion_contructor = GetConversionConstructor( expression_result.type, *function_context.return_type, names.GetErrors(), return_operator.src_loc_ ) )
				expression_result= ConvertVariable( expression_result, *function_context.return_type, *conversion_contructor, names, function_context, return_operator.src_loc_ );
			else
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.return_type, expression_result.type );
				return block_info;
			}
		}

		// Check correctness of returning references.
		if( expression_result.type.ReferencesTagsCount() > 0u && expression_result.node != nullptr )
		{
			for( const ReferencesGraphNodePtr& inner_reference : function_context.variables_state.GetAllAccessibleInnerNodes( expression_result.node ) )
			{
				for( const ReferencesGraphNodePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( inner_reference ) )
				{
					if( !IsReferenceAllowedForReturn( function_context, var_node ) )
						REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), return_operator.src_loc_ );
				}
			}
		}

		if( function_context.s_ret_ != nullptr )
		{
			if( expression_result.value_type == ValueType::Value )
			{
				if( expression_result.node != nullptr )
					function_context.variables_state.MoveNode( expression_result.node );
				CopyBytes( expression_result.llvm_value, function_context.s_ret_, *function_context.return_type, function_context );
			}
			else
			{
				if( !expression_result.type.IsCopyConstructible() )
				{
					REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), return_operator.src_loc_, expression_result.type );
					return block_info;
				}

				BuildCopyConstructorPart(
					function_context.s_ret_,
					CreateReferenceCast( expression_result.llvm_value, expression_result.type, *function_context.return_type, function_context ),
					*function_context.return_type,
					function_context );
			}

			CallDestructorsBeforeReturn( names, function_context, return_operator.src_loc_ );
			CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), return_operator.src_loc_ );
			function_context.llvm_ir_builder.CreateRetVoid();
		}
		else
		{
			// Now we can return by value only fundamentals end enums.
			U_ASSERT(
				expression_result.type.GetFundamentalType() != nullptr||
				expression_result.type.GetEnumType() != nullptr ||
				expression_result.type.GetRawPointerType() != nullptr ||
				expression_result.type.GetFunctionPointerType() != nullptr );

			if( expression_result.type == void_type_ )
			{
				CallDestructorsBeforeReturn( names, function_context, return_operator.src_loc_ );
				CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), return_operator.src_loc_ );
				if( function_context.destructor_end_block == nullptr )
					function_context.llvm_ir_builder.CreateRetVoid();
				else
				{
					// In explicit destructor, break to block with destructor calls for class members.
					function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
				}
			}
			else
			{
				// We must read return value before call of destructors.
				llvm::Value* const value_for_return= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );

				CallDestructorsBeforeReturn( names, function_context, return_operator.src_loc_ );
				CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), return_operator.src_loc_ );
				function_context.llvm_ir_builder.CreateRet( value_for_return );
			}
		}
	}

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ForOperator& for_operator )
{
	BlockBuildInfo block_build_info;

	const StackVariablesStorage temp_variables_storage( function_context );
	const Variable sequence_expression= BuildExpressionCodeEnsureVariable( for_operator.sequence_, names, function_context );

	std::optional<ReferencesGraphNodeHolder> sequence_lock;
	if( sequence_expression.node != nullptr )
		sequence_lock.emplace(
			std::make_shared<ReferencesGraphNode>(
				sequence_expression.node->name + " seequence lock",
				sequence_expression.value_type == ValueType::Reference ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut ),
			function_context );

	if( const Tuple* const tuple_type= sequence_expression.type.GetTupleType() )
	{
		llvm::BasicBlock* const finish_basic_block= tuple_type->elements.empty() ? nullptr : llvm::BasicBlock::Create( llvm_context_ );

		std::vector<ReferencesGraph> break_variables_states;

		U_ASSERT( sequence_expression.location == Variable::Location::Pointer );
		for( const Type& element_type : tuple_type->elements )
		{
			const size_t element_index= size_t( &element_type - tuple_type->elements.data() );
			NamesScope loop_names( "", &names );
			const StackVariablesStorage element_pass_variables_storage( function_context );

			Variable variable;
			variable.type= element_type;
			variable.value_type= for_operator.mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::Reference : ValueType::ConstReference;

			ReferencesGraphNode::Kind node_kind;
			if( for_operator.reference_modifier_ != ReferenceModifier::Reference )
				node_kind= ReferencesGraphNode::Kind::Variable;
			else if( for_operator.mutability_modifier_ == MutabilityModifier::Mutable )
				node_kind= ReferencesGraphNode::Kind::ReferenceMut;
			else
				node_kind= ReferencesGraphNode::Kind::ReferenceImut;
			const auto var_node= std::make_shared<ReferencesGraphNode>( for_operator.loop_variable_name_, node_kind );

			if( for_operator.reference_modifier_ == ReferenceModifier::Reference )
			{
				if( for_operator.mutability_modifier_ == MutabilityModifier::Mutable && sequence_expression.value_type != ValueType::Reference )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), for_operator.src_loc_ );
					continue;
				}

				variable.llvm_value= function_context.llvm_ir_builder.CreateGEP( sequence_expression.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( element_index ) } );
				function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( var_node, variable ) );
				variable.node= var_node;

				const bool is_mutable= variable.value_type == ValueType::Reference;
				if( sequence_lock != std::nullopt )
				{
					if( is_mutable )
					{
						if( function_context.variables_state.HaveOutgoingLinks( sequence_expression.node ) )
							REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), for_operator.src_loc_, sequence_expression.node->name );
					}
					else if( function_context.variables_state.HaveOutgoingMutableNodes( sequence_expression.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), for_operator.src_loc_, sequence_expression.node->name );
					function_context.variables_state.AddLink( sequence_lock->Node(), var_node );
				}
			}
			else
			{
				if( !EnsureTypeComplete( element_type ) )
				{
					REPORT_ERROR( UsingIncompleteType, names.GetErrors(), for_operator.src_loc_, element_type );
					continue;
				}
				if( !element_type.IsCopyConstructible() )
				{
					REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), for_operator.src_loc_, element_type );
					continue;
				}

				variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( element_type.GetLLVMType(), nullptr, for_operator.loop_variable_name_ + std::to_string(element_index) );
				function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( var_node, variable ) );
				variable.node= var_node;

				SetupReferencesInCopyOrMove( function_context, variable, sequence_expression, names.GetErrors(), for_operator.src_loc_ );

				BuildCopyConstructorPart(
					variable.llvm_value,
					function_context.llvm_ir_builder.CreateGEP( sequence_expression.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( element_index ) } ),
					element_type,
					function_context );
			}

			loop_names.AddName( for_operator.loop_variable_name_, Value( std::move(variable), for_operator.src_loc_ ) );

			const bool is_last_iteration= element_index + 1u == tuple_type->elements.size();
			llvm::BasicBlock* const next_basic_block=
				is_last_iteration ? finish_basic_block : llvm::BasicBlock::Create( llvm_context_ );
			function_context.loops_stack.emplace_back();
			function_context.loops_stack.back().block_for_continue= next_basic_block;
			function_context.loops_stack.back().block_for_break= finish_basic_block;
			function_context.loops_stack.back().stack_variables_stack_size= function_context.stack_variables_stack.size() - 1u; // Extra 1 for loop variable destruction in 'break' or 'continue'.

			// TODO - create template errors context.
			const BlockBuildInfo inner_block_build_info= BuildBlock( loop_names, function_context, for_operator.block_ );
			if( !inner_block_build_info.have_terminal_instruction_inside )
			{
				CallDestructors( element_pass_variables_storage, names, function_context, for_operator.src_loc_ );
				function_context.llvm_ir_builder.CreateBr( next_basic_block );
				function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
			}

			// Variables state for next iteration is combination of variables states in "continue" branches in previous iteration.
			const bool continue_branches_is_empty= function_context.loops_stack.back().continue_variables_states.empty();
			if( !continue_branches_is_empty )
				function_context.variables_state= MergeVariablesStateAfterIf( function_context.loops_stack.back().continue_variables_states, names.GetErrors(), for_operator.block_.end_src_loc_ );

			for( ReferencesGraph& variables_state : function_context.loops_stack.back().break_variables_states )
				break_variables_states.push_back( std::move(variables_state) );

			// Args preevaluation uses addresses of syntax elements as keys. Reset it, because we use same syntax elements multiple times.
			function_context.args_preevaluation_cache.clear();

			function_context.loops_stack.pop_back();

			if( !continue_branches_is_empty )
			{
				function_context.function->getBasicBlockList().push_back( next_basic_block );
				function_context.llvm_ir_builder.SetInsertPoint( next_basic_block );

				if( is_last_iteration )
					break_variables_states.push_back( function_context.variables_state );
			}
			else
			{
				// Finish building tuple-for if current iteration have no "continue" branches.
				if( !is_last_iteration )
					delete next_basic_block;

				if( !break_variables_states.empty() )
				{
					function_context.function->getBasicBlockList().push_back( finish_basic_block );
					function_context.llvm_ir_builder.SetInsertPoint( finish_basic_block );
				}
				else
					delete finish_basic_block;

				break;
			}
		}

		if( tuple_type->elements.empty() )
		{} // Just keep variables state.
		// Variables state after tuple-for is combination of variables state of all branches with "break" of all iterations.
		else if( !break_variables_states.empty() )
			function_context.variables_state= MergeVariablesStateAfterIf( break_variables_states, names.GetErrors(), for_operator.block_.end_src_loc_ );
		else
			block_build_info.have_terminal_instruction_inside= true;
	}
	else
	{
		// TODO - support array types.
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), for_operator.src_loc_, sequence_expression.type );
		return BlockBuildInfo();
	}

	if( !block_build_info.have_terminal_instruction_inside )
		CallDestructors( temp_variables_storage, names, function_context, for_operator.src_loc_ );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CStyleForOperator& c_style_for_operator )
{
	// TODO - process variables state.

	const StackVariablesStorage loop_variables_storage( function_context );
	NamesScope loop_names_scope("", &names);

	// Variables declaration part.
	if( c_style_for_operator.variable_declaration_part_ != nullptr )
		std::visit(
			[&]( const auto& t )
			{
				SetCurrentDebugLocation( t.src_loc_, function_context );
				BuildBlockElementImpl( loop_names_scope, function_context, t );
			},
			*c_style_for_operator.variable_declaration_part_ );

	const ReferencesGraph variables_state_before_loop= function_context.variables_state;

	llvm::BasicBlock* const test_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const loop_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const loop_iteration_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_loop= llvm::BasicBlock::Create( llvm_context_ );

	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	if( std::get_if<Synt::EmptyVariant>( &c_style_for_operator.loop_condition_ ) != nullptr )
		function_context.llvm_ir_builder.CreateBr( loop_block );
	else
	{
		const StackVariablesStorage temp_variables_storage( function_context );
		const Variable condition_expression= BuildExpressionCodeEnsureVariable( c_style_for_operator.loop_condition_, loop_names_scope, function_context );

		const SrcLoc condition_src_loc= Synt::GetExpressionSrcLoc( c_style_for_operator.loop_condition_ );
		if( condition_expression.type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch,
					names.GetErrors(),
					condition_src_loc,
					bool_type_,
					condition_expression.type );
			function_context.llvm_ir_builder.CreateBr( loop_block );
		}
		else
		{
			llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
			CallDestructors( temp_variables_storage, names, function_context, condition_src_loc );
			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, loop_block, block_after_loop );
		}
	}

	// Loop block code.
	function_context.loops_stack.emplace_back();
	function_context.loops_stack.back().block_for_break= block_after_loop;
	function_context.loops_stack.back().block_for_continue= loop_iteration_block;
	function_context.loops_stack.back().stack_variables_stack_size= function_context.stack_variables_stack.size();

	function_context.function->getBasicBlockList().push_back( loop_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_block );

	const BlockBuildInfo loop_body_block_info= BuildBlock( loop_names_scope, function_context, c_style_for_operator.block_ );
	if( !loop_body_block_info.have_terminal_instruction_inside )
	{
		function_context.llvm_ir_builder.CreateBr( loop_iteration_block );
		function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
	}

	// Variables state before loop iteration block is combination of variables states of each branch terminated with "continue".
	if( !function_context.loops_stack.back().continue_variables_states.empty() )
		function_context.variables_state= MergeVariablesStateAfterIf( function_context.loops_stack.back().continue_variables_states, names.GetErrors(), c_style_for_operator.block_.end_src_loc_ );

	std::vector<ReferencesGraph> variables_state_for_merge= std::move( function_context.loops_stack.back().break_variables_states );
	variables_state_for_merge.push_back( variables_state_before_loop );

	function_context.loops_stack.pop_back();

	// Loop iteration block
	function_context.function->getBasicBlockList().push_back( loop_iteration_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_iteration_block );
	for( const auto& element : c_style_for_operator.iteration_part_elements_ )
	{
		std::visit(
			[&]( const auto& t )
			{
				SetCurrentDebugLocation( t.src_loc_, function_context );
				BuildBlockElementImpl( loop_names_scope, function_context, t );
			},
			element );
	}
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Disallow outer variables state change in loop iteration part and its predecessors.
	const auto errors= ReferencesGraph::CheckWhileBlokVariablesState( variables_state_before_loop, function_context.variables_state, c_style_for_operator.block_.end_src_loc_ );
	names.GetErrors().insert( names.GetErrors().end(), errors.begin(), errors.end() );

	function_context.variables_state= MergeVariablesStateAfterIf( variables_state_for_merge, names.GetErrors(), c_style_for_operator.src_loc_ );

	// Block after loop.
	function_context.function->getBasicBlockList().push_back( block_after_loop );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_loop );

	CallDestructors( loop_variables_storage, loop_names_scope, function_context, c_style_for_operator.src_loc_ );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::WhileOperator& while_operator )
{
	ReferencesGraph variables_state_before_loop= function_context.variables_state;

	llvm::BasicBlock* const test_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const while_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_while= llvm::BasicBlock::Create( llvm_context_ );

	// Break to test block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block code.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	{
		const StackVariablesStorage temp_variables_storage( function_context );
		const Variable condition_expression= BuildExpressionCodeEnsureVariable( while_operator.condition_, names, function_context );

		const SrcLoc condition_src_loc= Synt::GetExpressionSrcLoc( while_operator.condition_ );
		if( condition_expression.type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), condition_src_loc, bool_type_, condition_expression.type );

			// Create instruction even in case of error, because we needs to store basic blocs somewhere.
			function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), while_block, block_after_while );
		}
		else
		{
			llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
			CallDestructors( temp_variables_storage, names, function_context, condition_src_loc );

			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );
		}
	}

	// While block code.

	function_context.loops_stack.emplace_back();
	function_context.loops_stack.back().block_for_break= block_after_while;
	function_context.loops_stack.back().block_for_continue= test_block;
	function_context.loops_stack.back().stack_variables_stack_size= function_context.stack_variables_stack.size();

	function_context.function->getBasicBlockList().push_back( while_block );
	function_context.llvm_ir_builder.SetInsertPoint( while_block );

	const BlockBuildInfo loop_body_block_info= BuildBlock( names, function_context, while_operator.block_ );
	if( !loop_body_block_info.have_terminal_instruction_inside )
	{
		function_context.llvm_ir_builder.CreateBr( test_block );
		function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
	}

	// Block after while code.
	function_context.function->getBasicBlockList().push_back( block_after_while );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_while );

	// Disallow outer variables state change in "continue" branches.
	for( const ReferencesGraph& variables_state : function_context.loops_stack.back().continue_variables_states )
	{
		const auto errors= ReferencesGraph::CheckWhileBlokVariablesState( variables_state_before_loop, variables_state, while_operator.block_.end_src_loc_ );
		names.GetErrors().insert( names.GetErrors().end(), errors.begin(), errors.end() );
	}

	std::vector<ReferencesGraph> variables_state_for_merge= std::move( function_context.loops_stack.back().break_variables_states );
	variables_state_for_merge.push_back( std::move(variables_state_before_loop) );

	function_context.loops_stack.pop_back();

	// Result variables state is combination of variables state before loop and variables state of all branches terminated with "break".
	function_context.variables_state= MergeVariablesStateAfterIf( variables_state_for_merge, names.GetErrors(), while_operator.block_.end_src_loc_ );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BreakOperator& break_operator )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;

	if( function_context.loops_stack.empty() )
	{
		REPORT_ERROR( BreakOutsideLoop, names.GetErrors(), break_operator.src_loc_ );
		return block_info;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_break != nullptr );

	CallDestructorsForLoopInnerVariables( names, function_context, break_operator.src_loc_ );
	function_context.loops_stack.back().break_variables_states.push_back( function_context.variables_state );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_break );

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
		const Synt::ContinueOperator& continue_operator )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;

	if( function_context.loops_stack.empty() )
	{
		REPORT_ERROR( ContinueOutsideLoop, names.GetErrors(), continue_operator.src_loc_ );
		return block_info;
	}
	U_ASSERT( function_context.loops_stack.back().block_for_continue != nullptr );

	CallDestructorsForLoopInnerVariables( names, function_context, continue_operator.src_loc_ );
	function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
	function_context.llvm_ir_builder.CreateBr( function_context.loops_stack.back().block_for_continue );

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::WithOperator& with_operator )
{
	StackVariablesStorage variables_storage( function_context );

	const Variable expr= BuildExpressionCodeEnsureVariable( with_operator.expression_, names, function_context );

	Variable variable;
	variable.type= expr.type;
	variable.value_type= with_operator.mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::Reference : ValueType::ConstReference;
	variable.location= Variable::Location::Pointer;

	ReferencesGraphNode::Kind node_kind;
	if( with_operator.reference_modifier_ != ReferenceModifier::Reference )
		node_kind= ReferencesGraphNode::Kind::Variable;
	else if( with_operator.mutability_modifier_ == MutabilityModifier::Mutable )
		node_kind= ReferencesGraphNode::Kind::ReferenceMut;
	else
		node_kind= ReferencesGraphNode::Kind::ReferenceImut;
	const auto var_node= std::make_shared<ReferencesGraphNode>( with_operator.variable_name_, node_kind );

	if( with_operator.reference_modifier_ != ReferenceModifier::Reference &&
		!EnsureTypeComplete( variable.type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), with_operator.src_loc_, variable.type );
		return BlockBuildInfo();
	}
	if( with_operator.reference_modifier_ != ReferenceModifier::Reference && variable.type.IsAbstract() )
	{
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), with_operator.src_loc_, variable.type );
		return BlockBuildInfo();
	}

	if( with_operator.reference_modifier_ == ReferenceModifier::Reference )
	{
		if( expr.value_type == ValueType::ConstReference && variable.value_type != ValueType::ConstReference )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), with_operator.src_loc_ );
			return BlockBuildInfo();
		}

		if( expr.location == Variable::Location::LLVMRegister )
		{
			// Binding value to reference.
			llvm::Value* const storage= function_context.alloca_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
			if( expr.type != void_type_ )
				function_context.llvm_ir_builder.CreateStore( expr.llvm_value, storage );
			variable.llvm_value= storage;
		}
		else
			variable.llvm_value= expr.llvm_value;

		variable.constexpr_value= expr.constexpr_value;

		CreateVariableDebugInfo( variable, with_operator.variable_name_, with_operator.src_loc_, function_context );

		variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
		variable.node= var_node;

		const bool is_mutable= variable.value_type == ValueType::Reference;
		if( expr.node != nullptr )
		{
			if( is_mutable )
			{
				if( function_context.variables_state.HaveOutgoingLinks( expr.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), with_operator.src_loc_, expr.node->name );
			}
			else if( function_context.variables_state.HaveOutgoingMutableNodes( expr.node ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), with_operator.src_loc_, expr.node->name );
			function_context.variables_state.AddLink( expr.node, var_node );
		}
	}
	else if( with_operator.reference_modifier_ == ReferenceModifier::None )
	{
		if( !variable.type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable.type.GetLLVMType(), nullptr, with_operator.variable_name_ );

		CreateVariableDebugInfo( variable, with_operator.variable_name_, with_operator.src_loc_, function_context );

		variables_storage.RegisterVariable( std::make_pair( var_node, variable ) );
		variable.node= var_node;

		SetupReferencesInCopyOrMove( function_context, variable, expr, names.GetErrors(), with_operator.src_loc_ );

		if( expr.value_type == ValueType::Value )
		{
			const ReferencesGraphNodePtr& variable_for_move= expr.node;
			if( variable_for_move != nullptr )
			{
				U_ASSERT(variable_for_move->kind == ReferencesGraphNode::Kind::Variable );
				function_context.variables_state.MoveNode( variable_for_move );
			}

			CopyBytes( expr.llvm_value, variable.llvm_value, variable.type, function_context );
		}
		else
		{
			if( !variable.type.IsCopyConstructible() )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), with_operator.src_loc_, variable.type );
				return BlockBuildInfo();
			}
			BuildCopyConstructorPart(
				variable.llvm_value, expr.llvm_value,
				variable.type,
				function_context );
		}
		// constexpr preserved for move/copy.
		variable.constexpr_value= expr.constexpr_value;
	}
	else U_ASSERT(false);

	// Reset constexpr initial value for mutable variables.
	if( variable.value_type != ValueType::ConstReference )
		variable.constexpr_value= nullptr;

	if( IsKeyword( with_operator.variable_name_ ) )
		REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), with_operator.src_loc_ );

	{ // Destroy unused temporaries after variable initialization.
		const ReferencesGraphNodeHolder variable_lock(
			std::make_shared<ReferencesGraphNode>( "lock " + var_node->name, ReferencesGraphNode::Kind::ReferenceImut ),
			function_context );
		function_context.variables_state.AddLink( var_node, variable_lock.Node() );
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), with_operator.src_loc_ );
	}

	// Create separate namespace for variable. Redefinition here is not possible.
	NamesScope variable_names_scope( "", &names );
	variable_names_scope.AddName( with_operator.variable_name_, Value( variable, with_operator.src_loc_ ) );

	// Build block. This creates new variables frame and prevents destruction of initializer expression and/or created variable.
	const BlockBuildInfo block_build_info= BuildBlock( variable_names_scope, function_context, with_operator.block_ );

	if( !block_build_info.have_terminal_instruction_inside )
	{
		// Destroy all temporaries.
		CallDestructors( variables_storage, variable_names_scope, function_context, with_operator.src_loc_ );
	}

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
		const Synt::IfOperator& if_operator )
{
	U_ASSERT( !if_operator.branches_.empty() );

	BlockBuildInfo if_operator_blocks_build_info;
	if_operator_blocks_build_info.have_terminal_instruction_inside= true;

	// TODO - optimize this method. Make less basic blocks.
	//

	llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	ReferencesGraph variables_state_before_if= function_context.variables_state;
	std::vector<ReferencesGraph> bracnhes_variables_state;

	for( unsigned int i= 0u; i < if_operator.branches_.size(); i++ )
	{
		const Synt::IfOperator::Branch& branch= if_operator.branches_[i];

		llvm::BasicBlock* const body_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* const current_condition_block= next_condition_block;

		if( i + 1u < if_operator.branches_.size() )
			next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
		else
			next_condition_block= block_after_if;

		// Build condition block.
		function_context.function->getBasicBlockList().push_back( current_condition_block );
		function_context.llvm_ir_builder.SetInsertPoint( current_condition_block );

		if( std::get_if<Synt::EmptyVariant>(&branch.condition) != nullptr )
		{
			U_ASSERT( i + 1u == if_operator.branches_.size() );

			// Make empty condition block - move to it unconditional break to body.
			function_context.llvm_ir_builder.CreateBr( body_block );
		}
		else
		{
			const StackVariablesStorage temp_variables_storage( function_context );
			const Variable condition_expression= BuildExpressionCodeEnsureVariable( branch.condition, names, function_context );
			if( condition_expression.type != bool_type_ )
			{
				REPORT_ERROR( TypesMismatch,
					names.GetErrors(),
					Synt::GetExpressionSrcLoc( branch.condition ),
					bool_type_,
					condition_expression.type );

				// Create instruction even in case of error, because we needs to store basic blocs somewhere.
				function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), body_block, next_condition_block );
			}
			else
			{
				llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
				CallDestructors( temp_variables_storage, names, function_context, Synt::GetExpressionSrcLoc( branch.condition ) );

				function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
			}
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );

		ReferencesGraph variables_state_before_this_branch= function_context.variables_state;

		const BlockBuildInfo block_build_info= BuildBlock( names, function_context, branch.block );

		if( !block_build_info.have_terminal_instruction_inside )
		{
			// Create break instruction, only if block does not contains terminal instructions.
			if_operator_blocks_build_info.have_terminal_instruction_inside= false;
			function_context.llvm_ir_builder.CreateBr( block_after_if );
			bracnhes_variables_state.push_back( function_context.variables_state );
		}

		function_context.variables_state= variables_state_before_this_branch;
	}

	U_ASSERT( next_condition_block == block_after_if );

	if( std::get_if<Synt::EmptyVariant>( &if_operator.branches_.back().condition ) == nullptr ) // Have no unconditional "else" at end.
	{
		bracnhes_variables_state.push_back( function_context.variables_state );
		if_operator_blocks_build_info.have_terminal_instruction_inside= false;
	}

	if( !bracnhes_variables_state.empty() )
		function_context.variables_state= MergeVariablesStateAfterIf( bracnhes_variables_state, names.GetErrors(), if_operator.end_src_loc_ );
	else
		function_context.variables_state= std::move(variables_state_before_if);

	// Block after if code.
	if( if_operator_blocks_build_info.have_terminal_instruction_inside )
		delete block_after_if;
	else
	{
		function_context.function->getBasicBlockList().push_back( block_after_if );
		function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
	}

	return if_operator_blocks_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::StaticIfOperator& static_if_operator )
{
	const auto& branches= static_if_operator.if_operator_.branches_;
	for( unsigned int i= 0u; i < branches.size(); i++ )
	{
		const auto& branch= branches[i];
		if( std::get_if<Synt::EmptyVariant>(&branch.condition) == nullptr )
		{
			const Synt::Expression& condition= branch.condition;
			const SrcLoc condition_src_loc= Synt::GetExpressionSrcLoc( condition );

			const StackVariablesStorage temp_variables_storage( function_context );

			const Variable condition_expression= BuildExpressionCodeEnsureVariable( condition, names, function_context );
			if( condition_expression.type != bool_type_ )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), condition_src_loc, bool_type_, condition_expression.type );
				continue;
			}
			if( condition_expression.constexpr_value == nullptr )
			{
				REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), condition_src_loc );
				continue;
			}

			if( condition_expression.constexpr_value->getUniqueInteger().getLimitedValue() != 0u )
				return BuildBlock( names, function_context, branch.block ); // Ok, this static if produdes block.

			CallDestructors( temp_variables_storage, names, function_context, condition_src_loc );
		}
		else
		{
			U_ASSERT( i == branches.size() - 1u );
			return BuildBlock( names, function_context, branch.block );
		}
	}

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::SingleExpressionOperator& single_expression_operator )
{
	BuildExpressionCodeAndDestroyTemporaries( single_expression_operator.expression_, names, function_context );
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::AssignmentOperator& assignment_operator	)
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	if(
		TryCallOverloadedBinaryOperator(
			OverloadedOperator::Assign,
			assignment_operator.l_value_,
			assignment_operator.r_value_,
			true, // evaluate args in reverse order
			assignment_operator.src_loc_,
			names,
			function_context ) == std::nullopt )
	{ // Here process default assignment operator for fundamental types.
		// Evaluate right part
		Variable r_var= BuildExpressionCodeEnsureVariable( assignment_operator.r_value_, names, function_context );

		if( r_var.type.GetFundamentalType() != nullptr ||
			r_var.type.GetEnumType() != nullptr ||
			r_var.type.GetRawPointerType() != nullptr ||
			r_var.type.GetFunctionPointerType() != nullptr )
		{
			// We must read value, because referenced by reference value may be changed in l_var evaluation.
			if( r_var.location != Variable::Location::LLVMRegister )
			{
				r_var.llvm_value= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				r_var.location= Variable::Location::LLVMRegister;
			}
			r_var.value_type= ValueType::Value;
		}
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), assignment_operator.src_loc_ ); // Destroy temporaries of right expression.

		// Evaluate left part.
		const Variable l_var= BuildExpressionCodeEnsureVariable( assignment_operator.l_value_, names, function_context );

		if( l_var.type == invalid_type_ || r_var.type == invalid_type_ )
			return BlockBuildInfo();

		if( l_var.value_type != ValueType::Reference )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), assignment_operator.src_loc_ );
			return BlockBuildInfo();
		}
		if( l_var.type != r_var.type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), assignment_operator.src_loc_, l_var.type, r_var.type );
			return BlockBuildInfo();
		}

		// Check references of destination.
		if( l_var.node != nullptr && function_context.variables_state.HaveOutgoingLinks( l_var.node ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), assignment_operator.src_loc_, l_var.node->name );

		if( l_var.type.GetFundamentalType() != nullptr ||
			l_var.type.GetEnumType() != nullptr ||
			l_var.type.GetRawPointerType() != nullptr ||
			l_var.type.GetFunctionPointerType() != nullptr )
		{
			if( l_var.location != Variable::Location::Pointer )
			{
				U_ASSERT(false);
				return BlockBuildInfo();
			}
			U_ASSERT( r_var.location == Variable::Location::LLVMRegister );
			if( r_var.type != void_type_ )
				function_context.llvm_ir_builder.CreateStore( r_var.llvm_value, l_var.llvm_value );
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), assignment_operator.src_loc_, l_var.type );
			return BlockBuildInfo();
		}
	}
	// Destruct temporary variables of right and left expressions.
	CallDestructors( temp_variables_storage, names, function_context, assignment_operator.src_loc_ );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::AdditiveAssignmentOperator& additive_assignment_operator )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	if( // TODO - create temp variables frame here.
		TryCallOverloadedBinaryOperator(
			GetOverloadedOperatorForAdditiveAssignmentOperator( additive_assignment_operator.additive_operation_ ),
			additive_assignment_operator.l_value_,
			additive_assignment_operator.r_value_,
			true, // evaluate args in reverse order
			additive_assignment_operator.src_loc_,
			names,
			function_context ) == std::nullopt )
	{ // Here process default additive assignment operators for fundamental types or raw pointers.
		Variable r_var=
			BuildExpressionCodeEnsureVariable(
				additive_assignment_operator.r_value_,
				names,
				function_context );

		if( r_var.type.GetFundamentalType() != nullptr || r_var.type.GetRawPointerType() != nullptr )
		{
			// We must read value, because referenced by reference value may be changed in l_var evaluation.
			if( r_var.location != Variable::Location::LLVMRegister )
			{
				r_var.llvm_value= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				r_var.location= Variable::Location::LLVMRegister;
			}
			r_var.value_type= ValueType::Value;
		}
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), additive_assignment_operator.src_loc_ ); // Destroy temporaries of right expression.

		const Variable l_var=
			BuildExpressionCodeEnsureVariable(
				additive_assignment_operator.l_value_,
				names,
				function_context );

		if( l_var.type == invalid_type_ || r_var.type == invalid_type_ )
			return BlockBuildInfo();

		if( l_var.value_type != ValueType::Reference )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), additive_assignment_operator.src_loc_ );
			return BlockBuildInfo();
		}

		// Check references of destination.
		if( l_var.node != nullptr && function_context.variables_state.HaveOutgoingLinks( l_var.node ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), additive_assignment_operator.src_loc_, l_var.node->name );

		// Allow additive assignment operators only for fundamentals and raw pointers.
		if( !( l_var.type.GetFundamentalType() != nullptr || l_var.type.GetRawPointerType() != nullptr ) )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), additive_assignment_operator.src_loc_, l_var.type );
			return BlockBuildInfo();
		}

		// Generate binary operator and assignment for fundamental types.
		const Value operation_result_value=
			BuildBinaryOperator(
				l_var, r_var,
				additive_assignment_operator.additive_operation_,
				additive_assignment_operator.src_loc_,
				names,
				function_context );
		if( operation_result_value.GetVariable() == nullptr ) // Not variable in case of error.
			return BlockBuildInfo();

		const Variable& operation_result= *operation_result_value.GetVariable();

		if( operation_result.type != l_var.type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), additive_assignment_operator.src_loc_, l_var.type, operation_result.type );
			return BlockBuildInfo();
		}

		U_ASSERT( l_var.location == Variable::Location::Pointer );
		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( operation_result, function_context );
		function_context.llvm_ir_builder.CreateStore( value_in_register, l_var.llvm_value );
	}
	// Destruct temporary variables of right and left expressions.
	CallDestructors( temp_variables_storage, names, function_context, additive_assignment_operator.src_loc_ );

	return  BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::IncrementOperator& increment_operator )
{
	BuildDeltaOneOperatorCode(
		increment_operator.expression,
		increment_operator.src_loc_,
		true,
		names,
		function_context );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::DecrementOperator& decrement_operator )
{
	BuildDeltaOneOperatorCode(
		decrement_operator.expression,
		decrement_operator.src_loc_,
		false,
		names,
		function_context );

	return  BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::StaticAssert& static_assert_ )
{
	BlockBuildInfo block_info;

	// Destruction frame for temporary variables of static assert expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Variable variable= BuildExpressionCodeEnsureVariable( static_assert_.expression, names, function_context );

	// Destruct temporary variables of right and left expressions.
	// In non-error case, this call produces no code.
	CallDestructors( temp_variables_storage, names, function_context, static_assert_.src_loc_ );

	if( variable.type != bool_type_ )
	{
		REPORT_ERROR( StaticAssertExpressionMustHaveBoolType, names.GetErrors(), static_assert_.src_loc_ );
		return block_info;
	}
	if( variable.constexpr_value == nullptr )
	{
		REPORT_ERROR( StaticAssertExpressionIsNotConstant, names.GetErrors(), static_assert_.src_loc_ );
		return block_info;
	}

	if( !variable.constexpr_value->isOneValue() )
	{
		REPORT_ERROR( StaticAssertionFailed, names.GetErrors(), static_assert_.src_loc_ );
	}

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope&,
	FunctionContext& function_context,
	const Synt::Halt& )
{
	function_context.llvm_ir_builder.CreateCall( halt_func_ );

	// We needs terminal , because call to "halt" is not terminal instruction.
	function_context.llvm_ir_builder.CreateUnreachable();

	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;
	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::HaltIf& halt_if	)
{
	BlockBuildInfo block_info;

	llvm::BasicBlock* const true_block = llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const false_block= llvm::BasicBlock::Create( llvm_context_ );

	const StackVariablesStorage temp_variables_storage( function_context );
	const Variable condition_expression= BuildExpressionCodeEnsureVariable( halt_if.condition, names, function_context );
	const SrcLoc condition_expression_src_loc= Synt::GetExpressionSrcLoc( halt_if.condition );
	if( condition_expression.type!= bool_type_ )
	{
		REPORT_ERROR( TypesMismatch,
			names.GetErrors(),
			condition_expression_src_loc,
			bool_type_,
			condition_expression.type );
		return block_info;
	}

	llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
	CallDestructors( temp_variables_storage, names, function_context, condition_expression_src_loc );

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, true_block, false_block );

	// True branch
	function_context.function->getBasicBlockList().push_back( true_block );
	function_context.llvm_ir_builder.SetInsertPoint( true_block );

	function_context.llvm_ir_builder.CreateCall( halt_func_ );
	function_context.llvm_ir_builder.CreateUnreachable();

	// False branch
	function_context.function->getBasicBlockList().push_back( false_block );
	function_context.llvm_ir_builder.SetInsertPoint( false_block );

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlock(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::Block& block )
{
	DebugInfoStartBlock( block.src_loc_, function_context );

	NamesScope block_names( "", &names );
	const StackVariablesStorage block_variables_storage( function_context );

	// Save unsafe flag.
	const bool prev_unsafe= function_context.is_in_unsafe_block;
	if( block.safety_ == Synt::Block::Safety::Unsafe )
	{
		function_context.have_non_constexpr_operations_inside= true; // Unsafe operations can not be used in constexpr functions.
		function_context.is_in_unsafe_block= true;
	}
	else if( block.safety_ == Synt::Block::Safety::Safe )
		function_context.is_in_unsafe_block= false;
	else if( block.safety_ == Synt::Block::Safety::None ) {}
	else U_ASSERT(false);

	BlockBuildInfo block_build_info;
	size_t block_element_index= 0u;
	for( const Synt::BlockElement& block_element : block.elements_ )
	{
		++block_element_index;

		const BlockBuildInfo info= BuildBlockElement( block_names, function_context, block_element );
		if( info.have_terminal_instruction_inside )
		{
			block_build_info.have_terminal_instruction_inside= true;
			break;
		}
	}

	if( block_element_index < block.elements_.size() )
		REPORT_ERROR( UnreachableCode, names.GetErrors(), Synt::GetBlockElementSrcLoc( block.elements_[ block_element_index ] ) );

	SetCurrentDebugLocation( block.end_src_loc_, function_context );

	// If there are undconditional "break", "continue", "return" operators,
	// we didn`t need call destructors, it must be called in this operators.
	if( !block_build_info.have_terminal_instruction_inside )
		CallDestructors( block_variables_storage, block_names, function_context, block.end_src_loc_ );

	// Restire unsafe flag.
	function_context.is_in_unsafe_block= prev_unsafe;

	DebugInfoEndBlock( function_context );

	return block_build_info;
}

void CodeBuilder::BuildDeltaOneOperatorCode(
	const Synt::Expression& expression,
	const SrcLoc& src_loc,
	bool positive, // true - increment, false - decrement
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expressions.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Value value= BuildExpressionCode( expression, block_names, function_context );
	const Variable* const variable= value.GetVariable();
	if( variable == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, block_names.GetErrors(), src_loc, value.GetKindName() );
		return;
	}

	if( variable->value_type != ValueType::Reference )
	{
		REPORT_ERROR( ExpectedReferenceValue, block_names.GetErrors(), src_loc );
		return;
	}

	if( variable->node != nullptr && function_context.variables_state.HaveOutgoingLinks( variable->node ) )
		REPORT_ERROR( ReferenceProtectionError, block_names.GetErrors(), src_loc, variable->node->name );

	ArgsVector<Function::Arg> args;
	args.emplace_back();
	args.back().type= variable->type;
	args.back().is_mutable= variable->value_type == ValueType::Reference;
	args.back().is_reference= variable->value_type != ValueType::Value;
	const FunctionVariable* const overloaded_operator=
		GetOverloadedOperator( args, positive ? OverloadedOperator::Increment : OverloadedOperator::Decrement, block_names, src_loc );
	if( overloaded_operator != nullptr )
	{
		if( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr )
			function_context.have_non_constexpr_operations_inside= true;

		const auto fetch_result= TryFetchVirtualFunction( *variable, *overloaded_operator, function_context, block_names.GetErrors(), src_loc );
		DoCallFunction( fetch_result.second, *overloaded_operator->type.GetFunctionType(), src_loc, &fetch_result.first, {}, false, block_names, function_context );
	}
	else if( const FundamentalType* const fundamental_type= variable->type.GetFundamentalType() )
	{
		if( !IsInteger( fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, block_names.GetErrors(), src_loc, variable->type );
			return;
		}

		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( *variable, function_context );
		llvm::Value* const one=
			llvm::Constant::getIntegerValue(
				fundamental_type->llvm_type,
				llvm::APInt( fundamental_type->llvm_type->getIntegerBitWidth(), uint64_t(1u) ) );

		llvm::Value* const new_value=
			positive
				? function_context.llvm_ir_builder.CreateAdd( value_in_register, one )
				: function_context.llvm_ir_builder.CreateSub( value_in_register, one );

		U_ASSERT( variable->location == Variable::Location::Pointer );
		function_context.llvm_ir_builder.CreateStore( new_value, variable->llvm_value );
	}
	else if( const auto raw_poiter_type= variable->type.GetRawPointerType() )
	{
		if( !EnsureTypeComplete( raw_poiter_type->type ) )
		{
			REPORT_ERROR( UsingIncompleteType, block_names.GetErrors(), src_loc, raw_poiter_type->type );
			return;
		}

		llvm::Value* const ptr_value= CreateMoveToLLVMRegisterInstruction( *variable, function_context );
		llvm::Value* const one= llvm::ConstantInt::get( fundamental_llvm_types_.int_ptr, positive ? uint64_t(1u) : ~uint64_t(0), true );
		llvm::Value* const new_value= function_context.llvm_ir_builder.CreateGEP( ptr_value, one );

		U_ASSERT( variable->location == Variable::Location::Pointer );
		function_context.llvm_ir_builder.CreateStore( new_value, variable->llvm_value );
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, block_names.GetErrors(), src_loc, variable->type );
		return;
	}

	CallDestructors( temp_variables_storage, block_names, function_context, src_loc );
}

} // namespace CodeBuilderPrivate

} // namespace U
