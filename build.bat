@echo off
if not exist build mkdir build
pushd build
cl /nologo /Z7 /FC /W4 ..\main.c ..\cave.c ..\graphics.c /link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS user32.lib gdi32.lib
cl /nologo /Z7 /FC /W4 ..\pack_data.c ..\cave.c /link /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
popd
