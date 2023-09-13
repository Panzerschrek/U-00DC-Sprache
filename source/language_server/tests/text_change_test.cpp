#include "../text_change.hpp"
#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"

namespace U
{

namespace LangServer
{

namespace
{

U_TEST( TextChange_Test0 )
{
	// Remove single range.
	const TextChangesSequence changes
	{
		{ 3, 7, 0 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 6 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 7 ) == uint32_t(3) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 8 ) == uint32_t(4) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 108 ) == uint32_t(104) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == uint32_t(7) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == uint32_t(8) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 104 ) == uint32_t(108) );
}

U_TEST( TextChange_Test1 )
{
	// Remove single range at start.
	const TextChangesSequence changes
	{
		{ 0, 3, 0 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 104 ) == uint32_t(101) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(4) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 101 ) == uint32_t(104) );
}

U_TEST( TextChange_Test2 )
{
	// Insert single range.
	const TextChangesSequence changes
	{
		{ 3, 3, 5 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(8) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == uint32_t(9) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 104 ) == uint32_t(109) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 7 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 8 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 109 ) == uint32_t(104) );
}

U_TEST( TextChange_Test3 )
{
	// Insert single range at start.
	const TextChangesSequence changes
	{
		{ 0, 0, 3 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(3) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(4) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 101 ) == uint32_t(104) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 104 ) == uint32_t(101) );
}

U_TEST( TextChange_Test4 )
{
	// Replace single range with greater one.
	const TextChangesSequence changes
	{
		{ 3, 5, 4 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(2) );
	// No position in replaced range can be mapped old to new.
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == uint32_t(7) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 6 ) == uint32_t(8) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 106 ) == uint32_t(108) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(2) );
	// No position in replaced range can be mapped new to old.
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 7 ) == uint32_t(5) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 8 ) == uint32_t(6) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 108 ) == uint32_t(106) );
}

U_TEST( TextChange_Test5 )
{
	// Replace single range with greater one at start.
	const TextChangesSequence changes
	{
		{ 0, 2, 3 },
	};

	// No position in replaced range can be mapped old to new.
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(3) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(4) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 103 ) == uint32_t(104) );

	// No position in replaced range can be mapped new to old.
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 104 ) == uint32_t(103) );
}

U_TEST( TextChange_Test6 )
{
	// Replace single range with smaller one.
	const TextChangesSequence changes
	{
		{ 3, 5, 1 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(2) );
	// No position in replaced range can be mapped old to new.
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == uint32_t(4) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 6 ) == uint32_t(5) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 106 ) == uint32_t(105) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(2) );
	// No position in replaced range can be mapped new to old.
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == uint32_t(5) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == uint32_t(6) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 105 ) == uint32_t(106) );
}

U_TEST( TextChange_Test7 )
{
	// Replace single range with smaller one at start.
	const TextChangesSequence changes
	{
		{ 0, 3, 1 },
	};

	// No position in replaced range can be mapped old to new.
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == uint32_t(2) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 104 ) == uint32_t(102) );

	// No position in replaced range can be mapped new to old.
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(4) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 102 ) == uint32_t(104) );
}

U_TEST( ComplexTextChange_Test0 )
{
	// Insert sequentially two ranges with sizes 3 and 2.
	const TextChangesSequence changes
	{
		{ 2, 2, 3 },
		{ 5, 5, 2 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(7) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(8) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 103 ) == uint32_t(108) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 7 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 8 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 108 ) == uint32_t(103) );
}

U_TEST( ComplexTextChange_Test1 )
{
	// Insert sequentially two ranges second before first one.
	const TextChangesSequence changes
	{
		{ 2, 2, 3 },
		{ 1, 1, 2 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(3) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(7) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(8) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 103 ) == uint32_t(108) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 7 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 8 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 108 ) == uint32_t(103) );
}

U_TEST( ComplexTextChange_Test2 )
{
	// Insert range and remove it.
	const TextChangesSequence changes
	{
		{ 2, 2, 3 },
		{ 2, 5, 0 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(3) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == uint32_t(4) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == uint32_t(5) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 6 ) == uint32_t(6) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 106 ) == uint32_t(106) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == uint32_t(4) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == uint32_t(5) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == uint32_t(6) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 106 ) == uint32_t(106) );
}

U_TEST( ComplexTextChange_Test3 )
{
	// Remove range and insert it.
	const TextChangesSequence changes
	{
		{ 3, 5, 0 },
		{ 3, 3, 2 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == uint32_t(5) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 6 ) == uint32_t(6) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 106 ) == uint32_t(106) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == uint32_t(5) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == uint32_t(6) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 106 ) == uint32_t(106) );
}

U_TEST( ComplexTextChange_Test4 )
{
	// Insert text and remove text after it.
	const TextChangesSequence changes
	{
		{ 2, 2, 3 },
		{ 6, 7, 0 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(5) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == uint32_t(6) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == uint32_t(7) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 105 ) == uint32_t(107) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == uint32_t(4) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 7 ) == uint32_t(5) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 107 ) == uint32_t(105) );
}

U_TEST( ComplexTextChange_Test5 )
{
	// Insert text and remove text before it.
	const TextChangesSequence changes
	{
		{ 3, 3, 2 },
		{ 1, 2, 0 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(4) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == uint32_t(5) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 104 ) == uint32_t(105) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == uint32_t(4) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 105 ) == uint32_t(104) );
}

U_TEST( ComplexTextChange_Test6 )
{
	// Insert range and insert withing this range.
	const TextChangesSequence changes
	{
		{ 2, 2, 2 },
		{ 3, 3, 2 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == uint32_t(6) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == uint32_t(7) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 103 ) == uint32_t(107) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(1) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 6 ) == uint32_t(2) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 7 ) == uint32_t(3) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 107 ) == uint32_t(103) );
}

U_TEST( ComplexTextChange_Test7 )
{
	// Insert range and insert withing this range.
	const TextChangesSequence changes
	{
		{ 2, 4, 0 },
		{ 1, 4, 0 },
	};

	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 1 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 2 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 3 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 4 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 5 ) == std::nullopt );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 6 ) == uint32_t(1) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 7 ) == uint32_t(2) );
	U_TEST_ASSERT( MapOldPositionToNewPosition( changes, 107 ) == uint32_t(102) );

	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 0 ) == uint32_t(0) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 1 ) == uint32_t(6) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 2 ) == uint32_t(7) );
	U_TEST_ASSERT( MapNewPositionToOldPosition( changes, 102 ) == uint32_t(107) );
}

} // namespace

} // namespace LangServer

} // namespace U
