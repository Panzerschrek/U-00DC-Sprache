Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/llvm-15.0.7.src.tar.xz" -OutFile "llvm-15.0.7.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/cmake-15.0.7.src.tar.xz" -OutFile "cmake-15.0.7.src.tar.xz"
7z x llvm-15.0.7.src.tar.xz
7z x llvm-15.0.7.src.tar
7z x cmake-15.0.7.src.tar.xz
7z x cmake-15.0.7.src.tar
