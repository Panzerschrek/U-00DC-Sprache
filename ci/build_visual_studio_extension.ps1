Set-Location -path source/visual_studio_extension
Start-Process -FilePath "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\msbuild.exe" -ArgumentList "-restore /property:Configuration=Debug"
Start-Process -FilePath "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\msbuild.exe" -ArgumentList "-restore /property:Configuration=Release"
