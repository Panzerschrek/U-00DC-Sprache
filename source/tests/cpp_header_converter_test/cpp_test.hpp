extern "C" int ExternCPrefixedFunc( int x );

extern "C"
{

float ExternCBlockFunc0( float x, float y );

unsigned int ExternCBlockFunc1();

} // extern "C"

using CppStyleTypeAlias= double;
using CppStyleArrayTypeAlias= unsigned int[4];
using CppStyleFunctionPointer= int(*)( float x );
using CppFunctionPointerWithReferences= const int&(*)( int& a, const float& b );
using CppDoubleReference= CppStyleTypeAlias&;
using CppDoubleConstReference= const CppStyleTypeAlias&;
using CppDoubleRValueReference= double&&;

enum CppOldStyleEnum
{
	OldStyle0, OldStyle1, OldStyle2,
};

enum class CppEnumClass
{
	S, Z, T, L, J, I, O,
};

enum class CppUnsequentialEnumClass
{
	One= 1, TwentyTwo=22, Eight= 8,
};

enum class CppEnumWithUnderlyingType : unsigned char
{
	UC0, UC1, UC2, UC45= 45,
};

struct Vec2f
{
	float x;
	float y;
};

struct CppEmptyStruct{};

class CppClass
{
public:
	CppClass();
	~CppClass();

private:
	bool b_;
	float x_;
	int y_;
};

class CppInterface
{
public:
	// Adding virtual destructor creates implicit virtual table pointer field in this class.
	virtual ~CppInterface()= default;
};

class CppDerived : public CppInterface
{
public:
	int* some_field;
};

constexpr int g_some_int_contant= 37;

constexpr unsigned int g_group_constant0= 11, g_group_constant1= 0xFF, g_group_constant2= 2633676, g_group_constant3= g_some_int_contant * 5;

const float g_float_constant= -17.5f;

const CppEnumClass g_enum_constant= CppEnumClass::I;

// Can't convert this constant, since its initializer isn't constexpr.
const int g_constant_with_dynamic_initializer= ExternCPrefixedFunc( 1 );

constexpr inline char GetChar() { return '7'; }

const char g_constant_with_constexpr_call_initializer= GetChar();

// Initializer is constexpr, but variable itself isn't constant and thus shouldn't be converted.
unsigned int g_not_a_constant= 797;

extern unsigned int g_extern_non_constant;

const double g_constant_with_constructor_initializer( 3.1415926535 );

constexpr int g_constant_with_universal_initializer{ -612 };

const int g_constant_array[5]{ 88, 77, 66, 55, 44 };
const float g_constant_2d_array[2][3]{ { -1.0f, 0.0f, 32.0f }, { 13.2f, -45.3f, 1111.0f } };
constexpr long long unsigned int g_u64_array[2]{ 12345678945, 0xFEDCBA9876543210 };

// For now can't convert such arrays.
const int g_array_with_zero_filler[100]{};

// For now can't convert such arrays.
const double g_array_with_not_enough_initializers[4]{ 1.0f, 2.0f };

// For now don't support constants of struct types.
const Vec2f g_constant_struct{ 78.2f, -13.3f };

const auto g_cpp_auto_constant= 78767556676333;
constexpr auto g_cpp_auto_float_constant= -13.2f + 76.0f;

// Can't convert it, since it may be changed.
auto g_cpp_auto_non_constant= 376766;

// Can't convert this - raw pointers in Ü can't be used for global variables.
const int* g_ptr_constant= nullptr;

// For now can't convert global references.
const unsigned int& g_global_const_ref= g_not_a_constant;
