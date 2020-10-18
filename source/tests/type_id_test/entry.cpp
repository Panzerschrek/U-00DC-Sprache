#include <cassert>
#include <iostream>

extern "C" void* GetSomeClassIdA();
extern "C" void* GetSomeClassIdB();
extern "C" void* GetSomeClassIdTypeinfo();

int main()
{
	std::cout << "A id: " << GetSomeClassIdA() << std::endl;
	std::cout << "B id: " << GetSomeClassIdB() << std::endl;
	std::cout << "Typeinfo id: " << GetSomeClassIdTypeinfo() << std::endl;
	assert( GetSomeClassIdA() == GetSomeClassIdB() );
	assert( GetSomeClassIdA() == GetSomeClassIdTypeinfo() );
}
