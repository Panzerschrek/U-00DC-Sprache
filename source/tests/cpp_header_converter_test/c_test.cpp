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
	return nullptr;
}

const int* Function_RetConstPtr()
{
	return nullptr;
}

float** Function_RetPtrToPtr()
{
	return nullptr;
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

static_assert( sizeof(LargeStructWithSingleBitField) == 56, "Unexpected size!" );

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

static_assert( LV_Large == 987654321uLL );
static_assert( LV_EvenLarger == 123456789101112uLL );

static_assert( Anon_Large == 88888888888uLL );
static_assert( Anon_EvenLarger == 7654321098765uLL );

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

MY_NODISCARD int CNoDiscardFunc()
{
	return 321;
}

static_assert( sizeof(EmptyStruct) == 1, "Invalid size!" );
static_assert( sizeof(StructOfEmptyStructs) == 5, "Invalid size!" );

} // extern "C"
