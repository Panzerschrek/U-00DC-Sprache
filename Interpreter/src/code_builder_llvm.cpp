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
	llvm::Function* function )
	: function_basic_block( llvm::BasicBlock::Create( llvm_context, "", function ) )
	, llvm_ir_builder( function_basic_block )
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
	} // for program elements

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

	// Fill arrays hierarchy.
	for( auto rit= type_name.array_sizes.rbegin(); rit != type_name.array_sizes.rend(); ++rit )
	{
		const NumericConstant& num= * *rit;

		last_type->kind= Type::Kind::Array;
		last_type->array.reset( new Array() );

		U_FundamentalType size_type= GetNumericConstantType( num );
		if( !( IsInteger(size_type) && num.value_ >= 0 ) )
			error_messages_.push_back( "Error, array size must be nonnegative integer" );

		last_type->array->size= size_t(num.value_);

		last_type= &last_type->array->type;
	}

	// TODO - setup llvm array types.

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
				// TODO - fill llvm class type.
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

	return result;
}

void CodeBuilderLLVM::BuildFuncCode(
	Variable& func_variable,
	const ProgramString& func_name,
	const std::vector<ProgramString>& arg_names,
	const Block& block )
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

	unsigned int arg_number= 0u;
	for( llvm::Argument& llvm_arg : llvm_function->args() )
	{
		Variable var;
		var.type= func_variable.type.function->args[ arg_number ];
		var.location= Variable::Location::LLVMRegister;
		var.llvm_value= &llvm_arg;

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

	FunctionContext function_context( llvm_context_, llvm_function );

	BuildBlockCode( block, function_names, function_context );


}

void CodeBuilderLLVM::BuildBlockCode(
	const Block& block,
	const NamesScope& names,
	FunctionContext& function_context )
{
	NamesScope block_names( &names );

	for( const IBlockElementPtr& block_element : block.elements_ )
	{
		const IBlockElement* const block_element_ptr= block_element.get();

		if(
			const SingleExpressionOperator* expression=
			dynamic_cast<const SingleExpressionOperator*>( block_element_ptr ) )
		{
			BuildExpressionCode(
				*expression->expression_,
				block_names,
				function_context );
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
		else
		{
			U_ASSERT(false);
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
			return result;
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
				// TODO - emit error
			}
			else
			{
				if( result_type.SizeOf() < 4u )
				{
					// TODO - emit error
					// Operation supported only for 32 and 64bit operands
				}
				// TODO - add floats support.
				if( !IsInteger( result_type.fundamental ) )
				{
					// TODO - emit error
					// this operations allowed only for integer and floating point operands.
				}

				const bool is_signed= IsSignedInteger( result_type.fundamental );

				llvm::Value* l_value_for_op= nullptr;
				if( l_var.location == Variable::Location::LLVMRegister )
					l_value_for_op= l_var.llvm_value;
				else if( l_var.location == Variable::Location::PointerToStack )
					l_value_for_op= function_context.llvm_ir_builder.CreateLoad( l_var.llvm_value );
				else
				{
					U_ASSERT( false );
				}

				llvm::Value* r_value_for_op= nullptr;
				if( r_var.location == Variable::Location::LLVMRegister )
					r_value_for_op= r_var.llvm_value;
				else if( r_var.location == Variable::Location::PointerToStack )
					r_value_for_op= function_context.llvm_ir_builder.CreateLoad( r_var.llvm_value );
				else
				{
					U_ASSERT( false );
				}

				llvm::Value* result_value;

				switch( comp.operator_ )
				{
				case BinaryOperator::Add:
					result_value=
						function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Sub:
					result_value=
						function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Div:
					if( is_signed )
						result_value=
							function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Mul:
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
		case BinaryOperator::Less:
		case BinaryOperator::LessEqual:
		case BinaryOperator::Greater:
		case BinaryOperator::GreaterEqual:
		case BinaryOperator::And:
		case BinaryOperator::Or:
		case BinaryOperator::Xor:
		case BinaryOperator::LazyLogicalAnd:
		case BinaryOperator::LazyLogicalOr:
			// TODO
			U_ASSERT(false);
			break;

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
			}
			if( name_entry->second.class_ )
			{
				error_messages_.push_back( "Error, using class name as variable" );
			}
			result= name_entry->second.variable;
		}
		else if( const NumericConstant* numeric_constant=
			dynamic_cast<const NumericConstant*>(&operand) )
		{
			// TODO
		}
		else
		{
			// TODO
			U_ASSERT(false);
		}

		for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operand_operators )
		{
			// TODO
		} // for unary postfix operators

		for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operand_operators )
		{
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
	const Variable expression_result=
		BuildExpressionCode(
			*return_operator.expression_,
			names,
			function_context );

	// TODO - check function result/expression result types mismatch.

	llvm::Value* value_for_return= nullptr;
	if( expression_result.location == Variable::Location::LLVMRegister )
		value_for_return= expression_result.llvm_value;
	else if( expression_result.location == Variable::Location::PointerToStack )
		value_for_return=
			function_context.llvm_ir_builder.CreateLoad( expression_result.llvm_value );
	else
	{
		U_ASSERT( false );
	}

	function_context.llvm_ir_builder.CreateRet( value_for_return );
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

} // namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
