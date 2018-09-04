#pragma once

#include "program_string.hpp"

namespace U
{

// Identificators of all language keywords.
// Names same, as in Ü-Sprache, but with '_' at the end, for prevention of intersection with C++ keywords.
enum class Keywords : unsigned int
{
	// Common keywords.
	fn_,
	op_,
	var_,
	auto_,
	return_,
	while_,
	break_,
	continue_,
	if_,
	static_if_,
	enable_if_,
	else_,
	move_,

	struct_,
	class_,
	final_,
	polymorph_,
	interface_,
	abstract_,

	virtual_,
	override_,
	pure_,

	namespace_,

	public_,
	private_,
	protected_,

	// Fundamental types names.
	void_,
	bool_,
	i8_ ,
	u8_ ,
	i16_,
	u16_,
	i32_,
	u32_,
	i64_,
	u64_,
	f32_,
	f64_,
	char8_,
	char16_,
	char32_,
	size_type_,

	// Boolean constants.
	true_,
	false_,

	// Mutability modifiers.
	mut_,
	imut_,
	constexpr_,

	zero_init_,
	uninitialized_,

	this_,
	base_,
	constructor_,
	destructor_,

	static_assert_,
	halt_,
	unsafe_,

	type_,
	typeinfo_,
	template_,
	enum_,

	cast_ref,
	cast_ref_unsafe,
	cast_imut,
	cast_mut,

	import_,

	default_,
	delete_,

	// Reserved keywords for future usage.
	for_,
	do_,
	switch_,
	case_,
	typename_,
	const_,
	lambda_,
	static_,
	package_,
	module_,
	export_,

	LastKeyword,
};

bool IsKeyword( const ProgramString& str );

const ProgramString& Keyword( Keywords keyword );
const char* KeywordAscii( Keywords keyword );

// Relation operators for program string and keyword enum.
// This operators make posible to write "str == Keywords::var_".
bool operator==( Keywords keyword, const ProgramString& str );
bool operator==( const ProgramString& str, Keywords keyword );
bool operator!=( Keywords keyword, const ProgramString& str );
bool operator!=( const ProgramString& str, Keywords keyword );

} // namespace U
