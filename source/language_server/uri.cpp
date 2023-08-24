#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Path.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "uri.hpp"

namespace U
{

namespace LangServer
{

namespace
{

bool ShouldEscape( const char c )
{
	// Unreserved characters.
	if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') )
		return false;
	switch(c)
	{
		case '-':
		case '_':
		case '.':
		case '~':
		case '/': // '/' is only reserved when parsing.
		// ':' is only reserved for relative URI paths, which clangd doesn't produce.
		case ':':
		return false;
	}
	return true;
}

void PercentEncode( llvm::StringRef content, std::string& out )
{
	for( const char c : content )
	{
		if( ShouldEscape(c) )
		{
			out.push_back('%');
			out.push_back(llvm::hexdigit(uint32_t(uint8_t(c)) / 16));
			out.push_back(llvm::hexdigit(uint32_t(uint8_t(c)) % 16));
		}
		else
			out.push_back(c);
	}
}

std::string PercentDecode( const llvm::StringRef content )
{
	std::string result;
	for( auto i= content.begin(), e= content.end(); i != e; ++i )
	{
		if( *i != '%' )
		{
			result += *i;
			continue;
		}
		if( *i== '%' && i + 2 < content.end() && llvm::isHexDigit(*(i + 1)) && llvm::isHexDigit(*(i + 2)) )
		{
			result.push_back( char( llvm::hexFromNibbles(*(i + 1), *(i + 2)) ) );
			i += 2;
		}
		else
			result.push_back(*i);
	}
	return result;
}


bool IsWindowsPath( const llvm::StringRef path )
{
	return path.size() > 1 && llvm::isAlpha(path[0]) && path[1]== ':';
}

bool IsNetworkPath( const llvm::StringRef path )
{
	return
		path.size() > 2 &&
		path[0]== path[1] &&
		llvm::sys::path::is_separator(path[0]);
}

bool IsValidScheme( const llvm::StringRef scheme )
{
	if (scheme.empty())
		return false;

	if (!llvm::isAlpha(scheme[0]))
		return false;

	return std::all_of(
		scheme.begin() + 1, scheme.end(),
		[]( const char c )
		{
			return llvm::isAlnum(c) || c== '+' || c== '.' || c== '-';
		} );
}

} // namespace

Uri::Uri( std::string scheme, std::string authority, std::string body )
	: scheme_(std::move(scheme)), authority_(std::move(authority)), body_(std::move(body))
{}

std::optional<Uri> Uri::Parse( const llvm::StringRef str )
{
	llvm::StringRef uri_str= str;

	auto pos= uri_str.find(':');
	if( pos== llvm::StringRef::npos )
		return std::nullopt;

	auto scheme_str= uri_str.substr( 0, pos );

	Uri result;

	result.scheme_= PercentDecode( scheme_str );
	if( !IsValidScheme(result.scheme_) )
		return std::nullopt;

	uri_str= uri_str.substr(pos + 1);
	if( uri_str.consume_front("//") )
	{
		pos= uri_str.find('/');
		result.authority_= PercentDecode( uri_str.substr(0, pos) );
		uri_str= uri_str.substr(pos);
	}
	result.body_= PercentDecode(uri_str);

	return result;
}

Uri Uri::FromFilePath( llvm::StringRef absolute_path )
{
	std::string body;
	llvm::StringRef authority;
	const llvm::StringRef root= llvm::sys::path::root_name( absolute_path );
	if( IsNetworkPath(root) )
	{
		// Windows UNC paths e.g. \\server\share=> file://server/share
		authority= root.drop_front(2);
		absolute_path.consume_front(root);
	}
	else if( IsWindowsPath(root) )
	{
		// Windows paths e.g. X:\path=> file:///X:/path
		body= "/";
	}
	body+= llvm::sys::path::convert_to_slash( absolute_path );
	return Uri( "file", authority.str(), body );
}

std::string Uri::ToString() const
{
	std::string result;
	PercentEncode( scheme_, result );
	result.push_back(':');

	if( authority_.empty() && body_.empty() )
		return result;

	// If authority if empty, we only print body if it starts with "/"; otherwise,
	// the URI is invalid.
	if( !authority_.empty() || llvm::StringRef(body_).startswith("/") )
	{
		result.append("//");
		PercentEncode( authority_, result );
	}

	PercentEncode( body_, result );
	return result;
}

std::optional<std::string> Uri::AsFilePath() const
{
	if( !llvm::StringRef(body_).startswith("/") )
		return std::nullopt;

	llvm::SmallString<128> path;
	llvm::StringRef body= body_;
	if( !authority_.empty() )
	{
		// Windows UNC paths e.g. file://server/share=> \\server\share
		("//" + llvm::StringRef(authority_)).toVector(path);
	}
	else if( IsWindowsPath( body.substr(1)) )
	{
		// Windows paths e.g. file:///X:/path=> X:\path
		body.consume_front("/");
	}

	path.append( body );
	llvm::sys::path::native(path);
	return std::string(path);
}

} // namespace LangServer

} // namespace U
