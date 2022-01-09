%__U_void = type {}

;
; llvm intrinsics
;

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1 immarg) #0

;
; C standard library functions
;

declare i8* @malloc( i64 )
declare i8* @realloc( i8*, i64 )
declare void @free( i8* )
declare i32 @memcmp( i8*, i8*, i64 )

; Impl functions

$ust_memory_allocate_impl = comdat any
define linkonce_odr %__U_void* @ust_memory_allocate_impl( i64 %size ) unnamed_addr comdat
{
	%res = call i8* @malloc( i64 %size )
	%res_casted = bitcast i8* %res to %"__U_void"*
	ret %"__U_void"* %res_casted
}

$ust_memory_reallocate_impl = comdat any
define linkonce_odr %__U_void* @ust_memory_reallocate_impl( %"__U_void"* %ptr, i64 %size ) unnamed_addr comdat
{
	%ptr_casted = bitcast %"__U_void"* %ptr to i8*
	%res = call i8* @realloc( i8* %ptr_casted, i64 %size )
	%res_casted = bitcast i8* %res to %"__U_void"*
	ret %"__U_void"* %res_casted
}

$ust_memory_free_impl = comdat any
define linkonce_odr void @ust_memory_free_impl( %"__U_void"* %ptr) unnamed_addr comdat
{
	%ptr_casted = bitcast %"__U_void"* %ptr to i8*
	call void @free( i8* %ptr_casted )
	ret void
}

$ust_memory_copy_impl = comdat any
define linkonce_odr void @ust_memory_copy_impl( %__U_void* %dst, %__U_void* %src, i64 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* %dst_casted, i8* %src_casted, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_1_impl = comdat any
define linkonce_odr void @ust_memory_copy_align_1_impl( %__U_void* %dst, %__U_void* %src, i64 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 1 %dst_casted, i8* align 1 %src_casted, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_2_impl = comdat any
define linkonce_odr void @ust_memory_copy_align_2_impl( %__U_void* %dst, %__U_void* %src, i64 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 2 %dst_casted, i8* align 2 %src_casted, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_4_impl = comdat any
define linkonce_odr void @ust_memory_copy_align_4_impl( %__U_void* %dst, %__U_void* %src, i64 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %dst_casted, i8* align 4 %src_casted, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_8_impl = comdat any
define linkonce_odr void @ust_memory_copy_align_8_impl( %__U_void* %dst, %__U_void* %src, i64 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %dst_casted, i8* align 8 %src_casted, i64 %size, i1 false)
	ret void
}

$ust_memory_copy_align_16_impl = comdat any
define linkonce_odr void @ust_memory_copy_align_16_impl( %__U_void* %dst, %__U_void* %src, i64 %size ) unnamed_addr comdat
{
	%dst_casted = bitcast %"__U_void"* %dst to i8*
	%src_casted = bitcast %"__U_void"* %src to i8*
	call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %dst_casted, i8* align 16 %src_casted, i64 %size, i1 false)
	ret void
}

$ust_memory_compare_impl = comdat any
define linkonce_odr i32 @ust_memory_compare_impl( %__U_void* %a, %__U_void* %b, i64 %size ) unnamed_addr comdat
{
	%a_casted = bitcast %"__U_void"* %a to i8*
	%b_casted = bitcast %"__U_void"* %b to i8*
	%res = call i32 @memcmp(i8* align 16 %a_casted, i8* align 16 %b_casted, i64 %size)
	ret i32 %res
}

attributes #0 = { argmemonly nounwind }
