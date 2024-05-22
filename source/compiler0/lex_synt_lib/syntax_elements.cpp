#include "../../lex_synt_lib_common/assert.hpp"
#include "syntax_analyzer.hpp"
#include "keywords.hpp"
#include "../../lex_synt_lib_common/size_assert.hpp"

namespace U
{

namespace Synt
{

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

struct GetSrcLocVisitor
{
	SrcLoc operator()( const EmptyVariant& ) const
	{
		return SrcLoc();
	}

	SrcLoc operator()( const Expression& expression ) const
	{
		return GetExpressionSrcLoc(expression);
	}

	template<typename T>
	SrcLoc operator()( const T& element ) const
	{
		return element.src_loc;
	}

	template<typename T>
	SrcLoc operator()( const std::unique_ptr<T>& element ) const
	{
		return (*this)(*element);
	}
};

SrcLoc GetComplexNameSrcLoc( const ComplexName& complex_name )
{
	return std::visit( GetSrcLocVisitor(), complex_name );
}

SrcLoc GetExpressionSrcLoc( const Expression& expression )
{
	return std::visit( GetSrcLocVisitor(), expression );
}

SrcLoc GetInitializerSrcLoc( const Initializer& initializer )
{
	return std::visit( GetSrcLocVisitor(), initializer );
}

} // namespace Synt

} // namespace U
