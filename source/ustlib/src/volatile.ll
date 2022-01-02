; fn volatile_read(  i8& addr) :   i8;
$_ZN3ust13volatile_readERKa = comdat any
define linkonce_odr   i8 @_ZN3ust13volatile_readERKa(   i8* %addr ) unnamed_addr comdat
{
	%1= load volatile   i8,   i8* %addr
	ret   i8 %1
}

; fn volatile_read(  u8& addr) :   u8;
$_ZN3ust13volatile_readERKh = comdat any
define linkonce_odr   i8 @_ZN3ust13volatile_readERKh(   i8* %addr ) unnamed_addr comdat
{
	%1= load volatile   i8,   i8* %addr
	ret   i8 %1
}

; fn volatile_read( i16& addr) :  i16;
$_ZN3ust13volatile_readERKs = comdat any
define linkonce_odr  i16 @_ZN3ust13volatile_readERKs(  i16* %addr ) unnamed_addr comdat
{
	%1= load volatile  i16,  i16* %addr
	ret  i16 %1
}

; fn volatile_read( u16& addr) :  u16;
$_ZN3ust13volatile_readERKt = comdat any
define linkonce_odr  i16 @_ZN3ust13volatile_readERKt(  i16* %addr ) unnamed_addr comdat
{
	%1= load volatile  i16,  i16* %addr
	ret  i16 %1
}

; fn volatile_read( i32& addr) :  i132;
$_ZN3ust13volatile_readERKi = comdat any
define linkonce_odr  i32 @_ZN3ust13volatile_readERKi(  i32* %addr ) unnamed_addr comdat
{
	%1= load volatile  i32,  i32* %addr
	ret  i32 %1
}

; fn volatile_read( u32& addr) :  u32;
$_ZN3ust13volatile_readERKj = comdat any
define linkonce_odr  i32 @_ZN3ust13volatile_readERKj(  i32* %addr ) unnamed_addr comdat
{
	%1= load volatile  i32,  i32* %addr
	ret  i32 %1
}

; fn volatile_read( i64& addr) :  i64;
$_ZN3ust13volatile_readERKx = comdat any
define linkonce_odr  i64 @_ZN3ust13volatile_readERKx(  i64* %addr ) unnamed_addr comdat
{
	%1= load volatile  i64,  i64* %addr
	ret  i64 %1
}

; fn volatile_read( u16& addr) :  u64;
$_ZN3ust13volatile_readERKy = comdat any
define linkonce_odr  i64 @_ZN3ust13volatile_readERKy(  i64* %addr ) unnamed_addr comdat
{
	%1= load volatile  i64,  i64* %addr
	ret  i64 %1
}

; fn volatile_read(i128& addr) : i128;
$_ZN3ust13volatile_readERKn = comdat any
define linkonce_odr i128 @_ZN3ust13volatile_readERKn( i128* %addr ) unnamed_addr comdat
{
	%1= load volatile i128, i128* %addr
	ret i128 %1
}

; fn volatile_read(u128& addr) :  u128;
$_ZN3ust13volatile_readERKo = comdat any
define linkonce_odr i128 @_ZN3ust13volatile_readERKo( i128* %addr ) unnamed_addr comdat
{
	%1= load volatile i128, i128* %addr
	ret i128 %1
}

; fn volatile_write(  i8 &mut addr,   i8 x) :   i8;
$_ZN3ust14volatile_writeERaa = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERaa(   i8* %addr,   i8 %x ) unnamed_addr comdat
{
	store volatile   i8 %x,   i8* %addr
	ret void
}

; fn volatile_write(  u8 &mut addr,   u8 x) :   u8;
$_ZN3ust14volatile_writeERhh = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERhh(   i8* %addr,   i8 %x ) unnamed_addr comdat
{
	store volatile   i8 %x,   i8* %addr
	ret void
}

; fn volatile_write( i16 &mut addr,  i16 x) :  i16;
$_ZN3ust14volatile_writeERss = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERss(  i16* %addr,  i16 %x ) unnamed_addr comdat
{
	store volatile  i16 %x,  i16* %addr
	ret void
}

; fn volatile_write( u16 &mut addr,  u16 x) :  u16;
$_ZN3ust14volatile_writeERtt = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERtt(  i16* %addr,  i16 %x ) unnamed_addr comdat
{
	store volatile  i16 %x,  i16* %addr
	ret void
}

; fn volatile_write( i32 &mut addr,  i32 x) :  i32;
$_ZN3ust14volatile_writeERii = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERii(  i32* %addr,  i32 %x ) unnamed_addr comdat
{
	store volatile  i32 %x,  i32* %addr
	ret void
}

; fn volatile_write( u32 &mut addr,  u32 x) :  u32;
$_ZN3ust14volatile_writeERjj = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERjj(  i32* %addr,  i32 %x ) unnamed_addr comdat
{
	store volatile  i32 %x,  i32* %addr
	ret void
}

; fn volatile_write( i64 &mut addr,  i64 x) :  i64;
$_ZN3ust14volatile_writeERxx = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERxx(  i64* %addr,  i64 %x ) unnamed_addr comdat
{
	store volatile  i64 %x,  i64* %addr
	ret void
}

; fn volatile_write( u64 &mut addr,  u64 x) :  u64;
$_ZN3ust14volatile_writeERyy = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERyy(  i64* %addr,  i64 %x ) unnamed_addr comdat
{
	store volatile  i64 %x,  i64* %addr
	ret void
}

; fn volatile_write(i128 &mut addr, i128 x) :  i128;
$_ZN3ust14volatile_writeERnn = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERnn( i128* %addr, i128 %x ) unnamed_addr comdat
{
	store volatile i128 %x, i128* %addr
	ret void
}

; fn volatile_write(u128 &mut addr, u128 x) :  u128;
$_ZN3ust14volatile_writeERoo = comdat any
define linkonce_odr void @_ZN3ust14volatile_writeERoo( i128* %addr, i128 %x ) unnamed_addr comdat
{
	store volatile i128 %x, i128* %addr
	ret void
}
