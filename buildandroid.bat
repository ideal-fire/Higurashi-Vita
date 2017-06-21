@echo off
call copyandroid.bat
cd "C:\Users\knoob\Desktop\Nathan\Programing\Android\SDL2-2.0.5\android-project\jni\src"
@echo on
call ndk-build
@echo off
cd "C:\Users\knoob\Desktop\Nathan\Programing\C\Higurashi"
@echo on