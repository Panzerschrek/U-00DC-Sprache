#include "../lex_synt_lib_common/assert.hpp"
#include "connection.hpp"


namespace U
{

namespace LangServer
{

bool Connection::ReadMessageHeaderPart( MessageHeader& header )
{
	if( in_.peek() == '\r' )
	{
		const char c1= in_.get();
		const char c2= in_.get();
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
			header.content_type += in_.get();
		}
	}

	// Assume good delimeters
	const char c1= in_.get();
	const char c2= in_.get();
	U_ASSERT(c1 == '\r' && c2 == '\n');
	return true;
}

Connection::MessageHeader Connection::ReadMessageHeader()
{
	MessageHeader header;
	while (ReadMessageHeaderPart(header)){}
	return header;
}

std::string Connection::Read()
{
	const MessageHeader header= ReadMessageHeader();

	std::string content;
	content.resize( header.content_length );
	in_.read( content.data(), header.content_length );
	content[header.content_length]= '\0';

	return content;
}

} // namespace LangServer

} // namespace U
