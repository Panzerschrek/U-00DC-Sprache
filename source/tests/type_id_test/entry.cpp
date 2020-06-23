#include <cassert>
#include <iostream>

extern "C" void* GetSomeClassIdA();
extern "C" void* GetSomeClassIdB();

int main()
{
	std::cout << "A id: " << GetSomeClassIdA() << std::endl;
	std::cout << "A id: " << GetSomeClassIdB() << std::endl;
	assert( GetSomeClassIdA() == GetSomeClassIdB() );
}
