#include "cpp_tests.hpp"

namespace U
{

namespace
{

// Use this helper function, because C++ string_view can't be constructed properly from char array.
// Constructor from "const char*" is used instead, which causes "strlen" call, which produces wrong result for char arrays with zeros inside.
template<size_t S>
std::string_view MakeStringView( const char (&arr)[S] )
{
	return std::string_view( arr, S );
}

U_TEST( Embed_Test0 )
{
	static const char c_program_text_embed[]={ 0x37, 0x01, char(0xA6), char(0x8E) };

	static const char c_program_text_root[]=
	R"(
		fn Foo()
		{
			auto& embed_result= embed( "embed.bin" );
			var [ byte8, 4 ] expected_result[ (u8(0x37)), (u8(0x01)), (u8(0xA6)), (u8(0x8E)) ];
			static_assert( embed_result == expected_result );
		}
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test1 )
{
	static const char c_program_text_embed[]={ 0x37, 0x00, char(0xA6), char(0x8E), 0x00 };

	static const char c_program_text_root[]=
	R"(
		// Should handle zeros properly.
		auto& embed_result= embed( "embed.bin" );
		var [ byte8, 5 ] expected_result[ (u8(0x37)), (u8(0x00)), (u8(0xA6)), (u8(0x8E)), (u8(0x00)) ];
		static_assert( embed_result == expected_result );
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test2 )
{
	static const char c_program_text_embed[]={ 0x22, 0x3C, char(0xE6) };

	static const char c_program_text_root[]=
	R"(
		auto& extension= ".bin";

		// Embed result is constexpr reference to byte8 array.
		// Arbitrary expression for file path is allowed, as soon as it evaluates to constexpr char array.
		auto& constexpr embed_result= embed( "embed" + extension );

		var [ byte8, 3 ] expected_result[ (u8(0x22)), (u8(0x3C)), (u8(0xE6)) ];
		static_assert( embed_result == expected_result );
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test3 )
{
	// Embed itself.
	static const char c_program_text_root[]= "auto& embed_result= embed(\"root\");";

	BuildMultisourceProgram(
		{
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test4 )
{
	static const char c_program_text_embed[]={ 0x11, 0x22, 0x33, 0x44, 0x55 };

	static const char c_program_text_root[]=
	R"(
		// Embed the same file twice. Should get the same result.
		auto& res0= embed( "embed.bin" );
		auto& res1= embed( "embed.bin" );
		static_assert( res0 == res1 );
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test5 )
{
	static const char c_program_text_embed[]={ 0x11, 0x22, 0x33, 0x55 };

	static const char c_program_text_root[]=
	R"(
		// Embed the same file twice. Should get the same result.
		auto& res0= embed( "embed.bin" );
		auto& res1= embed( "embed.bin" );
		fn Foo()
		{
			// Should have exactly the same address for contents of the same embedded file.
			unsafe
			{
				auto mut ptr0= $<( cast_mut(res0) );
				auto mut ptr1= $<( cast_mut(res1) );
				halt if( ptr0 != ptr1 );
			}
		}
	)";

	const EnginePtr engine=
		CreateEngine(
			BuildMultisourceProgram(
				{
					{ "embed.bin", MakeStringView(c_program_text_embed) },
					{ "root", c_program_text_root }
				},
				"root" ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST( Embed_Test6 )
{
	static const char c_program_text_root[]=
	R"(
		auto& empty= embed( "empty.bin" );
		static_assert( typeinfo</ typeof(empty) />.element_count == 0s );

		auto& single_element= embed( "single_element.bin" );
		static_assert( typeinfo</ typeof(single_element) />.element_count == 1s );

		auto& two_elements= embed( "two_elements.bin" );
		static_assert( typeinfo</ typeof(two_elements) />.element_count == 2s );
	)";

	BuildMultisourceProgram(
		{
			{ "empty.bin", std::string_view() },
			{ "single_element.bin", "Q" },
			{ "two_elements.bin", "Cd" },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test7 )
{
	static const char c_program_text_a[]= "fn Foo(){}";

	static const char c_program_text_root[]=
	R"(
		import "a.u"
		auto& import_contents= embed( "a.u" ); // Embed contents of a file, which was imported previously.
	)";

	BuildMultisourceProgram(
		{
			{ "a.u", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test8 )
{
	static const char c_program_text_a[]= "some contents";

	static const char c_program_text_root[]=
	R"(
		// "embed" in argument expression.
		auto sum= Sum( embed( "a.txt" ) );

		template</type T, size_type S />
		fn Sum( [ T, S ]& arr ) : u32
		{
			auto mut r= 0u;
			for( auto mut i= 0s; i < S; ++i )
			{
				r+= u32( u8( arr[i] ) );
			}
			return r;
		}
	)";

	BuildMultisourceProgram(
		{
			{ "a.txt", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_Test9 )
{
	static const char c_program_text_a[]= "some contents";

	static const char c_program_text_root[]=
	R"(
		// Assign embed contents to an auto-variable.
		auto contents0= embed( "a.txt" );
		// Assign embed contents to a variable with explicit type.
		var[ byte8, 13 ] contents1= embed( "a.txt" );

		static_assert( contents0 == contents1 );
	)";

	BuildMultisourceProgram(
		{
			{ "a.txt", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_WithType_Test0 )
{
	static const char c_program_text_a[]= "fn Foo(){}";

	static const char c_program_text_root[]=
	R"(
		auto& import_contents= embed</char8/>( "a.u" ); // Embed as char8 array.
		static_assert( import_contents == "fn Foo(){}" );
	)";

	BuildMultisourceProgram(
		{
			{ "a.u", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_WithType_Test1 )
{
	static const char c_program_text_embed[]={ 0x37, 0x01, char(0xA6), char(0x8E) };

	static const char c_program_text_root[]=
	R"(
		auto& embed_result= embed</u8/>( "embed.bin" ); // Embed as u8.
		var [ u8, 4 ] expected_result[ (0x37), (0x01), (0xA6), (0x8E) ];
		static_assert( embed_result == expected_result );
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_WithType_Test2 )
{
	static const char c_program_text_embed[]={ 0x55, 0x66, 0x77 };

	static const char c_program_text_root[]=
	R"(
		auto& embed_result= embed</i8/>( "embed.bin" ); // Embed as i8.
		var [ i8, 3 ] expected_result[ (0x55), (0x66), (0x77) ];
		static_assert( embed_result == expected_result );
	)";

	BuildMultisourceProgram(
		{
			{ "embed.bin", MakeStringView(c_program_text_embed) },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_WithType_Test3 )
{
	static const char c_program_text_embed[]= "wieso?";

	static const char c_program_text_root[]=
	R"(
		namespace NN{ type CharAlias= char8; }
		auto& import_contents= embed</NN::CharAlias/>( "test.txt" ); // Embed as char8 array, using type alias.
		static_assert( import_contents == "wieso?" );
	)";

	BuildMultisourceProgram(
		{
			{ "test.txt", c_program_text_embed },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( Embed_WithType_Test4 )
{
	static const char c_program_text_a[]= "fn Bar(){}";

	static const char c_program_text_root[]=
	R"(
		fn Foo()
		{
			Bar(); // This function should be available.
		}
		mixin( embed</char8/>( "a.u" ) ); // use embed contents for mixin.
	)";

	BuildMultisourceProgram(
		{
			{ "a.u", c_program_text_a },
			{ "root", c_program_text_root }
		},
		"root" );
}

U_TEST( TypesMismatch_ForEmbed_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			embed( 42 ); // Expected char8 array, got integer
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4u ) );
}

U_TEST( TypesMismatch_ForEmbed_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( tup[ char8, char8 ]& t )
		{
			embed( t ); // Expected char8 array, got tuple
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4u ) );
}

U_TEST( TypesMismatch_ForEmbed_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			embed( "file.txt"u16 ); // Expected char8 array, got char16 array
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 4u ) );
}

U_TEST( ExpectedConstantExpression_ForEmbed_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto mut f= "file.txt";
			embed( f ); // Given value isn't constant.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedConstantExpression, 5u ) );
}

U_TEST( EmbedFileNotFound_Test0 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed( "cot" ); // can't find this file.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "cat", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::EmbedFileNotFound, 2u ) );
}

U_TEST( EmbedFileNotFound_Test1 )
{
	static const char c_program_text_root[]=
	R"(
		auto& file_name= "qwwrty";
		auto& f= embed( file_name ); // can't find this file.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "qwerty", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::EmbedFileNotFound, 3u ) );
}

U_TEST( NameIsNotTypeName_ForEmbedElementType_Test0 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</Foo/>( "test.txt" );
		fn Foo(){}
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::NameIsNotTypeName, 2u ) );
}

U_TEST( NameIsNotTypeName_ForEmbedElementType_Test1 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</x/>( "test.txt" );
		var u8 x= zero_init;
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::NameIsNotTypeName, 2u ) );
}

U_TEST( TypesMismatch_ForEmbedElementType_Test0 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</u16/>( "test.txt" ); // Can't embed as 16-bit integer.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::TypesMismatch, 2u ) );
}

U_TEST( TypesMismatch_ForEmbedElementType_Test1 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</i32/>( "test.txt" ); // Can't embed as 32-bit integer.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::TypesMismatch, 2u ) );
}

U_TEST( TypesMismatch_ForEmbedElementType_Test2 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</f64/>( "test.txt" ); // Can't embed as f64.
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::TypesMismatch, 2u ) );
}

U_TEST( TypesMismatch_ForEmbedElementType_Test3 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</S/>( "test.txt" ); // Can't embed as struct type.
		struct S{ i8 x; }
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::TypesMismatch, 2u ) );
}

U_TEST( TypesMismatch_ForEmbedElementType_Test4 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</A/>( "test.txt" ); // Can't embed as array type.
		type A= [ char8, 1 ];
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::TypesMismatch, 2u ) );
}

U_TEST( TypesMismatch_ForEmbedElementType_Test5 )
{
	static const char c_program_text_root[]=
	R"(
		auto& f= embed</E/>( "test.txt" ); // Can't embed as enum type.
		enum E{ A, B, C }
	)";

	ErrorTestBuildResult result=
		BuildMultisourceProgramWithErrors(
			{
				{ "test.txt", "contents" },
				{ "root", c_program_text_root }
			},
			"root" );

	U_TEST_ASSERT( !result.errors.empty() );
	U_TEST_ASSERT( HasError( result.errors, CodeBuilderErrorCode::TypesMismatch, 2u ) );
}

} // namespace

} // namespace U
