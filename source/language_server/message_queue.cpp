#include "../lex_synt_lib_common/assert.hpp"
#include "message_queue.hpp"

namespace U
{

namespace LangServer
{

void MessageQueue::Push( Message message )
{
	{
		const std::lock_guard<std::mutex> guard(mutex_);
		queue_.push( std::move(message) );
	}
	condition_variable_.notify_one();
}

std::optional<Message> MessageQueue::Pop()
{
	std::unique_lock<std::mutex> lock(mutex_);

	while(true)
	{
		if( closed_.load() )
			return std::nullopt;

		if( !queue_.empty() )
		{
			Message result= queue_.front();
			queue_.pop();
			return std::move(result);
		}

		// Wait until next iteration.
		// If notification is recived or spurious wakeup happens continue checking closed flag and queue in loop.
		condition_variable_.wait(lock);
	}

	return std::nullopt;
}

void  MessageQueue::Close()
{
	closed_.store(true);
	condition_variable_.notify_one();
}

bool MessageQueue::IsClosed() const
{
	return closed_.load();
}

} // namespace LangServer

} // namespace U
