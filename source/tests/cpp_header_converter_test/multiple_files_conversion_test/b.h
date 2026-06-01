#pragma once

#define U_CPP_HEADER_CONVERTER_IGNORE
#include "common.h"
#undef U_CPP_HEADER_CONVERTER_IGNORE

void BFuncUsingCommonDefinitions( CommonTypedef a, const struct CommonStruct* b, enum CommonEnum c );

typedef int BTypedef;

BTypedef BFunc();

struct BStruct
{
	int length;
	char data[240];
};

enum BEnum
{
	B_0, B_1, B_2
};

#define COMMON_DEFINE_FLOATING_POINT_CONSTANT_COPY COMMON_DEFINE_FLOATING_POINT_CONSTANT
#define B_CONSTANT 78.2
#define B_ALIAS BTypedef
