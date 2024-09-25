@echo off

:: Create the build dir if it dosen't exist.
if not exist build mkdir build

pushd build
cl ..\src\cvm\cvm_main.cpp /ZI /I..\src  /Fe"cvm.exe" 
popd
