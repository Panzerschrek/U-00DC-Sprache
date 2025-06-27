; Use strictest memory order - "seq_cst" for all atomic operations.

$ust_atomic_read_ssize_type_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_read_ssize_type_impl( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic volatile i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

$ust_atomic_read_size_type_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_read_size_type_impl( i32* %addr ) unnamed_addr comdat
{
	%1= load atomic volatile i32, i32* %addr seq_cst, align 4
	ret i32 %1
}

$ust_atomic_write_ssize_type_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_ssize_type_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic volatile i32 %x, i32* %addr seq_cst, align 4
	ret void
}

$ust_atomic_write_size_type_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_size_type_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	store atomic volatile i32 %x, i32* %addr seq_cst, align 4
	ret void
}

$ust_atomic_swap_ssize_type_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_swap_ssize_type_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	%1= atomicrmw volatile xchg i32* %addr, i32 %x seq_cst
	ret i32 %1
}

$ust_atomic_swap_size_type_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_swap_size_type_impl( i32* %addr, i32 %x ) unnamed_addr comdat
{
	%1= atomicrmw volatile xchg i32* %addr, i32 %x seq_cst
	ret i32 %1
}

$ust_atomic_add_ssize_type_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_add_ssize_type_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw volatile add i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_add_size_type_impl = comdat any
define linkonce_odr hidden i32 @ust_atomic_add_size_type_impl( i32* %x, i32 %y ) unnamed_addr comdat
{
	%1= atomicrmw volatile add i32* %x, i32 %y seq_cst
	ret i32 %1
}

$ust_atomic_compare_exchange_strong_ssize_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_ssize_type_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
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

$ust_atomic_compare_exchange_strong_size_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_size_type_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
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

$ust_atomic_compare_exchange_weak_ssize_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_ssize_type_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
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

$ust_atomic_compare_exchange_weak_size_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_size_type_impl( i32* %addr, i32* %expected, i32 %new ) unnamed_addr comdat
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
