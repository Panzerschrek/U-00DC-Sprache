$curDir = Get-Location

Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/llvm-15.0.7.src.tar.xz" -OutFile "llvm-15.0.7.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/cmake-15.0.7.src.tar.xz" -OutFile "cmake-15.0.7.src.tar.xz"
7z x llvm-15.0.7.src.tar.xz
7z x llvm-15.0.7.src.tar
7z x cmake-15.0.7.src.tar.xz
7z x cmake-15.0.7.src.tar

cmd 'C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat' amd64
mkdir build_dir
cmake -S source -B build_dir -DCMAKE_BUILD_TYPE=Release -DCMAKE_MODULE_PATH="$curDir/cmake-15.0.7.src/Modules/" -DLLVM_SRC_DIR="$curDir/llvm-15.0.7.src/" -DLLVM_TARGETS_TO_BUILD="x86" -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF -DU_BUILD_UNICODE_FILE_NAMES_TEST=OFF
cmake --build build_dir
