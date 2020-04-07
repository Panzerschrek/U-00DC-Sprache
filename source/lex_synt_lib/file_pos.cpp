#include "assert.hpp"
#include "file_pos.hpp"

namespace U
{

FilePos::FilePos()
	: file_index_(0u)
	, macro_expansion_index_(c_max_macro_expanison_index)
	, line_(0u)
	, column_(0u)
{}

FilePos::FilePos( const uint32_t file_index, const uint32_t line, const uint32_t column )
	: file_index_( uint16_t(file_index) )
	, macro_expansion_index_( c_max_macro_expanison_index )
	, line_( uint16_t(line) )
	, column_( uint16_t(column) )
{
	U_ASSERT( file_index <= c_max_file_index );
	U_ASSERT( line <= c_max_line );
	U_ASSERT( column <= c_max_column );
}

uint32_t FilePos::GetFileIndex() const
{
	return file_index_;
}

uint32_t FilePos::GetLine() const
{
	return line_;
}

uint32_t FilePos::GetColumn() const
{
	return column_;
}

void FilePos::SetFileIndex( const uint32_t file_index )
{
	U_ASSERT( file_index <= c_max_file_index );
	file_index_= uint16_t(file_index);
}

void FilePos::SetMacroExpansionIndex( const uint32_t macro_expansion_index )
{
	U_ASSERT( macro_expansion_index <= c_max_macro_expanison_index );
	macro_expansion_index_= uint16_t(macro_expansion_index);
}

bool FilePos::operator==( const FilePos& other ) const
{
	return
		this->file_index_ == other.file_index_ &&
		this->line_ == other.line_ &&
		this->column_ == other.column_;
}

bool FilePos::operator!=( const FilePos& other ) const
{
	return !( *this == other );
}

bool FilePos::operator< (const FilePos& other ) const
{
	if( this->file_index_ != other.file_index_ )
		return this->file_index_ < other.file_index_;
	if( this->line_ != other.line_ )
		return this->line_ < other.line_;
	return this->column_ < other.column_;
}

bool FilePos::operator<=( const FilePos& other ) const
{
	return *this < other || *this == other;
}

} // namespace U
