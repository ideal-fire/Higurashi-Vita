A completely unofficial port of Higurashi for the PS Vita.

Needs a modified version of Lua to run the game scripts correctly.


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
	Modified file is in /_Notes/ModifiedLuaSourceFile/
	I'm not really sure what version of Lua is used in my builds. Whatever Lua version was the latest around 5/20/17.
	The modified Lua source changes comments in Lua from "--" to "//" so I don't have to change the game's script comments.
You need libpng.
You need libvita2d.
..and probably some other stuff I forgot. Look in the Makefile for a perfect list.

In src/_GeneralGoodConfiguration.h, there are a bunch of constants.
The one you want to look at is called PRESET. Change this depending on the platform you want to compile for.
For the Vita, make sure it says
	#define PRESET PRE_VITA.

If a miracle happens and you actually get all the libraries that you need, it can be compiled by calling make.