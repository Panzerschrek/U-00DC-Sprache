#include "../../lex_synt_lib_common/assert.hpp"
#include "syntax_analyzer.hpp"
#include "keywords.hpp"
#include "../../lex_synt_lib_common/size_assert.hpp"

namespace U
{

namespace Synt
{

namespace
{

SrcLoc GetSrcLocImpl( const EmptyVariant& )
{
	return SrcLoc();
}

template<typename T>
SrcLoc GetSrcLocImpl( const T& e )
{
	return e.src_loc;
}

template<typename T>
SrcLoc GetSrcLocImpl( const std::unique_ptr<T>& e )
{
	return GetSrcLocImpl(*e);
}

template<typename ... Args>
SrcLoc GetSrcLocImpl( const std::variant<Args...>& v )
{
	return std::visit( [](const auto& el ){ return GetSrcLocImpl(el); }, v );
}

} // namespace

// Sizes for x86-64.
// If one of types inside variant becomes too big, put it inside "unique_ptr".
SIZE_ASSERT( ComplexName, 48u )
SIZE_ASSERT( TypeName, 48u )
SIZE_ASSERT( Expression, 48u )
SIZE_ASSERT( Initializer, 56u )
SIZE_ASSERT( BlockElementsList, 16u ) // Variant index + unique_ptr
SIZE_ASSERT( ClassElementsList, 16u ) // Variant index + unique_ptr
SIZE_ASSERT( ProgramElementsList, 16u ) // Variant index + unique_ptr

bool FunctionType::IsAutoReturn() const
{
	if( return_type != nullptr )
		if( const auto name_lookup= std::get_if<Synt::NameLookup>( return_type.get() ) )
			return name_lookup->name == Keywords::auto_;
	return false;
}

SrcLoc GetSrcLoc( const ComplexName& complex_name )
{
	return GetSrcLocImpl( complex_name );
}

SrcLoc GetSrcLoc( const Expression& expression )
{
	return GetSrcLocImpl( expression );
}

SrcLoc GetSrcLoc( const Initializer& initializer )
{
	return GetSrcLocImpl( initializer );
}

} // namespace Synt

} // namespace U
