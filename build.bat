@echo off

set RELEASE=false
set BUILD_TOOLS=true
set BAKE_FONTS=true

:: Possible graphics apis: OPEN_GL, DX12(Todo)
set GFX_API=OPEN_GL

set BIN_DIR=run_tree/bin/

if not exist "%BIN_DIR%" (
    mkdir "%BIN_DIR%"
)

set COMPILER_FLAGS=-std:c++20 -Zc:__cplusplus -nologo -Zi -FC -W3 -WX -EHsc ^
                   -GR- -Gm- -GS- -permissive- ^
                   -Isrc/ -Isrc/audio/ -Isrc/editor/ -Isrc/fonts/ -Isrc/game/ -Isrc/math/ -Isrc/os/ -Isrc/render/ -Isrc/vendor/ -Isrc/vendor/slang/ ^
                   -Fd%BIN_DIR% -Fo%BIN_DIR% ^
                   -D_CRT_SECURE_NO_WARNINGS -D%GFX_API%

if %RELEASE% == true (
  set COMPILER_FLAGS=%COMPILER_FLAGS% -O2 -DRELEASE
) else (
  set COMPILER_FLAGS=%COMPILER_FLAGS% -Od -DDEBUG -DDEVELOPER
)

set LINKER_FLAGS=-incremental:no -opt:icf -opt:ref -DEBUG -NOLOGO
set LINK_LIBS=kernel32.lib user32.lib dbghelp.lib shlwapi.lib shell32.lib gdi32.lib opengl32.lib ^
              run_tree/openal32.lib run_tree/slang.lib

if %BUILD_TOOLS% == true (
   echo.
   echo Building tools
   cl %COMPILER_FLAGS% -DTOOL_BUILD src/tools/font_baker.cpp ^
      -link %LINKER_FLAGS% -SUBSYSTEM:Console -Out:run_tree/font_baker.exe
)

if %BAKE_FONTS% == true (
   echo.
   echo Baking fonts
   "run_tree/font_baker.exe"
)

:: @Todo: use -SUBSYSTEM:Windows and handle windows console attach/detach
echo.
echo Building game
cl %COMPILER_FLAGS% -DGAME_BUILD src/main.cpp ^
   -link %LINKER_FLAGS% -SUBSYSTEM:Console %LINK_LIBS% -Out:run_tree/scopecorp.exe
