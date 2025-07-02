; Use strictest memory order - "seq_cst" for all atomic operations.

$ust_atomic_read_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_i64_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_read_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_u64_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_read_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_ssize_type_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_read_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_size_type_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_read_f64_impl = comdat any
define linkonce_odr hidden double @ust_atomic_read_f64_impl( double* %addr ) unnamed_addr comdat
{
	%1= load atomic double, double* %addr seq_cst, align 8
	ret double %1
}

$ust_atomic_read_byte64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_read_byte64_impl( i64* %addr ) unnamed_addr comdat
{
	%1= load atomic i64, i64* %addr seq_cst, align 8
	ret i64 %1
}

$ust_atomic_write_i64_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_i64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_write_u64_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_u64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_write_ssize_type_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_ssize_type_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_write_size_type_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_size_type_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_write_f64_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_f64_impl( double* %addr, double %x ) unnamed_addr comdat
{
	store atomic double %x, double* %addr seq_cst, align 8
	ret void
}

$ust_atomic_write_byte64_impl = comdat any
define linkonce_odr hidden void @ust_atomic_write_byte64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	store atomic i64 %x, i64* %addr seq_cst, align 8
	ret void
}

$ust_atomic_swap_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_swap_i64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i64* %addr, i64 %x seq_cst
	ret i64 %1
}

$ust_atomic_swap_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_swap_u64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i64* %addr, i64 %x seq_cst
	ret i64 %1
}

$ust_atomic_swap_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_swap_ssize_type_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i64* %addr, i64 %x seq_cst
	ret i64 %1
}

$ust_atomic_swap_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_swap_size_type_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i64* %addr, i64 %x seq_cst
	ret i64 %1
}

$ust_atomic_swap_f64_impl = comdat any
define linkonce_odr hidden double @ust_atomic_swap_f64_impl( double* %addr, double %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg float* %addr, double %x seq_cst
	ret double %1
}

$ust_atomic_swap_byte64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_swap_byte64_impl( i64* %addr, i64 %x ) unnamed_addr comdat
{
	%1= atomicrmw xchg i64* %addr, i64 %x seq_cst
	ret i64 %1
}

$ust_atomic_add_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_add_i64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw add i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_add_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_add_u64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw add i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_add_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_add_ssize_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw add i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_add_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_add_size_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw add i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_and_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_and_i64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw and i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_and_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_and_u64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw and i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_and_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_and_ssize_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw and i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_and_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_and_size_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw and i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_or_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_or_i64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw or i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_or_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_or_u64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw or i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_or_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_or_ssize_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw or i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_or_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_or_size_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw or i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_xor_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_xor_i64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw xor i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_xor_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_xor_u64_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw xor i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_xor_ssize_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_xor_ssize_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw xor i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_xor_size_type_impl = comdat any
define linkonce_odr hidden i64 @ust_atomic_xor_size_type_impl( i64* %x, i64 %y ) unnamed_addr comdat
{
	%1= atomicrmw xor i64* %x, i64 %y seq_cst
	ret i64 %1
}

$ust_atomic_compare_exchange_strong_i64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_i64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_strong_u64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_u64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_strong_byte64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_byte64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_i64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_i64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg weak i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_weak_u64_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_weak_u64_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg weak i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
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
	%res= cmpxchg weak i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}

$ust_atomic_compare_exchange_strong_ssize_type_impl = comdat any
define linkonce_odr hidden i1 @ust_atomic_compare_exchange_strong_ssize_type_impl( i64* %addr, i64* %expected, i64 %new ) unnamed_addr comdat
{
	%expected_read= load i64, i64* %expected
	%res= cmpxchg i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
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
	%res= cmpxchg i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
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
	%res= cmpxchg weak i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
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
	%res= cmpxchg weak i64* %addr, i64 %expected_read, i64 %new seq_cst seq_cst
	%success = extractvalue { i64, i1 } %res, 1
	br i1 %success, label %ok, label %not_ok
ok:
	ret i1 true
not_ok:
	%val = extractvalue { i64, i1 } %res, 0
	store i64 %val, i64* %expected
	ret i1 false
}
