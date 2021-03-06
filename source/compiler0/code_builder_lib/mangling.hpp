#pragma once
#include "class.hpp"
#include "names_scope.hpp"

namespace U
{

class ManglerState
{
private:
	using LenType = uint16_t;

public:
	void Push( char c );
	void Push( std::string_view name );
	void PushLengthPrefixed( std::string_view name );

	std::string TakeResult();

public:
	class NodeHolder;

private:
	LenType GetCurrentPos() const;
	LenType GetCurrentCompressedPos() const;
	void FinalizePart( LenType start, LenType compressed_start );

private:
	struct Substitution
	{
		LenType start;
		LenType size;
	};

private:
	std::vector<Substitution> substitutions_;
	std::string result_full_;
	std::string result_compressed_;
};

// Mangling with Itanium ABI rules.
// Use class instead of set of free functions for possibility of reu-use of internal buffers.

class Mangler
{
public:
	std::string MangleFunction(
		const NamesScope& parent_scope,
		const std::string& function_name,
		const FunctionType& function_type,
		const TemplateArgs* template_args= nullptr );
	std::string MangleGlobalVariable( const NamesScope& parent_scope, const std::string& variable_name );
	std::string MangleType( const Type& type );
	std::string MangleTemplateArgs( const TemplateArgs& template_args );
	std::string MangleVirtualTable( const Type& type );

private:
	ManglerState state_;
};

} // namespace U
