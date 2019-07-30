; ModuleID = 'Ü-std'

attributes #0 = { noreturn nounwind }

;
; llvm intrinsics
;

declare void @llvm.trap() #0

declare {i32, i1} @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
declare {i32, i1} @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
declare {i64, i1} @llvm.sadd.with.overflow.i64(i64 %a, i64 %b)
declare {i64, i1} @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)

declare {i32, i1} @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
declare {i32, i1} @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
declare {i64, i1} @llvm.ssub.with.overflow.i64(i64 %a, i64 %b)
declare {i64, i1} @llvm.usub.with.overflow.i64(i64 %a, i64 %b)

declare {i32, i1} @llvm.smul.with.overflow.i32(i32 %a, i32 %b)
declare {i32, i1} @llvm.umul.with.overflow.i32(i32 %a, i32 %b)
declare {i64, i1} @llvm.smul.with.overflow.i64(i64 %a, i64 %b)
declare {i64, i1} @llvm.umul.with.overflow.i64(i64 %a, i64 %b)

declare float  @llvm.sqrt.f32(float  %x)
declare double @llvm.sqrt.f64(double %x)
declare float  @llvm.exp.f32(float  %x)
declare double @llvm.exp.f64(double %x)
declare float  @llvm.exp2.f32(float  %x)
declare double @llvm.exp2.f64(double %x)
declare float  @llvm.log.f32(float  %x)
declare double @llvm.log.f64(double %x)
declare float  @llvm.log2.f32(float  %x)
declare double @llvm.log2.f64(double %x)


;
; lang functions
;

;
; halt
;

$_U_halt_handler = comdat any
@_U_halt_handler = global void()* null, comdat

$__U_default_halt_handler = comdat any
define linkonce_odr void @__U_default_halt_handler() unnamed_addr #0 comdat
{
	call void @llvm.trap()
	ret void
}

$__U_halt = comdat any
define linkonce_odr void @__U_halt() unnamed_addr #0 comdat
{
	%1 = load void ()*, void ()** @_U_halt_handler
	call void %1()
	ret void
}

;
; atomic
;

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

;
; common math
;

;fn sqrt( f32 x ) : f32;
$_ZN3ust4sqrtEf = comdat any
define linkonce_odr float @_ZN3ust4sqrtEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.sqrt.f32( float %x )
	ret float %1
}

;fn sqrt( f64 x ) : f64;
$_ZN3ust4sqrtEd = comdat any
define linkonce_odr double @_ZN3ust4sqrtEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.sqrt.f64( double %x )
	ret double %1
}

;fn exp( f32 x ) : f32;
$_ZN3ust3expEf = comdat any
define linkonce_odr float @_ZN3ust3expEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.exp.f32( float %x )
	ret float %1
}

;fn exp( f64 x ) : f64;
$_ZN3ust3expEd = comdat any
define linkonce_odr double @_ZN3ust3expEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.exp.f64( double %x )
	ret double %1
}

;fn exp2( f32 x ) : f32;
$_ZN3ust4exp2Ef = comdat any
define linkonce_odr float @_ZN3ust4exp2Ef( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.exp2.f32( float %x )
	ret float %1
}

;fn exp2( f64 x ) : f64;
$_ZN3ust4exp2Ed = comdat any
define linkonce_odr double @_ZN3ust4exp2Ed( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.exp2.f64( double %x )
	ret double %1
}

;fn log( f32 x ) : f32;
$_ZN3ust3logEf = comdat any
define linkonce_odr float @_ZN3ust3logEf( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log.f32( float %x )
	ret float %1
}

;fn log( f64 x ) : f64;
$_ZN3ust3logEd = comdat any
define linkonce_odr double @_ZN3ust3logEd( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log.f64( double %x )
	ret double %1
}

;fn log2( f32 x ) : f32;
$_ZN3ust4log2Ef = comdat any
define linkonce_odr float @_ZN3ust4log2Ef( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log2.f32( float %x )
	ret float %1
}

;fn log2( f64 x ) : f64;
$_ZN3ust4log2Ed = comdat any
define linkonce_odr double @_ZN3ust4log2Ed( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log2.f64( double %x )
	ret double %1
}

;
; checked math
;

; fn add_overflow_check_halt( i32 a, i32 b ) : i32;
$_ZN3ust23add_overflow_check_haltEii = comdat any
define linkonce_odr i32 @_ZN3ust23add_overflow_check_haltEii( i32 %a, i32 %b ) unnamed_addr comdat
{
	%1 = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)
	%2 = extractvalue {i32, i1} %1, 0
	%3 = extractvalue {i32, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i32 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn add_overflow_check_halt( u32 a, u32 b ) : u32;
$_ZN3ust23add_overflow_check_haltEjj = comdat any
define linkonce_odr i32 @_ZN3ust23add_overflow_check_haltEjj( i32 %a, i32 %b ) unnamed_addr comdat
{
	%1 = call {i32, i1} @llvm.uadd.with.overflow.i32(i32 %a, i32 %b)
	%2 = extractvalue {i32, i1} %1, 0
	%3 = extractvalue {i32, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i32 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn add_overflow_check_halt( i64 a, i64 b ) : i64;
$_ZN3ust23add_overflow_check_haltExx = comdat any
define linkonce_odr i64 @_ZN3ust23add_overflow_check_haltExx( i64 %a, i64 %b ) unnamed_addr comdat
{
	%1 = call {i64, i1} @llvm.sadd.with.overflow.i64(i64 %a, i64 %b)
	%2 = extractvalue {i64, i1} %1, 0
	%3 = extractvalue {i64, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i64 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn add_overflow_check_halt( u64 a, u64 b ) : u64;
$_ZN3ust23add_overflow_check_haltEyy = comdat any
define linkonce_odr i64 @_ZN3ust23add_overflow_check_haltEyy( i64 %a, i64 %b ) unnamed_addr comdat
{
	%1 = call {i64, i1} @llvm.uadd.with.overflow.i64(i64 %a, i64 %b)
	%2 = extractvalue {i64, i1} %1, 0
	%3 = extractvalue {i64, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i64 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn sub_overflow_check_halt( i32 a, i32 b ) : i32;
$_ZN3ust23sub_overflow_check_haltEii = comdat any
define linkonce_odr i32 @_ZN3ust23sub_overflow_check_haltEii( i32 %a, i32 %b ) unnamed_addr comdat
{
	%1 = call {i32, i1} @llvm.ssub.with.overflow.i32(i32 %a, i32 %b)
	%2 = extractvalue {i32, i1} %1, 0
	%3 = extractvalue {i32, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i32 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn sub_overflow_check_halt( u32 a, u32 b ) : u32;
$_ZN3ust23sub_overflow_check_haltEjj = comdat any
define linkonce_odr i32 @_ZN3ust23sub_overflow_check_haltEjj( i32 %a, i32 %b ) unnamed_addr comdat
{
	%1 = call {i32, i1} @llvm.usub.with.overflow.i32(i32 %a, i32 %b)
	%2 = extractvalue {i32, i1} %1, 0
	%3 = extractvalue {i32, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i32 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn sub_overflow_check_halt( i64 a, i64 b ) : i64;
$_ZN3ust23sub_overflow_check_haltExx = comdat any
define linkonce_odr i64 @_ZN3ust23sub_overflow_check_haltExx( i64 %a, i64 %b ) unnamed_addr comdat
{
	%1 = call {i64, i1} @llvm.ssub.with.overflow.i64(i64 %a, i64 %b)
	%2 = extractvalue {i64, i1} %1, 0
	%3 = extractvalue {i64, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i64 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn sub_overflow_check_halt( u64 a, u64 b ) : u64;
$_ZN3ust23sub_overflow_check_haltEyy = comdat any
define linkonce_odr i64 @_ZN3ust23sub_overflow_check_haltEyy( i64 %a, i64 %b ) unnamed_addr comdat
{
	%1 = call {i64, i1} @llvm.usub.with.overflow.i64(i64 %a, i64 %b)
	%2 = extractvalue {i64, i1} %1, 0
	%3 = extractvalue {i64, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i64 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn mul_overflow_check_halt( i32 a, i32 b ) : i32;
$_ZN3ust23mul_overflow_check_haltEii = comdat any
define linkonce_odr i32 @_ZN3ust23mul_overflow_check_haltEii( i32 %a, i32 %b ) unnamed_addr comdat
{
	%1 = call {i32, i1} @llvm.smul.with.overflow.i32(i32 %a, i32 %b)
	%2 = extractvalue {i32, i1} %1, 0
	%3 = extractvalue {i32, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i32 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn mul_overflow_check_halt( u32 a, u32 b ) : u32;
$_ZN3ust23mul_overflow_check_haltEjj = comdat any
define linkonce_odr i32 @_ZN3ust23mul_overflow_check_haltEjj( i32 %a, i32 %b ) unnamed_addr comdat
{
	%1 = call {i32, i1} @llvm.umul.with.overflow.i32(i32 %a, i32 %b)
	%2 = extractvalue {i32, i1} %1, 0
	%3 = extractvalue {i32, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i32 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn mul_overflow_check_halt( i64 a, i64 b ) : i64;
$_ZN3ust23mul_overflow_check_haltExx = comdat any
define linkonce_odr i64 @_ZN3ust23mul_overflow_check_haltExx( i64 %a, i64 %b ) unnamed_addr comdat
{
	%1 = call {i64, i1} @llvm.smul.with.overflow.i64(i64 %a, i64 %b)
	%2 = extractvalue {i64, i1} %1, 0
	%3 = extractvalue {i64, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i64 %2
overflow:
	call void @__U_halt()
	unreachable
}

; fn mul_overflow_check_halt( u64 a, u64 b ) : u64;
$_ZN3ust23mul_overflow_check_haltEyy = comdat any
define linkonce_odr i64 @_ZN3ust23mul_overflow_check_haltEyy( i64 %a, i64 %b ) unnamed_addr comdat
{
	%1 = call {i64, i1} @llvm.umul.with.overflow.i64(i64 %a, i64 %b)
	%2 = extractvalue {i64, i1} %1, 0
	%3 = extractvalue {i64, i1} %1, 1
	br i1 %3, label %overflow, label %normal
normal:
	ret i64 %2
overflow:
	call void @__U_halt()
	unreachable
}
