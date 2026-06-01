#include "a.h"
#include "b.h"

void CommonFunction( const CommonTypedef t )
{
	(void)t;
}

void AFuncUsingCommonDefinitions( const CommonTypedef a, const struct CommonStruct* const b, const enum CommonEnum c )
{
	(void)a;
	(void)b;
	(void)c;
}

ATypedef AFunc( const ATypedef a )
{
	return a;
}

void BFuncUsingCommonDefinitions( const CommonTypedef a, const struct CommonStruct* const b, const enum CommonEnum c )
{
	(void)a;
	(void)b;
	(void)c;
}

BTypedef BFunc()
{
	return 756;
}
