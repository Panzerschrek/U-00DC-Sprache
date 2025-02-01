// Test C header for convertion to Ü header.

#define ONE_CONSTANT 1
#define PI_CONSTANT   3.1415926535
#define PI_CONSTANT_F 3.1415926535f
#define UNSIGNED_CONSTANT 12345u
#define HEX_CONSTNT 0xFF
#define TWO_CONSTANT_DOUBLE 2.0
#define SMALL_CONSTANT 0.00000000056
#define LARGE_CONSTANT 560000000000.0
#define LARGE_INTEGER_CONSTANT 153124586353854499LL
#define LARGE_INTEGER_UNSIGNED_CONSTANT 7655554499878564uLL
#define LARGE_INTEGER_CONSTANT_TRUNCATED 9876543210
#define STRING_CONSTANT_ASCII "WTF?009a"
#define STRING_CONSTANT_UTF8_IMPLICIT "Чё?"
#define STRING_CONSTANT_WITH_SPECIAL_SYMBOLS "\u00DC \x13 one \t two \n quote \' double \" slash \\ "
#define STRING_CONSTANT_WIDE L"широченный чар - wide char str"
#define STRING_CONSTANT_UTF8 u8" утф8 - наше всё"
#define STRING_CONSTANT_UTF16 u"Таки да, utf-16 это"
#define STRING_CONSTANT_UTF32 U"да даже utf-32 могём"
#define CHAR_CONSTANT 'Z'

void Function_ZeroArgs();
void Function_ArgVal( int x );
void Function_ArgPtr( int* x );
void Function_ArgConstPtr( const int* x );
void Function_ArgPtrToPtr( float** f );
int Function_RetVal();
int* Function_RetPtr();
const int* Function_RetConstPtr();
float** Function_RetPtrToPtr();

void Function_UnnamedArgs( int, float, double );

void DuplicatedProto( int x );
void DuplicatedProto( int xx );

extern int ExternallyDeclaredFunction( const char* s );

typedef char CPP_char8;
typedef   signed char  CPP_i8;
typedef unsigned char  CPP_u8;
typedef   signed short CPP_i16;
typedef unsigned short CPP_u16;
typedef   signed int   CPP_i32;
typedef unsigned int   CPP_u32;
typedef   signed long long int CPP_i64;
typedef unsigned long long int CPP_u64;
typedef float  CPP_f32;
typedef double CPP_f64;
typedef int ArrayType15[15];
typedef float MultidimentionalArrayType44[4][4];

CPP_u32 UseTypedefAsTypeName( CPP_char8 c );

struct RegularStruct
{
	int x;
	float y;
	int* ptr_field;
	const int* const_ptr_field;
	int** ptr_to_ptr_field;
	void (*function_ptr_field)(float);
	void* void_ptr_field; // Should be translated as "$(byte8)".
	void** void_ptr_ptr_field; // Should be translated as "$($(byte8))".
};

typedef struct
{
	char data[16];
} TypedefedStruct;

typedef struct StupidStuctNaming
{
	int x;
} StupidStuctNaming;

typedef struct StructWithName
{
	unsigned int ff;
} TypedefForStructWithName, *PointerTypedefForStructWithName;

void StupidFunc( StupidStuctNaming* s );

struct StructUsedWithoutDeclaration* FunctionReturningUnknownStruct();

inline struct AnotherStructUsedWithoutDeclaration* InlineFunctionReturningUnknownStruct() { return 0; }

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

struct StructWithNamedRecordsInside
{
	struct InnerA
	{
		unsigned int x;
		float y;
	} a_arr[2];

	struct InnerB
	{
		char s[16];

		struct InnerC
		{
			int sign;
		} *c_ptr;
	} b;
};

struct StructWithAnonUnion
{
	unsigned int field_before;
	union
	{
		float float_val;
		int int_val;
	};
	unsigned int field_after;
};

union UnionAlign1 { char c; };
union UnionAlign2 { short s; };
union UnionAlign4 { int i; };

union UnionForwardDeclaration;

struct StructWithBitFields
{
	float x;
	int y : 13;
	int z : 3;
};

struct StructWithOnlyBitFields
{
	short a : 3;
	short b : 5;
	short c : 2;
};

struct LargeStructWithSingleBitField
{
	char arr[10];
	int val;
	int f: 2;
	double x;
	int arr2[5];
};

enum SequentialEnum
{
	Red, Green, Blue,
};

void SequentialEnumFunc( enum SequentialEnum s );

enum NonSequentialEnum
{
	Zero= 0, One= 1, Ten= 10, MinusTwo= -2, Large32bit= 5000000, LargeNegative= -142536 - 50
};

void NonSequentialEnumFunc(enum  NonSequentialEnum e );

typedef enum // Anonymous
{
	AnonA, AnonB, AnonC,
} TypedefForEnumABC;

void ABCFunc( TypedefForEnumABC arg );

typedef enum
{
	AnonX= 23, AnonY= 24, AnonZ= 25,
} TypedefForEnumXYZ;

enum // Anonymous and without typedef
{
	TotallyAnonym0,
	TotallyAnonym1,
};

void XYZFunc( TypedefForEnumXYZ arg );

enum LargeValuesEnum : unsigned long long int
{
	LV_Large= 987654321uLL,
	LV_EvenLarger= 123456789101112uLL,
};

enum : unsigned long long int
{
	Anon_Large= 88888888888uLL,
	Anon_EvenLarger= 7654321098765uLL,
};

typedef struct SameNameForStructAndTypedef
{
	int dummy;
} SameNameForStructAndTypedef;

typedef struct DifferentNamesForStructAndTypedef_0
{
	int dummy0;
	float dummy1;
} DifferentNamesForStructAndTypedef_1;

// Should properly process forward declaration without later definition.
struct SomeForwardDeclaration;

// Should properly process forward declaration with later definition.
struct SomeForwardDeclarationWithoutLaterDefinition;

struct SomeForwardDeclaration
{
	int contents;
	float contents2;
};

// Should process "typedef enum" for pointers.
typedef enum
{
	Eins, Zwei, Drei, Vier,
} NumbersEnum, *NumbersEnumPtr;

typedef float SillyName;

// In C it's fine - param name doesn't shadow type name.
void SillyFunction( SillyName SillyName );

struct SillyStructWithAStrangeField
{
	// In C it's fine - type name doesn't shadow field name.
	SillyName SillyName;
};

// A function with name as Ü keyword.
// Should ignore it.
void yield();

// Underscore functions should be ignored too.
void _IgnoreThis();

// Test for __stdcall functions under 32-bit windows.
#ifdef _WIN32
	#ifndef _WIN64
		void __stdcall StdCallFunc(void);
	#endif
#endif

struct SameNameForStructAndFunc1
{
	int contents;
};

void SameNameForStructAndFunc1( struct SameNameForStructAndFunc1* );

struct SameNameForStructAndFunc2* SameNameForStructAndFunc2(void);

struct SameNameForStructAndFunc2
{
	float contents;
};

typedef struct TypedefStructWithSameNameForwardDeclaration TypedefStructWithSameNameForwardDeclaration;

struct TypedefStructWithSameNameForwardDeclaration
{
	void* contents;
};

#ifdef _WIN32
	// Should generate prototype for this function, but it isn't possible to call it, because Ü doesn't support dllimport.
	__declspec(dllimport) int SomeDllImportedFunction(unsigned int, void*);

	// But this function should be callable.
	__declspec(dllexport) int SomeDllExportedFunction(void);
#endif

// Should convert function with variadic params, but skip them - Ü doesn't support C-style variadic params.
int VariadicFunc( int x, const char* s, ...);

// Should translate array params as raw pointers.
void ArrayArg( int arg[4] );
void IncompleteArrayArg( int arg[] );

typedef int TypedefForFunctionType( int param_a, void* param_b );

struct StructWithFunctionTypePtrField
{
	// Should translate this into proper function pointer.
	TypedefForFunctionType* ptr;
};

// Should translate this into type alias for function pointer with no params.
typedef int (*FunctionTypedefNoProto)();

void* VoidPtrRetFunc();
const void* ConstVoidPtrRetFunc();

void VoidPtrParamFunc(void* p);
void ConstVoidPtrParamFunc(const void* p);

typedef void MyVoid; // Should translate it as byte8
typedef MyVoid *MyVoidPtr; // Should translate it as $(byte8)

// Should translate "MyVoid" for return value as proper "Ü void".
MyVoid VoidPtrTypedefParamFunc(MyVoidPtr p);

#ifdef __cplusplus
	#if __cplusplus >= 201703L // Equal or newer then C++17
		#define MY_NODISCARD [[ nodiscard ]]
	#else
		#define MY_NODISCARD
	#endif
#else
	#if __STDC_VERSION__ > 201710L // Newer then C17
		#define MY_NODISCARD [[ nodiscard ]]
	#else
		#define MY_NODISCARD
	#endif
#endif

struct MY_NODISCARD SomeNoDiscardStruct
{
	int contents;
};

enum MY_NODISCARD SomeNoDiscardEnum
{
	NoDiscardA, NoDiscardB, NoDiscardC,
};

MY_NODISCARD int CNoDiscardFunc();
