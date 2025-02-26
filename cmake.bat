@echo off
setlocal

set "cmake_dir=%~dp0run_tree/cmake"
if not exist "%cmake_dir%" (
    mkdir "%cmake_dir%"
)

pushd "%cmake_dir%"
cmake ../..
popd

endlocal
