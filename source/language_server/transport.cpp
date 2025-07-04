#include <iostream>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/Program.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "transport.hpp"

namespace U
{

namespace LangServer
{

namespace
{

class JsonMessageRead final : public IJsonMessageRead
{
public:
	explicit JsonMessageRead( std::istream& in, Logger& log )
		: in_(in), log_(log)
	{}

	std::optional<Json::Value> Read() override
	{
		std::optional<size_t> content_length;
		while(true)
		{
			if( in_.eof() )
			{
				log_() << "Transport eof" << std::endl;
				return std::nullopt;
			}
			if( in_.fail() )
			{
				log_() << "Transport fail" << std::endl;
				return std::nullopt;
			}

			std::getline( in_, str_ );

			llvm::StringRef line_ref = str_;
			if( line_ref.consume_front("Content-Length: ") )
			{
				unsigned long long l= 0;
				const bool parse_error= llvm::getAsUnsignedInteger( line_ref.trim(), 0, l );
				if( parse_error )
				{
					log_() << "Failed to parse Content-Length!" << std::endl;
					break;
				}
				else
				{
					content_length= size_t(l);
					continue;
				}
			}

			if( line_ref.trim().empty() ) // End of headers.
				break;
		}

		if( content_length == std::nullopt )
		{
			log_() << "No Content-Length!" << std::endl;
			return std::nullopt;
		}

		if( *content_length >= 128 * 1024 * 1024 )
			return std::nullopt; // Protection agains overflows.

		str_.resize( *content_length );
		in_.read( str_.data(), std::streamsize(*content_length) );
		str_[ *content_length ]= '\0';

		llvm::Expected<llvm::json::Value> parse_result= llvm::json::parse( str_ );
		if( !parse_result )
		{
			log_() << "Failed to parse JSON" << std::endl;
			return std::nullopt;
		}

		return std::move(parse_result.get());
	}

private:
	std::istream& in_;
	Logger& log_;
	std::string str_; // Reuse input buffer.
};

class JsonMessageWrite final : public IJsonMessageWrite
{
public:
	explicit JsonMessageWrite( std::ostream& out )
		: out_(out)
	{}

	void Write( const Json::Value& value ) override
	{
		str_.clear();
		llvm::raw_string_ostream stream(str_);
		stream << value;
		stream.flush();

		out_
			<< ( "Content-Length: " + std::to_string(str_.length()) )
			<< "\r\n\r\n"
			<< str_
			<< std::flush;
	}

private:
	std::ostream& out_;
	std::string str_; // Reuse output buffer.
};

} // namespace

std::pair<IJsonMessageReadPtr, IJsonMessageWritePtr> OpenJSONStdioTransport( Logger& log )
{
	llvm::sys::ChangeStdinToBinary();
	llvm::sys::ChangeStdoutToBinary();
	return std::make_pair( std::make_unique<JsonMessageRead>( std::cin, log ), std::make_unique<JsonMessageWrite>( std::cout ) );
}

} // namespace LangServer

} // namespace U
