#pragma once
#include "common.h"

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
