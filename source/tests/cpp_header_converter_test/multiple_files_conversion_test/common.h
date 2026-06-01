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

#define COMMON_DEFINE_INTEGER_CONSTANT 78652
#define COMMON_DEFINE_FLOATING_POINT_CONSTANT 45.33
#define COMMON_DEFINE_NEGATIVE_INTEGER_CONSTANT -56
#define COMMON_DEFINE_NEGATIVE_FLOATING_POINT_CONSTANT -552123.4
#define COMMON_DEFINE_STRING "Some"
#define COMMON_DEFINE_ALIAS CommonTypedef
