#include <set>

#include "assert.hpp"

#include "keywords.hpp"

namespace U
{

namespace
{

struct KeywordEntry
{
	KeywordEntry( const char* str )
		: program_string( ToProgramString(str) )
		, ascii( str )
	{}

	const ProgramString program_string;
	const char* const ascii;
};

typedef std::set<ProgramString> KeywordsSet;

struct Globals
{
	const KeywordEntry (&keywords)[ size_t(Keywords::LastKeyword) ];
	const KeywordsSet& keywords_set;
};

// Hack for initialization.
// Use in-function static variables for cross-module initialization order setup.
static const Globals& GetGlobals()
{
	static const KeywordEntry c_keywords[ size_t(Keywords::LastKeyword) ]=
	{
		[ size_t(Keywords::fn_) ]= "fn",
		[ size_t(Keywords::op_) ]= "op",
		[ size_t(Keywords::var_) ]= "var",
		[ size_t(Keywords::auto_) ]= "auto",
		[ size_t(Keywords::return_) ]= "return",
		[ size_t(Keywords::while_) ]= "while",
		[ size_t(Keywords::break_) ]= "break",
		[ size_t(Keywords::continue_) ]= "continue",
		[ size_t(Keywords::if_) ]= "if",
		[ size_t(Keywords::static_if_) ]= "static_if",
		[ size_t(Keywords::enable_if_) ]= "enable_if",
		[ size_t(Keywords::else_) ]= "else",
		[ size_t(Keywords::move_) ]= "move",

		[ size_t(Keywords::struct_) ]= "struct",
		[ size_t(Keywords::class_) ]= "class",
		[ size_t(Keywords::final_) ]= "final",
		[ size_t(Keywords::polymorph_) ]= "polymorph",
		[ size_t(Keywords::interface_) ]= "interface",
		[ size_t(Keywords::abstract_) ]= "abstract",

		[ size_t(Keywords::nomangle_) ]= "nomangle",
		[ size_t(Keywords::virtual_) ]= "virtual",
		[ size_t(Keywords::override_) ]= "override",
		[ size_t(Keywords::pure_) ]= "pure",

		[ size_t(Keywords::namespace_) ]= "namespace",

		[ size_t(Keywords::public_) ]= "public",
		[ size_t(Keywords::private_) ]= "private",
		[ size_t(Keywords::protected_) ]= "protected",

		[ size_t(Keywords::void_) ]= "void",
		[ size_t(Keywords::bool_) ]= "bool",
		[ size_t(Keywords::i8_ ) ]= "i8" ,
		[ size_t(Keywords::u8_ ) ]= "u8" ,
		[ size_t(Keywords::i16_) ]= "i16",
		[ size_t(Keywords::u16_) ]= "u16",
		[ size_t(Keywords::i32_) ]= "i32",
		[ size_t(Keywords::u32_) ]= "u32",
		[ size_t(Keywords::i64_) ]= "i64",
		[ size_t(Keywords::u64_) ]= "u64",
		[ size_t(Keywords::f32_) ]= "f32",
		[ size_t(Keywords::f64_) ]= "f64",
		[ size_t(Keywords::char8_ ) ]= "char8" ,
		[ size_t(Keywords::char16_) ]= "char16",
		[ size_t(Keywords::char32_) ]= "char32",
		[ size_t(Keywords::size_type_) ]= "size_type",

		[ size_t(Keywords::true_) ]= "true",
		[ size_t(Keywords::false_) ]= "false",

		[ size_t(Keywords::mut_) ]= "mut",
		[ size_t(Keywords::imut_) ]= "imut",
		[ size_t(Keywords::constexpr_) ]= "constexpr",

		[ size_t(Keywords::zero_init_) ]= "zero_init",
		[ size_t(Keywords::uninitialized_) ]= "uninitialized",

		[ size_t(Keywords::this_) ]= "this",
		[ size_t(Keywords::base_) ]= "base",
		[ size_t(Keywords::constructor_) ]= "constructor",
		[ size_t(Keywords::destructor_) ]= "destructor",
		[ size_t(Keywords::conversion_constructor_) ]= "conversion_constructor",

		[ size_t(Keywords::static_assert_) ]= "static_assert",
		[ size_t(Keywords::halt_) ]= "halt",
		[ size_t(Keywords::unsafe_) ]= "unsafe",

		[ size_t(Keywords::type_) ]= "type",
		[ size_t(Keywords::typeinfo_) ]= "typeinfo",
		[ size_t(Keywords::template_) ]= "template",
		[ size_t(Keywords::enum_) ]= "enum",

		[ size_t(Keywords::cast_ref) ]= "cast_ref",
		[ size_t(Keywords::cast_ref_unsafe) ]= "cast_ref_unsafe",
		[ size_t(Keywords::cast_imut) ]= "cast_imut",
		[ size_t(Keywords::cast_mut) ]= "cast_mut",

		[ size_t(Keywords::import_) ]= "import",

		[ size_t(Keywords::default_) ]= "default",
		[ size_t(Keywords::delete_) ]= "delete",

		[ size_t(Keywords::for_) ]= "for",
		[ size_t(Keywords::do_) ]= "do",
		[ size_t(Keywords::switch_) ]= "switch",
		[ size_t(Keywords::case_) ]= "case",
		[ size_t(Keywords::typename_) ]= "typename",
		[ size_t(Keywords::const_) ]= "const",
		[ size_t(Keywords::lambda_) ]= "lambda",
		[ size_t(Keywords::static_) ]= "static",
		[ size_t(Keywords::package_) ]= "package",
		[ size_t(Keywords::module_) ]= "module",
		[ size_t(Keywords::export_) ]= "export",
	};

	static const KeywordsSet c_keywords_set=
	[]() -> KeywordsSet
	{
		KeywordsSet result;
		for( const KeywordEntry& k : c_keywords )
			result.emplace( k.program_string );
		return result;
	}
	();

	static const Globals c_globals{ c_keywords, c_keywords_set };

	return c_globals;
}

} // namespace

bool IsKeyword( const ProgramString& str )
{
	return GetGlobals().keywords_set.count( str ) != 0;
}

const ProgramString& Keyword( Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );

	return GetGlobals().keywords[ size_t(keyword) ].program_string;
}

const char* KeywordAscii( Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );

	return GetGlobals().keywords[ size_t(keyword) ].ascii;
}

bool operator==( Keywords keyword, const ProgramString& str )
{
	return Keyword( keyword ) == str;
}

bool operator==( const ProgramString& str, Keywords keyword )
{
	return keyword == str;
}

bool operator!=( Keywords keyword, const ProgramString& str )
{
	return Keyword( keyword ) != str;
}

bool operator!=( const ProgramString& str, Keywords keyword )
{
	return keyword != str;
}

} // namespace U
