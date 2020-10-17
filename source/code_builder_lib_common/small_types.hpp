#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/ADT/SmallVector.h>
#include "pop_llvm_warnings.hpp"

namespace U
{

template<class T>
using ArgsVector = llvm::SmallVector<T, 4u>;

template<class T>
using ClassFieldsVector = llvm::SmallVector<T, 8u>;

} // namespace U
