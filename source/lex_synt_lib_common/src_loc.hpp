#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>

namespace U
{

// Location in program source.
// Lines in source are started with 1. This was choosen in order to report about errors properly, because IDEs/editors use 1-based lines numbering.
// Column is represented starting with 0, in UTF-32 code points. It is maybe better to use 1-base, but it is too late to change this behavior.
class SrcLoc
{
public:
	// Source files usually have more lines than columns.
	// Even large (likely generated) files with more then 65535 lines are possible.
	// So, allocate 18 bits for line numbers (max 262143 lines) an 14 bits for column numbers (max 16383 columns).
	static constexpr uint32_t c_num_line_bits= 18;
	static constexpr uint32_t c_num_column_bits= 32 - c_num_line_bits;

	static constexpr uint32_t c_max_line= ( 1u << c_num_line_bits ) - 1u;
	static constexpr uint32_t c_max_column= ( 1u << c_num_column_bits ) - 1u;

	static constexpr uint32_t c_num_file_index_bits= 16;
	static constexpr uint32_t c_num_macro_expansion_index_bits= 32 - c_num_file_index_bits;

	static constexpr uint32_t c_max_file_index= ( 1u << c_num_file_index_bits ) - 1u;
	static constexpr uint32_t c_max_macro_expanison_index= ( 1u << c_num_macro_expansion_index_bits ) - 1u;

public:
	SrcLoc();
	SrcLoc( uint32_t file_index, uint32_t line, uint32_t column );

	uint32_t GetFileIndex() const;
	uint32_t GetMacroExpansionIndex() const;
	uint32_t GetLine() const;
	uint32_t GetColumn() const;

	void SetLine( uint32_t line );

	void SetFileIndex( uint32_t file_index );
	// = max for non-macro
	void SetMacroExpansionIndex( uint32_t macro_expansion_index );

	bool operator==( const SrcLoc& other ) const;
	bool operator!=( const SrcLoc& other ) const { return ! ( *this == other ); }
	bool operator< ( const SrcLoc& other ) const;
	bool operator<=( const SrcLoc& other ) const;

	size_t Hash() const;

private:
	uint32_t packed_file_index_macro_expansion_index_; // High bits are file index, low bits are macro expansion index.
	uint32_t packed_line_column_; // High bits are line number, low bits are column number.
};

struct SrcLocHasher
{
	size_t operator()( const SrcLoc& src_loc ) const { return src_loc.Hash(); }
};

} // namespace U
