@echo off
setlocal

set "usage=Usage: build.bat -^[debug^|release^]"

if [%1] == [] (
   echo %usage%
   exit 1
)

pushd %~dp0run_tree\cmake

if [%1] == [-debug] (
  cmake --build . --config Debug
) else if [%1] == [-release] (
  cmake --build . --config Release
) else (
    echo %usage%
)

popd

endlocal
