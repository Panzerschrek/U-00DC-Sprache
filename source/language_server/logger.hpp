#pragma once
#include <ostream>
#include <mutex>

namespace U
{

namespace LangServer
{

// Thread-safe loger. Writes into provided stream.
// Stream itself should not be used, when the logger instance exists.
class Logger
{
public:
	explicit Logger( std::ostream& out )
		: out_(out)
	{}

	template<typename T>
	Logger& operator<<( const T& el )
	{
		std::lock_guard<std::mutex> guard(mutex_);
		out_ << el;
		return *this;
	}

	Logger& operator<<( std::basic_ostream<char>& (*manipulator)(std::basic_ostream<char>&) )
	{
		std::lock_guard<std::mutex> guard(mutex_);
		manipulator(out_);
		return *this;
	}

private:
	std::mutex mutex_;
	std::ostream& out_;
};

} // namespace LangServer

} // namespace U
