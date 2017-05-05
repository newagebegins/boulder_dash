@echo off
if not exist build mkdir build
pushd build
cl /nologo /Z7 /FC /W4 ..\boulder_dash.c /link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS user32.lib gdi32.lib
cl /nologo /Z7 /FC /W4 ..\embed_sprites.c /link /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
popd
