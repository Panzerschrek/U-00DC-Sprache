#include <assert.h>
#include <stdio.h>

extern void* _Z15GetSomeClassIdAv();
extern void* _Z15GetSomeClassIdBv();

int main()
{
	printf( "A id: %p\n", _Z15GetSomeClassIdAv() );
	printf( "B id: %p\n", _Z15GetSomeClassIdBv() );
	assert( _Z15GetSomeClassIdAv() == _Z15GetSomeClassIdBv() );
}