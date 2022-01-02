declare void @__U_halt()

;
; llvm intrinsics
;

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
