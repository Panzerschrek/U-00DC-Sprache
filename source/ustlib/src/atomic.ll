; Use strictest memory order - "seq_cst" for all atomic operations.

; fn atomic_read( i32& addr ) : i32;
$_ZN3ust11atomic_readERKi = comdat any
define linkonce_odr i32 @_ZN3ust11atomic_readERKi( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

; fn atomic_read( u32& addr ) : i32;
$_ZN3ust11atomic_readERKj = comdat any
define linkonce_odr i32 @_ZN3ust11atomic_readERKj( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic volatile i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

; fn atomic_write( i32 &mut addr, i32 x );
$_ZN3ust12atomic_writeERii = comdat any
define linkonce_odr void @_ZN3ust12atomic_writeERii( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic volatile i32 %x, i32* %addr seq_cst, align 4
	ret void
}

; fn atomic_write( u32 &mut addr, u32 x );
$_ZN3ust12atomic_writeERjj = comdat any
define linkonce_odr void @_ZN3ust12atomic_writeERjj( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic volatile i32 %x, i32* %addr seq_cst, align 4
	ret void
}

; fn atomic_add( i32 &mut x, i32 y ) : i32;
$_ZN3ust10atomic_addERii = comdat any
define linkonce_odr i32 @_ZN3ust10atomic_addERii( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw volatile add i32* %x, i32 %y seq_cst
	ret i32 %1
}

; fn atomic_add( u32 &mut x, u32 y ) : i32;
$_ZN3ust10atomic_addERjj = comdat any
define linkonce_odr i32 @_ZN3ust10atomic_addERjj( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw volatile add i32* %x, i32 %y seq_cst
	ret i32 %1
}

; fn atomic_compare_exchange_strong( i32 &mut dst, i32 &mut expected, i32 new ) : bool;
$_ZN3ust30atomic_compare_exchange_strongERiS0_i = comdat any
define linkonce_odr i1 @_ZN3ust30atomic_compare_exchange_strongERiS0_i( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg volatile i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

; fn atomic_compare_exchange_strong( u32 &mut dst, u32 &mut expected, u32 new ) : bool;
$_ZN3ust30atomic_compare_exchange_strongERjS0_j = comdat any
define linkonce_odr i1 @_ZN3ust30atomic_compare_exchange_strongERjS0_j( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg volatile i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

; fn atomic_compare_exchange_weak( i32 &mut dst, i32 &mut expected, i32 new ) : bool;
$_ZN3ust28atomic_compare_exchange_weakERiS0_i = comdat any
define linkonce_odr i1 @_ZN3ust28atomic_compare_exchange_weakERiS0_i( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg weak volatile i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}

; fn atomic_compare_exchange_weak( u32 &mut dst, u32 &mut expected, u32 new ) : bool;
$_ZN3ust28atomic_compare_exchange_weakERjS0_j = comdat any
define linkonce_odr i1 @_ZN3ust28atomic_compare_exchange_weakERjS0_j( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
{
	%expected_read= load i32, i32* %expected
	%res= cmpxchg weak volatile i32* %addr, i32 %expected_read, i32 %new seq_cst monotonic
	%success = extractvalue { i32, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i32, i1 } %res, 0
	store i32 %val, i32* %expected
	ret i1 false
}
