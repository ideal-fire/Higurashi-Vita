@echo off
cd src
@echo on
gcc -I../copyrite/ ../copyrite/SDL_FontCache/SDL_FontCache.c main.c -L../lib-custom -lmingw32 -lSDL2_ttf -lSDL2_mixer -llua -lSDL2main -lSDL2 -lSDL2_image -o ./../build/a.exe
@echo off
cd ..
@echo on