@echo off
setlocal

set "usage=Usage: build.bat -^[debug^|release^]"

if [%1] == [] (
   echo %usage%
   exit 1
)

if [%1] == [-debug] (
  set "config=Debug"
) else if [%1] == [-release] (
  set "config=Release"
) else (
  echo %usage%
  exit 1
)

pushd %~dp0run_tree\cmake
cmake -E time cmake --build . --config %config%
popd

endlocal
