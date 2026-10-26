#pragma once
#include <cstdint>

namespace U
{

template<typename T>
struct RcPtrStorage
{
	uint32_t use_counter;
	T value;
};

// A simple reference-counted pointer allowing storing a constant value.
// It's lighter than std::shared_ptr and doesn't have its overheads for multithreaded usage and polymorphism.
// So, it isn't thread-safe.
// It's roughly similar to "ust::shared_ptr_final_nullable" class in Ü.
template< typename T >
class RcPtr
{
public:
	RcPtr() noexcept = default;

	RcPtr( std::nullptr_t ) noexcept
	{}

	RcPtr( const RcPtr& other ) noexcept
		: ptr_( other.ptr_ )
	{
		if( ptr_ != nullptr )
			++static_cast< RcPtrStorage<T>* >(ptr_)->use_counter;
	}

	RcPtr( RcPtr&& other ) noexcept
		: ptr_( other.ptr_ )
	{
		other.ptr_= nullptr;
	}

	explicit RcPtr( T value )
	{
		ptr_= new RcPtrStorage<T>{ 1u, std::move(value) };
	}

	RcPtr& operator=( const RcPtr& other ) noexcept
	{
		if( this == &other )
			return *this;

		if( ptr_ != nullptr )
		{
			const auto ptr= static_cast< RcPtrStorage<T>* >(ptr_);
			--ptr->use_counter;
			if( ptr->use_counter == 0 )
				delete ptr;
		}

		const auto ptr= static_cast< RcPtrStorage<T>* >( other.ptr_ );
		ptr_= ptr;
		if( ptr != nullptr )
			++ptr->use_counter;

		return *this;
	}

	RcPtr& operator=( RcPtr&& other ) noexcept
	{
		if( this == &other )
			return *this;

		if( ptr_ != nullptr )
		{
			const auto ptr= static_cast< RcPtrStorage<T>* >(ptr_);
			--ptr->use_counter;
			if( ptr->use_counter == 0 )
				delete ptr;
		}

		ptr_= other.ptr_;
		other.ptr_= nullptr;

		return *this;
	}

	RcPtr& operator=( std::nullptr_t ) noexcept
	{
		reset();
		return *this;
	}

	~RcPtr() noexcept
	{
		if( ptr_ != nullptr )
		{
			const auto ptr= static_cast< RcPtrStorage<T>* >(ptr_);
			--ptr->use_counter;
			if( ptr->use_counter == 0 )
				delete ptr;
		}
	}

	bool empty() const noexcept
	{
		return ptr_ == nullptr;
	}

	const T& operator*() const noexcept
	{
		return static_cast< RcPtrStorage<T>* >(ptr_)->value;
	}

	const T* operator->() const noexcept
	{
		return &static_cast< RcPtrStorage<T>* >(ptr_)->value;
	}

	void reset() noexcept
	{
		if( ptr_ != nullptr )
		{
			const auto ptr= static_cast< RcPtrStorage<T>* >(ptr_);
			--ptr->use_counter;
			if( ptr->use_counter == 0 )
				delete ptr;

			ptr_= nullptr;
		}
	}

	void* get_raw_ptr() const noexcept
	{
		return ptr_;
	}

private:
	void* ptr_= nullptr; // Is actually RcPtrStorage<T>, but stupid C++ doesn't allow instantiating it for incomplete "T".
};

template<typename T>
inline bool operator==( const RcPtr<T>& l, std::nullptr_t ) noexcept
{
	return l.empty();
}

template<typename T>
inline bool operator!=( const RcPtr<T>& l, std::nullptr_t ) noexcept
{
	return !l.empty();
}

template<typename T>
inline bool operator==( std::nullptr_t, const RcPtr<T>& r ) noexcept
{
	return r.empty();
}

template<typename T>
inline bool operator!=( std::nullptr_t, const RcPtr<T>& r ) noexcept
{
	return !r.empty();
}

template<typename T>
inline bool operator==( const RcPtr<T>& l, const RcPtr<T>& r ) noexcept
{
	return l.get_raw_ptr() == r.get_raw_ptr();
}

template<typename T>
inline bool operator!=( const RcPtr<T>& l, const RcPtr<T>& r ) noexcept
{
	return l.get_raw_ptr() != r.get_raw_ptr();
}

struct RcPtrHasher
{
	template<typename T>
	size_t operator()( const RcPtr<T>& ptr ) const
	{
		return reinterpret_cast<uintptr_t>( ptr.get_raw_ptr() );
	}
};

} // namespace U
