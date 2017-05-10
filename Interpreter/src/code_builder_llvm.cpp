#include <llvm/IR/LLVMContext.h>
#include "keywords.hpp"
#include "vm.hpp"

#include "code_builder_llvm.hpp"

// Hack for stupid compilers/ide.
// TODO - use designated intializer, when compilers become more clever.
#if 0
#define DESIGNATED_INITIALIZER( index, value ) [ size_t(index) ]= value
#else
#define DESIGNATED_INITIALIZER( index, value ) value
#endif

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
	DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, "InvalidType" ),
	DESIGNATED_INITIALIZER( U_FundamentalType::Void,  KeywordAscii( Keywords::void_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::Bool, KeywordAscii( Keywords::bool_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::i8 , KeywordAscii( Keywords::i8_  ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::u8 , KeywordAscii( Keywords::u8_  ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::i16, KeywordAscii( Keywords::i16_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::u16, KeywordAscii( Keywords::u16_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::i32, KeywordAscii( Keywords::i32_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::u32, KeywordAscii( Keywords::u32_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::i64, KeywordAscii( Keywords::i64_ ) ),
	DESIGNATED_INITIALIZER( U_FundamentalType::u64, KeywordAscii( Keywords::u64_ ) ),
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

CodeBuilderLLVM::CodeBuilderLLVM()
	: llvm_context_( llvm::getGlobalContext() )
{
	types_. i8= llvm::IntegerType::get( llvm_context_,  8u );
	types_. u8= llvm::IntegerType::get( llvm_context_,  8u );
	types_.i16= llvm::IntegerType::get( llvm_context_, 16u );
	types_.u16= llvm::IntegerType::get( llvm_context_, 16u );
	types_.i32= llvm::IntegerType::get( llvm_context_, 32u );
	types_.u32= llvm::IntegerType::get( llvm_context_, 32u );
	types_.i64= llvm::IntegerType::get( llvm_context_, 64u );
	types_.u64= llvm::IntegerType::get( llvm_context_, 64u );
}

CodeBuilderLLVM::~CodeBuilderLLVM()
{
}

CodeBuilderLLVM::BuildResult CodeBuilderLLVM::BuildProgram( const ProgramElements& program_elements )
{
	module_= std::unique_ptr<llvm::Module>( new llvm::Module( "U-Module", llvm_context_ ) );

	for( const IProgramElementPtr& program_element : program_elements )
	{
		if( const FunctionDeclaration* func=
			dynamic_cast<const FunctionDeclaration*>( program_element.get() ) )
		{
			if( IsKeyword( func->name_ ) )
				ReportUsingKeywordAsName( error_messages_, func->name_ );
		}
	} // for program elements

	BuildResult result;
	result.error_messages= error_messages_;
	error_messages_.clear();
	result.module= std::move( module_ );
	return result;
}

} // namespace Interpreter
