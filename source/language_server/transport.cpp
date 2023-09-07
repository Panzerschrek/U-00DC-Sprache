#include <iostream>
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
	explicit JsonMessageRead( std::istream& in )
		: in_(in)
	{
	}

	std::optional<Json::Value> Read() override
	{
		if( in_.eof() || in_.fail() )
			return std::nullopt;

		// TODO - fix this mess, perform HTTP parsing properly.

		const MessageHeader header= ReadMessageHeader();

		str_.resize( header.content_length );
		in_.read( str_.data(), header.content_length );
		str_[header.content_length]= '\0';

		llvm::Expected<llvm::json::Value> parse_result= llvm::json::parse( str_ );
		if( !parse_result )
			return std::nullopt;

		return std::move(parse_result.get());
	}

private:
	struct MessageHeader
	{
		uint32_t content_length= 0;
		std::string content_type;
	};

private:
	MessageHeader ReadMessageHeader()
	{
		MessageHeader header;
		while (ReadMessageHeaderPart(header)){}
		return header;
	}

	bool ReadMessageHeaderPart( MessageHeader& header )
	{
		if( in_.peek() == '\r' )
		{
			const auto c1= in_.get();
			const auto c2= in_.get();
			U_ASSERT(c1 == '\r' && c2 == '\n');
			return false;
		}

		// We assume it's 'Content-'
		U_ASSERT( in_.peek() == 'C' );
		in_.ignore(8);
		if( in_.peek() == 'L' )
		{
			// We assume 'Content-Length: '
			in_.ignore(8);
			in_ >> header.content_length;
		}
		else
		{
			// We assume 'Content-Type: '
			U_ASSERT( in_.peek() == 'T' );
			in_.ignore(6);
			header.content_type.clear();
			while( in_.peek() != '\r' )
			{
				header.content_type += char(in_.get());
			}
		}

		// Assume good delimeters
		const auto c1= in_.get();
		const auto c2= in_.get();
		U_ASSERT(c1 == '\r' && c2 == '\n');
		return true;
	}

private:
	std::istream& in_;
	std::string str_;
};

class JsonMessageWrite final : public IJsonMessageWrite
{
public:
	explicit JsonMessageWrite( std::ostream& out )
		: out_(out)
	{
	}

	void Write( const Json::Value& value ) override
	{
		str_.clear();
		llvm::raw_string_ostream stream(str_);
		stream << value;
		stream.flush();

		out_
			<< "Content-Length: "
			<< str_.length()
			<< "\r\n\r\n"
			<< str_
			<< std::flush;
	}

private:
	std::ostream& out_;
	std::string str_;
};

} // namespace

std::pair<IJsonMessageReadPtr, IJsonMessageWritePtr> OpenJSONStdioTransport()
{
	return std::make_pair( std::make_unique<JsonMessageRead>( std::cin ), std::make_unique<JsonMessageWrite>( std::cout ) );
}

} // namespace LangServer

} // namespace U
