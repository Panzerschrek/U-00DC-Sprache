#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"


namespace U
{

/*

Run force-inlining for async functions calls via "await".
This function should be run before any other optimization and coroutine pases,
since it relies on code structure produced by the compiler.

This optimization exists as direct follow-up for frontend compiler generation,
because LLVM optimization/coroutine passes can't deal properly with nested async calls.

Typical usage of async functions consists of calling of other async functions via "await".
Internally this "await" is just a loop with "resume" for given async function object and "yield".
LLVM can inline this calls and replaces heap allocations with stack allocations.

However it can't properly optimize control flow instructions.
For each called async function a state counter is created, later switch instruction is used for control flow dispatching.
For deep call chains stacks ths leads to dozens of such switches.
This is a significant overhead both for code size and runtime performance.

But really such chains of switches are useless.
Any such chain may be replaced with one giant switch, which requires less code - only big switch table.
So, the optimization implemented by the function below leads to creation of such giant switch.

This function force-inlines async calls via "await".
Coroutine object creation in an inlined function is eliminated entirely.
"yield" of an inlined function becomes "yield" in a target function.
"return" is replaced with control flow pass to a target function.

This force inlining not only removes unnecessary switches, but also removes leftover resume/destroy load/stores for each async call frame
and leads to significant overall coroutine memory size reduction.

There are of corse some disadvantages.
Inlining may increase code size if async functions contain a lot of code that does something non-trivial rather than calling other async and non-async functons.
Inlining also can't properly work for async call graphs with cycles, inlining still works, but it is suboptimal.

But it seems like benefints are greater than disadvantages.
Especially it works great for cases,
where an async function contains just single "await" call to another async function with some args preparation and result processing.
Another common case - with several sequential "await" calls, also works good.

The optimization works (obviously) only if a body for an async function is known.
External functions can't be inlined.
However functions of this module with external linkage may be inlined into another functions of this module.

*/
void InlineAsyncCalls( llvm::Module& module );

} // namespace U
