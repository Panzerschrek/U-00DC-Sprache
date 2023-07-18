#include <cassert>
#include <iostream>

extern "C"
{

size_t GetSomeClassIdA();
size_t GetSomeClassIdB();
size_t GetSomeClassIdTypeinfo();

} // extern "C"

int main()
{
	std::cout << "A id: " << GetSomeClassIdA() << std::endl;
	std::cout << "B id: " << GetSomeClassIdB() << std::endl;
	std::cout << "Typeinfo id: " << GetSomeClassIdTypeinfo() << std::endl;
	assert( GetSomeClassIdA() == GetSomeClassIdB() );
	assert( GetSomeClassIdA() == GetSomeClassIdTypeinfo() );
}
