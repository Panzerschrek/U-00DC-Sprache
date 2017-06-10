#pragma once
#include <memory>
#include <vector>
#include <map>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include "pop_llvm_warnings.hpp"

#include "lang_types.hpp"
#include "program_string.hpp"

namespace Interpreter
{

namespace CodeBuilderLLVMPrivate
{

struct Function;
struct Array;
struct Class;

typedef std::shared_ptr<Class> ClassPtr;

struct Type final
{
	enum class Kind
	{
		Fundamental,
		Function,
		Array,
		Class,
	};

	Kind kind;
	U_FundamentalType fundamental;
	std::unique_ptr<Function> function;
	std::unique_ptr<Array> array;

	// We use one instance for Class, because class has one point of definition.
	// Therefore, class are aquals, if equals pointers.
	ClassPtr class_;

	llvm::Type* fundamental_llvm_type= nullptr;

	explicit Type( U_FundamentalType in_fundamental= U_FundamentalType::InvalidType );
	Type( const Type& other );
	Type( Type&& other ) noexcept;

	Type& operator=( const Type& other );
	Type& operator=( Type&& other ) noexcept;

	// TODO - does this method needs?
	size_t SizeOf() const;

	llvm::Type* GetLLVMType() const;
	ProgramString ToString() const;
};

bool operator==( const Type& r, const Type& l );
bool operator!=( const Type& r, const Type& l );

struct Function final
{
	Type return_type;
	std::vector<Type> args;

	llvm::FunctionType* llvm_function_type;
};

bool operator==( const Function& r, const Function& l );
bool operator!=( const Function& r, const Function& l );

struct Array final
{
	Type type;
	size_t size;

	llvm::ArrayType* llvm_type;
};

struct Class final
{
	Class();
	~Class();

	Class( const Class& )= delete;
	Class( Class&& )= delete;

	Class& operator=( const Class& )= delete;
	Class& operator=( Class&& )= delete;

	struct Field
	{
		ProgramString name;
		Type type;
		unsigned int index;
	};

	const Field* GetField( const ProgramString& name );

	ProgramString name;
	std::vector<Field> fields;

	llvm::StructType* llvm_type;
};

enum class ValueType
{
	Value,
	Reference,
	ConstReference,
};

struct Variable final
{
	enum class Location
	{
		Global,
		PointerToStack,
		LLVMRegister,
	};

	Location location;
	ValueType value_type;
	Type type;

	llvm::Value* llvm_value;
};

struct Name final
{
	// If ptr not null - name is class, else - variable
	ClassPtr class_;
	Variable variable;
};

class NamesScope final
{
public:

	typedef std::map< ProgramString, Name > NamesMap;
	typedef NamesMap::value_type InsertedName;

	NamesScope( const NamesScope* prev= nullptr );

	const InsertedName* AddName( const ProgramString& name, Variable variable );
	const InsertedName* AddName( const ProgramString& name, const ClassPtr& class_ );
	const InsertedName* AddName( const ProgramString& name, const Name name_value );

	const InsertedName* GetName( const ProgramString& name ) const;

private:
	const NamesScope* const prev_;
	NamesMap names_map_;
};

class ProgramError final : public std::exception
{
public:
	virtual ~ProgramError() override{}

	virtual const char* what() const noexcept override
	{
		return "ProgramError";
	}
};

const ProgramString& GetFundamentalTypeName( U_FundamentalType fundamental_type );
const char* GetFundamentalTypeNameASCII( U_FundamentalType fundamental_type );

} //namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
