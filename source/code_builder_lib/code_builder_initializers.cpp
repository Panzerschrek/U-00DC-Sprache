#include <set>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "../lex_synt_lib/lang_types.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

llvm::Constant* CodeBuilder::ApplyInitializer(
	const Variable& variable,
	const Synt::Initializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	struct Visitor final : public boost::static_visitor<llvm::Constant*>
	{
		CodeBuilder& this_;
		const Variable& variable;
		NamesScope& block_names;
		FunctionContext& function_context;

		Visitor( CodeBuilder& in_this, const Variable& in_variable, NamesScope& in_block_names, FunctionContext& in_function_context )
			: this_(in_this), variable(in_variable), block_names(in_block_names), function_context(in_function_context)
		{}

		llvm::Constant* operator()( const Synt::EmptyVariant& )
		{
			return nullptr;
		}
		llvm::Constant* operator()( const Synt::ArrayInitializer& array_initializer )
		{
			return this_.ApplyArrayInitializer( variable, array_initializer, block_names, function_context );
		}
		llvm::Constant* operator()( const Synt::StructNamedInitializer& struct_named_initializer )
		{
			return this_.ApplyStructNamedInitializer( variable, struct_named_initializer, block_names, function_context );
		}
		llvm::Constant* operator()( const Synt::ConstructorInitializer& constructor_initializer )
		{
			return this_.ApplyConstructorInitializer( variable, constructor_initializer.call_operator, block_names, function_context );
		}
		llvm::Constant* operator()( const Synt::ExpressionInitializer& expression_initializer )
		{
			return this_.ApplyExpressionInitializer( variable, expression_initializer, block_names, function_context );
		}
		llvm::Constant* operator()( const Synt::ZeroInitializer& zero_initializer )
		{
			return this_.ApplyZeroInitializer( variable, zero_initializer, block_names, function_context );
		}
		llvm::Constant* operator()( const Synt::UninitializedInitializer& uninitialized_initializer )
		{
			return this_.ApplyUninitializedInitializer( variable, uninitialized_initializer, block_names, function_context );
		}
	};

	Visitor visitor( *this, variable, block_names, function_context );
	return boost::apply_visitor( visitor, initializer );
}

void CodeBuilder::ApplyEmptyInitializer(
	const ProgramString& variable_name,
	const FilePos& file_pos,
	const Variable& variable,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( !variable.type.IsDefaultConstructible() )
	{
		REPORT_ERROR( ExpectedInitializer, block_names.GetErrors(), file_pos, variable_name );
		return;
	}

	if( variable.type.GetFundamentalType() != nullptr || variable.type.GetEnumType() != nullptr )
	{
		// Fundamentals and enums are not default-constructible, we should generate error about it before.
		U_ASSERT( false );
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		GenerateLoop(
			array_type->ArraySizeOrZero(),
			[&](llvm::Value* const counter_value)
			{
				array_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), counter_value } );

				ApplyEmptyInitializer( variable_name, file_pos, array_member, block_names, function_context );
			},
			function_context);
	}
	else if( const Tuple* const tuple_type= variable.type.GetTupleType() )
	{
		Variable tuple_member= variable;
		tuple_member.location= Variable::Location::Pointer;

		for( const Type& element_type : tuple_type->elements )
		{
			const size_t i= size_t( &element_type - tuple_type->elements.data() );
			tuple_member.type= element_type;
			tuple_member.llvm_value= function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex(i) } );

			ApplyEmptyInitializer( variable_name, file_pos, tuple_member, block_names, function_context );
		}
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		// If initializer for class variable is empty, try to call default constructor.

		const Value* constructor_value=
			class_type->members.GetThisScopeValue( Keyword( Keywords::constructor_ ) );
		U_ASSERT( constructor_value != nullptr );
		const OverloadedFunctionsSet* const constructors_set= constructor_value->GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.GetOverloadedFunctionsSet()= *constructors_set;

		// TODO - fix this.
		// "CallOperator" pointer used as key in overloading resolution cache. Passing stack object is not safe.
		const Synt::CallOperator call_operator( file_pos );
		BuildCallOperator( std::move(this_overloaded_methods_set), call_operator, block_names, function_context );
	}
	else U_ASSERT(false);
}

llvm::Constant* CodeBuilder::ApplyArrayInitializer(
	const Variable& variable,
	const Synt::ArrayInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const Array* const array_type= variable.type.GetArrayType() )
	{
		if( array_type->size != Array::c_undefined_size && initializer.initializers.size() != array_type->size )
		{
			REPORT_ERROR( ArrayInitializersCountMismatch,
				block_names.GetErrors(),
				initializer.file_pos_,
				array_type->size,
				initializer.initializers.size() );
			return nullptr;
			// SPRACHE_TODO - add array continious initializers.
		}

		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= GetZeroGEPIndex();

		bool is_constant= array_type->type.CanBeConstexpr();
		std::vector<llvm::Constant*> members_constants;

		for( size_t i= 0u; i < initializer.initializers.size(); i++ )
		{
			index_list[1]= GetFieldGEPIndex(i);
			array_member.llvm_value= function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );

			llvm::Constant* const member_constant=
				ApplyInitializer( array_member, initializer.initializers[i], block_names, function_context );

			if( is_constant && member_constant != nullptr )
				members_constants.push_back( member_constant );
			else
				is_constant= false;
		}

		U_ASSERT( members_constants.size() == initializer.initializers.size() || !is_constant );

		if( is_constant )
		{
			if( array_type->size == Array::c_undefined_size )
				return llvm::UndefValue::get( array_type->llvm_type );
			else
				return llvm::ConstantArray::get( array_type->llvm_type, members_constants );
		}
	}
	else if( const Tuple* const tuple_type= variable.type.GetTupleType() )
	{
		if( initializer.initializers.size() != tuple_type->elements.size() )
		{
			REPORT_ERROR( TupleInitializersCountMismatch,
				block_names.GetErrors(),
				initializer.file_pos_,
				tuple_type->elements.size(),
				initializer.initializers.size() );
			return nullptr;
		}

		Variable tuple_element= variable;
		tuple_element.location= Variable::Location::Pointer;

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= GetZeroGEPIndex();

		bool is_constant= variable.type.CanBeConstexpr();
		std::vector<llvm::Constant*> members_constants;

		for( size_t i= 0u; i < initializer.initializers.size(); ++i )
		{
			index_list[1]= GetFieldGEPIndex(i);
			tuple_element.llvm_value= function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );
			tuple_element.type= tuple_type->elements[i];

			llvm::Constant* const member_constant=
				ApplyInitializer( tuple_element, initializer.initializers[i], block_names, function_context );

			if( is_constant && member_constant != nullptr )
				members_constants.push_back( member_constant );
			else
				is_constant= false;
		}

		U_ASSERT( members_constants.size() == initializer.initializers.size() || !is_constant );

		if( is_constant )
			return llvm::ConstantStruct::get( tuple_type->llvm_type, members_constants );
	}
	else
	{
		REPORT_ERROR( ArrayInitializerForNonArray, block_names.GetErrors(), initializer.file_pos_ );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyStructNamedInitializer(
	const Variable& variable,
	const Synt::StructNamedInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Class* const class_type= variable.type.GetClassType();
	if( class_type == nullptr || class_type->kind != Class::Kind::Struct )
	{
		REPORT_ERROR( StructInitializerForNonStruct, block_names.GetErrors(), initializer.file_pos_ );
		return nullptr;
	}

	if( class_type->have_explicit_noncopy_constructors )
		REPORT_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors, block_names.GetErrors(), initializer.file_pos_ );

	ProgramStringSet initialized_members_names;

	ClassFieldsVector<llvm::Constant*> constant_initializers;
	bool all_fields_are_constant= false;
	if( class_type->can_be_constexpr )
	{
		constant_initializers.resize( class_type->llvm_type->getNumElements(), nullptr );
		all_fields_are_constant= true;
	}

	Variable struct_member= variable;
	struct_member.location= Variable::Location::Pointer;

	for( const Synt::StructNamedInitializer::MemberInitializer& member_initializer : initializer.members_initializers )
	{
		if( initialized_members_names.count( member_initializer.name ) != 0 )
		{
			REPORT_ERROR( DuplicatedStructMemberInitializer, block_names.GetErrors(), initializer.file_pos_, member_initializer.name );
			continue;
		}

		const Value* const class_member= class_type->members.GetThisScopeValue( member_initializer.name );
		if( class_member == nullptr )
		{
			REPORT_ERROR( NameNotFound, block_names.GetErrors(), initializer.file_pos_, member_initializer.name );
			continue;
		}
		const ClassField* const field= class_member->GetClassField();
		if( field == nullptr )
		{
			REPORT_ERROR( InitializerForNonfieldStructMember, block_names.GetErrors(), initializer.file_pos_, member_initializer.name );
			continue;
		}
		if( field->class_.lock() != variable.type )
		{
			REPORT_ERROR( InitializerForBaseClassField, block_names.GetErrors(), initializer.file_pos_, member_initializer.name );
			continue;
		}

		initialized_members_names.insert( member_initializer.name );

		llvm::Constant* constant_initializer= nullptr;
		if( field->is_reference )
			constant_initializer=
				InitializeReferenceField( variable, *field, member_initializer.initializer, block_names, function_context );
		else
		{
			struct_member.type= field->type;
			struct_member.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field->index ) } );

			constant_initializer=
				ApplyInitializer( struct_member, member_initializer.initializer, block_names, function_context );
		}

		if( constant_initializer == nullptr )
			all_fields_are_constant= false;
		if( all_fields_are_constant )
			constant_initializers[field->index]= constant_initializer;
	}

	U_ASSERT( initialized_members_names.size() <= class_type->field_count );
	class_type->members.ForEachValueInThisScope(
		[&]( const Value& class_member )
		{
			if( const ClassField* const field= class_member.GetClassField() )
			{
				if( initialized_members_names.count( field->syntax_element->name ) == 0 )
				{
					llvm::Constant* constant_initializer= nullptr;
					if( field->is_reference )
					{
						if( field->syntax_element->initializer == nullptr )
							REPORT_ERROR( ExpectedInitializer, block_names.GetErrors(), class_member.GetFilePos(), field->syntax_element->name ); // References is not default-constructible.
						else
							constant_initializer= InitializeReferenceClassFieldWithInClassIninitalizer( variable, *field, function_context );
					}
					else
					{
						struct_member.type= field->type;
						struct_member.llvm_value=
							function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field->index ) } );

						if( field->syntax_element->initializer != nullptr )
							constant_initializer=
								InitializeClassFieldWithInClassIninitalizer( struct_member, *field, function_context );
						else
							ApplyEmptyInitializer( field->syntax_element->name, initializer.file_pos_, struct_member, block_names, function_context );
					}

					if( constant_initializer == nullptr )
						all_fields_are_constant= false;
					if( all_fields_are_constant )
						constant_initializers[field->index]= constant_initializer;
				}
			}
		});

	if( all_fields_are_constant && constant_initializers.size() == class_type->field_count )
		return llvm::ConstantStruct::get( class_type->llvm_type, constant_initializers );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyConstructorInitializer(
	const Variable& variable,
	const Synt::CallOperator& call_operator,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const dst_type= variable.type.GetFundamentalType() )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			REPORT_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), call_operator.file_pos_ );
			return nullptr;
		}

		const Variable src_var= BuildExpressionCodeEnsureVariable( call_operator.arguments_.front(), block_names, function_context );

		const FundamentalType* src_type= src_var.type.GetFundamentalType();
		if( src_type == nullptr )
		{
			// Allow explicit conversions of enums to ints.
			if( const Enum* const enum_type= src_var.type.GetEnumType () )
				src_type= &enum_type->underlaying_type;
		}

		if( src_type == nullptr )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), call_operator.file_pos_, variable.type, src_var.type );
			return nullptr;
		}

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( src_var, function_context );

		llvm::Constant* constant_value= nullptr;
		const bool src_is_constant= src_var.constexpr_value != nullptr;

		if( dst_type->fundamental_type != src_type->fundamental_type )
		{
			// Perform fundamental types conversion.

			const SizeType src_size= src_type->GetSize();
			const SizeType dst_size= dst_type->GetSize();
			if( IsInteger( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) )
			{
				// int to int
				if( src_size < dst_size )
				{
					if( IsUnsignedInteger( dst_type->fundamental_type ) )
					{
						// We lost here some values in conversions, such i16 => u32, if src_type is signed.
						if( src_is_constant )
							constant_value= llvm::ConstantExpr::getZExt( src_var.constexpr_value, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
					}
					else
					{
						if( src_is_constant )
							constant_value= llvm::ConstantExpr::getSExt( src_var.constexpr_value, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateSExt( value_for_assignment, dst_type->llvm_type );
					}
				}
				else if( src_size > dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getTrunc( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= src_var.constexpr_value;
					// Same size integers - do nothing.
				}
			}
			else if( IsFloatingPoint( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) )
			{
				// float to float
				if( src_size < dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPExtend( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPExt( value_for_assignment, dst_type->llvm_type );
				}
				else if( src_size > dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPTrunc( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else U_ASSERT(false);
			}
			else if( IsFloatingPoint( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) )
			{
				// int to float
				if( IsSignedInteger( src_type->fundamental_type ) )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getSIToFP( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateSIToFP( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getUIToFP( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateUIToFP( value_for_assignment, dst_type->llvm_type );
				}
			}
			else if( IsInteger( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) )
			{
				// float to int
				if( IsSignedInteger( dst_type->fundamental_type ) )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPToSI( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPToSI( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPToUI( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPToUI( value_for_assignment, dst_type->llvm_type );
				}
			}
			else if( IsChar( dst_type->fundamental_type ) && ( IsInteger( src_type->fundamental_type ) || IsChar( src_type->fundamental_type ) ) )
			{
				// int to char or char to char
				if( src_size < dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getZExt( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
				}
				else if( src_size > dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getTrunc( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= src_var.constexpr_value;
				}
			}
			else if( IsInteger( dst_type->fundamental_type ) && IsChar( src_type->fundamental_type ) )
			{
				// char to int
				if( src_size < dst_size )
				{
					if( IsUnsignedInteger( dst_type->fundamental_type ) )
					{
						// We lost here some values in conversions, such i16 => u32, if src_type is signed.
						if( src_is_constant )
							constant_value= llvm::ConstantExpr::getZExt( src_var.constexpr_value, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
					}
					else
					{
						if( src_is_constant )
							constant_value= llvm::ConstantExpr::getSExt( src_var.constexpr_value, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateSExt( value_for_assignment, dst_type->llvm_type );
					}
				}
				else if( src_size > dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getTrunc( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= src_var.constexpr_value;
				}
			}
			else
			{
				if( dst_type->fundamental_type == U_FundamentalType::Bool )
				{
					// TODO - error, bool have no constructors from other types
				}
				REPORT_ERROR( TypesMismatch, block_names.GetErrors(), call_operator.file_pos_, variable.type, src_var.type );
				return nullptr;
			}
		} // If needs conversion
		else
		{
			if( src_is_constant )
				constant_value= src_var.constexpr_value;
		}

		if( src_is_constant )
		{
			U_ASSERT( constant_value != nullptr );
			value_for_assignment= constant_value;
		}

		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );

		DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), call_operator.file_pos_ );

		return constant_value;
	}
	else if( variable.type.GetEnumType() != nullptr )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			// TODO - generate separate error for enums.
			REPORT_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), call_operator.file_pos_ );
			return nullptr;
		}

		const Variable expression_result= BuildExpressionCodeEnsureVariable( call_operator.arguments_.front(), block_names, function_context );
		if( expression_result.type != variable.type )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), call_operator.file_pos_, variable.type, expression_result.type );
			return nullptr;
		}

		function_context.llvm_ir_builder.CreateStore(
			CreateMoveToLLVMRegisterInstruction( expression_result, function_context ),
			variable.llvm_value );

		DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), call_operator.file_pos_ );

		return expression_result.constexpr_value;
	}
	else if( variable.type.GetFunctionPointerType() != nullptr )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			// TODO - generate separate error for function pointers.
			REPORT_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), call_operator.file_pos_ );
			return nullptr;
		}

		return InitializeFunctionPointer( variable, call_operator.arguments_.front(), block_names, function_context );
	}
	else if( variable.type.GetTupleType() != nullptr )
	{
		if( call_operator.arguments_.empty() )
		{
			ApplyEmptyInitializer( variable.node == nullptr ? ""_SpC : variable.node->name, call_operator.file_pos_, variable, block_names, function_context );
			return nullptr;
		}
		else if( call_operator.arguments_.size() == 1u )
		{
			const Variable expression_result= BuildExpressionCodeEnsureVariable( call_operator.arguments_.front(), block_names, function_context );
			if( expression_result.type != variable.type )
			{
				REPORT_ERROR( TypesMismatch, block_names.GetErrors(), call_operator.file_pos_, variable.type, expression_result.type );
				return nullptr;
			}

			// TODO - process references.

			// Copy/move initialize tuple.
			if( expression_result.value_type == ValueType::Value )
			{
				CopyBytes( expression_result.llvm_value, variable.llvm_value, variable.type, function_context );

				const ReferencesGraphNodePtr& src_node= expression_result.node;
				const ReferencesGraphNodePtr& dst_node= variable.node;
				if( src_node != nullptr && dst_node != nullptr )
				{
					U_ASSERT( src_node->kind == ReferencesGraphNode::Kind::Variable );
					if( const auto moved_node_inner_reference= function_context.variables_state.GetNodeInnerReference( src_node ) )
					{
						ReferencesGraphNodePtr dst_inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
						if( dst_inner_reference == nullptr )
						{
							dst_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable"_SpC, moved_node_inner_reference->kind );
							function_context.variables_state.SetNodeInnerReference( dst_node, dst_inner_reference );
						}
						else
						{
							if( moved_node_inner_reference->kind != dst_inner_reference->kind )
							{
								// TODO - make separate error.
								REPORT_ERROR( NotImplemented, block_names.GetErrors(), call_operator.file_pos_, "inner reference mutability changing" );
								return nullptr;
							}
						}
						function_context.variables_state.AddLink( moved_node_inner_reference, dst_inner_reference );
					}
					function_context.variables_state.MoveNode( src_node );
				}
			}
			else
				CopyInitializeTupleElements_r(
					variable.type,
					variable.llvm_value,
					expression_result.llvm_value,
					call_operator.file_pos_,
					block_names,
					function_context );
			return nullptr;
		}

		REPORT_ERROR( ConstructorInitializerForUnsupportedType, block_names.GetErrors(), call_operator.file_pos_ );
		return nullptr;
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		// Try do move-construct.
		bool needs_move_constuct= false;
		if( call_operator.arguments_.size() == 1u )
		{
			const auto state= SaveInstructionsState( function_context );
			{
				const StackVariablesStorage dummy_stack_variables_storage( function_context );

				const Variable initializer_value= BuildExpressionCodeEnsureVariable( call_operator.arguments_.front(), block_names, function_context );
				needs_move_constuct= initializer_value.type == variable.type && initializer_value.value_type == ValueType::Value ;
			}
			RestoreInstructionsState( function_context, state );
		}
		if( needs_move_constuct )
		{
			const Variable initializer_variable= BuildExpressionCodeEnsureVariable( call_operator.arguments_.front(), block_names, function_context );
			CopyBytes( initializer_variable.llvm_value, variable.llvm_value, variable.type, function_context );

			const ReferencesGraphNodePtr& src_node= initializer_variable.node;
			const ReferencesGraphNodePtr& dst_node= variable.node;
			if( src_node != nullptr && dst_node != nullptr )
			{
				U_ASSERT( src_node->kind == ReferencesGraphNode::Kind::Variable );
				if( const auto moved_node_inner_reference= function_context.variables_state.GetNodeInnerReference( src_node ) )
				{
					ReferencesGraphNodePtr dst_inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
					if( dst_inner_reference == nullptr )
					{
						dst_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable"_SpC, moved_node_inner_reference->kind );
						function_context.variables_state.SetNodeInnerReference( dst_node, dst_inner_reference );
					}
					else
					{
						if( moved_node_inner_reference->kind != dst_inner_reference->kind )
						{
							// TODO - make separate error.
							REPORT_ERROR( NotImplemented, block_names.GetErrors(), call_operator.file_pos_, "inner reference mutability changing" );
							return nullptr;
						}
					}
					function_context.variables_state.AddLink( moved_node_inner_reference, dst_inner_reference );
				}
				function_context.variables_state.MoveNode( src_node );
			}

			return initializer_variable.constexpr_value; // Move can preserve constexpr.
		}

		const Value* constructor_value=
			class_type->members.GetThisScopeValue( Keyword( Keywords::constructor_ ) );
		if( constructor_value == nullptr )
		{
			REPORT_ERROR( ClassHaveNoConstructors, block_names.GetErrors(), call_operator.file_pos_ );
			return nullptr;
		}

		const OverloadedFunctionsSet* const constructors_set= constructor_value->GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.GetOverloadedFunctionsSet()= *constructors_set;

		BuildCallOperator( std::move(this_overloaded_methods_set), call_operator, block_names, function_context );
	}
	else
	{
		REPORT_ERROR( ConstructorInitializerForUnsupportedType, block_names.GetErrors(), call_operator.file_pos_ );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyExpressionInitializer(
	const Variable& variable,
	const Synt::ExpressionInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( variable.type.GetFundamentalType() != nullptr || variable.type.GetEnumType() != nullptr )
	{
		const Variable expression_result= BuildExpressionCodeEnsureVariable( initializer.expression, block_names, function_context );
		if( expression_result.type != variable.type )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer.file_pos_, variable.type, expression_result.type );
			return nullptr;
		}

		llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );

		DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), initializer.file_pos_ );

		if( llvm::Constant* const constexpr_value= expression_result.constexpr_value )
			return constexpr_value;
	}
	else if( variable.type.GetFunctionPointerType() != nullptr )
		return InitializeFunctionPointer( variable, initializer.expression, block_names, function_context );
	else if( variable.type.GetTupleType() != nullptr )
	{
		Variable expression_result= BuildExpressionCodeEnsureVariable( initializer.expression, block_names, function_context );
		if( expression_result.type != variable.type )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer.file_pos_, variable.type, expression_result.type );
			return nullptr;
		}

		const ReferencesGraphNodePtr& src_node= expression_result.node;
		const ReferencesGraphNodePtr& dst_node= variable.node;
		if( src_node != nullptr && dst_node != nullptr && variable.type.ReferencesTagsCount() > 0u )
		{
			const auto src_node_inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( src_node );
			if( !src_node_inner_references.empty() )
			{
				bool node_is_mutable= false;
				for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
					node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

				const auto dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable"_SpC, node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
				function_context.variables_state.SetNodeInnerReference( dst_node, dst_node_inner_reference );
				for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
					function_context.variables_state.AddLink( src_node_inner_reference, dst_node_inner_reference );
			}
		}

		// Move or try call copy constructor.
		// TODO - produce constant initializer for generated copy constructor, if source is constant.
		if( expression_result.value_type == ValueType::Value && expression_result.type == variable.type )
		{
			if( src_node != nullptr )
			{
				U_ASSERT( src_node->kind == ReferencesGraphNode::Kind::Variable );
				function_context.variables_state.MoveNode( src_node );
			}
			CopyBytes( expression_result.llvm_value, variable.llvm_value, variable.type, function_context );

			DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), initializer.file_pos_ );

			return expression_result.constexpr_value; // Move can preserve constexpr.
		}
		else
		{
			CopyInitializeTupleElements_r(
				variable.type,
				variable.llvm_value,
				expression_result.llvm_value,
				initializer.file_pos_,
				block_names,
				function_context );
			return nullptr; // Call to constructors may not preserve constexpr value.
		}
	}
	else if( variable.type.GetClassType() != nullptr )
	{
		// Currently we support "=" initializer for copying and moving of structs.

		Variable expression_result= BuildExpressionCodeEnsureVariable( initializer.expression, block_names, function_context );
		if( expression_result.type == variable.type )
		{} // Ok, same types.
		else if( ReferenceIsConvertible( expression_result.type, variable.type, block_names.GetErrors(), initializer.file_pos_ ) )
		{} // Ok, can do reference conversion.
		else if( const FunctionVariable* const conversion_constructor= GetConversionConstructor( expression_result.type, variable.type, block_names.GetErrors(), initializer.file_pos_ ) )
		{
			// Type conversion required.
			expression_result= ConvertVariable( expression_result, variable.type, *conversion_constructor, block_names, function_context, initializer.file_pos_ );
		}
		else
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer.file_pos_, variable.type, expression_result.type );
			return nullptr;
		}

		const ReferencesGraphNodePtr& src_node= expression_result.node;
		const ReferencesGraphNodePtr& dst_node= variable.node;
		if( src_node != nullptr && dst_node != nullptr && variable.type.ReferencesTagsCount() > 0u )
		{
			const auto src_node_inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( src_node );
			if( !src_node_inner_references.empty() )
			{
				bool node_is_mutable= false;
				for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
					node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

				const auto dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable"_SpC, node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
				function_context.variables_state.SetNodeInnerReference( dst_node, dst_node_inner_reference );
				for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
					function_context.variables_state.AddLink( src_node_inner_reference, dst_node_inner_reference );
			}
		}

		// Move or try call copy constructor.
		// TODO - produce constant initializer for generated copy constructor, if source is constant.
		if( expression_result.value_type == ValueType::Value && expression_result.type == variable.type )
		{
			if( src_node != nullptr )
			{
				U_ASSERT( src_node->kind == ReferencesGraphNode::Kind::Variable );
				function_context.variables_state.MoveNode( src_node );
			}
			CopyBytes( expression_result.llvm_value, variable.llvm_value, variable.type, function_context );

			DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), initializer.file_pos_ );

			return expression_result.constexpr_value; // Move can preserve constexpr.
		}
		else
		{
			llvm::Value* value_for_copy= expression_result.llvm_value;
			if( expression_result.type != variable.type )
				value_for_copy= CreateReferenceCast( value_for_copy, expression_result.type, variable.type, function_context );
			TryCallCopyConstructor(
				block_names.GetErrors(), initializer.file_pos_, variable.llvm_value, value_for_copy, variable.type.GetClassTypeProxy(), function_context );
		}
	}
	else
	{
		REPORT_ERROR( NotImplemented, block_names.GetErrors(), initializer.file_pos_, "expression initialization for arrays" );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyZeroInitializer(
	const Variable& variable,
	const Synt::ZeroInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
	{
		llvm::Constant* zero_value= nullptr;
		switch( fundamental_type->fundamental_type )
		{
		case U_FundamentalType::Bool:
		case U_FundamentalType::i8:
		case U_FundamentalType::u8:
		case U_FundamentalType::i16:
		case U_FundamentalType::u16:
		case U_FundamentalType::i32:
		case U_FundamentalType::u32:
		case U_FundamentalType::i64:
		case U_FundamentalType::u64:
		case U_FundamentalType::i128:
		case U_FundamentalType::u128:
		case U_FundamentalType::char8 :
		case U_FundamentalType::char16:
		case U_FundamentalType::char32:
		case U_FundamentalType::InvalidType:
			zero_value=
				llvm::Constant::getIntegerValue(
					fundamental_type->llvm_type,
					llvm::APInt( fundamental_type->llvm_type->getIntegerBitWidth(), uint64_t(0) ) );
			break;

		case U_FundamentalType::f32:
		case U_FundamentalType::f64:
			zero_value= llvm::ConstantFP::get( fundamental_type->llvm_type, 0.0 );
			break;

		case U_FundamentalType::Void:
		case U_FundamentalType::LastType:
			U_ASSERT(false);
			break;
		};

		function_context.llvm_ir_builder.CreateStore( zero_value, variable.llvm_value );
		return zero_value;
	}
	else if( const Enum* const enum_type= variable.type.GetEnumType() )
	{
		// Currently, first member of enum have zero value.

		llvm::Constant* const zero_value=
			llvm::Constant::getIntegerValue(
				enum_type->underlaying_type.llvm_type,
				llvm::APInt( enum_type->underlaying_type.llvm_type->getIntegerBitWidth(), uint64_t(0) ) );

		function_context.llvm_ir_builder.CreateStore( zero_value, variable.llvm_value );
		return zero_value;
	}
	else if( const FunctionPointer* const function_pointer_type= variable.type.GetFunctionPointerType() )
	{
		// Really? Allow zero function pointers?

		llvm::Constant* const null_value=
			llvm::Constant::getNullValue( function_pointer_type->llvm_function_pointer_type );

		function_context.llvm_ir_builder.CreateStore( null_value, variable.llvm_value );
		return null_value;
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		llvm::Constant* const_value= nullptr;

		GenerateLoop(
			array_type->ArraySizeOrZero(),
			[&](llvm::Value* const counter_value)
			{
				array_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), counter_value } );
				const_value= ApplyZeroInitializer( array_member, initializer, block_names, function_context );
			},
			function_context);

		if( const_value != nullptr && array_type->type.CanBeConstexpr() )
		{
			if( array_type->size == Array::c_undefined_size )
				return llvm::UndefValue::get( array_type->llvm_type );
			else
				return
					llvm::ConstantArray::get(
						array_type->llvm_type,
						std::vector<llvm::Constant*>( size_t(array_type->size), const_value ) );
		}
	}
	else if( const Tuple* const tuple_type= variable.type.GetTupleType() )
	{
		Variable tuple_member= variable;
		tuple_member.location= Variable::Location::Pointer;

		std::vector<llvm::Constant*> elements_const_values;

		for( const Type& element_type : tuple_type->elements )
		{
			const size_t i= size_t( &element_type - tuple_type->elements.data() );
			tuple_member.type= element_type;
			tuple_member.llvm_value= function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex(i) } );

			llvm::Constant* const const_value=
				ApplyZeroInitializer( tuple_member, initializer, block_names, function_context );
			if( const_value != nullptr )
				elements_const_values.push_back( const_value );
		}

		if( elements_const_values.size() == tuple_type->elements.size() )
			return llvm::ConstantStruct::get( tuple_type->llvm_type, elements_const_values );
		else
			return nullptr;
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		if( class_type->have_explicit_noncopy_constructors )
			REPORT_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors, block_names.GetErrors(), initializer.file_pos_ );
		if( class_type->kind != Class::Kind::Struct )
			REPORT_ERROR( ZeroInitializerForClass, block_names.GetErrors(), initializer.file_pos_ );

		ClassFieldsVector<llvm::Constant*> constant_initializers;
		bool all_fields_are_constant= false;
		if( class_type->can_be_constexpr )
		{
			constant_initializers.resize( class_type->llvm_type->getNumElements(), nullptr );
			all_fields_are_constant= true;
		}

		Variable struct_member= variable;
		struct_member.location= Variable::Location::Pointer;

		class_type->members.ForEachValueInThisScope(
			[&]( const Value& member )
			{
				const ClassField* const field= member.GetClassField();
				if( field == nullptr || field->class_.lock() != variable.type )
					return;
				if( field->is_reference )
				{
					all_fields_are_constant= false;
					REPORT_ERROR( UnsupportedInitializerForReference, block_names.GetErrors(), initializer.file_pos_ );
					return;
				}

				struct_member.type= field->type;
				struct_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field->index ) } );

				llvm::Constant* const constant_initializer=
					ApplyZeroInitializer( struct_member, initializer, block_names, function_context );

				if( constant_initializer == nullptr )
					all_fields_are_constant= false;
				if( all_fields_are_constant )
					constant_initializers[field->index]= constant_initializer;
			});

		if( all_fields_are_constant )
			return llvm::ConstantStruct::get( class_type->llvm_type, constant_initializers );
	}
	else U_ASSERT( false );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyUninitializedInitializer(
	const Variable& variable,
	const Synt::UninitializedInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( UninitializedInitializerOutsideUnsafeBlock, block_names.GetErrors(), initializer.file_pos_ );

	if( variable.type.CanBeConstexpr() )
		return llvm::UndefValue::get( variable.type.GetLLVMType() );
	else
		return nullptr;
}

void CodeBuilder::CopyInitializeTupleElements_r(
	const Type& type,
	llvm::Value* const dst, llvm::Value* const src,
	const FilePos& file_pos,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	// TODO - make reference pollution.

	if( type.GetFundamentalType() != nullptr || type.GetEnumType() != nullptr || type.GetFunctionPointerType() != nullptr )
	{
		if( src->getType() == dst->getType() )
			function_context.llvm_ir_builder.CreateStore( function_context.llvm_ir_builder.CreateLoad( src ), dst );
		else if( src->getType() == dst->getType()->getPointerElementType() )
			function_context.llvm_ir_builder.CreateStore( src, dst );
		else U_ASSERT( false );
	}
	else if( const Array* const array_type= type.GetArrayType() )
	{
		GenerateLoop(
			array_type->size,
			[&]( llvm::Value* const counter_value )
			{
				llvm::Value* const index_list[2]{ GetZeroGEPIndex(), counter_value };
				CopyInitializeTupleElements_r(
					array_type->type,
					function_context.llvm_ir_builder.CreateGEP( dst, index_list ),
					function_context.llvm_ir_builder.CreateGEP( src, index_list ),
					file_pos,
					block_names,
					function_context );
			},
			function_context );
	}
	else if( const Tuple* const tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			llvm::Value* const index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( &element_type - tuple_type->elements.data() ) };
			CopyInitializeTupleElements_r(
				element_type,
				function_context.llvm_ir_builder.CreateGEP( dst, index_list ),
				function_context.llvm_ir_builder.CreateGEP( src, index_list ),
				file_pos,
				block_names,
				function_context );
		}
	}
	else if( const ClassProxyPtr class_type= type.GetClassTypeProxy() )
		TryCallCopyConstructor( block_names.GetErrors(), file_pos, dst, src, class_type, function_context );
}

llvm::Constant* CodeBuilder::InitializeReferenceField(
	const Variable& variable,
	const ClassField& field,
	const Synt::Initializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	U_ASSERT( variable.type.GetClassType() != nullptr );
	U_ASSERT( variable.type.GetClassTypeProxy() == field.class_.lock() );

	const FilePos initializer_file_pos= Synt::GetInitializerFilePos( initializer );
	const Synt::Expression* initializer_expression= nullptr;
	if( const auto expression_initializer= boost::get<const Synt::ExpressionInitializer>( &initializer ) )
	{
		initializer_expression= &expression_initializer->expression;
	}
	else if( const auto constructor_initializer= boost::get<const Synt::ConstructorInitializer>( &initializer ) )
	{
		if( constructor_initializer->call_operator.arguments_.size() != 1u )
		{
			REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), constructor_initializer->file_pos_ );
			return nullptr;
		}
		initializer_expression= &constructor_initializer->call_operator.arguments_.front();
	}
	else
	{
		REPORT_ERROR( UnsupportedInitializerForReference, block_names.GetErrors(), initializer_file_pos );
		return nullptr;
	}

	const Variable initializer_variable= BuildExpressionCodeEnsureVariable( *initializer_expression, block_names, function_context );

	const FilePos initializer_expression_file_pos= Synt::GetExpressionFilePos( *initializer_expression );
	if( !ReferenceIsConvertible( initializer_variable.type, field.type, block_names.GetErrors(), initializer_expression_file_pos ) )
	{
		REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer_expression_file_pos, field.type, initializer_variable.type );
		return nullptr;
	}
	if( initializer_variable.value_type == ValueType::Value )
	{
		REPORT_ERROR( ExpectedReferenceValue, block_names.GetErrors(), initializer_expression_file_pos );
		return nullptr;
	}
	U_ASSERT( initializer_variable.location == Variable::Location::Pointer );

	if( field.is_mutable && initializer_variable.value_type == ValueType::ConstReference )
	{
		REPORT_ERROR( BindingConstReferenceToNonconstReference, block_names.GetErrors(), initializer_expression_file_pos );
		return nullptr;
	}

	// Check references.
	const ReferencesGraphNodePtr& src_node= initializer_variable.node;
	const ReferencesGraphNodePtr& dst_node= variable.node;
	if( src_node != nullptr && dst_node != nullptr )
	{
		if( ( field.is_mutable && function_context.variables_state.HaveOutgoingLinks( src_node ) ) ||
			(!field.is_mutable && function_context.variables_state.HaveOutgoingMutableNodes( src_node ) ) )
		{
			REPORT_ERROR( ReferenceProtectionError, block_names.GetErrors(), initializer_file_pos, src_node->name );
			return nullptr;
		}

		ReferencesGraphNodePtr inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
		if( inner_reference == nullptr )
		{
			inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + "/inner_variable"_SpC, field.is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
			function_context.variables_state.SetNodeInnerReference( dst_node, inner_reference );
		}
		else
		{
			if( inner_reference->kind == ReferencesGraphNode::Kind::ReferenceImut && field.is_mutable )
			{
				// TODO - make separate error.
				REPORT_ERROR( NotImplemented, block_names.GetErrors(), initializer_file_pos, "inner reference mutability changing" );
				return nullptr;
			}
		}
		function_context.variables_state.AddLink( src_node, inner_reference );
	}

	llvm::Value* const address_of_reference=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field.index ) } );

	llvm::Value* ref_to_store= initializer_variable.llvm_value;
	if( field.type != initializer_variable.type )
		ref_to_store= CreateReferenceCast( ref_to_store, initializer_variable.type, field.type, function_context );
	function_context.llvm_ir_builder.CreateStore( ref_to_store, address_of_reference );

	if( initializer_variable.constexpr_value != nullptr )
	{
		// We needs to store constant somewhere. Create global variable for it.
		llvm::Constant* constant_stored= CreateGlobalConstantVariable( initializer_variable.type, "_temp_const", initializer_variable.constexpr_value );

		if( field.type != initializer_variable.type )
			constant_stored=
				llvm::dyn_cast<llvm::Constant>( CreateReferenceCast( constant_stored, initializer_variable.type, field.type, function_context ) );

		return constant_stored;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::InitializeFunctionPointer(
	const Variable& variable,
	const Synt::Expression& initializer_expression,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	U_ASSERT( variable.type.GetFunctionPointerType() != nullptr );

	const FilePos initializer_expression_file_pos= Synt::GetExpressionFilePos( initializer_expression );
	const FunctionPointer& function_pointer_type= *variable.type.GetFunctionPointerType();

	const Value initializer_value= BuildExpressionCode( initializer_expression, block_names, function_context );

	if( const Variable* const initializer_variable= initializer_value.GetVariable() )
	{
		const FunctionPointer* const intitializer_type= initializer_variable->type.GetFunctionPointerType();
		if( intitializer_type == nullptr ||
			!intitializer_type->function.PointerCanBeConvertedTo( function_pointer_type.function ) )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer_expression_file_pos, variable.type, initializer_variable->type );
			return nullptr;
		}
		U_ASSERT( initializer_variable->type.GetFunctionPointerType() != nullptr );

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *initializer_variable, function_context );
		if( initializer_variable->type != variable.type )
			value_for_assignment= function_context.llvm_ir_builder.CreatePointerCast( value_for_assignment, variable.type.GetLLVMType() );

		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
		return initializer_variable->constexpr_value;
	}

	const OverloadedFunctionsSet* candidate_functions= nullptr;
	if( const OverloadedFunctionsSet* const overloaded_functions_set= initializer_value.GetFunctionsSet() )
		candidate_functions= overloaded_functions_set;
	else if( const ThisOverloadedMethodsSet* const overloaded_methods_set= initializer_value.GetThisOverloadedMethodsSet() )
		candidate_functions= &overloaded_methods_set->GetOverloadedFunctionsSet();
	else
	{
		// TODO - generate separate error
		REPORT_ERROR( ExpectedVariable, block_names.GetErrors(), initializer_expression_file_pos, initializer_value.GetKindName() );
		return nullptr;
	}

	// Try select one of overloaded functions.
	// Select function with same with pointer type, if it exists.
	// If there is no function with same type, select function, convertible to pointer type, but only if exists only one convertible function.
	const FunctionVariable* exact_match_function_variable= nullptr;
	std::vector<const FunctionVariable*> convertible_function_variables;

	for( const FunctionVariable& func : candidate_functions->functions )
	{
		if( *func.type.GetFunctionType() == function_pointer_type.function )
			exact_match_function_variable= &func;
		else if( func.type.GetFunctionType()->PointerCanBeConvertedTo( function_pointer_type.function ) )
			convertible_function_variables.push_back(&func);
	}
	// Try also select template functions with zero template parameters and template functions with all template parameters known.
	for( const FunctionTemplatePtr& function_template : candidate_functions->template_functions )
	{
		if( function_template->template_parameters.empty() )
		{
			const FunctionVariable* const func=
				GenTemplateFunction(
					block_names.GetErrors(),
					initializer_expression_file_pos,
					function_template,
					ArgsVector<Function::Arg>(), false, true );
			if( func != nullptr )
			{
				if( func->type == function_pointer_type.function )
				{
					if( exact_match_function_variable != nullptr )
					{
						// Error, exist more, then one non-exact match function.
						// TODO - maybe generate separate error?
						REPORT_ERROR( TooManySuitableOverloadedFunctions, block_names.GetErrors(), initializer_expression_file_pos );
						return nullptr;
					}
					exact_match_function_variable= func;
				}
				else if( func->type.GetFunctionType()->PointerCanBeConvertedTo( function_pointer_type.function ) )
					convertible_function_variables.push_back(func);
			}
		}
	}

	const FunctionVariable* function_variable= exact_match_function_variable;
	if( function_variable == nullptr )
	{
		if( convertible_function_variables.size() > 1u )
		{
			// Error, exist more, then one non-exact match function.
			// TODO - maybe generate separate error?
			REPORT_ERROR( TooManySuitableOverloadedFunctions, block_names.GetErrors(), initializer_expression_file_pos );
			return nullptr;
		}
		else if( !convertible_function_variables.empty() )
			function_variable= convertible_function_variables.front();
	}
	if( function_variable == nullptr )
	{
		REPORT_ERROR( CouldNotSelectOverloadedFunction, block_names.GetErrors(), initializer_expression_file_pos );
		return nullptr;
	}
	if( function_variable->is_deleted )
		REPORT_ERROR( AccessingDeletedMethod, block_names.GetErrors(), initializer_expression_file_pos );

	llvm::Value* function_value= function_variable->llvm_function;
	if( function_variable->type != function_pointer_type.function )
		function_value= function_context.llvm_ir_builder.CreatePointerCast( function_value, variable.type.GetLLVMType() );

	function_context.llvm_ir_builder.CreateStore( function_value, variable.llvm_value );
	return function_variable->llvm_function;
}

llvm::Constant* CodeBuilder::InitializeClassFieldWithInClassIninitalizer(
	const Variable& field_variable,
	const ClassField& class_field,
	FunctionContext& function_context )
{
	U_ASSERT( class_field.syntax_element->initializer != nullptr );
	U_ASSERT( !class_field.is_reference );

	// Reset "this" for function context.
	// TODO - maybe reset also other function context fields?
	const Variable* const prev_this= function_context.this_;
	function_context.this_= nullptr;

	llvm::Constant* const result=
		ApplyInitializer(
			field_variable,
			*class_field.syntax_element->initializer,
			class_field.class_.lock()->class_->members, // Use class members names scope.
			function_context );

	function_context.this_= prev_this;

	return result;
}

llvm::Constant* CodeBuilder::InitializeReferenceClassFieldWithInClassIninitalizer(
	const Variable& variable,
	const ClassField& class_field,
	FunctionContext& function_context )
{
	U_ASSERT( class_field.syntax_element->initializer != nullptr );
	U_ASSERT( class_field.is_reference );

	// Reset "this" for function context.
	// TODO - maybe reset also other function context fields?
	const Variable* const prev_this= function_context.this_;
	function_context.this_= nullptr;

	llvm::Constant* const result=
		InitializeReferenceField(
			variable,
			class_field,
			*class_field.syntax_element->initializer,
			class_field.class_.lock()->class_->members, // Use class members names scope.
			function_context );

	function_context.this_= prev_this;

	return result;
}

void CodeBuilder::CheckClassFieldsInitializers( const ClassProxyPtr& class_type )
{
	// Run code generation for initializers.
	// We must check it, becauseinitializers may not be executed later.

	const Class& class_= *class_type->class_;
	U_ASSERT( class_.completeness == TypeCompleteness::Complete );

	FunctionContext& function_context= *global_function_context_;
	const StackVariablesStorage dummy_stack_variables_storage( function_context );

	llvm::Value* const variable_llvm_value=
		function_context.alloca_ir_builder.CreateAlloca( class_.llvm_type );

	class_.members.ForEachValueInThisScope(
		[&]( const Value& value )
		{
			const ClassField* const class_field= value.GetClassField();
			if( class_field == nullptr || class_field->class_.lock() != class_type )
				return;

			if( class_field->syntax_element->initializer == nullptr )
				return;

			if( class_field->is_reference )
			{
				Variable variable;
				variable.type= class_type;
				variable.value_type= ValueType::Reference;
				variable.llvm_value= variable_llvm_value;
				InitializeReferenceClassFieldWithInClassIninitalizer( variable, *class_field, function_context );
			}
			else
			{
				Variable field_variable;
				field_variable.type= class_field->type;
				field_variable.value_type= ValueType::Reference;
				field_variable.llvm_value=
					function_context.llvm_ir_builder.CreateGEP(
						variable_llvm_value,
						{ GetZeroGEPIndex(), GetFieldGEPIndex( class_field->index ) } );
				InitializeClassFieldWithInClassIninitalizer( field_variable, *class_field, function_context );
			}
		});
}

} // namespace CodeBuilderPrivate

} // namespace U
