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
	static constexpr uint32_t c_max_file_index= std::numeric_limits<uint16_t>::max();
	static constexpr uint32_t c_max_macro_expanison_index= std::numeric_limits<uint16_t>::max();
	static constexpr uint32_t c_max_line= std::numeric_limits<uint16_t>::max();
	static constexpr uint32_t c_max_column= std::numeric_limits<uint16_t>::max();

public:
	SrcLoc();
	SrcLoc( uint32_t file_index, uint32_t line, uint32_t column );

	uint32_t GetFileIndex() const;
	uint32_t GetMacroExpansionIndex() const;
	uint32_t GetLine() const;
	uint32_t GetColumn() const;

	// = max for non-macro
	void SetFileIndex( uint32_t file_index );
	void SetMacroExpansionIndex( uint32_t macro_expansion_index );

	bool operator==( const SrcLoc& other ) const;
	bool operator!=( const SrcLoc& other ) const { return ! ( *this == other ); }
	bool operator< ( const SrcLoc& other ) const;
	bool operator<=( const SrcLoc& other ) const;

	size_t Hash() const;

private:
	uint16_t file_index_;
	uint16_t macro_expansion_index_;
	uint16_t line_;
	uint16_t column_;
};

struct SrcLocHasher
{
	size_t operator()( const SrcLoc& src_loc ) const { return src_loc.Hash(); }
};

} // namespace U
