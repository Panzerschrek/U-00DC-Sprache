#include "assert.hpp"
#include "keywords.hpp"

#include "code_builder_llvm_types.hpp"

namespace Interpreter
{

namespace CodeBuilderLLVMPrivate
{

namespace
{

const size_t g_fundamental_types_size[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, 0u ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void, 0 ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, sizeof(U_bool) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , sizeof(U_i8 ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , sizeof(U_u8 ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, sizeof(U_i16) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, sizeof(U_u16) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, sizeof(U_i32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, sizeof(U_u32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, sizeof(U_i64) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, sizeof(U_u64) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, sizeof(U_f32) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, sizeof(U_f64) ),
};

const char g_invalid_type_name_ascii[]= "InvalidType";
const ProgramString g_invalid_type_name= ToProgramString( g_invalid_type_name_ascii );

const char* const g_fundamental_types_names_ascii[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, g_invalid_type_name_ascii ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  KeywordAscii( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, KeywordAscii( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , KeywordAscii( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , KeywordAscii( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, KeywordAscii( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, KeywordAscii( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, KeywordAscii( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, KeywordAscii( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, KeywordAscii( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, KeywordAscii( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, KeywordAscii( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, KeywordAscii( Keywords::f64_ ) ),
};

const ProgramString (&g_fundamental_types_names)[ size_t(U_FundamentalType::LastType) ]=
{
	U_DESIGNATED_INITIALIZER( U_FundamentalType::InvalidType, g_invalid_type_name ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Void,  Keyword( Keywords::void_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::Bool, Keyword( Keywords::bool_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i8 , Keyword( Keywords::i8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u8 , Keyword( Keywords::u8_  ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i16, Keyword( Keywords::i16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u16, Keyword( Keywords::u16_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i32, Keyword( Keywords::i32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u32, Keyword( Keywords::u32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::i64, Keyword( Keywords::i64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::u64, Keyword( Keywords::u64_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f32, Keyword( Keywords::f32_ ) ),
	U_DESIGNATED_INITIALIZER( U_FundamentalType::f64, Keyword( Keywords::f64_ ) ),
};

} // namespace

FundamentalType::FundamentalType(
	const U_FundamentalType in_fundamental_type,
	llvm::Type* const in_llvm_type )
	: fundamental_type(in_fundamental_type)
	, llvm_type(in_llvm_type)
{}

bool operator==( const FundamentalType& r, const FundamentalType& l )
{
	return r.fundamental_type == l.fundamental_type;
}

bool operator!=( const FundamentalType& r, const FundamentalType& l )
{
	return !( r == l );
}

Type::Type( const Type& other )
{
	*this= other;
}

Type& Type::operator=( const Type& other )
{
	struct Visitor final : public boost::static_visitor<>
	{
		Type& this_;

		explicit Visitor( Type& in_this_ )
			: this_(in_this_)
		{}

		void operator()( const FundamentalType& fundamental )
		{
			this_.one_of_type_kind= fundamental;
		}

		void operator()( const FunctionPtr& function )
		{
			if( function == nullptr )
				this_.one_of_type_kind= FunctionPtr();
			else
				this_.one_of_type_kind= FunctionPtr( new Function( *function ) );
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr )
				this_.one_of_type_kind= ArrayPtr();
			else
				this_.one_of_type_kind= ArrayPtr( new Array( *array ) );
		}

		void operator()( const ClassPtr& class_ )
		{
			this_.one_of_type_kind= class_;
		}
	};

	Visitor visitor( *this );
	boost::apply_visitor( visitor, other.one_of_type_kind );
	return *this;
}

size_t Type::SizeOf() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		size_t size= 1u;

		void operator()( const FundamentalType& fundamental )
		{
			size= g_fundamental_types_size[ size_t( fundamental.fundamental_type ) ];
		}

		void operator()( const FunctionPtr& function )
		{
			if( function == nullptr ) return;
			U_ASSERT( false && "SizeOf method not supported for functions." );
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr ) return;
			size= array->type.SizeOf() * array->size;
		}

		void operator()( const ClassPtr& class_ )
		{
			if( class_ == nullptr ) return;
			U_ASSERT( false && "SizeOf method not supported for classes." );
		}
	};

	Visitor visitor;
	boost::apply_visitor( visitor, one_of_type_kind );
	return visitor.size;
}

llvm::Type* Type::GetLLVMType() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		llvm::Type* llvm_type= nullptr;

		void operator()( const FundamentalType& fundamental )
		{
			llvm_type= fundamental.llvm_type;
		}

		void operator()( const FunctionPtr& function )
		{
			if( function == nullptr ) return;
			llvm_type= function->llvm_function_type;
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr ) return;
			llvm_type= array->llvm_type;
		}

		void operator()( const ClassPtr& class_ )
		{
			if( class_ == nullptr ) return;
			llvm_type= class_->llvm_type;
		}
	};

	Visitor visitor;
	boost::apply_visitor( visitor, one_of_type_kind );
	return visitor.llvm_type;
}

ProgramString Type::ToString() const
{
	struct Visitor final : public boost::static_visitor<>
	{
		ProgramString result;

		void operator()( const FundamentalType& fundamental )
		{
			result= GetFundamentalTypeName( fundamental.fundamental_type );
		}

		void operator()( const FunctionPtr& function )
		{
			if( function == nullptr ) return;

			result+= "fn "_SpC;
			result+= function->return_type.ToString();
			result+= " ( "_SpC;
			for( const Function::Arg& arg : function->args )
			{
				if( arg.is_reference )
					result+= "&"_SpC;
				if( arg.is_mutable )
					result+= "mut "_SpC;
				else
					result+= "imut "_SpC;

				result+= arg.type.ToString();
				if( &arg != &function->args.back() )
					result+= ", "_SpC;
			}
			result+= " )"_SpC;
		}

		void operator()( const ArrayPtr& array )
		{
			if( array == nullptr ) return;

			result=
				"[ "_SpC + array->type.ToString() + ", "_SpC +
				ToProgramString( std::to_string( array->size ).c_str() ) + " ]"_SpC;
		}

		void operator()( const ClassPtr& class_ )
		{
			if( class_ == nullptr ) return;

			result= "class "_SpC + class_->name;
		}
	};

	Visitor visitor;
	boost::apply_visitor( visitor, one_of_type_kind );
	return std::move( visitor.result );
}

bool operator==( const Type& r, const Type& l )
{
	if( r.one_of_type_kind.which() != l.one_of_type_kind.which() )
		return false;

	if( r.one_of_type_kind.which() == 0 )
	{
		return boost::get<FundamentalType>(r.one_of_type_kind) == boost::get<FundamentalType>(l.one_of_type_kind);
	}
	else if( r.one_of_type_kind.which() == 1 )
	{
		const FunctionPtr& r_function= boost::get< FunctionPtr >(r.one_of_type_kind);
		const FunctionPtr& l_function= boost::get< FunctionPtr >(l.one_of_type_kind);
		if( r_function == l_function )
			return true;
		if( r_function != nullptr && l_function != nullptr )
			return *r_function == *l_function;
		return false;
	}
	else if( r.one_of_type_kind.which() == 2 )
	{
		const ArrayPtr& r_array= boost::get< ArrayPtr >(r.one_of_type_kind);
		const ArrayPtr& l_array= boost::get< ArrayPtr >(l.one_of_type_kind);
		if( r_array == l_array )
			return true;
		if( r_array != nullptr && l_array != nullptr )
			return *r_array == *l_array;
		return false;
	}
	else if( r.one_of_type_kind.which() == 3 )
	{
		return r.one_of_type_kind == l.one_of_type_kind;
	}

	U_ASSERT(false);
	return false;
}

bool operator!=( const Type& r, const Type& l )
{
	return !( r == l );
}

bool operator==( const Function::Arg& r, const Function::Arg& l )
{
	return r.type == l.type && r.is_mutable == l.is_mutable && r.is_reference == l.is_reference;
}

bool operator!=( const Function::Arg& r, const Function::Arg& l )
{
	return !( r == l );
}

bool operator==( const Function& r, const Function& l )
{
	return
		r.return_type == l.return_type &&
		r.return_value_is_mutable == l.return_value_is_mutable &&
		r.return_value_is_reference == l.return_value_is_reference &&
		r.args == l.args;
}

bool operator!=( const Function& r, const Function& l )
{
	return !( r == l );
}

bool operator==( const Array& r, const Array& l )
{
	return r.type == l.type && r.size == l.size;
}

bool operator!=( const Array& r, const Array& l )
{
	return !( r == l );
}

Class::Class()
{}

Class::~Class()
{}

const Class::Field* Class::GetField( const ProgramString& name )
{

	for( const Field& field : fields )
	{
		if( field.name == name )
			return &field;
	}

	return nullptr;
}

NamesScope::NamesScope( const NamesScope* prev )
	: prev_(prev)
{}

const NamesScope::InsertedName* NamesScope::AddName(
		const ProgramString& name,
		Variable variable )
{
	return AddName( name, Name{ nullptr, std::move( variable ) } );
}

const NamesScope::InsertedName* NamesScope::AddName(
	const ProgramString& name,
	const ClassPtr& class_ )
{
	return AddName( name, Name{ class_, Variable() } );
}

const NamesScope::InsertedName* NamesScope::AddName(
	const ProgramString& name,
	const Name name_value )
{
	auto it_bool_pair = names_map_.emplace( name, std::move( name_value ) );
	if( it_bool_pair.second )
		return &*it_bool_pair.first;

	return nullptr;
}

const NamesScope::InsertedName*
	NamesScope::GetName(
		const ProgramString& name ) const
{
	auto it= names_map_.find( name );
	if( it != names_map_.end() )
		return &*it;

	if( prev_ != nullptr )
		return prev_->GetName( name );

	return nullptr;
}

const ProgramString& GetFundamentalTypeName( const U_FundamentalType fundamental_type )
{
	if( fundamental_type >= U_FundamentalType::LastType )
		return g_invalid_type_name;

	return g_fundamental_types_names[ size_t( fundamental_type ) ];
}

const char* GetFundamentalTypeNameASCII( const U_FundamentalType fundamental_type )
{
	if( fundamental_type >= U_FundamentalType::LastType )
		return g_invalid_type_name_ascii;

	return g_fundamental_types_names_ascii[ size_t( fundamental_type ) ];
}

} //namespace CodeBuilderLLVMPrivate

} // namespace Interpreter
