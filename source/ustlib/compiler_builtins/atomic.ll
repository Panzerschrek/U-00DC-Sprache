; Use strictest memory order - "seq_cst" for all atomic operations.

$ust_atomic_read_bool_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_read_bool_impl( i1* %addr ) unnamed_addr comdat
{
	%1= load atomic i8, i8* %addr seq_cst, align 1
	%2 = trunc i8 %1 to i1
	ret i1 %2
}

$ust_atomic_read_i8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_read_i8_impl( i8* %addr ) unnamed_addr comdat
{
	%1= load atomic i8, i8* %addr seq_cst, align 1
	ret i8 %1
}

$ust_atomic_read_u8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_read_u8_impl( i8* %addr ) unnamed_addr comdat
{
	%1= load atomic i8, i8* %addr seq_cst, align 1
	ret i8 %1
}

$ust_atomic_read_i16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_read_i16_impl( i16* %addr ) unnamed_addr comdat
{
	%1= load atomic i16, i16* %addr seq_cst, align 2
	ret i16 %1
}

$ust_atomic_read_u16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_read_u16_impl( i16* %addr ) unnamed_addr comdat
{
	%1= load atomic i16, i16* %addr seq_cst, align 2
	ret i16 %1
}

$ust_atomic_read_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_read_i32_impl( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

$ust_atomic_read_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_read_u32_impl( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

$ust_atomic_read_f32_impl = comdat any
define linkonce_odr hidden float @ust_atomic_read_f32_impl( float* %addr ) unnamed_addr comdat
{
	%1= load atomic float, float* %addr seq_cst, align 4
	ret float %1
}

$ust_atomic_read_char8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_read_char8_impl( i8* %addr ) unnamed_addr comdat
{
	%1= load atomic i8, i8* %addr seq_cst, align 1
	ret i8 %1
}

$ust_atomic_read_char16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_read_char16_impl( i16* %addr ) unnamed_addr comdat
{
	%1= load atomic i16, i16* %addr seq_cst, align 2
	ret i16 %1
}

$ust_atomic_read_char32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_read_char32_impl( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

$ust_atomic_read_byte8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_read_byte8_impl( i8* %addr ) unnamed_addr comdat
{
	%1= load atomic i8, i8* %addr seq_cst, align 1
	ret i8 %1
}

$ust_atomic_read_byte16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_read_byte16_impl( i16* %addr ) unnamed_addr comdat
{
	%1= load atomic i16, i16* %addr seq_cst, align 2
	ret i16 %1
}

$ust_atomic_read_byte32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_read_byte32_impl( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

$ust_atomic_write_bool_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_bool_impl( i1* %addr, i1 %x ) unnamed_addr comdat
{
	%1 = zext i1 %x to i8
	store atomic i8 %1, i8* %addr seq_cst, align 1
	ret void
}

$ust_atomic_write_i8_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_i8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	store atomic i8 %x, i8* %addr seq_cst, align 1
	ret void
}

$ust_atomic_write_u8_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_u8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	store atomic i8 %x, i8* %addr seq_cst, align 1
	ret void
}

$ust_atomic_write_i16_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_i16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	store atomic i16 %x, i16* %addr seq_cst, align 2
	ret void
}

$ust_atomic_write_u16_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_u16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	store atomic i16 %x, i16* %addr seq_cst, align 2
	ret void
}

$ust_atomic_write_i32_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_i32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic i32 %x, i32* %addr seq_cst, align 4
	ret void
}

$ust_atomic_write_u32_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_u32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic i32 %x, i32* %addr seq_cst, align 4
	ret void
}

$ust_atomic_write_f32_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_f32_impl( float* %addr, float %x ) unnamed_addr comdat
{
	store atomic float %x, float* %addr seq_cst, align 4
	ret void
}

$ust_atomic_write_char8_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_char8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	store atomic i8 %x, i8* %addr seq_cst, align 1
	ret void
}

$ust_atomic_write_char16_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_char16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	store atomic i16 %x, i16* %addr seq_cst, align 2
	ret void
}

$ust_atomic_write_char32_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_char32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic i32 %x, i32* %addr seq_cst, align 4
	ret void
}

$ust_atomic_write_byte8_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_byte8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	store atomic i8 %x, i8* %addr seq_cst, align 1
	ret void
}

$ust_atomic_write_byte16_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_byte16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	store atomic i16 %x, i16* %addr seq_cst, align 2
	ret void
}

$ust_atomic_write_byte32_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_byte32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic i32 %x, i32* %addr seq_cst, align 4
	ret void
}

$ust_atomic_swap_bool_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_swap_bool_impl( i1* %addr, i1 %x ) unnamed_addr comdat
{
	%1= zext i1 %x to i8
	%2= atomicrmw xchg i8* %addr, i8 %1 seq_cst
	%3= trunc i8 %2 to i1
	ret i1 %3
}

$ust_atomic_swap_i8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_swap_i8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i8* %addr, i8 %x seq_cst
	ret i8 %1
}

$ust_atomic_swap_u8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_swap_u8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i8* %addr, i8 %x seq_cst
	ret i8 %1
}

$ust_atomic_swap_i16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_swap_i16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i16* %addr, i16 %x seq_cst
	ret i16 %1
}

$ust_atomic_swap_u16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_swap_u16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i16* %addr, i16 %x seq_cst
	ret i16 %1
}

$ust_atomic_swap_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_swap_i32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i32* %addr, i32 %x seq_cst
	ret i32 %1
}

$ust_atomic_swap_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_swap_u32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i32* %addr, i32 %x seq_cst
	ret i32 %1
}

$ust_atomic_swap_f32_impl = comdat any
define linkonce_odr hidden float @ust_atomic_swap_f32_impl( float* %addr, float %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg float* %addr, float %x seq_cst
	ret float %1
}
$ust_atomic_swap_char8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_swap_char8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i8* %addr, i8 %x seq_cst
	ret i8 %1
}

$ust_atomic_swap_char16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_swap_char16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i16* %addr, i16 %x seq_cst
	ret i16 %1
}

$ust_atomic_swap_char32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_swap_char32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i32* %addr, i32 %x seq_cst
	ret i32 %1
}

$ust_atomic_swap_byte8_impl = comdat any
define linkonce_odr hidden i8 @ust_atomic_swap_byte8_impl( i8* %addr, i8 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i8* %addr, i8 %x seq_cst
	ret i8 %1
}

$ust_atomic_swap_byte16_impl = comdat any
define linkonce_odr hidden i16 @ust_atomic_swap_byte16_impl( i16* %addr, i16 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i16* %addr, i16 %x seq_cst
	ret i16 %1
}

$ust_atomic_swap_byte32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_swap_byte32_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i32* %addr, i32 %x seq_cst
	ret i32 %1
}

$ust_atomic_add_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_add_i32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw add i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_add_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_add_u32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw add i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_and_bool_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_and_bool_impl( i1* %x, i1 %y ) unnamed_addr comdat
{
	%1= zext i1 %y to i8
	%2= atomicrmw and i8* %x, i8 %1 seq_cst
	%3= trunc i8 %2 to i1
	ret i1 %3
}

$ust_atomic_and_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_and_i32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw and i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_and_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_and_u32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw and i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_or_bool_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_or_bool_impl( i1* %x, i1 %y ) unnamed_addr comdat
{
	%1= zext i1 %y to i8
	%2= atomicrmw or i8* %x, i8 %1 seq_cst
	%3= trunc i8 %2 to i1
	ret i1 %3
}

$ust_atomic_or_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_or_i32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw or i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_or_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_or_u32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw or i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_xor_bool_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_xor_bool_impl( i1* %x, i1 %y ) unnamed_addr comdat
{
	%1= zext i1 %y to i8
	%2= atomicrmw xor i8* %x, i8 %1 seq_cst
	%3= trunc i8 %2 to i1
	ret i1 %3
}

$ust_atomic_xor_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_xor_i32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw xor i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_xor_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_xor_u32_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw xor i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_compare_exchange_strong_i32_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_i32_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_strong_u32_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_u32_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_strong_byte32_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_byte32_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_i32_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_i32_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg weak i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_u32_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_u32_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg weak i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_byte32_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_byte32_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg weak i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}
