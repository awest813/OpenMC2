# Dependencies:

Apt based distros:
```
sudo apt install cmake ninja-build libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev zlib1g-dev libglew-dev
```

A C++17 compiler is required (GCC 9+ or Clang 10+), and CMake 3.16+ (3.21+ to use presets).


# Directory Setup

This guide assumes you have the following directory structure:

```
some-empty-directory
|
+---mc2
\---mc2srcdata


```
To properly clone `mc2srcdata` you will need `git-lfs`. Make sure it's installed on your system.

From an empty directory issue the following commands.

```
git clone https://github.com/alariq/mc2.git
git clone https://github.com/alariq/mc2srcdata.git
```


# Compiling

## Executable

Using CMake presets (recommended):
```
cd mc2
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
```
Binaries end up in `build/relwithdebinfo/` (`mc2`), with the tools and the resource library
under `build/relwithdebinfo/out/` (`out/res/libmc2res_64.so`, `out/data_tools/`, `out/text_tool/`).
Other presets: `debug`, `release`, and `asan` (Debug build with AddressSanitizer + UBSan).
Run `cmake --list-presets` to see them all.

Unit tests (in `tests/`) are built by default and run with:
```
ctest --test-dir build/relwithdebinfo --output-on-failure
```

Or manually:
```
cd mc2
mkdir build64
cd build64
cmake -DCMAKE_LIBRARY_ARCHITECTURE=x64 ..
make
```

Copy `mc2` executable and the `shaders` folder to your executable folder of preference (say mc2exe):

```
cp -r mc2 ../shaders <mc2exe location>
```

## Resource library
```
cd mc2
cd test_scripts/res_conv
./res_conv.pl
cd ../../res
mkdir build64
cd build64
cmake -DCMAKE_LIBRARY_ARCHITECTURE=x64 ..
make
```
Copy `libmc2res_64.so` to your executable folder of preference (say mc2exe):
```
cp libmc2res_64.so <mc2exe location>
```

## Building data

`mc2srcdata/build_scripts` has its own `README.md`

```
cd mc2srcdata
cd build_scripts
```


### Prerequisites

You need to build data tools from mc2 repository. And copy executeables in `build_scripts` folder.
Those are the necessary tools (they are built along with a game, so no additional actions should be taken):
 - aseconv
 - makefst
 - makersp
 - pak
 - text\_tool

```
cp ../../mc2/build64/out/data_tools/{aseconv,makefst,makersp,pak} .
cp ../../mc2/build64/out/text_tool/text_tool .

```
### Dependencies
 * `ffmpeg` is used to convert movies. You should have it in the PATH environment variable or just puch absolute path in `config.mk`

### Building
`make all BUILD_PLATFORM=linux`

### How to clean data:
`make clean BUILD_PLATFORM=linux`

### Move data to where you've built the game

You'll need to copy the following files to the game directory (mc2exe):

* data
* assets
* \*.cfg
* \*.fst
* testtxm.tga

```
cp -r assets/ data/ *.cfg *.fst testttxm.tga <location of mc2exe>
```

# Running the game

Go to your `mc2exe` folder and execute `./mc2`, your game should hopefully run! Have fun!
