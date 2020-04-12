#pragma once
#include <cstdint>
#include <limits>

namespace U
{

class FilePos
{
public:
	static constexpr uint32_t c_max_file_index= std::numeric_limits<uint16_t>::max();
	static constexpr uint32_t c_max_macro_expanison_index= std::numeric_limits<uint16_t>::max();
	static constexpr uint32_t c_max_line= std::numeric_limits<uint16_t>::max();
	static constexpr uint32_t c_max_column= std::numeric_limits<uint16_t>::max();

public:
	FilePos();
	FilePos( uint32_t file_index, uint32_t line, uint32_t column );

	uint32_t GetFileIndex() const;
	uint32_t GetMacroExpansionIndex() const;
	uint32_t GetLine() const;
	uint32_t GetColumn() const;

	// = max for non-macro
	void SetFileIndex( uint32_t file_index );
	void SetMacroExpansionIndex( uint32_t macro_expansion_index );

	bool operator==( const FilePos& other ) const;
	bool operator!=( const FilePos& other ) const;
	bool operator< ( const FilePos& other ) const;
	bool operator<=( const FilePos& other ) const;

private:
	uint16_t file_index_;
	uint16_t macro_expansion_index_;
	uint16_t line_; // from 1
	uint16_t column_;
};

} // namespace U
