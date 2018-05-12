An unofficial port of Higurashi When They Cry for the PS Vita and 3ds.
Refer to the wiki for scripting API and more.

Setup (Vita)
=====
http://wololo.net/talk/viewtopic.php?f=116&t=48223

Setup (3ds)
=====
https://gbatemp.net/threads/release-higurashi-when-they-cry-3ds-port.492390/

Script converter: https://github.com/MyLegGuy/HigurashiVitaCovnerter

Compiling (3ds)
========
make -f Makefile.3ds
You need Lua, libsfil, and libsf2d, libpng.
	Modified Lua source code file is in /_Notes/ModifiedLuaSourceFile/
Every library you compile with needs to be compiled with -mfloat-abi=softfp

Compiling (Vita)
========
You need the modified Lua, version 5.3.4.
	Precompiled version is already at ./lib/libluaVita.a, you only need to recompile Lua yourself if the build I posted is incompatible in the future.
	Modified file is in /_Notes/ModifiedLuaSourceFile/
You need libsamplerate.
	Nobody's posted a makefile for it yet, but it's simple to make yourself. Use this as a template: https://github.com/xerpi/SDL-Vita/blob/master/Makefile.vita
The libGeneralGood build in the ./lib/ folder could be outdated. If so, compile it yourself. https://github.com/MyLegGuy/libGeneralGood

Everything else can be obtained with vdpm.
https://github.com/vitasdk/vdpm

If a miracle happens and you actually get all the libraries that you need, it can be compiled by just calling `make -f makefile.vita` and then `make -f makefile.vita stuffvpk`. (p)7zip is required for the `stuffvpk` target.