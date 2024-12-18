;
; llvm intrinsics
;

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1 immarg) #0

;
; C standard library functions
;

declare i32 @memcmp( i8*, i8*, i64 )

; Impl functions

$ust_memory_copy_impl = comdat any
define linkonce_odr hidden void @ust_memory_copy_impl( i8* %dst, i8* %src, i64 %size ) unnamed_addr comdat
{
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst, i8* %src, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_1_impl = comdat any
define linkonce_odr hidden void @ust_memory_copy_align_1_impl( i8* %dst, i8* %src, i64 %size ) unnamed_addr comdat
{
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 1 %dst, i8* align 1 %src, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_2_impl = comdat any
define linkonce_odr hidden void @ust_memory_copy_align_2_impl( i8* %dst, i8* %src, i64 %size ) unnamed_addr comdat
{
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 2 %dst, i8* align 2 %src, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_4_impl = comdat any
define linkonce_odr hidden void @ust_memory_copy_align_4_impl( i8* %dst, i8* %src, i64 %size ) unnamed_addr comdat
{
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %dst, i8* align 4 %src, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_8_impl = comdat any
define linkonce_odr hidden void @ust_memory_copy_align_8_impl( i8* %dst, i8* %src, i64 %size ) unnamed_addr comdat
{
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %dst, i8* align 8 %src, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_16_impl = comdat any
define linkonce_odr hidden void @ust_memory_copy_align_16_impl( i8* %dst, i8* %src, i64 %size ) unnamed_addr comdat
{
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %dst, i8* align 16 %src, i64 %size, i1 false)
	ret void
}

$ust_memory_compare_impl = comdat any
define linkonce_odr hidden i32 @ust_memory_compare_impl( i8* %a, i8* %b, i64 %size ) unnamed_addr comdat
{
	%res = call i32 @memcmp(i8* align 16 %a, i8* align 16 %b, i64 %size)
	ret i32 %res
}

attributes #0 = { argmemonly nounwind }
