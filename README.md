# [Mech Commander 2](https://alariq.github.io/mc2-website/) open source engine + Linux port.
[website](https://alariq.github.io/mc2-website/)

This port is an open source implementation of a closed MC2 engine code using available interface (.h) files.
Currently game can be run on both Linux and Windows in 64bit mode.
Fixed a lot of bugs (including ones present in original game).
Sound system is not fully implemented (panning, doppler, etc. not supported yet)


## !NB: 
as russia conducts war in Ukraine I have limited time to support this project until we will get rid of the orcs.

## Disclaimer:
I consider this project finished for now, there is a lot more to do for someone who wants to improve the game, but all functionality (except networking) is implemented and I've passed the game on my Linux box. Also found original game bugs and crashes are fixed.

## AI policy: 
This project has 0% LLM code and AIs (such as Claude, ChatGPT and other LLMs) are not welcome here. I have limited time for it and even less time and desire to dig through AI-generated stuff. This is a personal hobby project which started because I love retro games and coding - not using LLMs. It is ok if you use LLMs to help you figure out an issue or help with the code, but you need to understand what you are doing, not just throwing promts at it until it "works".

## TODO: 
* fix remaining memory leaks (finish implementation of memory heaps)
* ~~make nice data packs, so not only me can play the game :-)~~ (see [data repo](https://github.com/alariq/mc2srcdata) )
* ~~actually finish all missions in the game~~
* make sure no files are created outside of user directory
* reduce draw calls number
* ~~reimplement/optimize priority queue~~ (done: O(1) lookups)
* finish moving lighting to shaders (move whole lighting there, not only shader-based drawing of CPU-prelit vertices like I do now)
* Update graphics to ~~2018~~ ~~2020~~ 2021
* Movies support
* Implement network support?
* Editor?
* I am sure there is more

### Settings and save games
`options.cfg` and the save games are kept per user, not in the game directory:
`~/.mechcommander2` on Linux (`$XDG_CONFIG_HOME/.mechcommander2` if that is set) and
`%USERPROFILE%\.mechcommander2` on Windows. An `options.cfg` left in the game directory by an older
version is copied there on the first start.

File names in the game data can be in any case on Linux: a path that does not exist as written is
looked up ignoring case.

### Display options
The game keeps its aspect ratio when the window or screen has a different one (black bars on the
sides or top/bottom). These `options.cfg` settings (in the `MechCommander2` block) have no menu yet:

* `b StretchToFit = TRUE` stretches the picture to fill the window instead
* `l VSync = 0` turns vsync off (`1` on, the default; `-1` adaptive)
* `l MaxFrameRate = 144` caps the frame rate (`0`, the default, means no cap)

### Licensing
* Original game was released under Shared Source Limited Permission License (please refer to EULA.txt)
* My code is licenced under GPL v.3 (see license.txt)
* All third party libraries use their own licenses


Building on Windows
===================

Updated detailed build manual for Windows can be found in `BUILD-WIN.md`


Building on Linux
=================

Updated detailed build manual for Linux can be found in `BUILD-LINUX.md`
