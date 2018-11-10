@echo off

set __NDKCMD=e:\softdev\android-ndk-r10\ndk-build.cmd

rmdir /s /q obj >NUL 2>NUL
rmdir /s /q libs >NUL 2>NUL

call %__NDKCMD% -B V=1 NDK_LOG=1

copy /y libs\armeabi-v7a\libcfsurface.so ..\src\main\jniLibs\armeabi-v7a\.
copy /y libs\arm64-v8a\libcfsurface.so ..\src\main\jniLibs\arm64-v8a\.

rmdir /s /q obj >NUL 2>NUL
rmdir /s /q libs >NUL 2>NUL

set __NDKCMD=

:end