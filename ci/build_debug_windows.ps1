# Obtain current directoy path and replace stupid backslashes with forward-slashes
$curDir = Get-Location
$curDir = $curDir -replace '\\', '/'

# Obtain LLVM
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/llvm-15.0.7.src.tar.xz" -OutFile "llvm-15.0.7.src.tar.xz"
Invoke-WebRequest -Uri "https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/cmake-15.0.7.src.tar.xz" -OutFile "cmake-15.0.7.src.tar.xz"
7z x llvm-15.0.7.src.tar.xz
7z x llvm-15.0.7.src.tar
7z x cmake-15.0.7.src.tar.xz
7z x cmake-15.0.7.src.tar

# Delete archives
del llvm-15.0.7.src.tar.xz
del llvm-15.0.7.src.tar
del cmake-15.0.7.src.tar.xz
del cmake-15.0.7.src.tar

# Messy stuff. Call vcvarsall.bat, extract all environment variable prepared in this call and redefine them in context of this powershell script.
cmd /c "call `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat`" amd64 && set > %temp%\vcvars.txt"
Get-Content "$env:temp\vcvars.txt" | Foreach-Object {
	if ($_ -match "^(.*?)=(.*)$") {
		Set-Content "env:\$($matches[1])" $matches[2]
	}
}

# Perform cmake preparation
mkdir build_dir
cmake -S source -B build_dir  -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MODULE_PATH="$curDir/cmake-15.0.7.src/Modules/" -DLLVM_SRC_DIR="$curDir/llvm-15.0.7.src/" -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_NATIVE_ARCH="X86" -DLLVM_BUILD_BENCHMARKS=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_BUILD_DOCS=OFF -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF -DU_BUILD_COMPILER2=ON -DU_BUILD_COMPILER3=OFF -DU_BUILD_PY_TESTS=OFF -DU_BUILD_UNICODE_FILE_NAMES_TEST=OFF

# Since there is not enought space on Github hosted action runners, run builds one by one and than delete some huge files

cmake --build build_dir --target Tests
del build_dir/compiler0/Tests.exe
del build_dir/compiler0/Tests.pdb
del build_dir/compiler0/Tests.ilk

cmake --build build_dir --target LanguageServerTests
del build_dir/language_server/LanguageServerTests.exe
del build_dir/language_server/LanguageServerTests.pdb
del build_dir/language_server/LanguageServerTests.ilk

cmake --build build_dir --target LanguageServer
del build_dir/language_server/LanguageServer.exe
del build_dir/language_server/LanguageServer.pdb
del build_dir/language_server/LanguageServer.ilk
del build_dir/language_server/LanguageServerLib.lib

cmake --build build_dir --target Interpreter
del build_dir/interpreter/Interpreter.exe
del build_dir/interpreter/Interpreter.pdb
del build_dir/interpreter/Interpreter.ilk

cmake --build build_dir --target Tests1
del build_dir/compiler1/Tests1.exe
del build_dir/compiler1/Tests1.pdb
del build_dir/compiler1/Tests1.ilk

cmake --build build_dir --target Tests2
del build_dir/compiler2/Tests2.exe
del build_dir/compiler2/Tests2.pdb
del build_dir/compiler2/Tests2.ilk
