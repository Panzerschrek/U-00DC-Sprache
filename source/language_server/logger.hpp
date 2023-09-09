#pragma once
#include <ostream>
#include <mutex>

namespace U
{

namespace LangServer
{

// Class that holds lock to logger and allows to actually output something.
class LoggerLock
{
public:
	LoggerLock( std::mutex& mutex, std::ostream& out )
		: lock_(mutex), out_(out)
	{}

	template<typename T>
	LoggerLock& operator<<( const T& el )
	{
		out_ << el;
		return *this;
	}

	LoggerLock& operator<<( std::basic_ostream<char>& (*manipulator)(std::basic_ostream<char>&) )
	{
		manipulator(out_);
		return *this;
	}

private:
	std::unique_lock<std::mutex> lock_;
	std::ostream& out_;
};

// Thread-safe loger. Writes into provided stream.
// Stream itself should not be used, when the logger instance exists.
class Logger
{
public:
	explicit Logger( std::ostream& out )
		: out_(out)
	{}

	// This call creates logger lock object that allows to output something.
	// Normally this lock is destroyed at the end of whole expression and thus unlocks internal mutex.
	// This approach allows to log individual messages atomically.
	LoggerLock operator()()
	{
		return LoggerLock( mutex_, out_ );
	}

private:
	std::mutex mutex_;
	std::ostream& out_;
};

} // namespace LangServer

} // namespace U
