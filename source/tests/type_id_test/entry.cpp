#include <cassert>
#include <iostream>

extern "C" size_t GetSomeClassIdA();
extern "C" size_t GetSomeClassIdB();
extern "C" size_t GetSomeClassIdTypeinfo();

int main()
{
	std::cout << "A id: " << GetSomeClassIdA() << std::endl;
	std::cout << "B id: " << GetSomeClassIdB() << std::endl;
	std::cout << "Typeinfo id: " << GetSomeClassIdTypeinfo() << std::endl;
	assert( GetSomeClassIdA() == GetSomeClassIdB() );
	assert( GetSomeClassIdA() == GetSomeClassIdTypeinfo() );
}
