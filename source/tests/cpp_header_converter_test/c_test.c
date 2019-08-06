#include "c_test.h"

void Function_ZeroArgs()
{
}

void Function_ArgVal( int x )
{
	(void)x;
}

void Function_ArgPtr( int* x )
{
	(void)x;
}

void Function_ArgConstPtr( const int* x )
{
	(void)x;
}

int Function_RetVal()
{
	return 0;
}

int* Function_RetPtr()
{
	return 0;
}
const int* Function_RetConstPtr()
{
	return 0;
}

void Function_UnnamedArgs( int a, float b, double c )
{
	(void)a;
	(void)b;
	(void)c;
}
