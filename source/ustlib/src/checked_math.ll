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

$ust_add_overflow_check_halt_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_add_overflow_check_halt_i32_impl( i32 %a, i32 %b ) unnamed_addr comdat
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

$ust_add_overflow_check_halt_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_add_overflow_check_halt_u32_impl( i32 %a, i32 %b ) unnamed_addr comdat
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

$ust_add_overflow_check_halt_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_add_overflow_check_halt_i64_impl( i64 %a, i64 %b ) unnamed_addr comdat
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

$ust_add_overflow_check_halt_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_add_overflow_check_halt_u64_impl( i64 %a, i64 %b ) unnamed_addr comdat
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

$ust_sub_overflow_check_halt_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_sub_overflow_check_halt_i32_impl( i32 %a, i32 %b ) unnamed_addr comdat
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

$ust_sub_overflow_check_halt_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_sub_overflow_check_halt_u32_impl( i32 %a, i32 %b ) unnamed_addr comdat
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

$ust_sub_overflow_check_halt_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_sub_overflow_check_halt_i64_impl( i64 %a, i64 %b ) unnamed_addr comdat
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

$ust_sub_overflow_check_halt_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_sub_overflow_check_halt_u64_impl( i64 %a, i64 %b ) unnamed_addr comdat
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

$ust_mul_overflow_check_halt_i32_impl = comdat any
define linkonce_odr hidden i32 @ust_mul_overflow_check_halt_i32_impl( i32 %a, i32 %b ) unnamed_addr comdat
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

$ust_mul_overflow_check_halt_u32_impl = comdat any
define linkonce_odr hidden i32 @ust_mul_overflow_check_halt_u32_impl( i32 %a, i32 %b ) unnamed_addr comdat
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

$ust_mul_overflow_check_halt_i64_impl = comdat any
define linkonce_odr hidden i64 @ust_mul_overflow_check_halt_i64_impl( i64 %a, i64 %b ) unnamed_addr comdat
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

$ust_mul_overflow_check_halt_u64_impl = comdat any
define linkonce_odr hidden i64 @ust_mul_overflow_check_halt_u64_impl( i64 %a, i64 %b ) unnamed_addr comdat
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
