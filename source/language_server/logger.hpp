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

	Logger& operator<<( Logger& (*manipulator)(Logger&) )
	{
		manipulator(*this);
		return *this;
	}

	void endl()
	{
		std::lock_guard<std::mutex> guard(mutex_);
		out_ << std::endl;
	}

private:
	std::mutex mutex_;
	std::ostream& out_;
};

inline Logger& endl( Logger& l )
{
	l.endl();
	return l;
}

} // namespace LangServer

} // namespace U
