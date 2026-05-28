#include "assert.hpp"
#include "src_loc.hpp"

namespace U
{

SrcLoc::SrcLoc()
	: packed_file_index_macro_expansion_index_(
		( 0u << c_num_macro_expansion_index_bits ) | c_max_macro_expanison_index )
	, packed_line_column_(0)
{}

SrcLoc::SrcLoc( const uint32_t file_index, const uint32_t line, const uint32_t column )
	: packed_file_index_macro_expansion_index_(
		( file_index << c_num_macro_expansion_index_bits ) | c_max_macro_expanison_index )
	, packed_line_column_( ( line << c_num_column_bits ) | ( column & c_max_column ) )
{
	U_ASSERT( file_index <= c_max_file_index );
	U_ASSERT( line <= c_max_line );
	U_ASSERT( column <= c_max_column );
}

uint32_t SrcLoc::GetFileIndex() const
{
	return packed_file_index_macro_expansion_index_ >> c_num_macro_expansion_index_bits;
}

uint32_t SrcLoc::GetMacroExpansionIndex() const
{
	return packed_file_index_macro_expansion_index_ & c_max_macro_expanison_index;
}

uint32_t SrcLoc::GetLine() const
{
	return packed_line_column_ >> c_num_column_bits;
}

uint32_t SrcLoc::GetColumn() const
{
	return packed_line_column_ & c_max_column;
}

void SrcLoc::SetLine( const uint32_t line )
{
	U_ASSERT( line <= c_max_line );
	packed_line_column_ &= c_max_column;
	packed_line_column_ |= line << c_num_column_bits;
}

void SrcLoc::SetFileIndex( const uint32_t file_index )
{
	U_ASSERT( file_index <= c_max_file_index );
	packed_file_index_macro_expansion_index_ &= c_max_macro_expanison_index;
	packed_file_index_macro_expansion_index_ |= file_index << c_num_macro_expansion_index_bits;
}

void SrcLoc::SetMacroExpansionIndex( const uint32_t macro_expansion_index )
{
	U_ASSERT( macro_expansion_index <= c_max_macro_expanison_index );
	packed_file_index_macro_expansion_index_ &= ~c_max_macro_expanison_index;
	packed_file_index_macro_expansion_index_ |= macro_expansion_index;
}

bool SrcLoc::operator==( const SrcLoc& other ) const
{
	return
		this->packed_file_index_macro_expansion_index_ == other.packed_file_index_macro_expansion_index_ &&
		this->packed_line_column_ == other.packed_line_column_;
}

bool SrcLoc::operator< (const SrcLoc& other ) const
{
	if( this->packed_file_index_macro_expansion_index_ != other.packed_file_index_macro_expansion_index_ )
		return this->packed_file_index_macro_expansion_index_ < other.packed_file_index_macro_expansion_index_;
	return this->packed_line_column_ < other.packed_line_column_;
}

bool SrcLoc::operator<=( const SrcLoc& other ) const
{
	return *this < other || *this == other;
}

size_t SrcLoc::Hash() const
{
	if( sizeof(size_t) >= 8 )
	{
		// Can pack all into hash.
		return ( uint64_t(packed_file_index_macro_expansion_index_) << 32) | uint64_t(packed_line_column_);
	}
	else
	{
		// xor low and high parts.
		return uint32_t(packed_file_index_macro_expansion_index_) ^ uint32_t(packed_line_column_);
	}
}

} // namespace U
