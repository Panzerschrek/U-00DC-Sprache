extern "C"
{

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

void Function_ArgPtrToPtr( float** x )
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

float** Function_RetPtrToPtr()
{
	return 0;
}

void Function_UnnamedArgs( int a, float b, double c )
{
	(void)a;
	(void)b;
	(void)c;
}

void DuplicatedProto( int xx )
{
	(void)xx;
}

void StupidFunc( StupidStuctNaming* s )
{
	(void)s;
}

struct StructUsedWithoutDeclaration* FunctionReturningUnknownStruct()
{
	return nullptr;
}

void SequentialEnumFunc( enum SequentialEnum s )
{
	(void)s;
}

void NonSequentialEnumFunc( enum NonSequentialEnum e )
{
	(void)e;
}

void ABCFunc( TypedefForEnumABC arg )
{
	(void)arg;
}

void XYZFunc( TypedefForEnumXYZ arg )
{
	(void)arg;
}

void SillyFunction( SillyName SillyName )
{
	(void) SillyName;
}

} // extern "C"
