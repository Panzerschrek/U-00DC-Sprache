$ust_volatile_read_bool_impl = comdat any
define linkonce_odr hidden fastcc i1 @ust_volatile_read_bool_impl( i1* %addr ) unnamed_addr comdat
{
	%1= load volatile i1, i1* %addr
	ret i1 %1
}

$ust_volatile_write_bool_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_bool_impl( i1* %addr, i1 %x ) unnamed_addr comdat
{
	store volatile i1 %x, i1* %addr
	ret void
}

$ust_volatile_read_i8_impl = comdat any
define linkonce_odr hidden fastcc   i8 @ust_volatile_read_i8_impl(   i8* %addr ) unnamed_addr comdat
{
	%1= load volatile   i8,   i8* %addr
	ret   i8 %1
}

$ust_volatile_read_u8_impl = comdat any
define linkonce_odr hidden fastcc   i8 @ust_volatile_read_u8_impl(   i8* %addr ) unnamed_addr comdat
{
	%1= load volatile   i8,   i8* %addr
	ret   i8 %1
}

$ust_volatile_read_i16_impl = comdat any
define linkonce_odr hidden fastcc  i16 @ust_volatile_read_i16_impl(  i16* %addr ) unnamed_addr comdat
{
	%1= load volatile  i16,  i16* %addr
	ret  i16 %1
}

$ust_volatile_read_u16_impl = comdat any
define linkonce_odr hidden fastcc  i16 @ust_volatile_read_u16_impl(  i16* %addr ) unnamed_addr comdat
{
	%1= load volatile  i16,  i16* %addr
	ret  i16 %1
}

$ust_volatile_read_i32_impl = comdat any
define linkonce_odr hidden fastcc  i32 @ust_volatile_read_i32_impl(  i32* %addr ) unnamed_addr comdat
{
	%1= load volatile  i32,  i32* %addr
	ret  i32 %1
}

$ust_volatile_read_u32_impl = comdat any
define linkonce_odr hidden fastcc  i32 @ust_volatile_read_u32_impl(  i32* %addr ) unnamed_addr comdat
{
	%1= load volatile  i32,  i32* %addr
	ret  i32 %1
}

$ust_volatile_read_i64_impl = comdat any
define linkonce_odr hidden fastcc  i64 @ust_volatile_read_i64_impl(  i64* %addr ) unnamed_addr comdat
{
	%1= load volatile  i64,  i64* %addr
	ret  i64 %1
}

$ust_volatile_read_u64_impl = comdat any
define linkonce_odr hidden fastcc  i64 @ust_volatile_read_u64_impl(  i64* %addr ) unnamed_addr comdat
{
	%1= load volatile  i64,  i64* %addr
	ret  i64 %1
}

$ust_volatile_read_i128_impl = comdat any
define linkonce_odr hidden fastcc i128 @ust_volatile_read_i128_impl( i128* %addr ) unnamed_addr comdat
{
	%1= load volatile i128, i128* %addr
	ret i128 %1
}

$ust_volatile_read_u128_impl = comdat any
define linkonce_odr hidden fastcc i128 @ust_volatile_read_u128_impl( i128* %addr ) unnamed_addr comdat
{
	%1= load volatile i128, i128* %addr
	ret i128 %1
}

$ust_volatile_write_i8_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_i8_impl(   i8* %addr,   i8 %x ) unnamed_addr comdat
{
	store volatile   i8 %x,   i8* %addr
	ret void
}

$ust_volatile_write_u8_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_u8_impl(   i8* %addr,   i8 %x ) unnamed_addr comdat
{
	store volatile   i8 %x,   i8* %addr
	ret void
}

$ust_volatile_write_i16_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_i16_impl(  i16* %addr,  i16 %x ) unnamed_addr comdat
{
	store volatile  i16 %x,  i16* %addr
	ret void
}

$ust_volatile_write_u16_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_u16_impl(  i16* %addr,  i16 %x ) unnamed_addr comdat
{
	store volatile  i16 %x,  i16* %addr
	ret void
}

$ust_volatile_write_i32_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_i32_impl(  i32* %addr,  i32 %x ) unnamed_addr comdat
{
	store volatile  i32 %x,  i32* %addr
	ret void
}

$ust_volatile_write_u32_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_u32_impl(  i32* %addr,  i32 %x ) unnamed_addr comdat
{
	store volatile  i32 %x,  i32* %addr
	ret void
}

$ust_volatile_write_i64_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_i64_impl(  i64* %addr,  i64 %x ) unnamed_addr comdat
{
	store volatile  i64 %x,  i64* %addr
	ret void
}

$ust_volatile_write_u64_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_u64_impl(  i64* %addr,  i64 %x ) unnamed_addr comdat
{
	store volatile  i64 %x,  i64* %addr
	ret void
}

$ust_volatile_write_i128_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_i128_impl( i128* %addr, i128 %x ) unnamed_addr comdat
{
	store volatile i128 %x, i128* %addr
	ret void
}

$ust_volatile_write_u128_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_u128_impl( i128* %addr, i128 %x ) unnamed_addr comdat
{
	store volatile i128 %x, i128* %addr
	ret void
}

$ust_volatile_read_f32_impl = comdat any
define linkonce_odr hidden fastcc  float @ust_volatile_read_f32_impl(  float* %addr ) unnamed_addr comdat
{
	%1= load volatile  float,  float* %addr
	ret  float %1
}

$ust_volatile_read_f64_impl = comdat any
define linkonce_odr hidden fastcc  double @ust_volatile_read_f64_impl(  double* %addr ) unnamed_addr comdat
{
	%1= load volatile  double,  double* %addr
	ret  double %1
}

$ust_volatile_write_f32_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_f32_impl(  float* %addr,  float %x ) unnamed_addr comdat
{
	store volatile  float %x,  float* %addr
	ret void
}

$ust_volatile_write_f64_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_f64_impl(  double* %addr,  double %x ) unnamed_addr comdat
{
	store volatile  double %x,  double* %addr
	ret void
}

$ust_volatile_read_char8_impl = comdat any
define linkonce_odr hidden fastcc   i8 @ust_volatile_read_char8_impl(   i8* %addr ) unnamed_addr comdat
{
	%1= load volatile   i8,   i8* %addr
	ret   i8 %1
}

$ust_volatile_read_char16_impl = comdat any
define linkonce_odr hidden fastcc  i16 @ust_volatile_read_char16_impl(  i16* %addr ) unnamed_addr comdat
{
	%1= load volatile  i16,  i16* %addr
	ret  i16 %1
}

$ust_volatile_read_char32_impl = comdat any
define linkonce_odr hidden fastcc  i32 @ust_volatile_read_char32_impl(  i32* %addr ) unnamed_addr comdat
{
	%1= load volatile  i32,  i32* %addr
	ret  i32 %1
}

$ust_volatile_write_char8_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_char8_impl(   i8* %addr,   i8 %x ) unnamed_addr comdat
{
	store volatile   i8 %x,   i8* %addr
	ret void
}

$ust_volatile_write_char16_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_char16_impl(  i16* %addr,  i16 %x ) unnamed_addr comdat
{
	store volatile  i16 %x,  i16* %addr
	ret void
}

$ust_volatile_write_char32_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_char32_impl(  i32* %addr,  i32 %x ) unnamed_addr comdat
{
	store volatile  i32 %x,  i32* %addr
	ret void
}

$ust_volatile_read_byte8_impl = comdat any
define linkonce_odr hidden fastcc   i8 @ust_volatile_read_byte8_impl(   i8* %addr ) unnamed_addr comdat
{
	%1= load volatile   i8,   i8* %addr
	ret   i8 %1
}

$ust_volatile_read_byte16_impl = comdat any
define linkonce_odr hidden fastcc  i16 @ust_volatile_read_byte16_impl(  i16* %addr ) unnamed_addr comdat
{
	%1= load volatile  i16,  i16* %addr
	ret  i16 %1
}

$ust_volatile_read_byte32_impl = comdat any
define linkonce_odr hidden fastcc  i32 @ust_volatile_read_byte32_impl(  i32* %addr ) unnamed_addr comdat
{
	%1= load volatile  i32,  i32* %addr
	ret  i32 %1
}

$ust_volatile_read_byte64_impl = comdat any
define linkonce_odr hidden fastcc  i64 @ust_volatile_read_byte64_impl(  i64* %addr ) unnamed_addr comdat
{
	%1= load volatile  i64,  i64* %addr
	ret  i64 %1
}

$ust_volatile_read_byte128_impl = comdat any
define linkonce_odr hidden fastcc i128 @ust_volatile_read_byte128_impl( i128* %addr ) unnamed_addr comdat
{
	%1= load volatile i128, i128* %addr
	ret i128 %1
}

$ust_volatile_write_byte8_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_byte8_impl(   i8* %addr,   i8 %x ) unnamed_addr comdat
{
	store volatile   i8 %x,   i8* %addr
	ret void
}

$ust_volatile_write_byte16_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_byte16_impl(  i16* %addr,  i16 %x ) unnamed_addr comdat
{
	store volatile  i16 %x,  i16* %addr
	ret void
}

$ust_volatile_write_byte32_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_byte32_impl(  i32* %addr,  i32 %x ) unnamed_addr comdat
{
	store volatile  i32 %x,  i32* %addr
	ret void
}

$ust_volatile_write_byte64_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_byte64_impl(  i64* %addr,  i64 %x ) unnamed_addr comdat
{
	store volatile  i64 %x,  i64* %addr
	ret void
}

$ust_volatile_write_byte128_impl = comdat any
define linkonce_odr hidden fastcc void @ust_volatile_write_byte128_impl( i128* %addr, i128 %x ) unnamed_addr comdat
{
	store volatile i128 %x, i128* %addr
	ret void
}
