@echo off
set compilerFlags=/nologo /Od /Z7 /FC /W4 /wd4701 /wd4715
if not exist build mkdir build
pushd build
cl %compilerFlags% ..\boulder_dash.c /link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS user32.lib gdi32.lib
rem cl %compilerFlags% ..\embed_sprites.c /link /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
popd
