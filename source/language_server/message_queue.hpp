#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
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
	// Blocks until new message not added.
	// Returns nullopt if Queue is closed.
	std::optional<Message> Pop();

	void Close();
	bool IsClosed() const;

private:
	std::mutex mutex_;
	std::condition_variable condition_variable_;
	std::queue<Message> queue_;
	std::atomic<bool> closed_{ false };
};

} // namespace LangServer

} // namespace U
