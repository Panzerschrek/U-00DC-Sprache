#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

#include "code_builder_llvm.hpp"

namespace Interpreter
{

namespace
{

typedef std::map< ProgramString, U_FundamentalType > TypesMap;

const TypesMap g_types_map=
{
	{ Keyword( Keywords::void_ ), U_FundamentalType::Void },
	{ Keyword( Keywords::bool_ ), U_FundamentalType::Bool },
	{ Keyword( Keywords::i8_  ), U_FundamentalType::i8  },
	{ Keyword( Keywords::u8_  ), U_FundamentalType::u8  },
	{ Keyword( Keywords::i16_ ), U_FundamentalType::i16 },
	{ Keyword( Keywords::u16_ ), U_FundamentalType::u16 },
	{ Keyword( Keywords::i32_ ), U_FundamentalType::i32 },
	{ Keyword( Keywords::u32_ ), U_FundamentalType::u32 },
	{ Keyword( Keywords::i64_ ), U_FundamentalType::i64 },
	{ Keyword( Keywords::u64_ ), U_FundamentalType::u64 },
	{ Keyword( Keywords::f32_ ), U_FundamentalType::f32 },
	{ Keyword( Keywords::f64_ ), U_FundamentalType::f64 },
};

bool IsNumericType( U_FundamentalType type )
{
	return
		( type >= U_FundamentalType::i8 && type <= U_FundamentalType::u64 ) ||
		type == U_FundamentalType::f32 ||
		type == U_FundamentalType::f64;
}

bool IsUnsignedInteger( U_FundamentalType type )
{
	return
		type == U_FundamentalType::u8  ||
		type == U_FundamentalType::u16 ||
		type == U_FundamentalType::u32 ||
		type == U_FundamentalType::u64;
}

static bool IsSignedInteger( U_FundamentalType type )
{
	return
		type == U_FundamentalType::i8  ||
		type == U_FundamentalType::i16 ||
		type == U_FundamentalType::i32 ||
		type == U_FundamentalType::i64;
}

static bool IsInteger( U_FundamentalType type )
{
	return IsSignedInteger( type ) || IsUnsignedInteger( type );
}

bool IsFloatingPoint( U_FundamentalType type )
{
	return
		type == U_FundamentalType::f32 ||
		type == U_FundamentalType::f64;
}

U_FundamentalType GetNumericConstantType( const NumericConstant& number )
{
	if( number.type_suffix_.empty() )
	{
		if( number.has_fractional_point_ )
			return U_FundamentalType::f64;
		else
			return U_FundamentalType::i32;
	}

	auto it= g_types_map.find( number.type_suffix_ );
	if( it == g_types_map.end() )
		return U_FundamentalType::InvalidType;

	return it->second;
}

} // namespace

namespace CodeBuilderLLVMPrivate
{

CodeBuilderLLVM::FunctionContext::FunctionContext(
	llvm::LLVMContext& llvm_context,
	llvm::Function* in_function )
	: function(in_function)
	, function_basic_block( llvm::BasicBlock::Create( llvm_context, "", function ) )
	, llvm_ir_builder( function_basic_block )
	, block_for_break( nullptr )
	, block_for_continue( nullptr )
{
}

CodeBuilderLLVM::CodeBuilderLLVM()
	: llvm_context_( llvm::getGlobalContext() )
{
	fundamental_llvm_types_. i8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_. u8= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.i16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.u16= llvm::Type::getInt16Ty( llvm_context_ );
	fundamental_llvm_types_.i32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.u32= llvm::Type::getInt32Ty( llvm_context_ );
	fundamental_llvm_types_.i64= llvm::Type::getInt64Ty( llvm_context_ );
	fundamental_llvm_types_.u64= llvm::Type::getInt64Ty( llvm_context_ );

	fundamental_llvm_types_.f32= llvm::Type::getFloatTy( llvm_context_ );
	fundamental_llvm_types_.f64= llvm::Type::getDoubleTy( llvm_context_ );

	fundamental_llvm_types_.invalid_type_= llvm::Type::getInt8Ty( llvm_context_ );
	fundamental_llvm_types_.void_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );
}

CodeBuilderLLVM::~CodeBuilderLLVM()
{
}

CodeBuilderLLVM::BuildResult CodeBuilderLLVM::BuildProgram( const ProgramElements& program_elements )
{
	module_= std::unique_ptr<llvm::Module>( new llvm::Module( "U-Module", llvm_context_ ) );
	errors_.clear();
	error_count_= 0u;

	for( const IProgramElementPtr& program_element : program_elements )
	{
		if( const FunctionDeclaration* func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			if( IsKeyword( func->name_ ) )
				errors_.push_back( ReportUsingKeywordAsName( func->file_pos_ ) );

			Variable func_info;

			func_info.location= Variable::Location::Global;
			func_info.type.kind= Type::Kind::Function;

			// Return type.
			// TODO - add support for non-fundamental types.
			func_info.type.function.reset( new Function() );
			func_info.type.function->return_type.kind= Type::Kind::Fundamental;
			if( func->return_type_.empty() )
			{
				func_info.type.function->return_type.fundamental= U_FundamentalType::Void;
			}
			else
			{
				auto it= g_types_map.find( func->return_type_ );
				if( it == g_types_map.end() )
				{
					errors_.push_back( ReportNameNotFound( func->file_pos_, func->return_type_ ) );
					func_info.type.function->return_type.fundamental= U_FundamentalType::InvalidType;
				}
				else
					func_info.type.function->return_type.fundamental= it->second;
			}
			func_info.type.function->return_type.fundamental_llvm_type=
				GetFundamentalLLVMType( func_info.type.function->return_type.fundamental );

			if( global_names_.GetName( func->name_ ) != nullptr )
			{
				errors_.push_back( ReportRedefinition( func->file_pos_, func->name_ ) );
				continue;
			}
			else
			{
				// Args.
				func_info.type.function->args.reserve( func->arguments_.size() );
				for( const VariableDeclaration& arg : func->arguments_ )
				{
					if( IsKeyword( arg.name ) )
						errors_.push_back( ReportUsingKeywordAsName( arg.file_pos_ ) );

					func_info.type.function->args.push_back( PrepareType( arg.file_pos_, arg.type ) );
				}

				BuildFuncCode(
					func_info,
					func->name_,
					func->arguments_,
					*func->block_ );

				global_names_.AddName( func->name_, std::move( func_info ) );
			}
		}
		else if(
			const ClassDeclaration* class_=
			dynamic_cast<const ClassDeclaration*>( program_element.get() ) )
		{
			const NamesScope::InsertedName* inserted_name=
				global_names_.AddName( class_->name_, PrepareClass( *class_ ) );
			if( inserted_name == nullptr )
			{
				errors_.push_back( ReportRedefinition( class_->file_pos_, class_->name_ ) );
			}
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements

	if( error_count_ > 0u )
		errors_.push_back( ReportBuildFailed() );

	BuildResult result;
	result.errors= errors_;
	errors_.clear();
	result.module= std::move( module_ );
	return result;
}

Type CodeBuilderLLVM::PrepareType( const FilePos& file_pos, const TypeName& type_name )
{
	Type result;
	Type* last_type= &result;

	constexpr unsigned int c_max_array_dimensions= 16u;
	Type* arrays_stack[ c_max_array_dimensions ];
	unsigned int arrays_count= 0u;

	// Fill arrays hierarchy.
	for( auto rit= type_name.array_sizes.rbegin(); rit != type_name.array_sizes.rend(); ++rit )
	{
		if( arrays_count >= c_max_array_dimensions )
		{
			U_ASSERT( false && "WTF?" );
		}

		arrays_stack[ arrays_count ]= last_type;
		arrays_count++;

		const NumericConstant& num= * *rit;

		last_type->kind= Type::Kind::Array;
		last_type->array.reset( new Array() );

		U_FundamentalType size_type= GetNumericConstantType( num );
		if( !IsInteger(size_type) )
			errors_.push_back( ReportArraySizeIsNotInteger( num.file_pos_ ) );
		if( num.value_ < 0 )
			errors_.push_back( ReportArraySizeIsNegative( num.file_pos_ ) );

		last_type->array->size= size_t( std::max( num.value_, static_cast<NumericConstant::LongFloat>(0.0) ) );

		last_type= &last_type->array->type;
	}

	last_type->kind= Type::Kind::Fundamental;

	auto it= g_types_map.find( type_name.name );
	if( it == g_types_map.end() )
	{
		const NamesScope::InsertedName* custom_type_name=
			global_names_.GetName( type_name.name );
		if( custom_type_name != nullptr )
		{
			if( custom_type_name->second.class_ != nullptr )
			{
				last_type->class_= custom_type_name->second.class_;
				last_type->kind= Type::Kind::Class;
			}
			else
				errors_.push_back( ReportNameIsNotTypeName( file_pos, type_name.name ) );

		}
		else
		{
			errors_.push_back( ReportNameNotFound( file_pos, type_name.name ) );
			last_type->fundamental= U_FundamentalType::InvalidType;
			last_type->fundamental_llvm_type= GetFundamentalLLVMType( last_type->fundamental );
		}
	}
	else
	{
		last_type->fundamental= it->second;
		last_type->fundamental_llvm_type= GetFundamentalLLVMType( last_type->fundamental );
	}

	// Setup arrays llvm types.
	if( arrays_count > 0u )
	{
		arrays_stack[ arrays_count - 1u ]->array->llvm_type=
			llvm::ArrayType::get(
				last_type->GetLLVMType(),
				arrays_stack[ arrays_count - 1u ]->array->size );

		for( unsigned int i= arrays_count - 1u; i > 0u; i-- )
			arrays_stack[ i - 1u ]->array->llvm_type=
				llvm::ArrayType::get(
					arrays_stack[i]->array->llvm_type,
					arrays_stack[ i - 1u ]->array->size );
	}

	return result;
}

ClassPtr CodeBuilderLLVM::PrepareClass( const ClassDeclaration& class_declaration )
{
	if( IsKeyword( class_declaration.name_ ) )
		errors_.push_back( ReportUsingKeywordAsName( class_declaration.file_pos_ ) );

	ClassPtr result= std::make_shared<Class>();

	result->name= class_declaration.name_;

	std::vector<llvm::Type*> members_llvm_types;

	members_llvm_types.reserve( class_declaration.fields_.size() );
	result->fields.reserve( class_declaration.fields_.size() );
	for( const ClassDeclaration::Field& in_field : class_declaration.fields_ )
	{
		if( result->GetField( in_field.name ) != nullptr )
			errors_.push_back( ReportRedefinition( in_field.file_pos, in_field.name ) );

		Class::Field out_field;
		out_field.name= in_field.name;
		out_field.type= PrepareType( in_field.file_pos, in_field.type );
		out_field.index= result->fields.size();

		members_llvm_types.emplace_back( out_field.type.GetLLVMType() );
		result->fields.emplace_back( std::move( out_field ) );
	}

	result->llvm_type=
		llvm::StructType::create(
			llvm_context_,
			members_llvm_types,
			ToStdString(class_declaration.name_) );

	return result;
}

void CodeBuilderLLVM::BuildFuncCode(
	Variable& func_variable,
	const ProgramString& func_name,
	const std::vector<VariableDeclaration>& args,
	const Block& block ) noexcept
{
	//func.type.kind= Type::Kind::Function;
	//func.type.function.reset( new Function );

	std::vector<llvm::Type*> args_llvm_types;
	for( const Type& type : func_variable.type.function->args )
		args_llvm_types.push_back( type.GetLLVMType() );

	func_variable.type.function->llvm_function_type=
		llvm::FunctionType::get(
			func_variable.type.function->return_type.GetLLVMType(),
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(),args_llvm_types.size() ),
			false );

	llvm::Function* llvm_function=
		llvm::Function::Create(
			func_variable.type.function->llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			ToStdString( func_name ),
			module_.get() );

	NamesScope function_names( &global_names_ );
	FunctionContext function_context( llvm_context_, llvm_function );

	unsigned int arg_number= 0u;
	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		Variable var;
		var.type= func_variable.type.function->args[ arg_number ];
		var.location= Variable::Location::LLVMRegister;
		var.llvm_value= &llvm_arg;

		// Move parameters to stack for assignment possibility.
		// TODO - do it, only if parameters are not constant.
		if( var.location == Variable::Location::LLVMRegister )
		{
			llvm::Value* address= function_context.llvm_ir_builder.CreateAlloca( var.type.GetLLVMType() );
			function_context.llvm_ir_builder.CreateStore( var.llvm_value, address );

			var.llvm_value= address;
			var.location= Variable::Location::PointerToStack;
		}

		const NamesScope::InsertedName* inserted_arg=
			function_names.AddName(
				args[ arg_number ].name,
				std::move(var) );
		if( !inserted_arg )
		{
			errors_.push_back( ReportRedefinition( args[ arg_number ].file_pos_, args[ arg_number ].name ) );
			return;
		}

		llvm_arg.setName( ToStdString( args[ arg_number ].name ) );
		++arg_number;
	}

	func_variable.llvm_value= llvm_function;

	BuildBlockCode( block, function_names, function_context );
}

void CodeBuilderLLVM::BuildBlockCode(
	const Block& block,
	const NamesScope& names,
	FunctionContext& function_context ) noexcept
{
	NamesScope block_names( &names );

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		const IBlockElement* const block_element_ptr= block_element.get();

		try
		{
			if( const VariableDeclaration* variable_declaration=
				dynamic_cast<const VariableDeclaration*>( block_element_ptr ) )
			{
				if( IsKeyword( variable_declaration->name ) )
					errors_.push_back( ReportUsingKeywordAsName( variable_declaration->file_pos_ ) );

				Variable variable;
				variable.type= PrepareType( variable_declaration->file_pos_, variable_declaration->type );
				variable.location= Variable::Location::PointerToStack;
				variable.llvm_value= function_context.llvm_ir_builder.CreateAlloca( variable.type.GetLLVMType() );

				const NamesScope::InsertedName* inserted_name=
					block_names.AddName( variable_declaration->name, std::move(variable) );

				if( !inserted_name )
				{
					errors_.push_back( ReportRedefinition( variable_declaration->file_pos_, variable_declaration->name ) );
					throw ProgramError();
				}

				// TODO - add initisalizer.
			}
			else if(
				const SingleExpressionOperator* expression=
				dynamic_cast<const SingleExpressionOperator*>( block_element_ptr ) )
			{
				BuildExpressionCode(
					*expression->expression_,
					block_names,
					function_context );
			}
			else if(
				const AssignmentOperator* assignment_operator=
				dynamic_cast<const AssignmentOperator*>( block_element_ptr ) )
			{
				const BinaryOperatorsChain& l_value= *assignment_operator->l_value_;
				const BinaryOperatorsChain& r_value= *assignment_operator->r_value_;

				const Variable l_var= BuildExpressionCode( l_value, block_names, function_context );
				const Variable r_var= BuildExpressionCode( r_value, block_names, function_context );

				if( l_var.type != r_var.type )
				{
					// TODO - report types mismatch.
					throw ProgramError();
				}

				if( l_var.type.kind == Type::Kind::Fundamental )
				{
					if( l_var.location != Variable::Location::PointerToStack )
					{
						// TODO - write correct lvalue/rvalue flag into variable.
						throw ProgramError();
					}
					llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
					function_context.llvm_ir_builder.CreateStore( value_for_assignment, l_var.llvm_value );
				}
				else if( l_var.type.kind == Type::Kind::Function )
				{
					// TODO - functions is not copyable.
					throw ProgramError();
				}
				else if( l_var.type.kind == Type::Kind::Array )
				{
					// TODO - arrays not copyable.
					throw ProgramError();
				}
				else if( l_var.type.kind == Type::Kind::Class )
				{
					errors_.push_back( ReportNotImplemented( assignment_operator->file_pos_, "class assignment" ) );
					throw ProgramError();
				}
			}
			else if(
				const ReturnOperator* return_operator=
				dynamic_cast<const ReturnOperator*>( block_element_ptr ) )
			{
				BuildReturnOperatorCode(
					*return_operator,
					block_names,
					function_context );
			}
			else if(
				const WhileOperator* while_operator=
				dynamic_cast<const WhileOperator*>( block_element_ptr ) )
			{
				BuildWhileOperatorCode(
					*while_operator,
					block_names,
					function_context );
			}
			else if(
				const BreakOperator* break_operator=
				dynamic_cast<const BreakOperator*>( block_element_ptr ) )
			{
				BuildBreakOperatorCode(
					*break_operator,
					function_context );
			}
			else if(
				const ContinueOperator* continue_operator=
				dynamic_cast<const ContinueOperator*>( block_element_ptr ) )
			{
				BuildContinueOperatorCode(
					*continue_operator,
					function_context );
			}
			else if(
				const IfOperator* if_operator=
				dynamic_cast<const IfOperator*>( block_element_ptr ) )
			{
				BuildIfOperatorCode(
					*if_operator,
					block_names,
					function_context );
			}
			else if(
				const Block* block=
				dynamic_cast<const Block*>( block_element_ptr ) )
			{
				BuildBlockCode( *block, block_names, function_context );
			}
			else
			{
				U_ASSERT(false);
			}
		}
		catch( const ProgramError& )
		{
			error_count_++;
		}
	}
}

Variable CodeBuilderLLVM::BuildExpressionCode(
	const BinaryOperatorsChain& expression,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const InversePolishNotation ipn= ConvertToInversePolishNotation( expression );

	return
		BuildExpressionCode_r(
			ipn,
			ipn.size() - 1,
			names,
			function_context );
}

Variable CodeBuilderLLVM::BuildExpressionCode_r(
	const InversePolishNotation& ipn,
	unsigned int ipn_index,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( ipn_index < ipn.size() );
	const InversePolishNotationComponent& comp= ipn[ ipn_index ];

	const FilePos file_pos = ipn.front().operand->file_pos_;

	if( comp.operator_ != BinaryOperator::None )
	{
		Variable l_var=
			BuildExpressionCode_r(
				ipn, comp.l_index,
				names,
				function_context );

		Variable r_var=
			BuildExpressionCode_r(
				ipn, comp.r_index,
				names,
				function_context );

		Variable result;

		// TODO - add cast for some integers here.
		if( r_var.type != l_var.type )
		{
			// TODO - report types mismatch.
			throw ProgramError();
		}

		const Type& result_type= r_var.type;

		switch( comp.operator_ )
		{
		case BinaryOperator::Add:
		case BinaryOperator::Sub:
		case BinaryOperator::Div:
		case BinaryOperator::Mul:

			if( result_type.kind != Type::Kind::Fundamental )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}
			else
			{
				if( result_type.SizeOf() < 4u )
				{
					// Operation supported only for 32 and 64bit operands
					errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
					throw ProgramError();
				}
				const bool is_float= IsFloatingPoint( result_type.fundamental );
				if( !( IsInteger( result_type.fundamental ) || is_float ) )
				{
					// this operations allowed only for integer and floating point operands.
					errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
					throw ProgramError();
				}

				const bool is_signed= IsSignedInteger( result_type.fundamental );

				llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				llvm::Value* result_value;

				switch( comp.operator_ )
				{
				case BinaryOperator::Add:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Sub:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Div:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFDiv( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result_value=
							function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Mul:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFMul( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateMul( l_value_for_op, r_value_for_op );
					break;

				default: U_ASSERT( false ); break;
				};

				result.location= Variable::Location::LLVMRegister;
				result.type= r_var.type;
				result.llvm_value= result_value;
			}
			break;


		case BinaryOperator::Equal:
		case BinaryOperator::NotEqual:
		if( result_type.kind != Type::Kind::Fundamental )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( result_type.fundamental );
			if( !( IsInteger( result_type.fundamental ) || if_float || result_type.fundamental == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( comp.operator_ )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperator::Equal:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUEQ( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::NotEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUNE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.type.kind= Type::Kind::Fundamental;
			result.type.fundamental= U_FundamentalType::Bool;
			result.type.fundamental_llvm_type= fundamental_llvm_types_.bool_;
			result.llvm_value= result_value;
		}
			break;

		case BinaryOperator::Less:
		case BinaryOperator::LessEqual:
		case BinaryOperator::Greater:
		case BinaryOperator::GreaterEqual:
		if( result_type.kind != Type::Kind::Fundamental )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( result_type.fundamental );
			const bool is_signed= IsSignedInteger( result_type.fundamental );
			if( !( IsInteger( result_type.fundamental ) || if_float ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( comp.operator_ )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperator::Less:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::LessEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::Greater:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::GreaterEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.type.kind= Type::Kind::Fundamental;
			result.type.fundamental= U_FundamentalType::Bool;
			result.type.fundamental_llvm_type= fundamental_llvm_types_.bool_;
			result.llvm_value= result_value;
		}
			break;

		case BinaryOperator::And:
		case BinaryOperator::Or:
		case BinaryOperator::Xor:
		if( result_type.kind != Type::Kind::Fundamental )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			if( !( IsInteger( result_type.fundamental ) || result_type.fundamental == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( comp.operator_ )
			{
			case BinaryOperator::And:
				result_value=
					function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperator::Or:
				result_value=
					function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperator::Xor:
				result_value=
					function_context.llvm_ir_builder.CreateXor( l_value_for_op, r_value_for_op );
				break;
			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.type= result_type;
			result.llvm_value= result_value;
		}
			break;

		case BinaryOperator::LazyLogicalAnd:
		case BinaryOperator::LazyLogicalOr:
		case BinaryOperator::None:
		case BinaryOperator::Last:
			U_ASSERT(false);
			break;
		};

		return result;
	}
	else
	{
		U_ASSERT( comp.operand );
		U_ASSERT( comp.r_index == InversePolishNotationComponent::c_no_parent );
		U_ASSERT( comp.l_index == InversePolishNotationComponent::c_no_parent );

		const IBinaryOperatorsChainComponent& operand= *comp.operand;

		Variable result;

		if( const NamedOperand* named_operand=
			dynamic_cast<const NamedOperand*>(&operand) )
		{
			const NamesScope::InsertedName* name_entry=
				names.GetName( named_operand->name_ );
			if( !name_entry )
			{
				errors_.push_back( ReportNameNotFound( named_operand->file_pos_, named_operand->name_ ) );
				throw ProgramError();
			}
			if( name_entry->second.class_ )
			{
				// TODO - using class name sa variable.
				throw ProgramError();
			}
			result= name_entry->second.variable;
		}
		else if( const NumericConstant* numeric_constant=
			dynamic_cast<const NumericConstant*>(&operand) )
		{
			U_FundamentalType type= GetNumericConstantType( *numeric_constant );
			if( type == U_FundamentalType::InvalidType )
			{
				// TODO - report unknown numeric constant type.
				throw ProgramError();
			}

			result.location= Variable::Location::LLVMRegister;
			result.type.kind= Type::Kind::Fundamental;
			result.type.fundamental= type;

			llvm::Type* llvm_type= GetFundamentalLLVMType( type );

			if( IsInteger( type ) )
				result.llvm_value=
					llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( result.type.SizeOf() * 8u, uint64_t(numeric_constant->value_) ) );
			else if( IsFloatingPoint( type ) )
				result.llvm_value=
					llvm::ConstantFP::get( llvm_type, static_cast<double>( numeric_constant->value_) );
			else
			{
				U_ASSERT(false);
			}

			result.type.fundamental_llvm_type= llvm_type;
		}
		else if( const BooleanConstant* boolean_constant=
			dynamic_cast<const BooleanConstant*>(&operand) )
		{
			result.location= Variable::Location::LLVMRegister;
			result.type.kind= Type::Kind::Fundamental;
			result.type.fundamental= U_FundamentalType::Bool;
			result.type.fundamental_llvm_type= fundamental_llvm_types_.bool_;

			result.llvm_value=
				llvm::Constant::getIntegerValue(
					result.type.fundamental_llvm_type,
					llvm::APInt( 1u, uint64_t(boolean_constant->value_) ) );
		}
		else if( const BracketExpression* bracket_expression=
			dynamic_cast<const BracketExpression*>(&operand) )
		{
			result= BuildExpressionCode( *bracket_expression->expression_, names, function_context );
		}
		else
		{
			// TODO
			U_ASSERT(false);
		}

		for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operand_operators )
		{
			if( const IndexationOperator* const indexation_operator=
				dynamic_cast<const IndexationOperator*>( postfix_operator.get() ) )
			{
				if( result.type.kind != Type::Kind::Array )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( indexation_operator->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}

				Variable index=
					BuildExpressionCode(
						*indexation_operator->index_,
						names,
						function_context );

				if( index.type.kind != Type::Kind::Fundamental || !IsUnsignedInteger( index.type.fundamental ) )
				{
					errors_.push_back( ReportTypesMismatch( indexation_operator->file_pos_, "any unsigned integer"_SpC, index.type.ToString() ) );
					throw ProgramError();
				}

				if( result.location != Variable::Location::PointerToStack )
				{
					// TODO - Strange variable location.
					throw ProgramError();
				}

				result.type= result.type.array->type;
				result.location= Variable::Location::PointerToStack;

				// Make first index = 0 for array to pointer conversion.
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

				result.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( result.llvm_value, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );
			}
			else if( const MemberAccessOperator* const member_access_operator=
				dynamic_cast<const MemberAccessOperator*>( postfix_operator.get() ) )
			{
				if( result.type.kind != Type::Kind::Class )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( member_access_operator->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				U_ASSERT( result.type.class_ );

				const Class::Field* field= result.type.class_->GetField( member_access_operator->member_name_ );
				if( field == nullptr )
				{
					errors_.push_back( ReportNameNotFound( member_access_operator->file_pos_, member_access_operator->member_name_ ) );
					throw ProgramError();
				}

				U_ASSERT( result.location == Variable::Location::PointerToStack );

				// Make first index = 0 for array to pointer conversion.
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

				result.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( result.llvm_value, index_list );
				result.type= field->type;
				result.location= Variable::Location::PointerToStack;
			}
			else if( const CallOperator* const call_operator=
				dynamic_cast<const CallOperator*>( postfix_operator.get() ) )
			{
				if( result.type.kind != Type::Kind::Function )
				{
					// TODO - Call of non-function.
					throw ProgramError();
				}
				if( call_operator->arguments_.size() != result.type.function->args.size() )
				{
					errors_.push_back( ReportFunctionSignatureMismatch( call_operator->file_pos_ ) );
					throw ProgramError();
				}

				std::vector<llvm::Value*> llvm_args;
				llvm_args.resize( result.type.function->args.size() );

				for( unsigned int i= 0u; i < result.type.function->args.size(); i++ )
				{
					Variable arg= BuildExpressionCode( *call_operator->arguments_[i], names, function_context );
					if( arg.type != result.type.function->args[i] )
					{
						errors_.push_back( ReportFunctionSignatureMismatch( call_operator->arguments_[i]->file_pos_ ) );
						throw ProgramError();
					}

					llvm_args[i]= CreateMoveToLLVMRegisterInstruction( arg, function_context );
				}

				llvm::Value* call_result=
					function_context.llvm_ir_builder.CreateCall(
						llvm::dyn_cast<llvm::Function>(result.llvm_value),
						llvm_args );

				result.type= result.type.function->return_type;
				result.location= Variable::Location::LLVMRegister;
				result.llvm_value= call_result;
			}
			else
			{
				//TODO
				U_ASSERT(false);
			}

		} // for unary postfix operators

		for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operand_operators )
		{
			if( const UnaryMinus* const unary_minus=
				dynamic_cast<const UnaryMinus*>( prefix_operator.get() ) )
			{
				(void)unary_minus;

				if( result.type.kind != Type::Kind::Fundamental )
				{
					// TODO - report invalid type.
					errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				const bool is_float= IsFloatingPoint( result.type.fundamental );
				if( !( IsInteger( result.type.fundamental ) || is_float ) )
				{
					errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus->file_pos_, result.type.ToString() ) );
					throw ProgramError();
				}
				// TODO - maybe not support unary minus for 8 and 16 bot integer types?

				llvm::Value* value_for_neg= CreateMoveToLLVMRegisterInstruction( result, function_context );
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFNeg( value_for_neg );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateNeg( value_for_neg );

				result.location= Variable::Location::LLVMRegister;
			}
			else if( const UnaryPlus* const unary_plus=
				dynamic_cast<const UnaryPlus*>( prefix_operator.get() ) )
			{
				(void)unary_plus;
				// DO NOTHING
			}
			// TODO
		} // for unary prefix operators

		return result;
	}
}

void CodeBuilderLLVM::BuildReturnOperatorCode(
	const ReturnOperator& return_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{

	// TODO - check function result/expression result types mismatch.

	if( return_operator.expression_ == nullptr )
	{
		// Add only return instruction for void return operators.
		function_context.llvm_ir_builder.CreateRetVoid();
		return;
	}

	const Variable expression_result=
		BuildExpressionCode(
			*return_operator.expression_,
			names,
			function_context );

	llvm::Value* value_for_return= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
	function_context.llvm_ir_builder.CreateRet( value_for_return );
}

void CodeBuilderLLVM::BuildWhileOperatorCode(
	const WhileOperator& while_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	llvm::BasicBlock* test_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* while_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* block_after_while= llvm::BasicBlock::Create( llvm_context_ );

	// Break to test block. We must push terminal instruction at and of current block.
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Test block code.
	function_context.function->getBasicBlockList().push_back( test_block );
	function_context.llvm_ir_builder.SetInsertPoint( test_block );

	Variable condition_expression= BuildExpressionCode( *while_operator.condition_, names, function_context );
	if( condition_expression.type.kind != Type::Kind::Fundamental ||
		condition_expression.type.fundamental != U_FundamentalType::Bool )
	{
		errors_.push_back(
			ReportTypesMismatch(
				while_operator.condition_->file_pos_,
				GetFundamentalTypeName( U_FundamentalType::Bool ),
				condition_expression.type.ToString() ) );
		throw ProgramError();
	}

	llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, while_block, block_after_while );

	// While block code.

	// Save previous while block break/continue labels.
	// WARNING - between save/restore nobody should throw exceptions!!!
	llvm::BasicBlock* const prev_block_for_break   = function_context.block_for_break   ;
	llvm::BasicBlock* const prev_block_for_continue= function_context.block_for_continue;
	// Set current while block labels.
	function_context.block_for_break   = block_after_while;
	function_context.block_for_continue= test_block;

	function_context.function->getBasicBlockList().push_back( while_block );
	function_context.llvm_ir_builder.SetInsertPoint( while_block );

	BuildBlockCode( *while_operator.block_, names, function_context );
	function_context.llvm_ir_builder.CreateBr( test_block );

	// Restore previous while block break/continue labels.
	function_context.block_for_break   = prev_block_for_break   ;
	function_context.block_for_continue= prev_block_for_continue;

	// Block after while code.
	function_context.function->getBasicBlockList().push_back( block_after_while );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_while );
}

void CodeBuilderLLVM::BuildBreakOperatorCode(
	const BreakOperator& break_operator,
	FunctionContext& function_context ) noexcept
{
	if( function_context.block_for_break == nullptr )
	{
		errors_.push_back( ReportBreakOutsideLoop( break_operator.file_pos_ ) );
		return;
	}

	function_context.llvm_ir_builder.CreateBr( function_context.block_for_break );
}

void CodeBuilderLLVM::BuildContinueOperatorCode(
	const ContinueOperator& continue_operator,
	FunctionContext& function_context ) noexcept
{
	if( function_context.block_for_continue == nullptr )
	{
		errors_.push_back( ReportContinueOutsideLoop( continue_operator.file_pos_ ) );
		return;
	}

	function_context.llvm_ir_builder.CreateBr( function_context.block_for_continue );
}

void CodeBuilderLLVM::BuildIfOperatorCode(
	const IfOperator& if_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( !if_operator.branches_.empty() );

	// TODO - optimize this method. Make less basic blocks.
	//

	llvm::BasicBlock* block_after_if= llvm::BasicBlock::Create( llvm_context_ );

	llvm::BasicBlock* next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
	// Break to first condition. We must push terminal instruction at end of current block.
	function_context.llvm_ir_builder.CreateBr( next_condition_block );

	for( unsigned int i= 0u; i < if_operator.branches_.size(); i++ )
	{
		const IfOperator::Branch& branch= if_operator.branches_[i];

		llvm::BasicBlock* body_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* current_condition_block= next_condition_block;

		if( i + 1u < if_operator.branches_.size() )
			next_condition_block= llvm::BasicBlock::Create( llvm_context_ );
		else
			next_condition_block= block_after_if;

		// Build condition block.
		function_context.function->getBasicBlockList().push_back( current_condition_block );
		function_context.llvm_ir_builder.SetInsertPoint( current_condition_block );

		if( branch.condition == nullptr )
		{
			U_ASSERT( i + 1u == if_operator.branches_.size() );

			// Make empty condition block - move to it unconditional break to body.
			function_context.llvm_ir_builder.CreateBr( body_block );
		}
		else
		{
			Variable condition_expression= BuildExpressionCode( *branch.condition, names, function_context );
			if( condition_expression.type.kind != Type::Kind::Fundamental ||
				condition_expression.type.fundamental != U_FundamentalType::Bool )
			{
				errors_.push_back(
					ReportTypesMismatch(
						branch.condition->file_pos_,
						GetFundamentalTypeName( U_FundamentalType::Bool ),
						condition_expression.type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* condition_in_register= CreateMoveToLLVMRegisterInstruction( condition_expression, function_context );
			function_context.llvm_ir_builder.CreateCondBr( condition_in_register, body_block, next_condition_block );
		}

		// Make body block code.
		function_context.function->getBasicBlockList().push_back( body_block );
		function_context.llvm_ir_builder.SetInsertPoint( body_block );
		BuildBlockCode( *branch.block, names, function_context );
		function_context.llvm_ir_builder.CreateBr( block_after_if );
	}

	U_ASSERT( next_condition_block == block_after_if );

	// Block after if code.
	function_context.function->getBasicBlockList().push_back( block_after_if );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
}

llvm::Type* CodeBuilderLLVM::GetFundamentalLLVMType( const U_FundamentalType fundmantal_type )
{
	switch( fundmantal_type )
	{
	case U_FundamentalType::InvalidType:
		return fundamental_llvm_types_.invalid_type_;
	case U_FundamentalType::LastType:
		break;

	case U_FundamentalType::Void:
		return fundamental_llvm_types_.void_;
	case U_FundamentalType::Bool:
		return fundamental_llvm_types_.bool_;
	case U_FundamentalType::i8 :
		return fundamental_llvm_types_.i8 ;
	case U_FundamentalType::u8 :
		return fundamental_llvm_types_.u8 ;
	case U_FundamentalType::i16:
		return fundamental_llvm_types_.i16;
	case U_FundamentalType::u16:
		return fundamental_llvm_types_.u16;
	case U_FundamentalType::i32:
		return fundamental_llvm_types_.i32;
	case U_FundamentalType::u32:
		return fundamental_llvm_types_.u32;
	case U_FundamentalType::i64:
		return fundamental_llvm_types_.i64;
	case U_FundamentalType::u64:
		return fundamental_llvm_types_.u64;
	case U_FundamentalType::f32:
		return fundamental_llvm_types_.f32;
	case U_FundamentalType::f64:
		return fundamental_llvm_types_.f64;
	};

	U_ASSERT(false);
	return nullptr;
}

llvm::Value*CodeBuilderLLVM::CreateMoveToLLVMRegisterInstruction(
	const Variable& variable, FunctionContext& function_context )
{
	llvm::Value* register_value= nullptr;
	if( variable.location == Variable::Location::LLVMRegister )
		register_value= variable.llvm_value;
	else if( variable.location == Variable::Location::PointerToStack )
		register_value= function_context.llvm_ir_builder.CreateLoad( variable.llvm_value );
	else
	{
		U_ASSERT(false);
	}

	return register_value;
}

} // namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
