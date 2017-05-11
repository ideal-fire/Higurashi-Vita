

@echo off
for /R %%s in (.\*) do (

if "%%~xs"==".png" (

"C:\Program Files\ImageMagick-7.0.5-Q16\convert.exe" "%%~dpns.png" -resize 640x480 %%~dpns.png
echo Convert back to tid. %%~dpns


)


)
