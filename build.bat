@echo off
if not exist build mkdir build
pushd build
cl /nologo /Z7 /FC /W4 ..\boulder_dash.c ..\graphics.c ..\cave.c ..\game.c /link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS user32.lib gdi32.lib
REM cl /nologo /Z7 /FC /W4 ..\embed_sprites.c /link /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
popd
