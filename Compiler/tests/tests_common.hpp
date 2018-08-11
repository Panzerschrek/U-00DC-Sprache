#pragma once

namespace U
{

namespace Constants
{

// Hack. Use constant target triple and data layout for all tests.
const char tests_target_triple_str[]= "i686-pc-windows-gnu";
const char tests_data_layout_str[]= "e-m:x-p:32:32-i64:64-f80:32-n8:16:32-a:0:32-S32";

} // namespace Constants

} // namespace U
