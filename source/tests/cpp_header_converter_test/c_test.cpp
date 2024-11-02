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

int ExternallyDeclaredFunction( const char* s )
{
	(void)s;
	return 0;
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

void SameNameForStructAndFunc1( struct SameNameForStructAndFunc1* )
{
}

struct SameNameForStructAndFunc2* SameNameForStructAndFunc2(void)
{
	return nullptr;
}

#ifdef _WIN32
__declspec(dllexport) int SomeDllExportedFunction(void)
{
	return 34;
}
#endif

int VariadicFunc( int x, const char*  s, ...)
{
	int len= 0;
	while(s[0] != '\0')
	{
		++s;
		++len;
	}

	return x * len;
}

void ArrayArg( int arg[4] )
{
	(void)arg;
}

void IncompleteArrayArg( int arg[] )
{
	(void)arg;
}

void* VoidPtrRetFunc()
{
	return nullptr;
}

const void* ConstVoidPtrRetFunc()
{
	return nullptr;
}

void VoidPtrParamFunc(void* p)
{
	(void)p;
}

void ConstVoidPtrParamFunc(const void* p)
{
	(void)p;
}

MyVoid VoidPtrTypedefParamFunc(MyVoidPtr p)
{
	(void)p;
}

} // extern "C"
