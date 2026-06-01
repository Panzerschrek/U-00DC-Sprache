#pragma once

#define U_CPP_HEADER_CONVERTER_IGNORE
#include "common.h"
#undef U_CPP_HEADER_CONVERTER_IGNORE

typedef char ATypedef;

ATypedef AFunc( ATypedef a );

struct AStruct
{
	int a;
	float b;
};

enum AEnum
{
	A_0, A_1, A_2, A_3
};
