; Use strictest memory order - "seq_cst" for all atomic operations.

$ust_atomic_read_byte64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_byte64_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic volatile i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_write_byte64_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_byte64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic volatile i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_compare_exchange_strong_byte64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_byte64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg volatile i64* %addr, i64 %expected_read, i64 %new seq_cst monotonic
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_byte64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_byte64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg weak volatile i64* %addr, i64 %expected_read, i64 %new seq_cst monotonic
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_read_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_ssize_type_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic volatile i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_read_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_size_type_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic volatile i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_write_ssize_type_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_ssize_type_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic volatile i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_write_size_type_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_size_type_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic volatile i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_add_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_add_ssize_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw volatile add i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_add_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_add_size_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw volatile add i64* %x, i64 %y seq_cst
	ret i64 %1
}


$ust_atomic_compare_exchange_strong_ssize_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_ssize_type_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg volatile i64* %addr, i64 %expected_read, i64 %new seq_cst monotonic
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_strong_size_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_size_type_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg volatile i64* %addr, i64 %expected_read, i64 %new seq_cst monotonic
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_ssize_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_ssize_type_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg weak volatile i64* %addr, i64 %expected_read, i64 %new seq_cst monotonic
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_size_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_size_type_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg weak volatile i64* %addr, i64 %expected_read, i64 %new seq_cst monotonic
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}
