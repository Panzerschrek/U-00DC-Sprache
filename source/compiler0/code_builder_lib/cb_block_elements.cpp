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

namespace
{

// Return true, if root of expression is useless in single expression block element.
// "useless" meant that expression root is useless, not whole expression.
bool SingleExpressionIsUseless( const Synt::Expression& expression )
{
	struct Visitor
	{
		bool operator()( const Synt::EmptyVariant& ) { return false; }
		// Calls generally are not useless. Useless may be constexpr calls.
		// But sometimes constexpr/non-constepxr call result may depend on template context.
		// So, in order to avoid generating too many errors, assume, that all calls are not useless.
		bool operator()( const Synt::CallOperator& ) { return false; }
		// It is useless to call such operators, even if they are overloaded, because logically these operators are created to produce some value.
		bool operator()( const Synt::IndexationOperator& ) { return true; }
		bool operator()( const Synt::MemberAccessOperator& ) { return true; }
		bool operator()( const Synt::UnaryPlus& ) { return true; }
		bool operator()( const Synt::UnaryMinus& ) { return true; }
		bool operator()( const Synt::LogicalNot& ) { return true; }
		bool operator()( const Synt::BitwiseNot& ) { return true; }
		bool operator()( const Synt::BinaryOperator& ) { return true; }
		bool operator()( const Synt::TernaryOperator& ) { return true; }
		bool operator()( const Synt::ReferenceToRawPointerOperator& ) { return true; }
		bool operator()( const Synt::RawPointerToReferenceOperator& ) { return true; }
		// Name resolving itself has no side effects.
		bool operator()( const Synt::ComplexName& ) { return true; }
		// Simple constant expressions have no side effects.
		bool operator()( const Synt::NumericConstant& ) { return true; }
		bool operator()( const Synt::BooleanConstant& ) { return true; }
		bool operator()( const Synt::StringLiteral& ) { return true; }
		// Move and take have side effects.
		bool operator()( const Synt::MoveOperator& ) { return false; }
		bool operator()( const Synt::TakeOperator& ) { return false; }
		// Casts have no side effects.
		bool operator()( const Synt::CastMut& ) { return true; }
		bool operator()( const Synt::CastImut& ) { return true; }
		bool operator()( const Synt::CastRef& ) { return true; }
		bool operator()( const Synt::CastRefUnsafe& ) { return true; }
		bool operator()( const Synt::TypeInfo& ) { return true; }
		bool operator()( const Synt::NonSyncExpression& ) { return true; }
		// safe/unsafe expressions needs to be visited deeply.
		// safe/unsafe expression can't be discarded, because it has meaning.
		bool operator()( const Synt::SafeExpression& safe_expression ) { return SingleExpressionIsUseless( *safe_expression.expression_ ); }
		bool operator()( const Synt::UnsafeExpression& unsafe_expression ) { return SingleExpressionIsUseless( *unsafe_expression.expression_ ); }
		// Type names have no side-effects.
		bool operator()( const Synt::ArrayTypeName& ) { return true; }
		bool operator()( const Synt::FunctionTypePtr& ) { return true; }
		bool operator()( const Synt::TupleType& ) { return true; }
		bool operator()( const Synt::RawPointerType& ) { return true; }
		bool operator()( const Synt::GeneratorTypePtr& ) { return true; }
	};

	return std::visit( Visitor(), expression );
}

} // namespace

CodeBuilder::BlockBuildInfo CodeBuilder::BuildIfAlternative(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::IfAlternative& if_alterntative )
{
	return
		std::visit(
			[&]( const auto& t )
			{
				debug_info_builder_->SetCurrentLocation( t.src_loc_, function_context );
				return BuildBlockElementImpl( names, function_context, t );
			},
			if_alterntative );
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElement(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BlockElement& block_element )
{
	return
		std::visit(
			[&]( const auto& t )
			{
				debug_info_builder_->SetCurrentLocation( t.src_loc_, function_context );
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
	const Synt::ScopeBlock& block )
{
	// Save unsafe flag.
	const bool prev_unsafe= function_context.is_in_unsafe_block;
	if( block.safety_ == Synt::ScopeBlock::Safety::Unsafe )
	{
		function_context.have_non_constexpr_operations_inside= true; // Unsafe operations can not be used in constexpr functions.
		function_context.is_in_unsafe_block= true;
	}
	else if( block.safety_ == Synt::ScopeBlock::Safety::Safe )
		function_context.is_in_unsafe_block= false;
	else if( block.safety_ == Synt::ScopeBlock::Safety::None ) {}
	else U_ASSERT(false);

	llvm::BasicBlock* break_block= nullptr;
	if( block.label != std::nullopt )
	{
		break_block= llvm::BasicBlock::Create( llvm_context_ );
		AddLoopFrame( names, function_context, break_block, nullptr, block.label );
	}

	BlockBuildInfo block_build_info= BuildBlock( names, function_context, block );

	if( break_block != nullptr )
	{
		std::vector<ReferencesGraph> variables_state_for_merge= std::move( function_context.loops_stack.back().break_variables_states );
		if( !block_build_info.have_terminal_instruction_inside )
			variables_state_for_merge.push_back( function_context.variables_state );

		function_context.variables_state= MergeVariablesStateAfterIf( variables_state_for_merge, names.GetErrors(), block.end_src_loc_ );

		function_context.loops_stack.pop_back();

		if( !block_build_info.have_terminal_instruction_inside )
			function_context.llvm_ir_builder.CreateBr( break_block );

		block_build_info.have_terminal_instruction_inside= variables_state_for_merge.empty();

		if( !block_build_info.have_terminal_instruction_inside )
		{
			function_context.function->getBasicBlockList().push_back( break_block );
			function_context.llvm_ir_builder.SetInsertPoint( break_block );
		}
		else
		{
			// Block contains no "break" and ends with "return" or "break" to outer loop/block.
			// In such case we do not needs break block.
			delete break_block;
		}
	}

	// Restore unsafe flag.
	function_context.is_in_unsafe_block= prev_unsafe;

	return block_build_info;
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

		const VariableMutPtr variable_reference =
			std::make_shared<Variable>(
				type,
				variable_declaration.mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable_declaration.name );
		function_context.variables_state.AddNode( variable_reference );

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			const VariableMutPtr variable=
				std::make_shared<Variable>(
					type,
					ValueType::Value,
					Variable::Location::Pointer,
					variable_declaration.name + " variable itself" );

			// Do not forget to remove node in case of error-return!!!
			function_context.variables_state.AddNode( variable );

			variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable->type.GetLLVMType(), nullptr, variable_declaration.name );

			CreateLifetimeStart( function_context, variable->llvm_value );
			debug_info_builder_->CreateVariableInfo( *variable, variable_declaration.name, variable_declaration.src_loc, function_context );

			{
				const VariableMutPtr variable_for_initialization=
					std::make_shared<Variable>(
						type,
						ValueType::ReferenceMut,
						Variable::Location::Pointer,
						variable_declaration.name,
						variable->llvm_value );
				function_context.variables_state.AddNode( variable_for_initialization );
				function_context.variables_state.AddLink( variable, variable_for_initialization );

				variable->constexpr_value=
					variable_declaration.initializer == nullptr
						? ApplyEmptyInitializer( variable_declaration.name, variable_declaration.src_loc, variable_for_initialization, names, function_context )
						: ApplyInitializer( variable_for_initialization, names, function_context, *variable_declaration.initializer );

				function_context.variables_state.RemoveNode( variable_for_initialization );
			}

			variable_reference->llvm_value= variable->llvm_value;
			variable_reference->constexpr_value= variable->constexpr_value;

			prev_variables_storage.RegisterVariable( variable );
			function_context.variables_state.AddLink( variable, variable_reference );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			if( variable_declaration.initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names.GetErrors(), variables_declaration.src_loc_, variable_declaration.name );
				function_context.variables_state.RemoveNode( variable_reference );
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
					function_context.variables_state.RemoveNode( variable_reference );
					continue;
				}
				initializer_expression= &constructor_initializer->arguments.front();
			}
			else
			{
				REPORT_ERROR( UnsupportedInitializerForReference, names.GetErrors(), variable_declaration.src_loc );
				function_context.variables_state.RemoveNode( variable_reference );
				continue;
			}

			const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( *initializer_expression, names, function_context );
			if( !ReferenceIsConvertible( expression_result->type, variable_reference->type, names.GetErrors(), variables_declaration.src_loc_ ) )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), variables_declaration.src_loc_, variable_reference->type, expression_result->type );
				function_context.variables_state.RemoveNode( variable_reference );
				continue;
			}

			if( expression_result->value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), variables_declaration.src_loc_ );
				function_context.variables_state.RemoveNode( variable_reference );
				continue;
			}
			if( expression_result->value_type == ValueType::ReferenceImut && variable_reference->value_type == ValueType::ReferenceMut )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), variable_declaration.src_loc );
				function_context.variables_state.RemoveNode( variable_reference );
				continue;
			}

			// TODO - maybe make copy of varaible address in new llvm register?
			variable_reference->llvm_value=
				CreateReferenceCast( expression_result->llvm_value, expression_result->type, variable_reference->type, function_context );
			variable_reference->constexpr_value= expression_result->constexpr_value;

			debug_info_builder_->CreateReferenceVariableInfo( *variable_reference, variable_declaration.name, variable_declaration.src_loc, function_context );

			function_context.variables_state.TryAddLink( expression_result, variable_reference, names.GetErrors(), variable_declaration.src_loc );
		}
		else U_ASSERT(false);

		prev_variables_storage.RegisterVariable( variable_reference );

		if( variable_declaration.mutability_modifier == MutabilityModifier::Constexpr &&
			variable_reference->constexpr_value == nullptr )
		{
			REPORT_ERROR( VariableInitializerIsNotConstantExpression, names.GetErrors(), variable_declaration.src_loc );
			continue;
		}

		// Reset constexpr initial value for mutable variables.
		if( variable_reference->value_type == ValueType::ReferenceMut )
			variable_reference->constexpr_value= nullptr;

		const NamesScopeValue* const inserted_value=
			names.AddName( variable_declaration.name, NamesScopeValue( variable_reference, variable_declaration.src_loc ) );
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

	const VariablePtr initializer_experrsion= BuildExpressionCodeEnsureVariable( auto_variable_declaration.initializer_expression, names, function_context );
	if( initializer_experrsion->type == invalid_type_ )
		return BlockBuildInfo(); // Some error was generated before.

	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference ||
		auto_variable_declaration.mutability_modifier == Synt::MutabilityModifier::Constexpr )
	{
		// Full completeness required for value-variables and any constexpr variable.
		if( !EnsureTypeComplete( initializer_experrsion->type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), auto_variable_declaration.src_loc_, initializer_experrsion->type );
			return BlockBuildInfo();
		}
	}
	if( auto_variable_declaration.reference_modifier != ReferenceModifier::Reference && initializer_experrsion->type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), auto_variable_declaration.src_loc_, initializer_experrsion->type );

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && !initializer_experrsion->type.CanBeConstexpr() )
	{
		REPORT_ERROR( InvalidTypeForConstantExpressionVariable, names.GetErrors(), auto_variable_declaration.src_loc_ );
		return BlockBuildInfo();
	}

	const VariableMutPtr variable_reference=
		std::make_shared<Variable>(
			initializer_experrsion->type,
			auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
			Variable::Location::Pointer,
			auto_variable_declaration.name,
			nullptr,
			initializer_experrsion->constexpr_value );
	function_context.variables_state.AddNode( variable_reference );

	if( auto_variable_declaration.reference_modifier == ReferenceModifier::Reference )
	{
		if( initializer_experrsion->value_type == ValueType::ReferenceImut && auto_variable_declaration.mutability_modifier == MutabilityModifier::Mutable )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), auto_variable_declaration.src_loc_ );
			function_context.variables_state.RemoveNode( variable_reference );
			return BlockBuildInfo();
		}
		if( initializer_experrsion->value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), auto_variable_declaration.src_loc_ );
			function_context.variables_state.RemoveNode( variable_reference );
			return BlockBuildInfo();
		}

		variable_reference->llvm_value= initializer_experrsion->llvm_value;

		debug_info_builder_->CreateReferenceVariableInfo( *variable_reference, auto_variable_declaration.name, auto_variable_declaration.src_loc_, function_context );

		function_context.variables_state.TryAddLink( initializer_experrsion, variable_reference, names.GetErrors(), auto_variable_declaration.src_loc_ );
	}
	else if( auto_variable_declaration.reference_modifier == ReferenceModifier::None )
	{
		if( !initializer_experrsion->type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		const VariableMutPtr variable=
			std::make_shared<Variable>(
				initializer_experrsion->type,
				ValueType::Value,
				Variable::Location::Pointer,
				auto_variable_declaration.name + " variable itself",
				nullptr,
				initializer_experrsion->constexpr_value /* constexpr preserved for move/copy. */ );

		function_context.variables_state.AddNode( variable );

		if( initializer_experrsion->value_type == ValueType::Value &&
			initializer_experrsion->location == Variable::Location::Pointer &&
			initializer_experrsion->llvm_value->getType() == variable->type.GetLLVMType()->getPointerTo() &&
			( llvm::dyn_cast<llvm::AllocaInst>(initializer_experrsion->llvm_value) != nullptr || llvm::dyn_cast<llvm::Argument>(initializer_experrsion->llvm_value) != nullptr ) )
		{
			// Just reuse "alloca" instruction or function argument for move-initialization, avoid copying value into new memory location.
			variable->llvm_value= initializer_experrsion->llvm_value;
			variable->llvm_value->setName( auto_variable_declaration.name );
		}
		else
		{
			variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable->type.GetLLVMType(), nullptr, auto_variable_declaration.name );
			CreateLifetimeStart( function_context, variable->llvm_value );
		}

		debug_info_builder_->CreateVariableInfo( *variable, auto_variable_declaration.name, auto_variable_declaration.src_loc_, function_context );

		SetupReferencesInCopyOrMove( function_context, variable, initializer_experrsion, names.GetErrors(), auto_variable_declaration.src_loc_ );

		if( initializer_experrsion->value_type == ValueType::Value )
		{
			function_context.variables_state.MoveNode( initializer_experrsion );

			if( initializer_experrsion->llvm_value != variable->llvm_value )
			{
				if( initializer_experrsion->location == Variable::Location::LLVMRegister )
					CreateTypedStore( function_context, initializer_experrsion->type, initializer_experrsion->llvm_value, variable->llvm_value );
				else
					CopyBytes( variable->llvm_value, initializer_experrsion->llvm_value, variable->type, function_context );
				if( initializer_experrsion->location == Variable::Location::Pointer )
					CreateLifetimeEnd( function_context, initializer_experrsion->llvm_value );
			}
		}
		else
		{
			if( !variable->type.IsCopyConstructible() )
			{
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), auto_variable_declaration.src_loc_, variable->type );
				function_context.variables_state.RemoveNode(variable);
				return BlockBuildInfo();
			}

			BuildCopyConstructorPart(
				variable->llvm_value, initializer_experrsion->llvm_value,
				variable->type,
				function_context );
		}

		variable_reference->llvm_value= variable->llvm_value;

		prev_variables_storage.RegisterVariable( variable );

		function_context.variables_state.AddLink( variable, variable_reference );
	}
	else U_ASSERT(false);

	prev_variables_storage.RegisterVariable( variable_reference );

	if( auto_variable_declaration.mutability_modifier == MutabilityModifier::Constexpr && variable_reference->constexpr_value == nullptr )
	{
		REPORT_ERROR( VariableInitializerIsNotConstantExpression, names.GetErrors(), auto_variable_declaration.src_loc_ );
		return BlockBuildInfo();
	}

	// Reset constexpr initial value for mutable variables.
	if( variable_reference->value_type != ValueType::ReferenceImut )
		variable_reference->constexpr_value= nullptr;

	const NamesScopeValue* const inserted_value=
		names.AddName( auto_variable_declaration.name, NamesScopeValue( variable_reference, auto_variable_declaration.src_loc_ ) );
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
		if( function_context.coro_suspend_bb != nullptr )
		{
			// For generators enter into final suspend state in case of manual "return".
			GeneratorFinalSuspend( names, function_context, return_operator.src_loc_ );
			return block_info;
		}
		if( function_context.return_type == std::nullopt )
		{
			if( function_context.function_type.return_value_type != ValueType::Value )
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

		BuildEmptyReturn( names, function_context, return_operator.src_loc_ );

		return block_info;
	}

	if( function_context.coro_suspend_bb != nullptr )
	{
		// For generators process "return" with value as combination "yield" and empty "return".
		GeneratorYield( names, function_context, return_operator.expression_, return_operator.src_loc_ );
		GeneratorFinalSuspend( names, function_context, return_operator.src_loc_ );
		return block_info;
	}

	// Destruction frame for temporary variables of result expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	VariablePtr expression_result= BuildExpressionCodeEnsureVariable( return_operator.expression_, names, function_context );
	if( expression_result->type == invalid_type_ )
	{
		// Add "ret void", because we do not need to break llvm basic blocks structure.
		function_context.llvm_ir_builder.CreateRetVoid();
		return block_info;
	}

	// For functions with "auto" on return type use type of first return expression.
	if( function_context.return_type == std::nullopt )
	{
		if( function_context.deduced_return_type == std::nullopt )
			function_context.deduced_return_type = expression_result->type;
		else if( *function_context.deduced_return_type != expression_result->type )
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.deduced_return_type, expression_result->type );
		return block_info;
	}

	const VariablePtr return_value_node=
		std::make_shared<Variable>(
			*function_context.return_type,
			function_context.function_type.return_value_type,
			Variable::Location::Pointer,
			"return value lock" );
	function_context.variables_state.AddNode( return_value_node );

	llvm::Value* ret= nullptr;
	if( function_context.function_type.return_value_type == ValueType::Value )
	{
		if( expression_result->type != function_context.return_type )
		{
			if( const auto conversion_contructor= GetConversionConstructor( expression_result->type, *function_context.return_type, names.GetErrors(), return_operator.src_loc_ ) )
				expression_result= ConvertVariable( expression_result, *function_context.return_type, *conversion_contructor, names, function_context, return_operator.src_loc_ );
			else
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.return_type, expression_result->type );
				function_context.variables_state.RemoveNode( return_value_node );
				return block_info;
			}
		}

		// Check correctness of returning references.
		if( expression_result->type.ReferencesTagsCount() > 0u )
		{
			for( const VariablePtr& inner_reference : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( expression_result ) )
			{
				for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( inner_reference ) )
				{
					if( !IsReferenceAllowedForReturn( function_context, var_node ) )
						REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), return_operator.src_loc_ );
				}
			}
		}

		// Setup references in order to catch errors, when referene to local variable is returned inside struct.
		SetupReferencesInCopyOrMove( function_context, return_value_node, expression_result, names.GetErrors(), return_operator.src_loc_ );

		if( expression_result->type.GetFundamentalType() != nullptr||
			expression_result->type.GetEnumType() != nullptr ||
			expression_result->type.GetRawPointerType() != nullptr ||
			expression_result->type.GetFunctionPointerType() != nullptr ) // Just copy simple scalar.
		{
			if( expression_result->type != void_type_ )
				ret= CreateMoveToLLVMRegisterInstruction( *expression_result, function_context );
		}
		else if( expression_result->value_type == ValueType::Value ) // Move composite value.
		{
			function_context.variables_state.MoveNode( expression_result );

			if( const auto single_scalar_type= GetSingleScalarType( expression_result->type.GetLLVMType() ) )
			{
				U_ASSERT( function_context.s_ret == nullptr );
				ret= function_context.llvm_ir_builder.CreateLoad( single_scalar_type, expression_result->llvm_value );
			}
			else
			{
				U_ASSERT( function_context.s_ret != nullptr );
				CopyBytes( function_context.s_ret, expression_result->llvm_value, *function_context.return_type, function_context );
			}

			if( expression_result->location == Variable::Location::Pointer )
				CreateLifetimeEnd( function_context, expression_result->llvm_value );
		}
		else // Copy composite value.
		{
			if( !expression_result->type.IsCopyConstructible() )
			{
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), return_operator.src_loc_, expression_result->type );
				function_context.variables_state.RemoveNode( return_value_node );
				return block_info;
			}
			if( expression_result->type.IsAbstract() )
			{
				REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), return_operator.src_loc_, expression_result->type );
				function_context.variables_state.RemoveNode( return_value_node );
				return block_info;
			}

			if( const auto single_scalar_type= GetSingleScalarType( expression_result->type.GetLLVMType() ) )
			{
				U_ASSERT( function_context.s_ret == nullptr );
				// Call copy constructor on temp address, load then value from it.
				llvm::Value* const temp= function_context.alloca_ir_builder.CreateAlloca( expression_result->type.GetLLVMType() );
				CreateLifetimeStart( function_context, temp );

				BuildCopyConstructorPart(
					temp,
					CreateReferenceCast( expression_result->llvm_value, expression_result->type, *function_context.return_type, function_context ),
					*function_context.return_type,
					function_context );

				ret= function_context.llvm_ir_builder.CreateLoad( single_scalar_type, temp );

				CreateLifetimeEnd( function_context, temp );
			}
			else
			{
				// Call copy constructor on "s_ret".
				U_ASSERT( function_context.s_ret != nullptr );
				BuildCopyConstructorPart(
					function_context.s_ret,
					CreateReferenceCast( expression_result->llvm_value, expression_result->type, *function_context.return_type, function_context ),
					*function_context.return_type,
					function_context );
			}
		}
	}
	else
	{
		if( !ReferenceIsConvertible( expression_result->type, *function_context.return_type, names.GetErrors(), return_operator.src_loc_ ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), return_operator.src_loc_, *function_context.return_type, expression_result->type );
			function_context.variables_state.RemoveNode( return_value_node );
			return block_info;
		}

		if( expression_result->value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), return_operator.src_loc_ );
			function_context.variables_state.RemoveNode( return_value_node );
			return block_info;
		}
		if( expression_result->value_type == ValueType::ReferenceImut && function_context.function_type.return_value_type == ValueType::ReferenceMut )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), return_operator.src_loc_ );
		}

		// Check correctness of returning reference.
		for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( expression_result ) )
		{
			if( !IsReferenceAllowedForReturn( function_context, var_node ) )
				REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), return_operator.src_loc_ );
		}

		// Add link to return value in order to catch error, when reference to local variable is returned.
		function_context.variables_state.AddLink( expression_result, return_value_node );

		ret= CreateReferenceCast( expression_result->llvm_value, expression_result->type, *function_context.return_type, function_context );
	}

	CallDestructorsBeforeReturn( names, function_context, return_operator.src_loc_ );
	CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), return_operator.src_loc_ );
	function_context.variables_state.RemoveNode( return_value_node );

	if( function_context.destructor_end_block != nullptr )
	{
		// In explicit destructor, break to block with destructor calls for class members.
		function_context.llvm_ir_builder.CreateBr(function_context.destructor_end_block);
	}
	else if( ret != nullptr )
	{
		// Return simple scalar - fundamental type value, reference, pointer.
		function_context.llvm_ir_builder.CreateRet(ret);
	}
	else
	{
		// Return "void" or return value via "s_ret".
		function_context.llvm_ir_builder.CreateRetVoid();
	}

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::YieldOperator& yield_operator )
{
	// "Yield" is not a terminal operator. Execution (logically) continues after it.
	GeneratorYield( names, function_context, yield_operator.expression, yield_operator.src_loc_ );
	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::RangeForOperator& range_for_operator )
{
	BlockBuildInfo block_build_info;

	const StackVariablesStorage temp_variables_storage( function_context );
	const VariablePtr sequence_expression= BuildExpressionCodeEnsureVariable( range_for_operator.sequence_, names, function_context );

	const VariablePtr sequence_lock=
		std::make_shared<Variable>(
			sequence_expression->type,
			sequence_expression->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
			Variable::Location::Pointer,
			sequence_expression->name + " sequence lock" );

	function_context.variables_state.AddNode( sequence_lock );
	function_context.variables_state.TryAddLink( sequence_expression, sequence_lock,  names.GetErrors(), range_for_operator.src_loc_ );

	RegisterTemporaryVariable( function_context, sequence_lock );

	const bool is_mutable= range_for_operator.mutability_modifier_ == MutabilityModifier::Mutable;
	if( const TupleType* const tuple_type= sequence_expression->type.GetTupleType() )
	{
		llvm::BasicBlock* const finish_basic_block= tuple_type->element_types.empty() ? nullptr : llvm::BasicBlock::Create( llvm_context_ );

		llvm::SmallVector<ReferencesGraph, 4> break_variables_states;

		U_ASSERT( sequence_expression->location == Variable::Location::Pointer );
		for( const Type& element_type : tuple_type->element_types )
		{
			const size_t element_index= size_t( &element_type - tuple_type->element_types.data() );
			const std::string variable_name= range_for_operator.loop_variable_name_ + std::to_string(element_index);
			NamesScope loop_names( "", &names );
			const StackVariablesStorage element_pass_variables_storage( function_context );

			const VariableMutPtr variable_reference=
				std::make_shared<Variable>(
					element_type,
					is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
					Variable::Location::Pointer,
					range_for_operator.loop_variable_name_,
					nullptr,
					is_mutable || sequence_expression->constexpr_value == nullptr
						? nullptr
						: sequence_expression->constexpr_value->getAggregateElement( uint32_t(element_index)) );

			// Do not forget to remove node in case of error-return!!!
			function_context.variables_state.AddNode( variable_reference );

			if( range_for_operator.reference_modifier_ == ReferenceModifier::Reference )
			{
				if( range_for_operator.mutability_modifier_ == MutabilityModifier::Mutable && sequence_expression->value_type != ValueType::ReferenceMut )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), range_for_operator.src_loc_ );
					function_context.variables_state.RemoveNode( variable_reference );
					continue;
				}

				variable_reference->llvm_value= CreateTupleElementGEP( function_context, *sequence_expression, element_index );

				debug_info_builder_->CreateReferenceVariableInfo( *variable_reference, variable_name, range_for_operator.src_loc_, function_context );

				function_context.variables_state.TryAddLink( sequence_lock, variable_reference, names.GetErrors(), range_for_operator.src_loc_ );
			}
			else
			{
				const VariableMutPtr variable=
					std::make_shared<Variable>(
						element_type,
						ValueType::Value,
						Variable::Location::Pointer,
						range_for_operator.loop_variable_name_ + " variable itself",
						nullptr,
						nullptr );
				function_context.variables_state.AddNode( variable );

				if( !EnsureTypeComplete( element_type ) )
				{
					REPORT_ERROR( UsingIncompleteType, names.GetErrors(), range_for_operator.src_loc_, element_type );
					function_context.variables_state.RemoveNode( variable_reference );
					function_context.variables_state.RemoveNode( variable );
					continue;
				}
				if( !element_type.IsCopyConstructible() )
				{
					REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), range_for_operator.src_loc_, element_type );
					function_context.variables_state.RemoveNode( variable_reference );
					function_context.variables_state.RemoveNode( variable );
					continue;
				}
				if( element_type.IsAbstract() )
				{
					REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), range_for_operator.src_loc_, element_type );
					function_context.variables_state.RemoveNode( variable_reference );
					function_context.variables_state.RemoveNode( variable );
					continue;
				}

				variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( element_type.GetLLVMType(), nullptr, variable_name );
				CreateLifetimeStart( function_context, variable->llvm_value );
				debug_info_builder_->CreateVariableInfo( *variable, variable_name, range_for_operator.src_loc_, function_context );

				SetupReferencesInCopyOrMove( function_context, variable, sequence_expression, names.GetErrors(), range_for_operator.src_loc_ );

				BuildCopyConstructorPart(
					variable->llvm_value,
					CreateTupleElementGEP( function_context, *sequence_expression, element_index ),
					element_type,
					function_context );

				variable_reference->llvm_value= variable->llvm_value;

				function_context.stack_variables_stack.back()->RegisterVariable( variable );

				function_context.variables_state.AddLink( variable, variable_reference );
			}

			function_context.stack_variables_stack.back()->RegisterVariable( variable_reference );

			loop_names.AddName( range_for_operator.loop_variable_name_, NamesScopeValue( variable_reference, range_for_operator.src_loc_ ) );

			const bool is_last_iteration= element_index + 1u == tuple_type->element_types.size();
			llvm::BasicBlock* const next_basic_block=
				is_last_iteration ? finish_basic_block : llvm::BasicBlock::Create( llvm_context_ );

			AddLoopFrame(
				loop_names,
				function_context,
				finish_basic_block,
				next_basic_block,
				range_for_operator.label_ );
			function_context.loops_stack.back().stack_variables_stack_size= function_context.stack_variables_stack.size() - 1u; // Extra 1 for loop variable destruction in 'break' or 'continue'.

			// TODO - create template errors context.
			// Build block without creating inner namespace - reuse namespace of tuple-for variable.
			const BlockBuildInfo inner_block_build_info= BuildBlockElements( loop_names, function_context, range_for_operator.block_->elements_ );
			if( !inner_block_build_info.have_terminal_instruction_inside )
			{
				CallDestructors( element_pass_variables_storage, names, function_context, range_for_operator.src_loc_ );
				function_context.llvm_ir_builder.CreateBr( next_basic_block );
				function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
			}

			// Variables state for next iteration is combination of variables states in "continue" branches in previous iteration.
			const bool continue_branches_is_empty= function_context.loops_stack.back().continue_variables_states.empty();
			if( !continue_branches_is_empty )
				function_context.variables_state= MergeVariablesStateAfterIf( function_context.loops_stack.back().continue_variables_states, names.GetErrors(), range_for_operator.block_->end_src_loc_ );

			for( ReferencesGraph& variables_state : function_context.loops_stack.back().break_variables_states )
				break_variables_states.push_back( std::move(variables_state) );

			// Args preevaluation uses addresses of syntax elements as keys. Reset it, because we use same syntax elements multiple times.
			function_context.args_preevaluation_cache.clear();

			function_context.loops_stack.pop_back();

			CheckForUnusedLocalNames( loop_names );

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

		if( tuple_type->element_types.empty() )
		{} // Just keep variables state.
		// Variables state after tuple-for is combination of variables state of all branches with "break" of all iterations.
		else if( !break_variables_states.empty() )
			function_context.variables_state= MergeVariablesStateAfterIf( break_variables_states, names.GetErrors(), range_for_operator.block_->end_src_loc_ );
		else
			block_build_info.have_terminal_instruction_inside= true;
	}
	else
	{
		// TODO - support array types.
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), range_for_operator.src_loc_, sequence_expression->type );
		return BlockBuildInfo();
	}

	if( !block_build_info.have_terminal_instruction_inside )
		CallDestructors( temp_variables_storage, names, function_context, range_for_operator.src_loc_ );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CStyleForOperator& c_style_for_operator )
{
	const StackVariablesStorage loop_variables_storage( function_context );
	NamesScope loop_names_scope("", &names);

	// Variables declaration part.
	if( c_style_for_operator.variable_declaration_part_ != nullptr )
		std::visit(
			[&]( const auto& t )
			{
				debug_info_builder_->SetCurrentLocation( t.src_loc_, function_context );
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
		const VariablePtr condition_expression= BuildExpressionCodeEnsureVariable( c_style_for_operator.loop_condition_, loop_names_scope, function_context );

		const SrcLoc condition_src_loc= Synt::GetExpressionSrcLoc( c_style_for_operator.loop_condition_ );
		if( condition_expression->type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch,
					names.GetErrors(),
					condition_src_loc,
					bool_type_,
					condition_expression->type );
			function_context.llvm_ir_builder.CreateBr( loop_block );
		}
		else
		{
			llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression, function_context );
			CallDestructors( temp_variables_storage, names, function_context, condition_src_loc );
			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, loop_block, block_after_loop );
		}
	}

	ReferencesGraph variables_state_after_test_block= function_context.variables_state;

	// Loop block code.
	AddLoopFrame( names, function_context, block_after_loop, loop_iteration_block, c_style_for_operator.label_ );

	function_context.function->getBasicBlockList().push_back( loop_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_block );

	const BlockBuildInfo loop_body_block_info= BuildBlock( loop_names_scope, function_context, *c_style_for_operator.block_ );
	if( !loop_body_block_info.have_terminal_instruction_inside )
	{
		function_context.llvm_ir_builder.CreateBr( loop_iteration_block );
		function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
	}

	// Variables state before loop iteration block is combination of variables states of each branch terminated with "continue".
	if( !function_context.loops_stack.back().continue_variables_states.empty() )
		function_context.variables_state= MergeVariablesStateAfterIf( function_context.loops_stack.back().continue_variables_states, names.GetErrors(), c_style_for_operator.block_->end_src_loc_ );

	std::vector<ReferencesGraph> variables_state_for_merge= std::move( function_context.loops_stack.back().break_variables_states );
	variables_state_for_merge.push_back( std::move(variables_state_after_test_block) );

	function_context.loops_stack.pop_back();

	// Loop iteration block
	function_context.function->getBasicBlockList().push_back( loop_iteration_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_iteration_block );
	for( const auto& element : c_style_for_operator.iteration_part_elements_ )
	{
		std::visit(
			[&]( const auto& t )
			{
				debug_info_builder_->SetCurrentLocation( t.src_loc_, function_context );
				BuildBlockElementImpl( loop_names_scope, function_context, t );
			},
			element );
	}
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Disallow outer variables state change in loop iteration part and its predecessors.
	const auto errors= ReferencesGraph::CheckWhileBlockVariablesState( variables_state_before_loop, function_context.variables_state, c_style_for_operator.block_->end_src_loc_ );
	names.GetErrors().insert( names.GetErrors().end(), errors.begin(), errors.end() );

	function_context.variables_state= MergeVariablesStateAfterIf( variables_state_for_merge, names.GetErrors(), c_style_for_operator.block_->end_src_loc_ );

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
		const VariablePtr condition_expression= BuildExpressionCodeEnsureVariable( while_operator.condition_, names, function_context );

		const SrcLoc condition_src_loc= Synt::GetExpressionSrcLoc( while_operator.condition_ );
		if( condition_expression->type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), condition_src_loc, bool_type_, condition_expression->type );

			// Create instruction even in case of error, because we needs to store basic blocs somewhere.
			function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), while_block, block_after_while );
		}
		else
		{
			llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression, function_context );
			CallDestructors( temp_variables_storage, names, function_context, condition_src_loc );

			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );
		}
	}

	// While block code.

	AddLoopFrame( names, function_context, block_after_while, test_block, while_operator.label_ );

	function_context.function->getBasicBlockList().push_back( while_block );
	function_context.llvm_ir_builder.SetInsertPoint( while_block );

	const BlockBuildInfo loop_body_block_info= BuildBlock( names, function_context, *while_operator.block_ );
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
		const auto errors= ReferencesGraph::CheckWhileBlockVariablesState( variables_state_before_loop, variables_state, while_operator.block_->end_src_loc_ );
		names.GetErrors().insert( names.GetErrors().end(), errors.begin(), errors.end() );
	}

	std::vector<ReferencesGraph> variables_state_for_merge= std::move( function_context.loops_stack.back().break_variables_states );
	variables_state_for_merge.push_back( std::move(variables_state_before_loop) );

	function_context.loops_stack.pop_back();

	// Result variables state is combination of variables state before loop and variables state of all branches terminated with "break".
	function_context.variables_state= MergeVariablesStateAfterIf( variables_state_for_merge, names.GetErrors(), while_operator.block_->end_src_loc_ );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::LoopOperator& loop_operator )
{
	ReferencesGraph variables_state_before_loop= function_context.variables_state;

	llvm::BasicBlock* const loop_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_loop= llvm::BasicBlock::Create( llvm_context_ );

	// Break to loop block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( loop_block );

	AddLoopFrame( names, function_context, block_after_loop, loop_block, loop_operator.label_ );

	function_context.function->getBasicBlockList().push_back( loop_block );
	function_context.llvm_ir_builder.SetInsertPoint( loop_block );

	const BlockBuildInfo loop_body_block_info= BuildBlock( names, function_context, *loop_operator.block_ );
	if( !loop_body_block_info.have_terminal_instruction_inside )
	{
		function_context.llvm_ir_builder.CreateBr( loop_block );
		function_context.loops_stack.back().continue_variables_states.push_back( function_context.variables_state );
	}

	// Disallow outer variables state change in "continue" branches.
	for( const ReferencesGraph& variables_state : function_context.loops_stack.back().continue_variables_states )
	{
		const auto errors= ReferencesGraph::CheckWhileBlockVariablesState( variables_state_before_loop, variables_state, loop_operator.block_->end_src_loc_ );
		names.GetErrors().insert( names.GetErrors().end(), errors.begin(), errors.end() );
	}

	std::vector<ReferencesGraph> variables_state_for_merge= std::move( function_context.loops_stack.back().break_variables_states );

	function_context.loops_stack.pop_back();

	// Result variables state is combination of variables state of all branches terminated with "break".
	function_context.variables_state= MergeVariablesStateAfterIf( variables_state_for_merge, names.GetErrors(), loop_operator.block_->end_src_loc_ );

	// This loop is terminal, if it contains no "break" inside - only "break" to outer labels or "return".
	// Any code, that follows infinite loop without "break" inside is unreachable.
	BlockBuildInfo block_build_info;
	block_build_info.have_terminal_instruction_inside= variables_state_for_merge.empty();

	if( !block_build_info.have_terminal_instruction_inside )
	{
		// Block after loop code.
		function_context.function->getBasicBlockList().push_back( block_after_loop );
		function_context.llvm_ir_builder.SetInsertPoint( block_after_loop );
	}
	else
		delete block_after_loop;

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BreakOperator& break_operator )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;

	LoopFrame* const loop_frame= FetchLoopFrame( names, function_context, break_operator.label_ );
	if( loop_frame == nullptr )
	{
		REPORT_ERROR( BreakOutsideLoop, names.GetErrors(), break_operator.src_loc_ );
		return block_info;
	}

	U_ASSERT( loop_frame->block_for_break != nullptr );

	CallDestructorsForLoopInnerVariables( names, function_context, loop_frame->stack_variables_stack_size, break_operator.src_loc_ );
	loop_frame->break_variables_states.push_back( function_context.variables_state );
	function_context.llvm_ir_builder.CreateBr( loop_frame->block_for_break );

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ContinueOperator& continue_operator )
{
	BlockBuildInfo block_info;
	block_info.have_terminal_instruction_inside= true;

	LoopFrame* const loop_frame= FetchLoopFrame( names, function_context, continue_operator.label_ );
	if( loop_frame == nullptr )
	{
		REPORT_ERROR( ContinueOutsideLoop, names.GetErrors(), continue_operator.src_loc_ );
		return block_info;
	}

	if( loop_frame->block_for_continue == nullptr )
	{
		// This is non-loop frame.
		REPORT_ERROR( ContinueForBlock, names.GetErrors(), continue_operator.src_loc_ );
		return block_info;
	}

	CallDestructorsForLoopInnerVariables( names, function_context, loop_frame->stack_variables_stack_size, continue_operator.src_loc_ );
	loop_frame->continue_variables_states.push_back( function_context.variables_state );
	function_context.llvm_ir_builder.CreateBr( loop_frame->block_for_continue );

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::WithOperator& with_operator )
{
	StackVariablesStorage variables_storage( function_context );

	const VariablePtr expr= BuildExpressionCodeEnsureVariable( with_operator.expression_, names, function_context );

	const VariableMutPtr variable_reference=
		std::make_shared<Variable>(
			expr->type,
			with_operator.mutability_modifier_ == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
			Variable::Location::Pointer,
			with_operator.variable_name_,
			nullptr,
			expr->constexpr_value );
	// Do not forget to remove node in case of error-return!!!
	function_context.variables_state.AddNode( variable_reference );

	if( with_operator.reference_modifier_ != ReferenceModifier::Reference &&
		!EnsureTypeComplete( variable_reference->type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), with_operator.src_loc_, variable_reference->type );
		function_context.variables_state.RemoveNode( variable_reference );
		return BlockBuildInfo();
	}
	if( with_operator.reference_modifier_ != ReferenceModifier::Reference && variable_reference->type.IsAbstract() )
	{
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), with_operator.src_loc_, variable_reference->type );
		function_context.variables_state.RemoveNode( variable_reference );
		return BlockBuildInfo();
	}

	if( with_operator.reference_modifier_ == ReferenceModifier::Reference )
	{
		if( expr->value_type == ValueType::ReferenceImut && variable_reference->value_type != ValueType::ReferenceImut )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), with_operator.src_loc_ );
			function_context.variables_state.RemoveNode( variable_reference );
			return BlockBuildInfo();
		}

		if( expr->location == Variable::Location::LLVMRegister )
		{
			// Binding value to reference.
			llvm::Value* const storage= function_context.alloca_ir_builder.CreateAlloca( expr->type.GetLLVMType() );
			CreateTypedStore( function_context, expr->type, expr->llvm_value, storage );
			variable_reference->llvm_value= storage;
		}
		else
			variable_reference->llvm_value= expr->llvm_value;

		debug_info_builder_->CreateReferenceVariableInfo( *variable_reference, with_operator.variable_name_, with_operator.src_loc_, function_context );

		function_context.variables_state.TryAddLink( expr, variable_reference, names.GetErrors(), with_operator.src_loc_ );
	}
	else if( with_operator.reference_modifier_ == ReferenceModifier::None )
	{
		const VariableMutPtr variable=
			std::make_shared<Variable>(
				expr->type,
				ValueType::Value,
				Variable::Location::Pointer,
				with_operator.variable_name_ + " variable itself",
				nullptr,
				expr->constexpr_value /* constexpr preserved for move/copy. */ );
		function_context.variables_state.AddNode( variable );

		if( !variable->type.CanBeConstexpr() )
			function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

		if( expr->value_type == ValueType::Value &&
			expr->location == Variable::Location::Pointer &&
			expr->llvm_value->getType() == variable->type.GetLLVMType()->getPointerTo() &&
			( llvm::dyn_cast<llvm::AllocaInst>(expr->llvm_value) != nullptr || llvm::dyn_cast<llvm::Argument>(expr->llvm_value) != nullptr ) )
		{
			// Just reuse "alloca" instruction or argument for move-initialization, avoid copying value into new memory location.
			variable->llvm_value= expr->llvm_value;
			variable->llvm_value->setName( with_operator.variable_name_ );
		}
		else
		{
			variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( variable->type.GetLLVMType(), nullptr, with_operator.variable_name_ );
			CreateLifetimeStart( function_context, variable->llvm_value );
		}

		debug_info_builder_->CreateVariableInfo( *variable, with_operator.variable_name_, with_operator.src_loc_, function_context );

		SetupReferencesInCopyOrMove( function_context, variable, expr, names.GetErrors(), with_operator.src_loc_ );

		if( expr->value_type == ValueType::Value )
		{
			function_context.variables_state.MoveNode( expr );

			if( variable->llvm_value != expr->llvm_value )
			{
				if( expr->location == Variable::Location::LLVMRegister )
					CreateTypedStore( function_context, expr->type, expr->llvm_value, variable->llvm_value );
				else
				{
					CopyBytes( variable->llvm_value, expr->llvm_value, variable->type, function_context );
					CreateLifetimeEnd( function_context, expr->llvm_value );
				}
			}
		}
		else
		{
			if( !variable->type.IsCopyConstructible() )
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), with_operator.src_loc_, variable->type );
			else
				BuildCopyConstructorPart(
					variable->llvm_value, expr->llvm_value,
					variable->type,
					function_context );
		}

		variable_reference->llvm_value= variable->llvm_value;

		variables_storage.RegisterVariable( variable );

		function_context.variables_state.AddLink( variable, variable_reference );
	}
	else U_ASSERT(false);

	// Reset constexpr initial value for mutable variables.
	if( variable_reference->value_type != ValueType::ReferenceImut )
		variable_reference->constexpr_value= nullptr;

	if( IsKeyword( with_operator.variable_name_ ) )
		REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), with_operator.src_loc_ );

	// Destroy temporary variables of initializer expression. Do it before registretion of variable to prevent its destruction.
	DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), with_operator.src_loc_ );
	variables_storage.RegisterVariable( variable_reference );

	// Create separate namespace for variable. Redefinition here is not possible.
	NamesScope variable_names_scope( "", &names );
	variable_names_scope.AddName( with_operator.variable_name_, NamesScopeValue( variable_reference, with_operator.src_loc_ ) );

	// Build block. Do not create names scope, reuce names scope of "with" variable.
	const BlockBuildInfo block_build_info= BuildBlockElements( variable_names_scope, function_context, with_operator.block_.elements_ );

	if( !block_build_info.have_terminal_instruction_inside )
	{
		// Destroy all temporaries.
		CallDestructors( variables_storage, variable_names_scope, function_context, with_operator.src_loc_ );
	}

	CheckForUnusedLocalNames( variable_names_scope );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::IfOperator& if_operator )
{
	llvm::BasicBlock* const if_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const alternative_block= llvm::BasicBlock::Create( llvm_context_ );

	ReferencesGraph variables_state_before_branching= function_context.variables_state;

	{
		const StackVariablesStorage temp_variables_storage( function_context );
		const VariablePtr condition_expression= BuildExpressionCodeEnsureVariable( if_operator.condition, names, function_context );
		if( condition_expression->type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch,
				names.GetErrors(),
				Synt::GetExpressionSrcLoc( if_operator.condition ),
				bool_type_,
				condition_expression->type );

			// Create instruction even in case of error, because we needs to store basic blocs somewhere.
			function_context.llvm_ir_builder.CreateCondBr( llvm::UndefValue::get( fundamental_llvm_types_.bool_ ), if_block, alternative_block );
		}
		else
		{
			llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression, function_context );
			CallDestructors( temp_variables_storage, names, function_context, Synt::GetExpressionSrcLoc( if_operator.condition ) );

			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, if_block, alternative_block );
		}

		variables_state_before_branching= function_context.variables_state;
	}

	// If block.
	function_context.function->getBasicBlockList().push_back( if_block );
	function_context.llvm_ir_builder.SetInsertPoint( if_block );
	const BlockBuildInfo if_block_build_info= BuildBlock( names, function_context, if_operator.block );

	llvm::SmallVector<ReferencesGraph, 2> branches_variable_states;

	BlockBuildInfo block_build_info;

	if( if_operator.alternative == nullptr )
	{
		if( !if_block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( alternative_block );
			branches_variable_states.push_back( function_context.variables_state );
		}

		branches_variable_states.push_back( std::move( variables_state_before_branching ) );

		block_build_info.have_terminal_instruction_inside= false;

		function_context.function->getBasicBlockList().push_back( alternative_block );
		function_context.llvm_ir_builder.SetInsertPoint( alternative_block );
	}
	else
	{
		llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );

		if( !if_block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( block_after_if );
			branches_variable_states.push_back( function_context.variables_state );
		}

		// Else block.
		function_context.function->getBasicBlockList().push_back( alternative_block );
		function_context.llvm_ir_builder.SetInsertPoint( alternative_block );

		function_context.variables_state= std::move( variables_state_before_branching );
		const BlockBuildInfo alternative_block_build_info= BuildIfAlternative( names, function_context, *if_operator.alternative );

		if( !alternative_block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( block_after_if );
			branches_variable_states.push_back( function_context.variables_state );
		}

		block_build_info.have_terminal_instruction_inside=
			if_block_build_info.have_terminal_instruction_inside && alternative_block_build_info.have_terminal_instruction_inside;

		if( !block_build_info.have_terminal_instruction_inside )
		{
			function_context.function->getBasicBlockList().push_back( block_after_if );
			function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
		}
		else
			delete block_after_if;
	}

	function_context.variables_state= MergeVariablesStateAfterIf( branches_variable_states, names.GetErrors(), if_operator.end_src_loc );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::StaticIfOperator& static_if_operator )
{
	{
		const StackVariablesStorage temp_variables_storage( function_context );
		if( EvaluateBoolConstantExpression( names, function_context, static_if_operator.condition ) )
			return BuildBlock( names, function_context, static_if_operator.block ); // Ok, this static if produdes block.

		CallDestructors( temp_variables_storage, names, function_context, static_if_operator.src_loc_ );
	}

	if( static_if_operator.alternative != nullptr )
		return BuildIfAlternative( names, function_context, *static_if_operator.alternative );

	return BlockBuildInfo();
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::IfCoroAdvanceOperator& if_coro_advance )
{
	StackVariablesStorage variables_storage( function_context );

	const VariablePtr coro_expr= BuildExpressionCodeEnsureVariable( if_coro_advance.expression, names, function_context );

	const ClassPtr coro_class_type= coro_expr->type.GetClassType();
	if( coro_class_type == nullptr || coro_class_type->coroutine_type_description == std::nullopt )
	{
		REPORT_ERROR( IfCoroAdvanceForNonCoroutineValue, names.GetErrors(), if_coro_advance.src_loc_, coro_expr->type );
		return BlockBuildInfo();
	}

	if( coro_expr->value_type == ValueType::ReferenceImut )
	{
		REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), if_coro_advance.src_loc_ );
		return BlockBuildInfo();
	}

	 // TODO - maybe disallow this operator for temporary coroutines?
	const VariableMutPtr coro_expr_lock=
		std::make_shared<Variable>(
			coro_expr->type,
			ValueType::ReferenceMut,
			Variable::Location::Pointer,
			coro_expr->name + "lock" );
	function_context.variables_state.AddNode( coro_expr_lock );
	function_context.variables_state.TryAddLink( coro_expr, coro_expr_lock, names.GetErrors(), if_coro_advance.src_loc_ );
	variables_storage.RegisterVariable( coro_expr_lock );

	ReferencesGraph variables_state_before_branching= function_context.variables_state;

	llvm::Value* const coro_handle=
		function_context.llvm_ir_builder.CreateLoad( llvm::PointerType::get( llvm_context_, 0 ), coro_expr->llvm_value, false, "coro_handle" );

	/* for generators code looks like this:
			if( !llvm.coro.done( coro_handle ) )
			{
				llvm.coro.resume( coro_handle )
				if( !llvm.coro.done( coro_handle ) )
				{
					auto promise= llvm.coro.promise( coro_handle );
				}
			}
		// TODO - adopt this code for async functions (that return value only once and when they are done).
	 */

	llvm::Value* const done= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_done ),
		{ coro_handle },
		"coro_done" );

	const auto alternative_block= llvm::BasicBlock::Create( llvm_context_, "after_if_coro_advance" );

	const auto not_done_block= llvm::BasicBlock::Create( llvm_context_, "coro_not_done" );
	function_context.llvm_ir_builder.CreateCondBr( done, alternative_block, not_done_block );

	// Not done block.
	function_context.function->getBasicBlockList().push_back( not_done_block );
	function_context.llvm_ir_builder.SetInsertPoint( not_done_block );

	function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_resume ),
		{ coro_handle } );

	llvm::Value* const done_after_resume= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_done ),
		{ coro_handle },
		"coro_done_after_resume" );

	const auto not_done_after_resume_block= llvm::BasicBlock::Create( llvm_context_, "not_done_after_resume" );
	function_context.llvm_ir_builder.CreateCondBr( done_after_resume, alternative_block, not_done_after_resume_block );

	// Not done after resume block.
	function_context.function->getBasicBlockList().push_back( not_done_after_resume_block );
	function_context.llvm_ir_builder.SetInsertPoint( not_done_after_resume_block );

	EnsureTypeComplete( coro_class_type->coroutine_type_description->return_type );

	llvm::Type* const promise_llvm_type=
		coro_class_type->coroutine_type_description->return_value_type == ValueType::Value
			? coro_class_type->coroutine_type_description->return_type.GetLLVMType()
			: llvm::PointerType::get( llvm_context_, 0 );

	llvm::Value* const promise=
		function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_promise ),
			{
				coro_handle,
				llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, data_layout_.getABITypeAlignment( promise_llvm_type ) ) ),
				llvm::ConstantInt::getFalse( llvm_context_ ),
			},
			"promise" );

	BlockBuildInfo if_block_build_info;
	{
		StackVariablesStorage coro_result_variables_storage( function_context );

		const Type& result_type= coro_class_type->coroutine_type_description->return_type;
		const ValueType result_value_type= coro_class_type->coroutine_type_description->return_value_type;

		const VariableMutPtr variable_reference=
			std::make_shared<Variable>(
				result_type,
				if_coro_advance.mutability_modifier == MutabilityModifier::Mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				if_coro_advance.variable_name );
		// Do not forget to remove node in case of error-return!!!
		function_context.variables_state.AddNode( variable_reference );

		if( result_value_type == ValueType::Value )
		{
			// Create variable for value result of coroutine.
			const VariableMutPtr variable=
				std::make_shared<Variable>(
					result_type,
					ValueType::Value,
					Variable::Location::Pointer,
					if_coro_advance.variable_name + " variable itself",
					promise );
			function_context.variables_state.AddNode( variable );

			variable->llvm_value->setName( if_coro_advance.variable_name );

			debug_info_builder_->CreateVariableInfo( *variable, if_coro_advance.variable_name, if_coro_advance.src_loc_, function_context );

			if( !variable->type.CanBeConstexpr() )
				function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

			variable_reference->llvm_value= variable->llvm_value;

			coro_result_variables_storage.RegisterVariable( variable );
			function_context.variables_state.AddLink( variable, variable_reference );

			if( result_type.ReferencesTagsCount() > 0 )
			{
				const auto accessible_innder_nodes= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( coro_expr_lock );
				if( !accessible_innder_nodes.empty() )
				{
					bool inner_reference_is_mutable= false;
					for( const VariablePtr& accessible_inner_node : accessible_innder_nodes )
						inner_reference_is_mutable|= accessible_inner_node->value_type == ValueType::ReferenceMut;

					const VariablePtr inner_node=
						function_context.variables_state.CreateNodeInnerReference( variable, inner_reference_is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut );

					for( const VariablePtr& accessible_inner_node : accessible_innder_nodes )
						function_context.variables_state.TryAddLink( accessible_inner_node, inner_node, names.GetErrors(), if_coro_advance.src_loc_ );
				}
			}

			// TODO - maybe create additional reference node here in case of reference modifier for target variable?
		}
		else
		{
			llvm::Value* const coroutine_reference_result= CreateTypedReferenceLoad( function_context, result_type, promise );
			if( if_coro_advance.reference_modifier == ReferenceModifier::None )
			{
				// Create variable and copy into it reference result of coroutine.

				if( result_type.IsAbstract() )
					REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), if_coro_advance.src_loc_, result_type );

				const VariableMutPtr variable=
					std::make_shared<Variable>(
						result_type,
						ValueType::Value,
						Variable::Location::Pointer,
						if_coro_advance.variable_name + " variable itself",
						function_context.alloca_ir_builder.CreateAlloca( result_type.GetLLVMType(), nullptr, if_coro_advance.variable_name ) );
				function_context.variables_state.AddNode( variable );

				CreateLifetimeStart( function_context, variable->llvm_value );
				debug_info_builder_->CreateVariableInfo( *variable, if_coro_advance.variable_name, if_coro_advance.src_loc_, function_context );

				if( !result_type.CanBeConstexpr() )
					function_context.have_non_constexpr_operations_inside= true; // Declaring variable with non-constexpr type in constexpr function not allowed.

				if( !result_type.IsCopyConstructible() )
					REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), if_coro_advance.src_loc_, result_type );
				else
					BuildCopyConstructorPart(
						variable->llvm_value, coroutine_reference_result,
						variable->type,
						function_context );

				variable_reference->llvm_value= variable->llvm_value;

				coro_result_variables_storage.RegisterVariable( variable );
				function_context.variables_state.AddLink( variable, variable_reference );

				//No need to setup references here, because we can't return from generator reference to type with references inside.
			}
			else
			{
				// Create reference to reference result of coroutine.

				if( result_value_type == ValueType::ReferenceImut && variable_reference->value_type != ValueType::ReferenceImut )
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), if_coro_advance.src_loc_ );

				variable_reference->llvm_value= coroutine_reference_result;

				for( const VariablePtr& internal_reference_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( coro_expr ) )
					function_context.variables_state.TryAddLink( internal_reference_node, variable_reference, names.GetErrors(), if_coro_advance.src_loc_ );
			}
		}

		if( IsKeyword( if_coro_advance.variable_name ) )
			REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), if_coro_advance.src_loc_ );

		coro_result_variables_storage.RegisterVariable( variable_reference );

		NamesScope variable_names_scope( "", &names );
		variable_names_scope.AddName( if_coro_advance.variable_name, NamesScopeValue( variable_reference, if_coro_advance.src_loc_ ) );

		// Reuse variable names scope for block.
		if_block_build_info= BuildBlockElements( variable_names_scope, function_context, if_coro_advance.block.elements_ );
		if( !if_block_build_info.have_terminal_instruction_inside )
		{
			// Destroy coro result variable.
			CallDestructors( coro_result_variables_storage, variable_names_scope, function_context, if_coro_advance.src_loc_ );
		}

		CheckForUnusedLocalNames( variable_names_scope );
	}

	llvm::SmallVector<ReferencesGraph, 2> branches_variable_states;

	BlockBuildInfo block_build_info;

	if( if_coro_advance.alternative == nullptr )
	{
		if( !if_block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( alternative_block );
			branches_variable_states.push_back( function_context.variables_state );
		}
		branches_variable_states.push_back( std::move( variables_state_before_branching ) );

		block_build_info.have_terminal_instruction_inside= false;

		function_context.function->getBasicBlockList().push_back( alternative_block );
		function_context.llvm_ir_builder.SetInsertPoint( alternative_block );

		// Destroy temporarie in coroutine expression.
		CallDestructors( variables_storage, names, function_context, if_coro_advance.end_src_loc );
	}
	else
	{
		alternative_block->setName( "if_coro_advance_else" );
		llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_, "after_if_coro_advance" );

		if( !if_block_build_info.have_terminal_instruction_inside )
		{
			// Destroy temporarie in coroutine expression.
			CallDestructors( variables_storage, names, function_context, if_coro_advance.end_src_loc );

			function_context.llvm_ir_builder.CreateBr( block_after_if );
			branches_variable_states.push_back( function_context.variables_state );
		}

		// Else block.
		function_context.function->getBasicBlockList().push_back( alternative_block );
		function_context.llvm_ir_builder.SetInsertPoint( alternative_block );

		function_context.variables_state= std::move( variables_state_before_branching );

		// Destroy temporarie in coroutine expression.
		CallDestructors( variables_storage, names, function_context, if_coro_advance.end_src_loc );

		const BlockBuildInfo alternative_block_build_info= BuildIfAlternative( names, function_context, *if_coro_advance.alternative );

		if( !alternative_block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( block_after_if );
			branches_variable_states.push_back( function_context.variables_state );
		}

		block_build_info.have_terminal_instruction_inside=
			if_block_build_info.have_terminal_instruction_inside && alternative_block_build_info.have_terminal_instruction_inside;

		if( !block_build_info.have_terminal_instruction_inside )
		{
			function_context.function->getBasicBlockList().push_back( block_after_if );
			function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
		}
		else
			delete block_after_if;
	}

	function_context.variables_state= MergeVariablesStateAfterIf( branches_variable_states, names.GetErrors(), if_coro_advance.end_src_loc );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::SwitchOperator& switch_operator )
{
	Type switch_type;
	llvm::Value* switch_value= nullptr;
	{
		const StackVariablesStorage temp_variables_storage( function_context );
		const VariablePtr expression= BuildExpressionCodeEnsureVariable( switch_operator.value, names, function_context );

		bool type_ok= false;
		if( expression->type.GetEnumType() != nullptr )
			type_ok= true;
		else if( const auto fundamental_type= expression->type.GetFundamentalType() )
			type_ok= IsInteger( fundamental_type->fundamental_type ) || IsChar( fundamental_type->fundamental_type );
		else
			type_ok= false;

		const SrcLoc src_loc= Synt::GetExpressionSrcLoc( switch_operator.value );

		if( !type_ok )
		{
			REPORT_ERROR(
				TypesMismatch,
				names.GetErrors(),
				src_loc,
				"Enum, integer or char type",
				expression->type );
			return BlockBuildInfo();
		}

		switch_type= expression->type;
		switch_value= CreateMoveToLLVMRegisterInstruction( *expression, function_context );

		CallDestructors( temp_variables_storage, names, function_context, src_loc );
	}

	llvm::APInt type_low, type_high;
	bool is_signed= false;
	if( const auto enum_= switch_type.GetEnumType() )
	{
		const uint32_t size_in_bits= 8 * uint32_t(enum_->underlaying_type.GetSize());
		type_low= llvm::APInt( size_in_bits, uint64_t(0) );
		type_high= llvm::APInt( size_in_bits, uint64_t(enum_->element_count - 1) );
	}
	else if( const auto fundamental_type= switch_type.GetFundamentalType() )
	{
		const uint32_t size_in_bits= 8 * uint32_t(fundamental_type->GetSize());
		if( IsSignedInteger( fundamental_type->fundamental_type ) )
		{
			is_signed= true;
			type_low = llvm::APInt::getSignedMinValue( size_in_bits );
			type_high= llvm::APInt::getSignedMaxValue( size_in_bits );
		}
		else
		{
			// Unsigned integers and chars (chars are unsigned).
			type_low = llvm::APInt::getMinValue( size_in_bits );
			type_high= llvm::APInt::getMaxValue( size_in_bits );
		}
	}
	else U_ASSERT(false);

	struct CaseRange
	{
		llvm::APInt low;
		llvm::APInt high;
		SrcLoc src_loc;
	};

	// Preevaluate case values. It is fine, since only constexpr expressions are allowed.
	using CaseValues= llvm::SmallVector<CaseRange, 4>;
	constexpr size_t c_expected_num_branches= 16;
	llvm::SmallVector<CaseValues, c_expected_num_branches> branches_ranges;
	bool all_cases_are_ok= true;
	const Synt::Block* default_branch_synt_block= nullptr;
	branches_ranges.reserve( switch_operator.cases.size() );
	{
		const StackVariablesStorage temp_variables_storage( function_context );
		for( const Synt::SwitchOperator::Case& case_ : switch_operator.cases )
		{
			CaseValues case_values;
			if( const auto values = std::get_if<std::vector<Synt::SwitchOperator::CaseValue>>( &case_.values ) )
			{
				case_values.reserve( values->size() );
				for( const Synt::SwitchOperator::CaseValue& value : *values )
				{
					if( const auto single_value= std::get_if<Synt::Expression>( &value ) )
					{
						const VariablePtr expression_variable= BuildExpressionCodeEnsureVariable( *single_value, names, function_context );
						const auto src_loc= Synt::GetExpressionSrcLoc( *single_value );
						if( expression_variable->type != switch_type )
						{
							REPORT_ERROR(
								TypesMismatch,
								names.GetErrors(),
								src_loc,
								switch_type,
								expression_variable->type );
							all_cases_are_ok= false;
							continue;
						}
						if( expression_variable->constexpr_value == nullptr )
						{
							REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), src_loc );
							all_cases_are_ok= false;
							continue;
						}
						const llvm::APInt value_int= expression_variable->constexpr_value->getUniqueInteger();
						case_values.emplace_back( CaseRange{ value_int, value_int, src_loc } );
					}
					else if( const auto range= std::get_if<Synt::SwitchOperator::CaseRange>( &value ) )
					{
						llvm::APInt range_constants[2]{ type_low, type_high };
						for( size_t i= 0; i < 2; ++i )
						{
							const Synt::Expression& expression = i == 0 ? range->low : range->high;
							if( std::get_if<Synt::EmptyVariant>( &expression ) != nullptr )
								continue;

							const VariablePtr expression_variable= BuildExpressionCodeEnsureVariable( expression, names, function_context );
							if( expression_variable->type != switch_type )
							{
								REPORT_ERROR(
									TypesMismatch,
									names.GetErrors(),
									Synt::GetExpressionSrcLoc( expression ),
									switch_type,
									expression_variable->type );
								all_cases_are_ok= false;
								continue;
							}
							if( expression_variable->constexpr_value == nullptr )
							{
								REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ) );
								all_cases_are_ok= false;
								continue;
							}
							range_constants[i]= expression_variable->constexpr_value->getUniqueInteger();
						}

						const SrcLoc src_loc= case_.block.src_loc_; // TODO - use proper src_loc;
						if( !( is_signed ? range_constants[0].sle( range_constants[1] ) : range_constants[0].ule( range_constants[1] ) ) )
						{
							REPORT_ERROR(
								SwitchInvalidRange,
								names.GetErrors(),
								src_loc,
								range_constants[0].getLimitedValue(),
								range_constants[1].getLimitedValue() );
							all_cases_are_ok= false;
							continue;
						}

						case_values.emplace_back( CaseRange{ range_constants[0], range_constants[1], src_loc } );
					}
					else U_ASSERT(false);
				}
			}
			else if( std::get_if<Synt::SwitchOperator::DefaultPlaceholder>( &case_.values ) != nullptr )
			{
				if( default_branch_synt_block != nullptr )
				{
					REPORT_ERROR(
						SwitchDuplicatedDefaultLabel,
						names.GetErrors(),
						case_.block.src_loc_ ); // TODO - use proper src_loc
					all_cases_are_ok= false;
				}
				else
					default_branch_synt_block= &case_.block;
			}
			else U_ASSERT( false );

			branches_ranges.push_back( std::move(case_values) );
		}
		CallDestructors( temp_variables_storage, names, function_context, switch_operator.src_loc_ );
	}

	if( !all_cases_are_ok || branches_ranges.size() != switch_operator.cases.size() )
		return BlockBuildInfo(); // Some error generated before.

	// Perform checks of ranges.
	{
		// Collect all ranges.
		llvm::SmallVector<CaseRange, 32> all_ranges;
		for( const auto& case_ranges : branches_ranges )
			all_ranges.append( case_ranges );

		// Sort by low. Use comparator depending on switch type signness.
		// Sorting is needed in order to simplify ranges overlapping and gaps searching.
		if( is_signed )
			std::sort(
				all_ranges.begin(), all_ranges.end(),
				[]( const CaseRange& l, const CaseRange& r ) -> bool { return l.low.slt(r.low); });
		else
			std::sort(
				all_ranges.begin(), all_ranges.end(),
				[]( const CaseRange& l, const CaseRange& r ) -> bool { return l.low.ult(r.low); });

		// Check for overlaps and gaps between ranges.
		bool has_gaps= false;
		for( size_t i = 0; i + 1 < all_ranges.size(); ++i )
		{
			const CaseRange& current_range= all_ranges[i];
			const CaseRange& next_range= all_ranges[i + 1];

			const llvm::APInt& current_high= current_range.high;
			const llvm::APInt& next_low= next_range.low;

			if( is_signed ? current_high.slt(next_low) : current_high.ult(next_low) )
			{
				// Because of condition above diff is always non-negative.
				const llvm::APInt gap_size= next_low - current_high - 1;
				if( !gap_size.isZero() )
				{
					has_gaps= true;
					if( default_branch_synt_block == nullptr )
					{
						if( gap_size.isOne() )
							REPORT_ERROR(
								SwitchUndhandledValue,
								names.GetErrors(),
								std::max( current_range.src_loc, next_range.src_loc ), // Report error furter in the source code.
								(current_high + 1).getLimitedValue() );
						else
							REPORT_ERROR(
								SwitchUndhandledRange,
								names.GetErrors(),
								std::max( current_range.src_loc, next_range.src_loc ), // Report error furter in the source code.
								(current_high + 1).getLimitedValue(),
								(next_low - 1).getLimitedValue() );
					}
				}
			}
			else
				REPORT_ERROR(
					SwitchRangesOverlapping,
					names.GetErrors(),
					std::max( current_range.src_loc, next_range.src_loc ), // Report error furter in the source code.
					current_range.low .getLimitedValue(),
					current_range.high.getLimitedValue(),
					next_range.low .getLimitedValue(),
					next_range.high.getLimitedValue() );
		}

		if( !all_ranges.empty() )
		{
			// Process begin range.
			const llvm::APInt& first_range_low= all_ranges.front().low;
			const llvm::APInt begin_gap_size= first_range_low - type_low;
			if( !begin_gap_size.isZero() )
			{
				has_gaps= true;
				if( default_branch_synt_block == nullptr )
				{
					if( begin_gap_size.isOne() )
						REPORT_ERROR(
							SwitchUndhandledValue,
							names.GetErrors(),
							all_ranges.front().src_loc,
							type_low.getLimitedValue() );
					else
						REPORT_ERROR(
							SwitchUndhandledRange,
							names.GetErrors(),
							all_ranges.front().src_loc,
							type_low.getLimitedValue(),
							(first_range_low - 1).getLimitedValue() );
				}
			}
			// Process end range.
			const llvm::APInt& last_range_high= all_ranges.back().high;
			const llvm::APInt end_gap_size= type_high - last_range_high;
			if( !end_gap_size.isZero() )
			{
				has_gaps= true;
				if( default_branch_synt_block == nullptr )
				{
					if( end_gap_size.isOne() )
						REPORT_ERROR(
							SwitchUndhandledValue,
							names.GetErrors(),
							all_ranges.back().src_loc,
							type_high.getLimitedValue() );
					else
						REPORT_ERROR(
							SwitchUndhandledRange,
							names.GetErrors(),
							all_ranges.back().src_loc,
							(last_range_high + 1).getLimitedValue(),
							type_high.getLimitedValue() );
				}
			}
		}
		else
		{
			// No ranges at all - this is a gap.
			has_gaps= true;
			if( default_branch_synt_block == nullptr )
			{
				if( type_high == type_low )
					REPORT_ERROR(
						SwitchUndhandledValue,
						names.GetErrors(),
						switch_operator.src_loc_,
						type_high.getLimitedValue() );
				else
					REPORT_ERROR(
						SwitchUndhandledRange,
						names.GetErrors(),
						switch_operator.src_loc_,
						type_low.getLimitedValue(),
						type_high.getLimitedValue() );
			}
		}

		if( default_branch_synt_block != nullptr && !has_gaps )
			REPORT_ERROR(
				SwithcUnreachableDefaultBranch,
				names.GetErrors(),
				default_branch_synt_block->src_loc_ );
	}

	const ReferencesGraph variables_state_before_branching= function_context.variables_state;
	llvm::SmallVector<ReferencesGraph, c_expected_num_branches> breances_states_after_case;
	breances_states_after_case.reserve( switch_operator.cases.size() + 1 );

	llvm::BasicBlock* const block_after_switch= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* next_case_block= nullptr;
	llvm::BasicBlock* const default_branch= default_branch_synt_block == nullptr ? nullptr : llvm::BasicBlock::Create( llvm_context_ );
	bool all_branches_are_terminal= true;

	llvm::Type* const switch_llvm_type= switch_type.GetLLVMType();
	for( size_t i= 0; i < switch_operator.cases.size(); ++i )
	{
		const Synt::SwitchOperator::Case& case_= switch_operator.cases[i];
		if( std::get_if<Synt::SwitchOperator::DefaultPlaceholder>( &case_.values ) != nullptr )
		{
			// Default branch - handle it later.
			continue;
		}

		llvm::BasicBlock* const case_handle_block= llvm::BasicBlock::Create( llvm_context_ );
		next_case_block= llvm::BasicBlock::Create( llvm_context_ );

		const CaseValues& case_values= branches_ranges[i];
		U_ASSERT( !case_values.empty() );
		llvm::Value* value_equals= nullptr;
		for( const CaseRange& case_range : case_values )
		{
			llvm::Value* current_value_equals= nullptr;

			llvm::Constant* const constant_low = llvm::ConstantInt::get( switch_llvm_type, case_range.low  );

			if( case_range.low == case_range.high )
				current_value_equals= function_context.llvm_ir_builder.CreateICmpEQ( switch_value, constant_low );
			else
			{
				llvm::Constant* const constant_high= llvm::ConstantInt::get( switch_llvm_type, case_range.high );
				if( is_signed )
					current_value_equals=
						function_context.llvm_ir_builder.CreateAnd(
							function_context.llvm_ir_builder.CreateICmpSGE( switch_value, constant_low ),
							function_context.llvm_ir_builder.CreateICmpSLE( switch_value, constant_high ) );
				else
					current_value_equals=
						function_context.llvm_ir_builder.CreateAnd(
							function_context.llvm_ir_builder.CreateICmpUGE( switch_value, constant_low ),
							function_context.llvm_ir_builder.CreateICmpULE( switch_value, constant_high ) );
			}

			if( value_equals == nullptr )
				value_equals= current_value_equals;
			else
				value_equals= function_context.llvm_ir_builder.CreateOr( value_equals, current_value_equals );
		}

		function_context.llvm_ir_builder.CreateCondBr( value_equals, case_handle_block, next_case_block );

		// Case handle block
		function_context.function->getBasicBlockList().push_back( case_handle_block );
		function_context.llvm_ir_builder.SetInsertPoint( case_handle_block );

		function_context.variables_state= variables_state_before_branching;
		const BlockBuildInfo block_build_info= BuildBlock( names, function_context, case_.block );

		if( !block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( block_after_switch );
			breances_states_after_case.push_back( function_context.variables_state );
			all_branches_are_terminal= false;
		}

		// Next case block
		function_context.function->getBasicBlockList().push_back( next_case_block );
		function_context.llvm_ir_builder.SetInsertPoint( next_case_block );
	}

	// Handle default branch lastly.
	if( default_branch_synt_block != nullptr && default_branch != nullptr )
	{
		if( next_case_block == nullptr )
			function_context.llvm_ir_builder.CreateBr( default_branch );
		else
		{
			next_case_block->replaceAllUsesWith( default_branch );
			next_case_block->eraseFromParent();
		}

		function_context.function->getBasicBlockList().push_back( default_branch );
		function_context.llvm_ir_builder.SetInsertPoint( default_branch );

		function_context.variables_state= variables_state_before_branching;
		const BlockBuildInfo block_build_info= BuildBlock( names, function_context, *default_branch_synt_block );

		if( !block_build_info.have_terminal_instruction_inside )
		{
			function_context.llvm_ir_builder.CreateBr( block_after_switch );
			breances_states_after_case.push_back( function_context.variables_state );
			all_branches_are_terminal= false;
		}
	}
	else
	{
		if( next_case_block == nullptr )
			function_context.llvm_ir_builder.CreateBr( block_after_switch );
		else
		{
			next_case_block->replaceAllUsesWith( block_after_switch );
			next_case_block->eraseFromParent();
		}

		// No default branch - all values must be handled in normal branches.
		// This was checked after ranges calculation.
	}

	BlockBuildInfo block_build_info;

	if( all_branches_are_terminal )
	{
		block_build_info.have_terminal_instruction_inside= true;
		// There is no reason to merge variables state here.

		// Hack - preserve LLVM basic blocks structure in case of impossible jump to block after swith.
		if( block_after_switch->hasNPredecessorsOrMore(1) )
		{
			function_context.function->getBasicBlockList().push_back( block_after_switch );
			function_context.llvm_ir_builder.SetInsertPoint( block_after_switch );
			function_context.llvm_ir_builder.CreateUnreachable();
		}
		else
			delete block_after_switch;
	}
	else
	{
		function_context.function->getBasicBlockList().push_back( block_after_switch );
		function_context.llvm_ir_builder.SetInsertPoint( block_after_switch );
		function_context.variables_state= MergeVariablesStateAfterIf( breances_states_after_case, names.GetErrors(), switch_operator.end_src_loc );
	}

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::SingleExpressionOperator& single_expression_operator )
{
	const StackVariablesStorage temp_variables_storage( function_context );
	const Value value= BuildExpressionCode( single_expression_operator.expression_, names, function_context );
	CallDestructors( temp_variables_storage, names, function_context, single_expression_operator.src_loc_ );

	if( const auto variable_ptr= value.GetVariable() )
	{
		if( SingleExpressionIsUseless( single_expression_operator.expression_ ) &&
			!( variable_ptr->type == void_type_ && variable_ptr->value_type == ValueType::Value ) )
			REPORT_ERROR( UselessExpressionRoot, names.GetErrors(), Synt::GetExpressionSrcLoc( single_expression_operator.expression_ ) );
	}
	else if(
		value.GetFunctionsSet() != nullptr ||
		value.GetTypeName() != nullptr ||
		value.GetClassField() != nullptr ||
		value.GetThisOverloadedMethodsSet() != nullptr ||
		value.GetNamespace() != nullptr ||
		value.GetTypeTemplatesSet() != nullptr )
		REPORT_ERROR( UselessExpressionRoot, names.GetErrors(), Synt::GetExpressionSrcLoc( single_expression_operator.expression_ ) );

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
		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( assignment_operator.r_value_, names, function_context );

		const auto r_var_in_register= CreateMoveToLLVMRegisterInstruction( *r_var, function_context );

		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), assignment_operator.src_loc_ ); // Destroy temporaries of right expression.

		// Evaluate left part.
		const VariablePtr l_var= BuildExpressionCodeEnsureVariable( assignment_operator.l_value_, names, function_context );

		if( l_var->type == invalid_type_ || r_var->type == invalid_type_ )
			return BlockBuildInfo();

		if( l_var->value_type != ValueType::ReferenceMut )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), assignment_operator.src_loc_ );
			return BlockBuildInfo();
		}
		if( l_var->type != r_var->type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), assignment_operator.src_loc_, l_var->type, r_var->type );
			return BlockBuildInfo();
		}

		// Check references of destination.
		if( function_context.variables_state.HaveOutgoingLinks( l_var ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), assignment_operator.src_loc_, l_var->name );

		if( l_var->type.GetFundamentalType() != nullptr ||
			l_var->type.GetEnumType() != nullptr ||
			l_var->type.GetRawPointerType() != nullptr ||
			l_var->type.GetFunctionPointerType() != nullptr )
		{
			if( l_var->location != Variable::Location::Pointer )
			{
				U_ASSERT(false);
				return BlockBuildInfo();
			}
			CreateTypedStore( function_context, r_var->type, r_var_in_register, l_var->llvm_value );
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), assignment_operator.src_loc_, l_var->type );
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
		VariablePtr r_var=
			BuildExpressionCodeEnsureVariable(
				additive_assignment_operator.r_value_,
				names,
				function_context );

		if( r_var->type.GetFundamentalType() != nullptr || r_var->type.GetRawPointerType() != nullptr )
		{
			// We must read value, because referenced by reference value may be changed in l_var evaluation.
			r_var=
				std::make_shared<Variable>(
					r_var->type,
					ValueType::Value,
					Variable::Location::LLVMRegister,
					r_var->name + " in register",
					r_var->location == Variable::Location::LLVMRegister
						? r_var->llvm_value
						: CreateMoveToLLVMRegisterInstruction( *r_var, function_context ),
					r_var->constexpr_value );
		}
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), additive_assignment_operator.src_loc_ ); // Destroy temporaries of right expression.

		const VariablePtr l_var=
			BuildExpressionCodeEnsureVariable(
				additive_assignment_operator.l_value_,
				names,
				function_context );

		if( l_var->type == invalid_type_ || r_var->type == invalid_type_ )
			return BlockBuildInfo();

		if( l_var->value_type != ValueType::ReferenceMut )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), additive_assignment_operator.src_loc_ );
			return BlockBuildInfo();
		}

		// Check references of destination.
		if( function_context.variables_state.HaveOutgoingLinks( l_var ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), additive_assignment_operator.src_loc_, l_var->name );

		// Allow additive assignment operators only for fundamentals and raw pointers.
		if( !( l_var->type.GetFundamentalType() != nullptr || l_var->type.GetRawPointerType() != nullptr ) )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), additive_assignment_operator.src_loc_, l_var->type );
			return BlockBuildInfo();
		}

		// Generate binary operator and assignment for fundamental types.
		const Value operation_result_value=
			BuildBinaryOperator(
				*l_var, *r_var,
				additive_assignment_operator.additive_operation_,
				additive_assignment_operator.src_loc_,
				names,
				function_context );
		if( operation_result_value.GetVariable() == nullptr ) // Not variable in case of error.
			return BlockBuildInfo();

		const Variable& operation_result= *operation_result_value.GetVariable();

		if( operation_result.type != l_var->type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), additive_assignment_operator.src_loc_, l_var->type, operation_result.type );
			return BlockBuildInfo();
		}

		U_ASSERT( l_var->location == Variable::Location::Pointer );
		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( operation_result, function_context );
		CreateTypedStore( function_context, r_var->type, value_in_register, l_var->llvm_value );
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

	const VariablePtr variable= BuildExpressionCodeEnsureVariable( static_assert_.expression, names, function_context );

	// Destruct temporary variables of right and left expressions.
	// In non-error case, this call produces no code.
	CallDestructors( temp_variables_storage, names, function_context, static_assert_.src_loc_ );

	if( variable->type != bool_type_ )
	{
		REPORT_ERROR( StaticAssertExpressionMustHaveBoolType, names.GetErrors(), static_assert_.src_loc_ );
		return block_info;
	}
	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( StaticAssertExpressionIsNotConstant, names.GetErrors(), static_assert_.src_loc_ );
		return block_info;
	}

	if( !variable->constexpr_value->isOneValue() )
	{
		CodeBuilderError error;
		error.code= CodeBuilderErrorCode::StaticAssertionFailed;
		error.src_loc= static_assert_.src_loc_;
		error.text= static_assert_.message == std::nullopt
				? "Static assertion failed."
				: ("Static assertion failed: " + *static_assert_.message + "." );

		names.GetErrors().push_back( std::move(error) );
	}

	return block_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElementImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TypeAlias& type_alias )
{
	BlockBuildInfo block_info;

	if( IsKeyword( type_alias.name ) )
		REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), type_alias.src_loc_ );

	Type type= PrepareType( type_alias.value, names, function_context );
	if( names.AddName( type_alias.name, NamesScopeValue( std::move(type), type_alias.src_loc_ ) ) == nullptr )
		REPORT_ERROR( Redefinition, names.GetErrors(), type_alias.src_loc_, type_alias.name );

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
	const VariablePtr condition_expression= BuildExpressionCodeEnsureVariable( halt_if.condition, names, function_context );
	const SrcLoc condition_expression_src_loc= Synt::GetExpressionSrcLoc( halt_if.condition );
	if( condition_expression->type!= bool_type_ )
	{
		REPORT_ERROR( TypesMismatch,
			names.GetErrors(),
			condition_expression_src_loc,
			bool_type_,
			condition_expression->type );
		return block_info;
	}

	llvm::Value* const condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition_expression, function_context );
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
	debug_info_builder_->StartBlock( block.src_loc_, function_context );

	NamesScope block_names( "", &names );
	const StackVariablesStorage block_variables_storage( function_context );

	const BlockBuildInfo block_build_info= BuildBlockElements( block_names, function_context, block.elements_ );

	debug_info_builder_->SetCurrentLocation( block.end_src_loc_, function_context );

	// If there are undconditional "break", "continue", "return" operators,
	// we didn`t need call destructors, it must be called in this operators.
	if( !block_build_info.have_terminal_instruction_inside )
		CallDestructors( block_variables_storage, block_names, function_context, block.end_src_loc_ );

	debug_info_builder_->EndBlock( function_context );

	CheckForUnusedLocalNames( block_names );

	return block_build_info;
}

CodeBuilder::BlockBuildInfo CodeBuilder::BuildBlockElements(
	NamesScope& names, FunctionContext& function_context, const Synt::BlockElements& block_elements )
{
	BlockBuildInfo block_build_info;
	size_t block_element_index= 0u;
	for( const Synt::BlockElement& block_element : block_elements )
	{
		++block_element_index;

		const BlockBuildInfo info= BuildBlockElement( names, function_context, block_element );
		if( info.have_terminal_instruction_inside )
		{
			block_build_info.have_terminal_instruction_inside= true;
			break;
		}
	}

	if( block_element_index < block_elements.size() )
		REPORT_ERROR( UnreachableCode, names.GetErrors(), Synt::GetBlockElementSrcLoc( block_elements[ block_element_index ] ) );

	return block_build_info;
}

void CodeBuilder::BuildEmptyReturn( NamesScope& names, FunctionContext& function_context, const SrcLoc& src_loc )
{
	if( !( function_context.return_type == void_type_ && function_context.function_type.return_value_type == ValueType::Value ) )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, void_type_, *function_context.return_type );
		return;
	}

	CallDestructorsBeforeReturn( names, function_context, src_loc );
	CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), src_loc );

	if( function_context.destructor_end_block == nullptr )
		function_context.llvm_ir_builder.CreateRetVoid();
	else
	{
		// In explicit destructor, break to block with destructor calls for class members.
		function_context.llvm_ir_builder.CreateBr( function_context.destructor_end_block );
	}
}

void CodeBuilder::AddLoopFrame(
	NamesScope& names,
	FunctionContext& function_context,
	llvm::BasicBlock* const break_block,
	llvm::BasicBlock* const continue_block,
	const std::optional<Synt::Label>& label )
{
	LoopFrame loop_frame;
	loop_frame.block_for_break= break_block;
	loop_frame.block_for_continue= continue_block;
	loop_frame.stack_variables_stack_size= function_context.stack_variables_stack.size();

	if( label != std::nullopt )
	{
		const std::string& label_name= label->name;

		if( IsKeyword( label_name ) )
			REPORT_ERROR( UsingKeywordAsName, names.GetErrors(), label->src_loc_ );

		for( const LoopFrame& prev_frame : function_context.loops_stack )
			if( prev_frame.name == label_name )
				REPORT_ERROR( Redefinition, names.GetErrors(), label->src_loc_, label_name );

		loop_frame.name= label_name;

		if( break_block != nullptr )
			break_block->setName( label_name + "_break" );

		if( continue_block != nullptr )
			continue_block->setName( label_name + "_continue" );
	}

	function_context.loops_stack.push_back( std::move(loop_frame) );
}

LoopFrame* CodeBuilder::FetchLoopFrame( NamesScope& names, FunctionContext& function_context, const std::optional<Synt::Label>& label )
{
	if( label != std::nullopt )
	{
		LoopFrame* loop_frame= nullptr;
		for( LoopFrame& check_loop_frame : function_context.loops_stack )
			if( check_loop_frame.name == label->name )
			{
				loop_frame= &check_loop_frame;
				break;
			}

		if( loop_frame == nullptr )
			REPORT_ERROR( NameNotFound, names.GetErrors(), label->src_loc_, label->name );

		return loop_frame;
	}
	else
	{
		// In case of "break" or "continue" without label skip non-loop frames (block frames, where only "break" is available).
		for( auto it= function_context.loops_stack.rbegin(); it != function_context.loops_stack.rend(); ++it )
		{
			const bool is_loop_frame= it->block_for_continue != nullptr;
			if( is_loop_frame )
				return &*it;
		}
		return nullptr;
	}
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
	const VariablePtr variable= value.GetVariable();
	if( variable == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, block_names.GetErrors(), src_loc, value.GetKindName() );
		return;
	}

	if( variable->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, block_names.GetErrors(), src_loc );
		return;
	}

	if( function_context.variables_state.HaveOutgoingLinks( variable ) )
		REPORT_ERROR( ReferenceProtectionError, block_names.GetErrors(), src_loc, variable->name );

	ArgsVector<FunctionType::Param> args;
	args.emplace_back();
	args.back().type= variable->type;
	args.back().value_type= variable->value_type;
	const FunctionVariable* const overloaded_operator=
		GetOverloadedOperator( args, positive ? OverloadedOperator::Increment : OverloadedOperator::Decrement, block_names, src_loc );
	if( overloaded_operator != nullptr )
	{
		if( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::NonConstexpr )
			function_context.have_non_constexpr_operations_inside= true;

		const auto fetch_result= TryFetchVirtualFunction( variable, *overloaded_operator, function_context, block_names.GetErrors(), src_loc );
		DoCallFunction( fetch_result.second, overloaded_operator->type, src_loc, fetch_result.first, {}, false, block_names, function_context );
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
		CreateTypedStore( function_context, variable->type, new_value, variable->llvm_value );
	}
	else if( const auto raw_poiter_type= variable->type.GetRawPointerType() )
	{
		if( !EnsureTypeComplete( raw_poiter_type->element_type ) )
		{
			REPORT_ERROR( UsingIncompleteType, block_names.GetErrors(), src_loc, raw_poiter_type->element_type );
			return;
		}

		llvm::Value* const ptr_value= CreateMoveToLLVMRegisterInstruction( *variable, function_context );
		llvm::Value* const one= llvm::ConstantInt::get( fundamental_llvm_types_.int_ptr, positive ? uint64_t(1u) : ~uint64_t(0), true );
		llvm::Value* const new_value= function_context.llvm_ir_builder.CreateGEP( raw_poiter_type->element_type.GetLLVMType(), ptr_value, one );

		U_ASSERT( variable->location == Variable::Location::Pointer );
		CreateTypedStore( function_context, variable->type, new_value, variable->llvm_value );
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, block_names.GetErrors(), src_loc, variable->type );
		return;
	}

	CallDestructors( temp_variables_storage, block_names, function_context, src_loc );
}

} // namespace U
