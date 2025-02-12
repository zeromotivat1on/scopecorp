@echo off
setlocal

set "usage_message=Usage: run_tree_build.bat ^[debug^|release^] ^[run^]"
set "build_config=%1"

if /I "%build_config%"=="debug" (
    cmake --build .\run_tree\cmake\ --config Debug
) else if /I "%build_config%"=="release" (
    cmake --build .\run_tree\cmake\ --config Release
) else (
    echo %usage_message%
    exit /b 1
)

if errorlevel 1 (
   exit /b 1
)

if "%2"=="run" (
   .\run_tree\build\windows\%build_config%\scopecorp.exe
)
    
endlocal
