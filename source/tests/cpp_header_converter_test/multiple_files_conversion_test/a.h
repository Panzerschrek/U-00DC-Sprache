#pragma once

#define U_CPP_HEADER_CONVERTER_IGNORE
#include "common.h"
#undef U_CPP_HEADER_CONVERTER_IGNORE

void AFuncUsingCommonDefinitions( CommonTypedef a, const struct CommonStruct* b, enum CommonEnum c );

typedef char ATypedef;

ATypedef AFunc( ATypedef a );

struct AStruct
{
	int a;
	float b;
};

// Should ignore names where the ignore directive is defined.
#define U_CPP_HEADER_CONVERTER_IGNORE
void AIgnoredFunc();
typedef unsigned int AIgnoredTypedef;
struct AIgnoredStruct{ char c; short s; };
enum AIgnoredEnum{ AIgnored0, AIgnored1, AIgnored2, AIgnored3 };
#undef U_CPP_HEADER_CONVERTER_IGNORE

enum AEnum
{
	A_0, A_1, A_2, A_3
};

#define COMMON_DEFINE_INTEGER_CONSTANT_COPY COMMON_DEFINE_INTEGER_CONSTANT
#define A_CONSTANT 78.2
#define A_ALIAS AFunc
