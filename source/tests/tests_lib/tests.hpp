#pragma once
#include <stdexcept>

namespace U
{

class TestException final : public std::runtime_error
{
public:
	TestException( const char* const what )
		: std::runtime_error( what )
	{}
};

class DisableTestException final : public std::runtime_error
{
public:
	DisableTestException()
		: std::runtime_error( "" )
	{}
};

// Main tests assertion handler. Aborts test.
#define U_TEST_ASSERT(x) \
	if( !(x) )\
	{\
		throw TestException( #x );\
	}

#define DISABLE_TEST throw DisableTestException()

// Utility tests functions.

#define ASSERT_NEAR( x, y, eps ) U_TEST_ASSERT( std::abs( (x) - (y) ) <= (eps) )

} // namespace U
