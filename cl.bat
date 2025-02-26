@echo off

set dir_src=%~dp0src
set dir_build=%~dp0run_tree\build\win32

set compiler_flags=-std:c++17 -Od -W4 -Zi -nologo
set linker_flags=-incremental:no -opt:ref -out:scopecorp.exe
set macro_defines=-DDEBUG -DDIR_SHADERS=%~dp0run_tree\data\shaders
REM user32.lib gdi32.lib

If NOT EXIST %dir_build% mkdir %dir_build%

pushd %dir_build%
del *.pdb > NUL 2> NUL
call cl %compiler_flags% %dir_src%\main.cpp %macro_defines% /link %linker_flags%
popd
