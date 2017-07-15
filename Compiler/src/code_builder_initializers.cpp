#include <set>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

void CodeBuilder::ApplyInitializer_r(
	const Variable& variable,
	const IInitializer* const initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( initializer == nullptr )
	{
		if( boost::get<FundamentalType>( &variable.type.one_of_type_kind ) != nullptr )
		{
			// TODO - set file_pos
			errors_.push_back( ReportExpectedInitializer( FilePos() ) );
		}
		return;
	}

	if( const ArrayInitializer* const array_initializer=
		dynamic_cast<const ArrayInitializer*>(initializer) )
	{
		const ArrayPtr* const array_type_ptr= boost::get<ArrayPtr>( &variable.type.one_of_type_kind );
		if( array_type_ptr == nullptr )
		{
			errors_.push_back( ReportArrayInitializerForNonArray( array_initializer->file_pos_ ) );
			return;
		}
		U_ASSERT( *array_type_ptr != nullptr );
		const Array& array_type= **array_type_ptr;

		if( array_initializer->initializers.size() != array_type.size )
		{
			errors_.push_back(
				ReportArrayInitializersCountMismatch(
					array_initializer->file_pos_,
					array_type.size,
					array_initializer->initializers.size() ) );
			return;
			// SPRACHE_TODO - add array continious initializers.
		}

		Variable array_member= variable;
		array_member.type= array_type.type;
		array_member.location= Variable::Location::Pointer;

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

		for( size_t i= 0u; i < array_initializer->initializers.size(); i++ )
		{
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(i) ) );
			array_member.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyInitializer_r( array_member, array_initializer->initializers[i].get(), block_names, function_context );
		}
	}
	else if( const StructNamedInitializer* const struct_named_initializer=
		dynamic_cast<const StructNamedInitializer*>(initializer) )
	{
		const ClassPtr* const class_type_ptr= boost::get<ClassPtr>( &variable.type.one_of_type_kind );
		if( class_type_ptr == nullptr )
		{
			errors_.push_back( ReportStructInitializerForNonStruct( struct_named_initializer->file_pos_ ) );
			return;
		}
		U_ASSERT( *class_type_ptr != nullptr );
		const Class& class_type= **class_type_ptr;

		std::set<ProgramString> initializerd_members_names;

		Variable struct_member= variable;
		struct_member.location= Variable::Location::Pointer;
		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

		for( const StructNamedInitializer::MemberInitializer& member_initializer : struct_named_initializer->members_initializers )
		{
			if( initializerd_members_names.count( member_initializer.name ) != 0 )
			{
				errors_.push_back( ReportDuplicatedStructMemberInitializer( struct_named_initializer->file_pos_, member_initializer.name ) );
				continue;
			}

			const Class::Field* const field= class_type.GetField( member_initializer.name );
			if( field == nullptr )
			{
				errors_.push_back( ReportNameNotFound( struct_named_initializer->file_pos_, member_initializer.name ) );
				continue;
			}

			initializerd_members_names.insert( member_initializer.name );

			struct_member.type= field->type;
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			struct_member.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			ApplyInitializer_r( struct_member, member_initializer.initializer.get(), block_names, function_context );
		}

		U_ASSERT( initializerd_members_names.size() <= class_type.fields.size() );
		if( initializerd_members_names.size() < class_type.fields.size() )
		{
			for( const Class::Field& field : class_type.fields )
			{
				// SPRACHE_TODO - allow missed initialziers for default-constructed classes.
				if( initializerd_members_names.count( field.name ) == 0 )
					errors_.push_back(ReportMissingStructMemberInitializer( struct_named_initializer->file_pos_, field.name ) );
			}
			return;
		}
	}
	else if( const ConstructorInitializer* const constructor_initializer=
		dynamic_cast<const ConstructorInitializer*>(initializer) )
	{
		if( const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &variable.type.one_of_type_kind ) )
		{
			U_UNUSED(fundamental_type);

			if( constructor_initializer->call_operator.arguments_.size() != 1u )
			{
				errors_.push_back( ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( constructor_initializer->file_pos_ ) );
				return;
			}

			const Variable expression_result=
				BuildExpressionCode( *constructor_initializer->call_operator.arguments_.front(), block_names, function_context );
			if( expression_result.type != variable.type )
			{
				errors_.push_back( ReportTypesMismatch( constructor_initializer->file_pos_, variable.type.ToString(), expression_result.type.ToString() ) );
				return;
			}

			llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
			function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
		}
		else if( const ClassPtr* const class_type= boost::get<ClassPtr>( &variable.type.one_of_type_kind ) )
		{
			U_UNUSED(class_type);
			errors_.push_back( ReportNotImplemented( initializer->file_pos_, "constructors for classes" ) );
			return;
		}
		else
		{
			errors_.push_back( ReportConstructorInitializerForUnsupportedType( constructor_initializer->file_pos_ ) );
			return;
		}
	}
	else if( const ExpressionInitializer* const expression_initializer=
		dynamic_cast<const ExpressionInitializer*>(initializer) )
	{
		if( const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &variable.type.one_of_type_kind ) )
		{
			U_UNUSED(fundamental_type);

			const Variable expression_result=
				BuildExpressionCode( *expression_initializer->expression, block_names, function_context );
			if( expression_result.type != variable.type )
			{
				errors_.push_back( ReportTypesMismatch( expression_initializer->file_pos_, variable.type.ToString(), expression_result.type.ToString() ) );
				return;
			}

			llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
			function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
		}
		else
		{
			errors_.push_back( ReportNotImplemented( initializer->file_pos_, "expression initialization for nonfundamental types" ) );
			return;
		}
	}
	else if( const ZeroInitializer* const zero_initializer=
		dynamic_cast<const ZeroInitializer*>(initializer) )
	{
		if( const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &variable.type.one_of_type_kind ) )
		{
			llvm::Value* zero_value= nullptr;
			switch( fundamental_type->fundamental_type )
			{
			case U_FundamentalType::Bool:
				zero_value=
					llvm::Constant::getIntegerValue(
						fundamental_llvm_types_.bool_,
						llvm::APInt( 1u, uint64_t(0) ) );
				break;

			case U_FundamentalType::i8:
			case U_FundamentalType::u8:
			case U_FundamentalType::i16:
			case U_FundamentalType::u16:
			case U_FundamentalType::i32:
			case U_FundamentalType::u32:
			case U_FundamentalType::i64:
			case U_FundamentalType::u64:
					zero_value=
						llvm::Constant::getIntegerValue(
							GetFundamentalLLVMType( fundamental_type->fundamental_type ),
							llvm::APInt( variable.type.SizeOf() * 8u, uint64_t(0) ) );
				break;

			case U_FundamentalType::f32:
				zero_value= llvm::ConstantFP::get( fundamental_llvm_types_.f32, 0.0 );
				break;
			case U_FundamentalType::f64:
				zero_value= llvm::ConstantFP::get( fundamental_llvm_types_.f64, 0.0 );
				break;

			case U_FundamentalType::Void:
			case U_FundamentalType::InvalidType:
			case U_FundamentalType::LastType:
				U_ASSERT(false);
				break;
			};

			function_context.llvm_ir_builder.CreateStore( zero_value, variable.llvm_value );
		}
		else if( const ArrayPtr* const array_type_ptr= boost::get<ArrayPtr>( &variable.type.one_of_type_kind ) )
		{
			U_ASSERT( *array_type_ptr != nullptr );
			const Array& array_type= **array_type_ptr;

			Variable array_member= variable;
			array_member.type= array_type.type;
			array_member.location= Variable::Location::Pointer;

			// Make first index = 0 for array to pointer conversion.
			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

			for( size_t i= 0u; i < array_type.size; i++ )
			{
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(i) ) );
				array_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

				ApplyInitializer_r( array_member, zero_initializer, block_names, function_context );
			}
		}
		else if( const ClassPtr* const class_type_ptr = boost::get<ClassPtr>( &variable.type.one_of_type_kind ) )
		{
			// SPRACHE_TODO - disallow zero initializers for all except structs without constructors.

			U_ASSERT( *class_type_ptr != nullptr );
			const Class& class_type= **class_type_ptr;

			Variable struct_member= variable;
			struct_member.location= Variable::Location::Pointer;
			// Make first index = 0 for array to pointer conversion.
			llvm::Value* index_list[2];
			index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

			for( const Class::Field& field : class_type.fields )
			{
				struct_member.type= field.type;
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field.index) ) );
				struct_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

				ApplyInitializer_r( struct_member, zero_initializer, block_names, function_context );
			}
		}
		else
		{
			// REPORT unsupported type for zero initializer
			return;
		}
	}
	else
	{
		U_ASSERT(false);
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
