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

		// Process cancellation specially - clear requests from queue.
		if( const auto notification= std::get_if<Notification>( &message ) )
		{
			if( const auto cancel= std::get_if<Notifications::CancelRequest>( notification ) )
			{
				const auto new_end_it=
					std::remove_if(
						queue_.begin(), queue_.end(),
						[&]( const Message& m )
						{
							if( const auto request= std::get_if<Request>( &m ) )
								return request->id == cancel->id;
							return false;
						} );

				queue_.erase( new_end_it, queue_.end() );
				return;
			}
		}

		queue_.push_back( std::move(message) );
	}
	condition_variable_.notify_one();
}

std::optional<Message> MessageQueue::TryPop( const std::chrono::milliseconds wait_time )
{
	std::unique_lock<std::mutex> lock(mutex_);

	if( closed_.load() )
		return std::nullopt;

	// Check if queue is non-empty first.
	if( !queue_.empty() )
	{
		Message result= queue_.front();
		queue_.pop_front();
		return std::move(result);
	}

	// Wait a bit.
	condition_variable_.wait_for( lock, wait_time );

	// Check if queue is non-empty again.
	// It may be non-empty if new message was pushed.
	// But is also may be empty if timeout has expired or in case of spurious wakeup.
	if( !queue_.empty() )
	{
		Message result= queue_.front();
		queue_.pop_front();
		return std::move(result);
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
