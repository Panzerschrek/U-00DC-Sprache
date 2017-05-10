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

CodeBuilderLLVM::CodeBuilderLLVM()
	: llvm_context_( llvm::getGlobalContext() )
{
	fundamental_llvm_types_. i8= llvm::IntegerType::get( llvm_context_,  8u );
	fundamental_llvm_types_. u8= llvm::IntegerType::get( llvm_context_,  8u );
	fundamental_llvm_types_.i16= llvm::IntegerType::get( llvm_context_, 16u );
	fundamental_llvm_types_.u16= llvm::IntegerType::get( llvm_context_, 16u );
	fundamental_llvm_types_.i32= llvm::IntegerType::get( llvm_context_, 32u );
	fundamental_llvm_types_.u32= llvm::IntegerType::get( llvm_context_, 32u );
	fundamental_llvm_types_.i64= llvm::IntegerType::get( llvm_context_, 64u );
	fundamental_llvm_types_.u64= llvm::IntegerType::get( llvm_context_, 64u );

	// TODO - found llvm-floating types.
	fundamental_llvm_types_.f32= fundamental_llvm_types_.i32= llvm::IntegerType::get( llvm_context_, 32u );
	fundamental_llvm_types_.f64= fundamental_llvm_types_.i32= llvm::IntegerType::get( llvm_context_, 64u );

	fundamental_llvm_types_.void_= nullptr;
	fundamental_llvm_types_.bool_= llvm::IntegerType::get( llvm_context_, 8u );
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

			const NamesScope::InsertedName* inserted_func_name =
				global_names_.AddName( func->name_, std::move( func_info ) );
			if( inserted_func_name )
			{
				/*
				Variable& func_variable= inserted_func_name->second.variable;
				BuildFuncCode(
					func_variable,
					func->name_ );
				*/
			}
			else
			{
				error_count_++;
				ReportRedefinition( error_messages_, func->name_ );
				continue;
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
		switch( last_type->fundamental )
		{
		case U_FundamentalType::InvalidType:
		case U_FundamentalType::LastType:
			break;
		case U_FundamentalType::Void:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.void_; break;
		case U_FundamentalType::Bool:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.bool_; break;
		case U_FundamentalType::i8 :
			last_type->fundamental_llvm_type= fundamental_llvm_types_.i8 ; break;
		case U_FundamentalType::u8 :
			last_type->fundamental_llvm_type= fundamental_llvm_types_.u8 ; break;
		case U_FundamentalType::i16:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.i16; break;
		case U_FundamentalType::u16:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.u16; break;
		case U_FundamentalType::i32:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.i32; break;
		case U_FundamentalType::u32:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.u32; break;
		case U_FundamentalType::i64:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.i64; break;
		case U_FundamentalType::u64:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.u64; break;
		case U_FundamentalType::f32:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.f32; break;
		case U_FundamentalType::f64:
			last_type->fundamental_llvm_type= fundamental_llvm_types_.f64; break;
		};
	}

	return result;
}

void CodeBuilderLLVM::BuildFuncCode( Variable& func, const ProgramString& func_name )
{
	func.type.kind= Type::Kind::Function;
	func.type.function.reset( new Function );

	std::vector<llvm::Type*> args_llvm_types;
	for( const Type& type : func.type.function->args )
		args_llvm_types.push_back( type.GetLLVMType() );

	func.type.function->llvm_function_type=
		llvm::FunctionType::get(
			func.type.function->return_type.GetLLVMType(),
			llvm::ArrayRef<llvm::Type*>( args_llvm_types.data(),args_llvm_types.size() ),
			false );

	llvm::Function* llvm_function=
		llvm::Function::Create(
			func.type.function->llvm_function_type,
			llvm::Function::LinkageTypes::ExternalLinkage, // TODO - select linkage
			ToStdString( func_name ),
			module_.get() );

	func.llvm_value= llvm_function;
}

} // namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
