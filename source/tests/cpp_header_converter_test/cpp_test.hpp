extern "C" int ExternCPrefixedFunc( int x );

extern "C"
{

float ExternCBlockFunc0( float x, float y );

unsigned int ExternCBlockFunc1();

} // extern "C"

using CppStyleTypeAlias= double;
using CppStyleArrayTypeAlias= unsigned int[4];
using CppStyleFunctionPointer= int(*)( float x );

enum class CppEnumClass
{
	S, Z, T, L, J, I, O,
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
