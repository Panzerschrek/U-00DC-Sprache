#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../lex_synt_lib_common/assert.hpp"

#include "template_types.hpp"

namespace U
{

namespace
{

size_t TemplateArgHashImpl( const TemplateVariableArg& template_variable_arg )
{
	size_t hash= template_variable_arg.type.Hash();

	U_ASSERT( template_variable_arg.constexpr_value != nullptr );
	// TODO - handle large constants. For now hash only lower bits of constants.
	hash= llvm::hash_combine( hash, size_t( template_variable_arg.constexpr_value->getUniqueInteger().getLimitedValue() ) );

	return hash;
}

size_t TemplateArgHashImpl( const Type& template_type_arg )
{
	return template_type_arg.Hash();
}

size_t TemplateArgHash( const TemplateArg& arg )
{
	return std::visit( []( const auto& el ) { return TemplateArgHashImpl(el); }, arg );
}

} // namespace

bool operator==( const TemplateVariableArg& l, const TemplateVariableArg& r )
{
	U_ASSERT( l.constexpr_value != nullptr );
	U_ASSERT( r.constexpr_value != nullptr );
	return
		l.type == r.type &&
		l.constexpr_value->getUniqueInteger() == r.constexpr_value->getUniqueInteger();
}

size_t TemplateKey::Hash() const
{
	size_t hash= size_t( reinterpret_cast<uintptr_t>( template_.get() ) );

	for( const TemplateArg& arg : args )
		hash= llvm::hash_combine( hash, TemplateArgHash(arg) );

	return hash;
}

bool operator==( const TemplateKey& l, const TemplateKey& r )
{
	return l.template_ == r.template_ && l.args == r.args;
}

size_t ParameterizedFunctionTemplateKey::Hash() const
{
	size_t hash= size_t( reinterpret_cast<uintptr_t>( functions_set.get() ) );

	for( const TemplateArg& arg : args )
		hash= llvm::hash_combine( hash, TemplateArgHash(arg) );

	return hash;
}

bool operator==( const ParameterizedFunctionTemplateKey& l, const ParameterizedFunctionTemplateKey& r )
{
	return l.functions_set == r.functions_set && l.args == r.args;
}

} // namespace U
