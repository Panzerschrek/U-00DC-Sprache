#pragma once
#include <optional>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/StringRef.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

namespace LangServer
{

// This code is based on URI code in clangd.

class Uri
{
public:
	Uri()= default;
	Uri( std::string scheme, std::string authority, std::string body );

	static std::optional<Uri> Parse( llvm::StringRef str );
	static Uri FromFilePath( llvm::StringRef absolute_path );

	std::string ToString() const; // Combine all components together.

	std::optional<std::string> AsFilePath() const;

public:
	friend bool operator==( const Uri& l, const Uri& r )
	{
		return
			std::tie(l.scheme_, l.authority_, l.body_) ==
			std::tie(r.scheme_, r.authority_, r.body_);
	}

	friend bool operator<( const Uri& l, const Uri& r )
	{
		return
			std::tie(l.scheme_, l.authority_, l.body_) <
			std::tie(r.scheme_, r.authority_, r.body_);
	}

private:
	std::string scheme_;
	std::string authority_;
	std::string body_;
};

} // namespace LangServer

} // namespace U
