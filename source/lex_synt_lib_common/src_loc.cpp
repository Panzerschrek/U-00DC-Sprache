#include "assert.hpp"
#include "src_loc.hpp"

namespace U
{

SrcLoc::SrcLoc()
	: file_index_(0u)
	, macro_expansion_index_(c_max_macro_expanison_index)
	, line_(0u)
	, column_(0u)
{}

SrcLoc::SrcLoc( const uint32_t file_index, const uint32_t line, const uint32_t column )
	: file_index_( uint16_t(file_index) )
	, macro_expansion_index_( c_max_macro_expanison_index )
	, line_( uint16_t(line) )
	, column_( uint16_t(column) )
{
	U_ASSERT( file_index <= c_max_file_index );
	U_ASSERT( line <= c_max_line );
	U_ASSERT( column <= c_max_column );
}

uint32_t SrcLoc::GetFileIndex() const
{
	return file_index_;
}

uint32_t SrcLoc::GetMacroExpansionIndex() const
{
	return macro_expansion_index_;
}

uint32_t SrcLoc::GetLine() const
{
	return line_;
}

uint32_t SrcLoc::GetColumn() const
{
	return column_;
}

void SrcLoc::SetFileIndex( const uint32_t file_index )
{
	U_ASSERT( file_index <= c_max_file_index );
	file_index_= uint16_t(file_index);
}

void SrcLoc::SetMacroExpansionIndex( const uint32_t macro_expansion_index )
{
	U_ASSERT( macro_expansion_index <= c_max_macro_expanison_index );
	macro_expansion_index_= uint16_t(macro_expansion_index);
}

bool SrcLoc::operator==( const SrcLoc& other ) const
{
	return
		this->file_index_ == other.file_index_ &&
		this->macro_expansion_index_ == other.macro_expansion_index_ &&
		this->line_ == other.line_ &&
		this->column_ == other.column_;
}

bool SrcLoc::operator< (const SrcLoc& other ) const
{
	if( this->file_index_ != other.file_index_ )
		return this->file_index_ < other.file_index_;
	if( this->macro_expansion_index_ != other.macro_expansion_index_ )
		return this->macro_expansion_index_ < other.macro_expansion_index_;
	if( this->line_ != other.line_ )
		return this->line_ < other.line_;
	return this->column_ < other.column_;
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
		return (uint64_t(file_index_) << 48) | (uint64_t(macro_expansion_index_) << 32) | (uint64_t(line_) << 16) | uint64_t(column_);
	}
	else
	{
		// xor low and high parts.
		return ( (uint32_t(file_index_) << 16) | uint32_t(macro_expansion_index_) ) ^ ( (uint32_t(line_) << 16) | uint32_t(column_) );
	}
}

} // namespace U
