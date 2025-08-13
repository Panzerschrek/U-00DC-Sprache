extern "C" int ExternCPrefixedFunc( int x );

extern "C"
{

float ExternCBlockFunc0( float x, float y );

unsigned int ExternCBlockFunc1();

} // extern "C"

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
