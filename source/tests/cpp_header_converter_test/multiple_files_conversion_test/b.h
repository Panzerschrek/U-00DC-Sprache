#pragma once

#define U_CPP_HEADER_CONVERTER_IGNORE
#include "common.h"
#undef U_CPP_HEADER_CONVERTER_IGNORE

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
