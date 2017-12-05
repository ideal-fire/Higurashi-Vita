An unofficial port of Higurashi When They Cry for the PS Vita and 3ds.

Setup
=====
Refer to the thread on Wololo for more info on setting this up.
http://wololo.net/talk/viewtopic.php?f=116&t=48223

Script converter: https://github.com/MyLegGuy/HigurashiVitaCovnerter
Refer to its README.

Compiling (3ds)
========
make -f Makefile.3ds
You need Lua, libsfil, and libsf2d, libpng.
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

Put the contents of the VpkContents folder into the VPK manually.