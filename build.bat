@echo off
if not exist build mkdir build
pushd build
cl /nologo /Z7 /FC /W4 ..\boulder_dash.cpp ..\graphics.cpp ..\cave.cpp ..\game.cpp /link /INCREMENTAL:NO /SUBSYSTEM:WINDOWS user32.lib gdi32.lib
cl /nologo /Z7 /FC /W4 ..\embed_sprites.cpp /link /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
popd
