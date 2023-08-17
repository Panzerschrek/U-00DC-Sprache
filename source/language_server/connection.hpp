#pragma once
#include <string>
#include <istream>
#include <ostream>

namespace U
{

class Connection
{
public:
	Connection( std::istream& in, std::ostream& out )
		: in_(in), out_(out)
	{}

	std::string Read();

private:
	struct MessageHeader
	{
		uint32_t content_length= 0;
		std::string content_type;
	};

private:
	bool ReadMessageHeaderPart( MessageHeader& h );
	MessageHeader ReadMessageHeader();

private:
	std::istream& in_;
	std::ostream& out_;
};

} // namespace U
