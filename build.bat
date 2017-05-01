@echo off
cd src
@echo on
gcc main.c -L../lib-custom -lmingw32 -llua -lSDL2main -lSDL2 -lSDL2_image -o ./../build/a.exe
@echo off
cd ..
@echo on