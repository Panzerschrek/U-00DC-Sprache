#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <deque>
#include "messages.hpp"

namespace U
{

namespace LangServer
{

// Is thread-safe.
class MessageQueue
{
public:
	MessageQueue()= default;
	MessageQueue( const MessageQueue& )= delete;
	MessageQueue& operator=( const MessageQueue& )= delete;

public:
	// Push message into queue.
	void Push( Message message );

	// Extract message from queue.
	// Blocks until new message not added or timeout is expired, but may return earlier.
	// Returns nullopt if there is no messages in the queue, timeout is expired or queue is closed.
	std::optional<Message> TryPop( std::chrono::milliseconds wait_time );

	void Close();
	bool IsClosed() const;

private:
	std::mutex mutex_;
	std::condition_variable condition_variable_;
	std::deque<Message> queue_;
	std::atomic<bool> closed_{ false };
};

} // namespace LangServer

} // namespace U
