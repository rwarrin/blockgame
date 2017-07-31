@echo off

SET CompilerFlags=/nologo /Z7 /MTd /Oi
REM SET CompilerFlags=/nologo /MTd /Odi
SET LinkerFlags=/incremental:no user32.lib gdi32.lib

if not exist ..\build mkdir ..\build
pushd ..\build

cl.exe %CompilerFlags% ..\code\win32_blockgame.cpp /link %LinkerFlags%

popd
