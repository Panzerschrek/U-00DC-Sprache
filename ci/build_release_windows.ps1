# Obtain current directoy path and replace stupid backslashes with forward-slashes
$curDir = Get-Location
$curDir = $curDir -replace '\\', '/'

# Obtain LLVM
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/llvm-17.0.6.src.tar.xz" -OutFile "llvm-17.0.6.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/cmake-17.0.6.src.tar.xz" -OutFile "cmake-17.0.6.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/clang-17.0.6.src.tar.xz" -OutFile "clang-17.0.6.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/lld-17.0.6.src.tar.xz" -OutFile "lld-17.0.6.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.6/libunwind-17.0.6.src.tar.xz" -OutFile "libunwind-17.0.6.src.tar.xz"
7z x llvm-17.0.6.src.tar.xz
7z x llvm-17.0.6.src.tar
7z x cmake-17.0.6.src.tar.xz
7z x cmake-17.0.6.src.tar
7z x clang-17.0.6.src.tar.xz
7z x clang-17.0.6.src.tar
7z x lld-17.0.6.src.tar.xz
7z x lld-17.0.6.src.tar
7z x libunwind-17.0.6.src.tar.xz
7z x libunwind-17.0.6.src.tar

# LLVM includes its cmake modules using path like "../cmake/Modules/CMakePolicy.cmake". So, reaname this directory.
Rename-Item -path "cmake-17.0.6.src" -NewName "cmake"

# lld Mach-O library includes libunwind using path relative to llvm src directory and without a version. So, rename it.
Rename-Item -path "libunwind-17.0.6.src" -NewName "libunwind"

# Messy stuff. Call vcvarsall.bat, extract all environment variable prepared in this call and redefine them in context of this powershell script.
cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat`" amd64 && set > %temp%\vcvars.txt"
Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
	if ($_ -match "^(.*?)=(.*)$") {
		Set-Content "env:\$($matches[1])" $matches[2]
	}
}

# Perform cmake preparation and build
mkdir build_dir
cmake -S source -B build_dir  -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_MODULE_PATH="$curDir/cmake/Modules/" -DLLVM_SRC_DIR="$curDir/llvm-17.0.6.src/" -DLLVM_EXTERNAL_CLANG_SOURCE_DIR="$curDir/clang-17.0.6.src/" -DLLVM_EXTERNAL_LLD_SOURCE_DIR="$curDir/lld-17.0.6.src/" -DLLVM_TOOL_LLD_BUILD=ON -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_NATIVE_ARCH="X86" -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_INCLUDE_TESTS=OFF -DLLVM_BUILD_TESTS=OFF -DU_BUILD_COMPILER2=ON -DU_BUILD_COMPILER3=ON -DU_BUILD_CPP_HEADER_CONVERTER=ON -DU_BUILD_PY_TESTS=OFF -DU_BUILD_UNICODE_FILE_NAMES_TEST=OFF
cmake --build build_dir

# Run ustlib tests
python source/annotated_tests_run.py --compiler-executable build_dir/compiler0/Compiler  --add-library=build_dir/ustlib0/ustlib.lib  --input-dir source/ustlib/tests
python source/annotated_tests_run.py --compiler-executable build_dir/compiler1/Compiler1 --add-library=build_dir/ustlib1/ustlib1.lib --input-dir source/ustlib/tests
python source/annotated_tests_run.py --compiler-executable build_dir/compiler2/Compiler2 --add-library=build_dir/ustlib2/ustlib2.lib --input-dir source/ustlib/tests
python source/annotated_tests_run.py --compiler-executable build_dir/compiler3/Compiler3 --add-library=build_dir/ustlib3/ustlib3.lib --input-dir source/ustlib/tests

# install
mkdir install
cmake --install build_dir --prefix install
