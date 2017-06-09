A completely unofficial port of Higurashi for the PS Vita.

Needs a modified version of Lua to run the game scripts correctly.
Modified file is in /_Notes/ModifiedLuaSourceFile/
I'm not really sure what version of Lua is used in my builds. Whatever Lua version was the latest around 5/20/17.
The modified Lua source changes comments in Lua from "--" to "//" so I don't have to change the game's script comments.

Setup
=====
Refer to the thread on Wololo for more info on setting this up.
Script converter: https://github.com/MyLegGuy/HigurashiVitaCovnerter
Refer to its README.

Compiling
========
You need a version of SDL_Mixer-Vita with OGG support. I will post the one I made, if I have time.
You need libvorbis. I think it's from the EasyRPG Vita toolchain.
	https://ci.easyrpg.org/view/Toolchains/job/toolchain-vita/
You need SDL2_Vita
You need the modified Lua
You need libpng and libjpg.
You need libvita2d.
..and probably some other stuff I forgot. Look in the Makefile for a perfect list.

There are 3 constants.
RENDERER 
PLATFORM
SOUNDPLAYER

You change these depending on the platform you compile for.
There's a chance I forgot to change these constants before uploading my code, so you may need to fix these.

#define PLATFORM PLAT_WINDOWS/PLAT_VITA/PLAT_3DS
#define RENDERER REND_SDL/REND_VITA2D/REND_SF2D
#define SOUNDPLAYER SND_SDL

PLAT_3DS, REND_SF2D don't work, but are planned.
Vita uses PLAT_VITA for platform and REND_VITA2D for renderer. If you somehow got SDL_Image to work on the Vita, you could use REND_SDL.