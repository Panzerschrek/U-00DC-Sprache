#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>

#include "keywords.hpp"
#include "vm.hpp"

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

const char* const g_fundamental_types_names[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, "InvalidType" ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  KeywordAscii( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, KeywordAscii( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , KeywordAscii( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , KeywordAscii( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, KeywordAscii( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, KeywordAscii( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, KeywordAscii( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, KeywordAscii( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, KeywordAscii( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, KeywordAscii( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, KeywordAscii( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, KeywordAscii( Keywords::f64_ ) ),
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

void ReportUsingKeywordAsName(
	std::vector<std::string>& error_messages,
	const ProgramString& name )
{
	error_messages.push_back(
		"Using keyword as name: " + ToStdString(name) );
}

void ReportUnknownFuncReturnType(
	std::vector<std::string>& error_messages,
	const FunctionDeclaration& func )
{
	error_messages.push_back(
		"Unknown return type " + ToStdString( func.return_type_ ) +
		" for function " + ToStdString( func.name_ ) );
}

void ReportUnknownVariableType(
	std::vector<std::string>& error_messages,
	const TypeName& type_name )
{
	error_messages.push_back(
		"Variable has unknown type " + ToStdString( type_name.name ) );
}

void ReportNameNotFound(
	std::vector<std::string>& error_messages,
	const ProgramString& name )
{
	error_messages.push_back(
		ToStdString( name ) +
		" was not declarated in this scope" );
}

void ReportNotImplemented(
	std::vector<std::string>& error_messages,
	const char* what )
{
	error_messages.push_back(
		std::string("Sorry, ") +
		what +
		" not implemented" );
}

void ReportRedefinition(
	std::vector<std::string>& error_messages,
	const ProgramString& name )
{
	error_messages.push_back(
		ToStdString(name) +
		" redifinition" );
}

void ReportTypesMismatch(
	std::vector<std::string>& error_messages,
	U_FundamentalType type,
	U_FundamentalType expected_type )
{
	error_messages.push_back(
		std::string("Unexpected type: ") +
		g_fundamental_types_names[ size_t(type) ] +
		" expected " +
		g_fundamental_types_names[ size_t(expected_type) ]);
}

void ReportArgumentsCountMismatch(
	std::vector<std::string>& error_messages,
	unsigned int count,
	unsigned int expected )
{
	error_messages.push_back(
		"Arguments count mismatch. actual " +
		std::to_string(count) +
		" expected " +
		std::to_string(expected) );
}

void ReportArithmeticOperationWithUnsupportedType(
	std::vector<std::string>& error_messages,
	U_FundamentalType type )
{
	error_messages.push_back(
		"Expected numeric arguments for arithmetic operators. Supported 32 and 64 bit types. Got " +
		std::string( g_fundamental_types_names[ size_t(type) ] ) );
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

	fundamental_llvm_types_.void_= llvm::Type::getVoidTy( llvm_context_ );
	fundamental_llvm_types_.bool_= llvm::Type::getInt1Ty( llvm_context_ );
}

CodeBuilderLLVM::~CodeBuilderLLVM()
{
}

CodeBuilderLLVM::BuildResult CodeBuilderLLVM::BuildProgram( const ProgramElements& program_elements )
{
	module_= std::unique_ptr<llvm::Module>( new llvm::Module( "U-Module", llvm_context_ ) );
	error_count_= 0u;

	for( const IProgramElementPtr& program_element : program_elements )
	{
		if( const FunctionDeclaration* func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			if( IsKeyword( func->name_ ) )
				ReportUsingKeywordAsName( error_messages_, func->name_ );

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
					++error_count_;
					ReportUnknownFuncReturnType( error_messages_, *func );
					func_info.type.function->return_type.fundamental= U_FundamentalType::Void;
				}
				else
					func_info.type.function->return_type.fundamental= it->second;
			}
			func_info.type.function->return_type.fundamental_llvm_type=
				GetFundamentalLLVMType( func_info.type.function->return_type.fundamental );

			if( global_names_.GetName( func->name_ ) != nullptr )
			{
				error_count_++;
				ReportRedefinition( error_messages_, func->name_ );
				continue;
			}
			else
			{
				// Args.
				std::vector<ProgramString> arg_names;
				arg_names.reserve( func->arguments_.size() );

				func_info.type.function->args.reserve( func->arguments_.size() );
				for( const VariableDeclaration& arg : func->arguments_ )
				{
					if( IsKeyword( arg.name ) )
						ReportUsingKeywordAsName( error_messages_, arg.name );

					func_info.type.function->args.push_back( PrepareType( arg.type ) );
					arg_names.push_back( arg.name );
				}

				BuildFuncCode(
					func_info,
					func->name_,
					arg_names,
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
				error_count_++;
				error_messages_.push_back(
					ToStdString( class_->name_ ) +
					" redefinition" );
			}
		}
		else
		{
			U_ASSERT(false);
		}
	} // for program elements

	if( error_count_ > 0u )
		error_messages_.emplace_back( "Code build failed - there are some errors." );

	BuildResult result;
	result.error_messages= error_messages_;
	error_messages_.clear();
	result.module= std::move( module_ );
	return result;
}

Type CodeBuilderLLVM::PrepareType( const TypeName& type_name )
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
		if( !( IsInteger(size_type) && num.value_ >= 0 ) )
			error_messages_.push_back( "Error, array size must be nonnegative integer" );

		last_type->array->size= size_t(num.value_);

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
			{
				error_messages_.push_back(
					"Using name, which is not type, as type name: " +
					ToStdString( type_name.name ) );
			}
		}
		else
		{
			last_type->fundamental= U_FundamentalType::i32;
			ReportUnknownVariableType( error_messages_, type_name );
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
	ClassPtr result= std::make_shared<Class>();

	result->name= class_declaration.name_;

	std::vector<llvm::Type*> members_llvm_types;

	members_llvm_types.reserve( class_declaration.fields_.size() );
	result->fields.reserve( class_declaration.fields_.size() );
	for( const ClassDeclaration::Field& in_field : class_declaration.fields_ )
	{
		if( result->GetField( in_field.name ) != nullptr )
		{
			error_messages_.push_back(
				ToStdString( in_field.name ) +
				" redefinition" );
		}

		Class::Field out_field;
		out_field.name= in_field.name;
		out_field.type= PrepareType( in_field.type );
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
	const std::vector<ProgramString>& arg_names,
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
				arg_names[ arg_number ],
				std::move(var) );
		if( !inserted_arg )
		{
			error_count_++;
			ReportRedefinition( error_messages_, arg_names[ arg_number ] );
			return;
		}

		llvm_arg.setName( ToStdString( arg_names[ arg_number ] ) );
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
					ReportUsingKeywordAsName( error_messages_, variable_declaration->name );

				Variable variable;
				variable.type= PrepareType( variable_declaration->type );
				variable.location= Variable::Location::PointerToStack;
				variable.llvm_value= function_context.llvm_ir_builder.CreateAlloca( variable.type.GetLLVMType() );

				const NamesScope::InsertedName* inserted_name=
					block_names.AddName( variable_declaration->name, std::move(variable) );

				if( !inserted_name )
				{
					ReportRedefinition( error_messages_, variable_declaration->name );
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
					ReportTypesMismatch( error_messages_, l_var.type.fundamental, r_var.type.fundamental );
					throw ProgramError();
				}

				if( l_var.location != Variable::Location::PointerToStack )
				{
					// TODO - write correct lvalue/rvalue flag into variable.
					throw ProgramError();
				}
				llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
				function_context.llvm_ir_builder.CreateStore( value_for_assignment, l_var.llvm_value );
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
			ReportTypesMismatch( error_messages_, r_var.type.fundamental, l_var.type.fundamental );
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
				throw ProgramError();
			}
			else
			{
				if( result_type.SizeOf() < 4u )
				{
					// TODO - emit error
					// Operation supported only for 32 and 64bit operands
					throw ProgramError();
				}
				const bool is_float= IsFloatingPoint( result_type.fundamental );
				if( !( IsInteger( result_type.fundamental ) || is_float ) )
				{
					// TODO - emit error
					// this operations allowed only for integer and floating point operands.
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
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( result_type.fundamental );
			if( !( IsInteger( result_type.fundamental ) || if_float || result_type.fundamental == U_FundamentalType::Bool ) )
			{
				error_messages_.emplace_back( "Exact equality operators exist only for integers, floating point, boolean types." );
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
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( result_type.fundamental );
			const bool is_signed= IsSignedInteger( result_type.fundamental );
			if( !( IsInteger( result_type.fundamental ) || if_float ) )
			{
				error_messages_.emplace_back( "Equality operators exist only for integers and floating point types." );
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
			throw ProgramError();
		}
		else
		{
			if( !( IsInteger( result_type.fundamental ) || result_type.fundamental == U_FundamentalType::Bool ) )
			{
				// TODO - emit error
				// this operations allowed only for integer or boolean operands.
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
				ReportNameNotFound( error_messages_, named_operand->name_ );
				throw ProgramError();
			}
			if( name_entry->second.class_ )
			{
				error_messages_.push_back( "Error, using class name as variable" );
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
				error_messages_.push_back( "Unknown numeric constant type" );
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
					llvm::Constant::getIntegerValue( llvm_type, llvm::APInt::doubleToBits( numeric_constant->value_ ) );
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
					error_messages_.push_back( "Error, indexation for non array." );
					throw ProgramError();
				}

				Variable index=
					BuildExpressionCode(
						*indexation_operator->index_,
						names,
						function_context );

				if( index.type.kind != Type::Kind::Fundamental )
				{
					error_messages_.push_back( "Error, index must be fundamental type." );
					throw ProgramError();
				}
				if( !IsUnsignedInteger( index.type.fundamental ) )
				{
					error_messages_.push_back( "Error, index must be unsigned integer." );
					throw ProgramError();
				}

				if( result.location != Variable::Location::PointerToStack )
				{
					error_messages_.push_back( "WTF? Strange variable location." );
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
					error_messages_.push_back(
						"Can not take member of non-class " +
						ToStdString( member_access_operator->member_name_ ) );
					throw ProgramError();
				}
				U_ASSERT( result.type.class_ );

				const Class::Field* field= result.type.class_->GetField( member_access_operator->member_name_ );
				if( field == nullptr )
				{
					error_messages_.push_back(
						ToStdString( member_access_operator->member_name_ ) +
						" not found in class " +
						ToStdString( result.type.class_->name ) );
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
					error_messages_.emplace_back( "Call of non-function." );
					throw ProgramError();
				}
				if( call_operator->arguments_.size() != result.type.function->args.size() )
				{
					error_messages_.emplace_back( "Argument count mismatch." );
					throw ProgramError();
				}

				std::vector<llvm::Value*> llvm_args;
				llvm_args.resize( result.type.function->args.size() );

				for( unsigned int i= 0u; i < result.type.function->args.size(); i++ )
				{
					Variable arg= BuildExpressionCode( *call_operator->arguments_[i], names, function_context );
					if( arg.type != result.type.function->args[i] )
					{
						error_messages_.emplace_back( "Argument type mismatch for argument " + std::to_string(i) + "." );
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
					error_messages_.emplace_back( "Unary minus supported only for fundamental types" );
					throw ProgramError();
				}
				const bool is_float= IsFloatingPoint( result.type.fundamental );
				if( !( IsInteger( result.type.fundamental ) || is_float ) )
				{
					ReportArithmeticOperationWithUnsupportedType( error_messages_, result.type.fundamental );
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
		error_messages_.emplace_back( "Unexpected type of while-loop condition. Expected bool." );
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
	U_UNUSED( break_operator );

	if( function_context.block_for_break == nullptr )
	{
		error_messages_.push_back( "Break outside while-loop." );
		return;
	}

	function_context.llvm_ir_builder.CreateBr( function_context.block_for_break );
}

void CodeBuilderLLVM::BuildContinueOperatorCode(
	const ContinueOperator& continue_operator,
	FunctionContext& function_context ) noexcept
{
	U_UNUSED( continue_operator );

	if( function_context.block_for_continue == nullptr )
	{
		error_messages_.push_back( "Continue outside while-loop." );
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
				error_messages_.emplace_back( "Unexpected type of if condition. Expected bool." );
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
