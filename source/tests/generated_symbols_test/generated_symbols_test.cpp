#include <cassert>
#include <iostream>

extern "C"
{


// Custom halt handler should be available.
extern void (*_U_halt_handler)();

} // extern "C"

static void TestHaltHandler(){}

int main()
{
	_U_halt_handler= TestHaltHandler;
}
