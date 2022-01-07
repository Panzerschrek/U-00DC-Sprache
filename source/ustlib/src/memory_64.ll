%__U_void = type {}

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

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1 immarg) #0

attributes #0 = { argmemonly nounwind }
