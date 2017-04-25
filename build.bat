@echo off
if not exist build mkdir build
pushd build
cl /nologo /Z7 /FC /W4 ..\main.c ..\cave.c /link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS user32.lib gdi32.lib
popd
