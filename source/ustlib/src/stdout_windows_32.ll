;
; C stdlib functions
;

declare i32 @fwrite( i8*, i32, i32, i8* )

declare i8* @__acrt_iob_func( i32 )

; Impl functions

; Use fwrite because native windows GetStdHandle/WriteFile doesn't work properly in program with existing C and/or C++ stdin/stdout streams.

; STDIN - 0
; STDOUT - 1
; STDERR - 2

$ust_stdout_print_impl = comdat any
define linkonce_odr void @ust_stdout_print_impl( i8* %start, i32 %size ) unnamed_addr comdat
{
	%stdout_handle = call i8* @__acrt_iob_func( i32 1 )
	%write_res= call i32 @fwrite( i8* %start, i32 1, i32 %size, i8* %stdout_handle )
	; TODO - halt if fwrite fails
	ret void
}

$ust_stderr_print_impl = comdat any
define linkonce_odr void @ust_stderr_print_impl( i8* %start, i32 %size ) unnamed_addr comdat
{
	%stderr_handle = call i8* @__acrt_iob_func( i32 2 )
	%write_res= call i32 @fwrite( i8* %start, i32 1, i32 %size, i8* %stderr_handle )
	; TODO - halt if fwrite fails
	ret void
}
