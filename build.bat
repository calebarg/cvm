@echo off

:: Create the build dir if it dosen't exist.
if not exist build mkdir build

pushd build
cl ..\src\selvm\selvm_main.cpp /ZI /I..\src  /Fe"selvm.exe" 
popd
