# libCFSurface

Direct SurfaceFlinger access as root

## License

Copyright &copy; 2020 Jorrit *Chainfire* Jongma

This code is released under the [Apache License version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

If you use this library (or code inspired by it) in your projects,
crediting me is appreciated.

If you modify the library itself when you use it in your projects,
you are kindly requested to share the sources of those modifications.

## Spaghetti Sauce Project

This library is part of the [Spaghetti Sauce Project](https://github.com/Chainfire/spaghetti_sauce_project).

## About

A long time ago in Android 2.x days, there were the *Live Logcat Boot Animation*,
*Live Dmesg Boot Animation*, and *Mobile ODIN* root apps, as well as the
original *CF-Auto-Root* packages. These provided their screen output by
directly blitting into the Linux framebuffer, using the first incarnation
of *libCFSurface*.

Android changed significantly between 2.x and 4.x (we don't speak of 3.x)
and easy framebuffer access is one of the things that was lost. At first
just in Android itself, but later also in recovery mode.

For *LiveBoot* (successor to *Live Logcat/Dmesg Boot Animation*) and
*FlashFire* (successor to *Mobile ODIN*), SurfaceFlinger had to be
used to get anything on-screen when Android itself wasn't fully loaded.

This code is the third incarnation of *libCFSurface*. As *LiveBoot* was
my first app that used it, it grew there, and was later separated out
when *FlashFire* was created. This incarnation is based on
[libRootJava](https://github.com/Chainfire/librootjava) and uses only
Java and reflection.

I've provided the previous (second) incarnation, which was C++-based
(and a horrendous mess to get to build in the NDK, with lots of files
and code copy/pasted from AOSP), in the git history for those interested
in these things. Of course, that will not work on any recent Android
version.

The original framebuffer-using code is probably somewhere, though
certainly not here.

Some of the GL-related code is partially copy/pasted from some GL
code Google released somewhere, but though I saved the copyright
notice I don't remember it's exact origin.

I doubt anyone will use this library for anything, but you can't
build *LiveBoot* without it, and as I'm uploading that, I need to
upload this.

## How it works, the really short version

I'm not going to provide proper example code. You can look at
*LiveBoot*'s source (**TODO: link when released**) if so inclined.

It's meant to run as a
[libRootJava](https://github.com/Chainfire/librootjava) process' main
loop. You run your own descendant of [SurfaceHost](libcfsurface/src/main/java/eu/chainfire/libcfsurface/SurfaceHost.java),
implementing the abstract methods, as well as exactly one of the
`IGLRenderCallback`, `ICanvasRenderCallback`, `IHardwareCanvasRenderCallback`,
or `ISurfaceRenderCallback` interfaces.

The callbacks are called in a separate thread, that thread (and execution)
ends once your implementation of `SurfaceHost::onMainLoop()` returns.

## Disclaimers

My own code using this library is all GL-based, hence the GL utility
and helper classes. The Canvas-based callbacks were tested when their
support was written, but are not actively used anywhere, I merely
assume they still work.

This code has barely been touched in the past few years, aside from
yearly updates to *LiveBoot*. Experience shows every major Android
release it needs to be updated as the signatures of the methods used
through reflection changes.

Comments (if any) may be outdated or plain wrong.

I did not check anything prior to release other than that it builds and
works for *LiveBoot* on a Pixel2XL running Android 11. Otherwise I'd
never get around to pushing it to GitHub.

## Gradle

```
implementation 'eu.chainfire:libcfsurface:1.0.0'
```
