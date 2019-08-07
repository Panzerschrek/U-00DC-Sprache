// Test C header for convertion to Ü header.

#define ONE_CONSTANT 1
#define PI_CONSTANT   3.1415926535
#define PI_CONSTANT_F 3.1415926535f
#define UNSIGNED_CONSTANT 12345u
#define HEX_CONSTNT 0xFF

void Function_ZeroArgs();
void Function_ArgVal( int x );
void Function_ArgPtr( int* x );
void Function_ArgConstPtr( const int* x );
int Function_RetVal();
int* Function_RetPtr();
const int* Function_RetConstPtr();

void Function_UnnamedArgs( int, float, double );

void DuplicatedProto( int x );
void DuplicatedProto( int xx );

typedef char CPP_char8;
typedef   signed char  CPP_i8;
typedef unsigned char  CPP_u8;
typedef   signed short CPP_i16;
typedef unsigned short CPP_u16;
typedef   signed int   CPP_i32;
typedef unsigned int   CPP_u32;
typedef float  CPP_f32;
typedef double CPP_f64;
typedef int ArrayType15[15];
typedef float MultidimentionalArrayType44[4][4];

CPP_u32 UseTypedefAsTypeName( CPP_char8 c );

struct RegularStruct; // Forward declaration

struct RegularStruct
{
	int x;
	float y;
	int* ptr_field;
	const int* const_ptr_field;
	void (*function_ptr_filed)(float);
};

typedef struct
{
	char data[16];
} TypedefedStruct;

struct StructWithAnonimousRecordsInside
{
	struct
	{
		int anon_struct_content;
	} anon_struct;
	
	union
	{
		int i; float f;
	} anon_union;

	struct
	{
		int anon_struct_as_array_content;
	} anon_struct_as_array[5];
};

union UnionForwardDeclaration;

enum SequentialEnum
{
	Red, Green, Blue,
};

enum NonSequentialEnum
{
	Zero= 0, One= 1, Ten= 10, MinusTwo= -2, Large32bit= 5000000, LargeNegative= -142536 - 50
};
