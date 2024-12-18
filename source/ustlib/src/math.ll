;
; llvm intrinsics
;

declare float  @llvm.sqrt.f32(float  %x)
declare double @llvm.sqrt.f64(double %x)
declare float  @llvm.pow.f32(float  %x, float  %exp)
declare double @llvm.pow.f64(double %x, double %exp)
declare float  @llvm.exp.f32(float  %x)
declare double @llvm.exp.f64(double %x)
declare float  @llvm.exp2.f32(float  %x)
declare double @llvm.exp2.f64(double %x)
declare float  @llvm.log.f32(float  %x)
declare double @llvm.log.f64(double %x)
declare float  @llvm.log2.f32(float  %x)
declare double @llvm.log2.f64(double %x)
declare float  @llvm.log10.f32(float  %x)
declare double @llvm.log10.f64(double %x)
declare float  @llvm.sin.f32(float  %x)
declare double @llvm.sin.f64(double %x)
declare float  @llvm.cos.f32(float  %x)
declare double @llvm.cos.f64(double %x)

declare float  @llvm.floor.f32(float  %x)
declare double @llvm.floor.f64(double %x)
declare float  @llvm.ceil.f32(float  %x)
declare double @llvm.ceil.f64(double %x)
declare float  @llvm.round.f32(float  %x)
declare double @llvm.round.f64(double %x)
declare float  @llvm.trunc.f32(float  %x)
declare double @llvm.trunc.f64(double %x)

declare float  @llvm.fma.f32(float  %a, float  %b, float  %c)
declare double @llvm.fma.f64(double %a, double %b, double %c)

;
; C99 math.h functions
;

declare float  @expm1f(float  %x)
declare double @expm1 (double %x)
declare float  @log1pf(float  %x)
declare double @log1p (double %x)
declare float  @tanf(float  %x)
declare double @tan (double %x)
declare float  @asinf(float  %x)
declare double @asin (double %x)
declare float  @acosf(float  %x)
declare double @acos (double %x)
declare float  @atanf(float  %x)
declare double @atan (double %x)
declare float  @atan2f(float  %y, float  %x)
declare double @atan2 (double %y, double %x)

;
; common math
;

$ust_sqrt_f32_impl = comdat any
define linkonce_odr hidden float @ust_sqrt_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.sqrt.f32( float %x )
	ret float %1
}

$ust_sqrt_f64_impl = comdat any
define linkonce_odr hidden double @ust_sqrt_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.sqrt.f64( double %x )
	ret double %1
}

$ust_pow_f32_impl = comdat any
define linkonce_odr hidden float @ust_pow_f32_impl( float %x, float %exp ) unnamed_addr comdat
{
	%1= call float @llvm.pow.f32( float %x, float %exp )
	ret float %1
}

$ust_pow_f64_impl = comdat any
define linkonce_odr hidden double @ust_pow_f64_impl( double %x, double %exp ) unnamed_addr comdat
{
	%1= call double @llvm.pow.f64( double %x, double %exp )
	ret double %1
}

$ust_exp_f32_impl = comdat any
define linkonce_odr hidden float @ust_exp_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.exp.f32( float %x )
	ret float %1
}

$ust_exp_f64_impl = comdat any
define linkonce_odr hidden double @ust_exp_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.exp.f64( double %x )
	ret double %1
}

$ust_exp2_f32_impl = comdat any
define linkonce_odr hidden float @ust_exp2_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.exp2.f32( float %x )
	ret float %1
}

$ust_exp2_f64_impl = comdat any
define linkonce_odr hidden double @ust_exp2_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.exp2.f64( double %x )
	ret double %1
}

$ust_expm1_f32_impl = comdat any
define linkonce_odr hidden float @ust_expm1_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @expm1f( float %x )
	ret float %1
}

$ust_expm1_f64_impl = comdat any
define linkonce_odr hidden double @ust_expm1_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @expm1 ( double %x )
	ret double %1
}

$ust_log_f32_impl = comdat any
define linkonce_odr hidden float @ust_log_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log.f32( float %x )
	ret float %1
}

$ust_log_f64_impl = comdat any
define linkonce_odr hidden double @ust_log_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log.f64( double %x )
	ret double %1
}

$ust_log2_f32_impl = comdat any
define linkonce_odr hidden float @ust_log2_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log2.f32( float %x )
	ret float %1
}

$ust_log2_f64_impl = comdat any
define linkonce_odr hidden double @ust_log2_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log2.f64( double %x )
	ret double %1
}

$ust_log10_f32_impl = comdat any
define linkonce_odr hidden float @ust_log10_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.log10.f32( float %x )
	ret float %1
}

$ust_log10_f64_impl = comdat any
define linkonce_odr hidden double @ust_log10_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.log10.f64( double %x )
	ret double %1
}

$ust_log1p_f32_impl = comdat any
define linkonce_odr hidden float @ust_log1p_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @log1pf( float %x )
	ret float %1
}

$ust_log1p_f64_impl = comdat any
define linkonce_odr hidden double @ust_log1p_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @log1p( double %x )
	ret double %1
}

$ust_sin_f32_impl = comdat any
define linkonce_odr hidden float @ust_sin_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.sin.f32( float %x )
	ret float %1
}

$ust_sin_f64_impl = comdat any
define linkonce_odr hidden double @ust_sin_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.sin.f64( double %x )
	ret double %1
}

$ust_cos_f32_impl = comdat any
define linkonce_odr hidden float @ust_cos_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.cos.f32( float %x )
	ret float %1
}

$ust_cos_f64_impl = comdat any
define linkonce_odr hidden double @ust_cos_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.cos.f64( double %x )
	ret double %1
}

$ust_tan_f32_impl = comdat any
define linkonce_odr hidden float @ust_tan_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @tanf( float %x )
	ret float %1
}

$ust_tan_f64_impl = comdat any
define linkonce_odr hidden double @ust_tan_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @tan( double %x )
	ret double %1
}

$ust_asin_f32_impl = comdat any
define linkonce_odr hidden float @ust_asin_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @asinf( float %x )
	ret float %1
}

$ust_asin_f64_impl = comdat any
define linkonce_odr hidden double @ust_asin_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @asin( double %x )
	ret double %1
}

$ust_acos_f32_impl = comdat any
define linkonce_odr hidden float @ust_acos_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @acosf( float %x )
	ret float %1
}

$ust_acos_f64_impl = comdat any
define linkonce_odr hidden double @ust_acos_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @acos( double %x )
	ret double %1
}

$ust_atan_f32_impl = comdat any
define linkonce_odr hidden float @ust_atan_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @atanf( float %x )
	ret float %1
}

$ust_atan_f64_impl = comdat any
define linkonce_odr hidden double @ust_atan_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @atan( double %x )
	ret double %1
}

$ust_atan2_f32_impl = comdat any
define linkonce_odr hidden float @ust_atan2_f32_impl( float %y, float %x ) unnamed_addr comdat
{
	%1= call float @atan2f( float %y, float %x )
	ret float %1
}

$ust_atan2_f64_impl = comdat any
define linkonce_odr hidden double @ust_atan2_f64_impl( double %y, double %x ) unnamed_addr comdat
{
	%1= call double @atan2( double %y, double %x )
	ret double %1
}

$ust_floor_f32_impl = comdat any
define linkonce_odr hidden float @ust_floor_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.floor.f32( float %x )
	ret float %1
}

$ust_floor_f64_impl = comdat any
define linkonce_odr hidden double @ust_floor_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.floor.f64( double %x )
	ret double %1
}

$ust_ceil_f32_impl = comdat any
define linkonce_odr hidden float @ust_ceil_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.ceil.f32( float %x )
	ret float %1
}

$ust_ceil_f64_impl = comdat any
define linkonce_odr hidden double @ust_ceil_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.ceil.f64( double %x )
	ret double %1
}

$ust_round_f32_impl = comdat any
define linkonce_odr hidden float @ust_round_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.round.f32( float %x )
	ret float %1
}

$ust_round_f64_impl = comdat any
define linkonce_odr hidden double @ust_round_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.round.f64( double %x )
	ret double %1
}

$ust_trunc_f32_impl = comdat any
define linkonce_odr hidden float @ust_trunc_f32_impl( float %x ) unnamed_addr comdat
{
	%1= call float @llvm.trunc.f32( float %x )
	ret float %1
}

$ust_trunc_f64_impl = comdat any
define linkonce_odr hidden double @ust_trunc_f64_impl( double %x ) unnamed_addr comdat
{
	%1= call double @llvm.trunc.f64( double %x )
	ret double %1
}

$ust_fma_f32_impl = comdat any
define linkonce_odr hidden float @ust_fma_f32_impl( float %x, float %y, float %z ) unnamed_addr comdat
{
	%1= call float @llvm.fma.f32( float %x, float %y, float %z )
	ret float %1
}

$ust_fma_f64_impl = comdat any
define linkonce_odr hidden double @ust_fma_f64_impl( double %x, double %y, double %z ) unnamed_addr comdat
{
	%1= call double @llvm.fma.f64( double %x, double %y, double %z )
	ret double %1
}
