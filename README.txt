An unofficial port of Higurashi When They Cry for the PS Vita and 3ds.
Refer to the wiki for scripting API and more.

Setup (Vita)
=====
http://wololo.net/talk/viewtopic.php?f=116&t=48223

Setup (3ds)
=====
(Coming soon)

Script converter: https://github.com/MyLegGuy/HigurashiVitaCovnerter

Compiling (3ds)
========
make -f Makefile.3ds
You need Lua, libsfil, and libsf2d, libpng.
	Modified Lua source code file is in /_Notes/ModifiedLuaSourceFile/
Every library you compile with needs to be compiled with -mfloat-abi=softfp

Compiling (Vita)
========
You need SoLoud.
You need the modified Lua, version 5.3.4.
	Modified file is in /_Notes/ModifiedLuaSourceFile/
You need libpng.
You need libvita2d.

All of those, except Lua, can be obtained with vdpm.
https://github.com/vitasdk/vdpm

If a miracle happens and you actually get all the libraries that you need, it can be compiled by doing the following:

cmake CMakeLists.txt
make

Put the contents of the VpkContents/Vita and VpkContents/Shared folders into the VPK manually.