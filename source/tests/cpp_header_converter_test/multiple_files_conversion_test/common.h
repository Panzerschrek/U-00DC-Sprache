#pragma once

typedef double CommonTypedef;

void CommonFunction( CommonTypedef t );

struct CommonStruct
{
	int x;
	int y;
};

enum CommonEnum
{
	CommonA, CommonB, CommonC,
};

#define COMMON_DEFINE_CONSTANT 78652
#define COMMON_DEFINE_ALIAS CommonTypedef
