#pragma once

namespace U
{

namespace Constants
{

// Hack. Use constant target triple and data layout for all tests.
const bool is_32bit= sizeof(void*) == 4u;
const char* const tests_target_triple_str= is_32bit ? "i686-pc-windows-gnu" : "x86_64-unknown-linux-gnu";
const char* const tests_data_layout_str= is_32bit ? "e-m:x-p:32:32-i64:64-f80:32-n8:16:32-a:0:32-S32" : "e-m:e-i64:64-f80:128-n8:16:32:64-S128";

} // namespace Constants

} // namespace U
