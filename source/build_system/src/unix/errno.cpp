#include <cerrno>

// An ugly hack!
// Some system functions are poorly designed and return their error code via a pseudo-global variable "errno" instead of using return code.
// Extract it here.
// Generally it's impossible to access "errno" in Ü directly, since "errno" is actually a macro, which may be expanded into a function call, thread-local variable or something else.
extern "C" int BKGetErrno()
{
	return errno;
}
