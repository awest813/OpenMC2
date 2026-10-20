# OpenMC2 Modernization Audit & Plan

_Audit date: 2026-10-20 · Baseline: commit `73edc36` · ~440k LOC C++ (code/ 154k, mclib/ 186k, GameOS/ 19k, gui/ 7k)_

This document records the current state of the engine and a phased plan to make it build,
run and perform well on modern computers (current Linux/Windows toolchains, HiDPI and
widescreen displays, high-refresh monitors, multi-core CPUs, modern GPUs, ARM64).

---

## 1. Executive summary

The port is in better shape than its age suggests: the live render path is already
shader/VBO-based (OpenGL core profile), there is no remaining x86 assembly in the build,
serialized structs have mostly been converted to fixed-width types, and nothing is threaded
(so there are no data races to untangle). The problems fall into four buckets:

1. **It does not build on a current toolchain.** On Ubuntu 24.04 / GCC 13 the build fails with
   9 `-Werror=array-bounds` errors in 3 files. Each one is a real bug, not a false positive (see §2.1).
2. **There is no safety net.** No CI, no tests, C++0x mode, `-fpermissive`, ~680 warnings,
   Windows dependencies supplied as a Git-LFS zip.
3. **The platform layer assumes a 2002-era PC.** It assumes 4:3 at 640×480/800×600/1024×768, no HiDPI,
   window resize is ignored, vsync is forced and there is no limiter, US-only keyboard text entry, no gamepad,
   22 kHz audio with no panning or 3D, and config is written into the install directory.
4. **The renderer works, but it is wasteful.** Every `gos_Draw*` call re-uploads its VBO with
   `glBufferData`, re-applies all sampler state and issues its own draw call. Texture locks do a
   synchronous `glGetTexImage` readback, and the GL debug context and synchronous debug
   output are always on, even in release.

The plan below fixes these in that order, so that the project always stays shippable.

---

## Progress

| Item | Status |
|---|---|
| 0.1 GCC 13 build blockers | ✅ Done |
| 0.2 C++17 · 0.3 no `-fpermissive` · 0.4 warning flags, presets, `netlib` removed | ✅ Done |
| 0.5 vcpkg manifest (Windows) | ✅ Done |
| 0.6 CI: Linux x64 GCC/Clang, Linux arm64, Windows MSVC | ✅ Done, all green |
| 0.7 `MC2_SANITIZE` / `asan` preset | ✅ Done (also fixed the sized `operator delete` mismatch it exposed) |
| 0.9 `-fsigned-char` on non-x86 | ✅ Done (plus portable replacements for x86 inline asm in `heap.cpp` / `rdtsc()`) |
| 0.8 Unit-test target (doctest + CTest, run in CI incl. an ASan+UBSan job) | ✅ Done. The tests found and fixed: `EString`/`ECharString`/`EWCharString` `operator+(String, Char)` infinite recursion, a `PriorityQueue::insert` heap overflow when full, a heap-name leak in `UserHeap::init` |
| 0.10 Dead-file cleanup | ✅ Done: 45 files / ~30k lines (unbuilt sources and their headers, `.vcproj`/`.rc`/`.aps`, duplicate `GameDebugWindow`). `ARM/` is kept because `msl.cpp` and `aseconv` use it |
| **Phase 1** | |
| 1.1 Renderer | ✅ `drawQuads` overrun, `gos_UpdateBuffer` offset, GL 4.2 context/GLSL check, unpack alignment, shader reload timer. Added a headless startup smoke test (Xvfb) to CI |
| 1.2 Timing | ✅ hi-res timer (dead PLATFORM_LINUX branch, 1e10), `timeGetTime` overflow, `timing::sleep` ≥ 1 s, Windows `ticks2ms` |
| 1.3 Audio | ✅ volume range (254 vs 128), panning read-back, resource freeing, `Mix_OpenAudio` failure |
| 1.4 Input | ✅ full key table (F-keys/Return/Space/... were reported as Escape), right Ctrl/Shift/Alt, Alt (`KEY_LMENU`), clicks/taps between frames, accumulated mouse deltas |
| 1.5 Files & shims | ✅ `READWRITE` was read-only, `gos_SeekFile` missing, file stubs implemented, CD retry hang, `FindFirstFile` size/time, `CopyFile` |
| 1.6 Engine | ✅ pathfinding `change(door)`, ABL group NULL derefs, order flags, `getFileMTime`, special area packet size |
| 1.7 Overflows & formats | ✅ 65 `-Wformat-overflow` sites, 433 printf length mismatches, plus bugs found on the way (infowindow `inventory[-1]`, Clang-only waypoint key bug, missing virtual dtor). Warnings 679 → 140 |
| 1.8 Struct layout | ✅ `tests/test_struct_layout.cpp`: compile-time size checks for data file and save game structs on all CI platforms |
| **Phase 2** | |
| 2.1 Window, HiDPI, aspect ratio | ✅ Logical resolution fitted into the drawable with black bars (`StretchToFit` to stretch), drawable size re-read every frame (window resizing, display scaling), no manual DPI multiply, HiDPI/letterbox-correct mouse position and deltas |
| 2.4 Frame pacing | ✅ `VSync` (off/on/adaptive) and `MaxFrameRate` in options.cfg, frame limiter (sleep + short spin) |
| 2.6 Depth buffer | ✅ 24-bit |
| 2.2/2.3 Arbitrary resolutions, UI scaling · 2.5 fixed timestep · MSAA/anisotropic options | ⏳ Not started: need in-game testing with data (UI layouts, gameplay timing) |
| Open: save game layout | ⚠️ `TeamStaticData`, `TeamData`, `StaticCommanderData`, `CommanderData`, `GateData`, and via `WeaponShotInfo` also `MoverData`/`MechData`/`GroundVehicleData`/`WeaponBoltData` (confirmed by the size checks in Windows CI), are saved as raw bytes but contain `long` (8 bytes on Linux, 4 on Windows/original), `GateData` also a pointer. Making them `int32_t` makes saves portable but breaks existing Linux saves; needs a decision (or a versioned save format) |

---

## 2. Audit findings

### 2.1 Build status (GCC 13.3, CMake 3.28, Ninja, RelWithDebInfo)

`mc2` **fails to link** because 3 translation units fail to compile. The other tools and `libmc2res_64.so` build.
There are 681 warnings in total (472 `-Wformat`, 114 `-Wunused-variable`, 69 `-Wformat-overflow`, ...).

| File | Error | Underlying bug |
|---|---|---|
| `data_tools/aseconv.cpp:385` | `MechAnimationNames[i]` for `i` in `25..26` | `extern` declared the array with `MaxGestures` (25) entries, while the definition in `mech3d.cpp` has `MaxGestures+2` |
| `code/mpstats.cpp:527` | `(TGAFileHeader*)MPlayer->insigniaList[id]` | `insigniaList` is `bool[]`; a bool is cast to a pointer and dereferenced |
| `code/mpprefs.cpp:574-623` | `strcpy(pInfo->...)` | `MultiPlayer::getPlayerInfo()` is a stub returning `NULL`, so this is a guaranteed null dereference |

Other build-system issues:
- `-std=c++0x`, `-fpermissive` (only `mclib/vfx_ellipse.cpp:625,838` actually needs it: function pointer → `void*`).
- `-Wno-unknown-warning-option` is clang-only and produces a GCC note on every TU.
- `include_directories(... "./netlib")` refers to a directory that does not exist.
- Debug defines are injected via `CMAKE_CXX_FLAGS_DEBUG` with `-D...` on MSVC as well.
- Global `include_directories`/`add_definitions` are used instead of per-target usage requirements.
- Windows deps: manual multi-page procedure or `3rdparty.zip` (23 MB Git-LFS blob).
- 8 stale `.vcproj` files and `mclib/MCLib.rc/.aps` sit alongside CMake.

### 2.2 Rendering (GameOS/gameos, mclib/mlr, mclib/txmmgr)

**Correctness**
- Context requests **GL 4.0 core** (`gos_render.cpp:113`), but every shader is prefixed `#version 420`
  (`gameos_graphics.cpp:171`), and the startup check only requires GL 3.0 / GLSL 3.30 (`:2275,2286`).
  Drivers that cap at 4.1 (macOS, some Mesa configs) pass the check and then fail shader compile.
- `#define X = 1` is generated for material defines (`gameos_graphics.cpp:177`). It is malformed, and only works because of `#ifdef`.
- `glewExperimental` is not set before `glewInit()` on a core context. `glGetString(GL_EXTENSIONS)` is invalid in core (`gos_render.cpp:312`).
- **`drawQuads` indexing bug:** the loop does `i += 4` *and* indexes `vertices + 4*i` (`gameos_graphics.cpp:1717-1725`). This reads out of bounds whenever `count > 4`, which `mlrsorter.cpp:185` does.
- `gos_UpdateBuffer` ignores `offset` and calls `glBufferData(num_bytes)`, which reallocates/shrinks the buffer (`:2913-2920`).
- An empty luminance texture is only partly cleared: `memset` covers 1/4 of the buffer (`:930`).
- `glPixelStorei(GL_UNPACK_ALIGNMENT)` is never set, so R8 textures with odd widths (movie planes) can skew.
- The depth buffer is 16-bit (`gos_render.cpp:98`), which causes z-fighting at high resolutions.

**Performance**
- There is no GPU-side batching. Each draw does `applyRenderStates()` (`glTexParameteri` × 3 units), a full
  `glBufferData` VBO re-upload, uniform uploads, attribute re-enable, and then a draw (`:508-601, 1577-1599, 1728, 1813`).
- `gos_LockTexture` does a `glGetTexImage` readback plus a per-pixel swizzle on every lock (`:755`). It is used
  per-frame by mech icons, the control GUI, txmmgr and others.
- The debug context and `GL_DEBUG_OUTPUT_SYNCHRONOUS` are always on. `CHECK_GL_ERROR` calls `glGetError` several times per draw.
- The shader hot-reload timer never resets (`:1612-1619`), so it `stat()`s every shader file every frame after 500 ms.
- There are no mipmaps, no anisotropic filtering and no MSAA (it is commented out).
- Lighting is still CPU-prelit. The hardware T&L path is experimental (Ctrl+Shift+G).

### 2.3 Display, windowing, frame timing

- The startup window is hardcoded to 800×600 and then multiplied by `SDL_GetDisplayDPI/96` **and** `SDL_WINDOW_ALLOW_HIGHDPI`,
  so it is double-scaled on macOS and some Wayland setups (`gos_render.cpp:229-241`).
- `SDL_WINDOWEVENT_RESIZED` is only logged (`gameosmain.cpp:88-95`), so the drawable size and viewport go stale.
- Fullscreen is desktop-only. Logical resolution is stretched over the drawable with **no letterboxing**, so 4:3 is stretched on 16:9/21:9.
- 640×480/800×600/1024×768 are hardcoded across `prefs.cpp:474-500`, `mechcmd2.cpp:1483-1499,1781,2363`,
  `optionsarea.cpp:343-348`, `logisticsscreen.cpp:585`, `mainmenu.cpp`, `missionbegin.cpp`, and others.
- Vsync is forced on by a compile-time constant (`gos_render.cpp:21,297`). There is no frame limiter, so with vsync off the game runs
  uncapped and burns a full core and GPU.
- The simulation uses a variable timestep: `frameLength = 1/frameRate`, clamped to 0.25 s, measured from the *previous* frame
  (`mechcmd2.cpp:2073`, `gameosmain.cpp:189-213`).
- `gos_GetHiResTime` divides nanoseconds by **1e10** instead of 1e9 (`gameos.cpp:113`). `timeGetTime` converts a double to
  DWORD, which is UB after ~49 days of uptime (`platform_mmsystem.cpp:14`). On Windows, `ticks2ms` returns `1000*ticks` (`utils/timing.cpp:64`).

### 2.4 Audio (SDL2_mixer)

- Output is 22050 Hz / S16 / stereo / 1024 frames (`gameos_sound.cpp:176,329`).
- `MAX_VOLUME = 254`, but `MIX_MAX_VOLUME` is 128, so every volume ≥ 0.5 saturates (`:9,629`).
- Panning, frequency and 3D/positional audio are stubs (`:633-655`). A `GetChannelSlider` panning case falls through into a PAUSE (`:674`).
- "Streamed" files are fully loaded with `Mix_LoadWAV` (`:410`). Resources are leaked unless they are bound to a channel (`:534-543`).
- There is a `printf` on every `Mix_PlayChannel`, and a failure in `Mix_OpenAudio` is ignored.

### 2.5 Input

- Text entry uses a hardcoded US shift table (Shift+6 → `:`). There is no `SDL_TEXTINPUT` and the IME functions are stubs, so non-US layouts are broken.
- Unknown scancodes map to **`KEY_ESCAPE`** (`gameos_input.cpp:179-183`). Right Ctrl and right Shift are ignored.
- Mouse buttons are polled once per frame, so fast clicks are lost. The event handler is commented out and its logic inverted.
- Under HiDPI, the mouse position is window coordinates divided by drawable pixels (`gameos_input.cpp:35-43`).
- There is no gamepad support (`gosJoystick_*` are stubs) and keys cannot be rebound.

### 2.6 Filesystem & platform shims

- Paths are converted backslash→slash and **forced to lowercase** (`mclib/file.cpp:224-242`), so data must be lowercase on disk.
- `options.cfg` and `.old` are written to the CWD/install dir (`prefs.cpp:95,294`). This is a README TODO.
- Saves go to `$XDG_CONFIG_HOME` or `$HOME/.mechcommander2` rather than a proper XDG data dir. Registry functions are stubs.
- `fopen` mode `"rwb"` is invalid (`gameos_fileio.cpp:38`). File seek returns the status, not the position.
- `FindFirstFile` swaps the size high/low words and returns wrong timestamps. `CopyFile` stamps the source file.
- The `MessageBox` stub never returns `IDCANCEL`, so the "insert CD" loop in `file.cpp:292-308` can spin forever.
- Many `gos_*` file functions are `STOP`-only stubs that silently return false/NULL in release.

### 2.7 Game/engine code (code/, mclib/, gui/)

**64-bit and portability.** `DWORD` is 32-bit, but `LONG`/`ULONG`/`*_PTR` are `long`, which is 64-bit on LP64.
`INT_PTR`/`UINT_PTR` are `int` (`platform_basetsd.h:21`), a latent bug. There are ~10.5k uses of `long`, and serialized
structs are mostly migrated already. `mclib/move.cpp:639,645` writes `sizeof(long)` (8 bytes) into a packet, so the format
diverges from Windows. 48 `#pragma pack` structs have no `static_assert` on their size. **ARM caveat:** `char` is unsigned
by default on ARM. The `char → unsigned char` fix in `3bfc4ae` shows the code depends on signedness.

**Real bugs found**
- `operator+(String, Char)` in `estring.cpp:917`, `echarstring.cpp:~923` and `ewcharstring.cpp:~928` calls itself, causing **infinite recursion** (it should use `Tmp`).
- `move.cpp:3350` passes `openList->change(door, ...)` with the door id where `openIndex` belongs.
- `ablmc2.cpp:2368,2836`: a NULL `object1` is dereferenced (`-Wnonnull`).
- `missiongui.cpp:4909`: `x = (++x % 6)`. `ablenv.cpp:751,758`: `1 << 31` on a signed int.
- Possibly uninitialized: `mission.cpp:801` `tempAmbientLight`, `file.cpp:1717` `mTime`.

**Safety.** `sprintf` 1289, `strcat` 440, `strcpy` 315, but only 32 `snprintf`. There are ~1500 fixed `char[N]` buffers and 57
`-Wformat-overflow` sites (e.g. `controlgui.cpp:2254` writes 255 bytes into a 235-byte buffer). `PacketFile::readPacket` has no bounds check (60 callers).
gcc also reports 49 `-Wdelete-non-virtual-dtor` and 69 `-Wclass-memaccess`.

**Memory.** On Linux the 18 named heaps are pass-throughs to `malloc`. Destroying a heap does not free its blocks,
which is the source of the "memory leaks" TODO. Global `new`/`delete` are overridden.

**Hotspots**
- `PriorityQueue::find` (`pqueue.cpp:125`) is a linear scan called on every A* decrease-key, which makes it **O(n²) per path search** (README TODO).
- `GameObjectManager::findByPartId` (`objmgr.cpp:2098`) is linear and has 42 callers.
- Sensor scan is sensors × movers × LOS, throttled to 0.5 s. Collision cells are O(k²).

**Dead code**
- Networking: `multplyr.cpp` is entirely stubs, but there are ~218 `if (MPlayer)` branches and 9 MP screens.
- 37 `USE_ASSEMBLER_CODE`/`_asm` blocks (205 asm sites).
- Unbuilt files: `code/actor.cpp`, `debugging.cpp`, `fastshp.cpp`, `logmain.cpp`, `mpaddaiplayer.cpp`, `mpdirecttcpip.cpp`,
  `vfxshape.cpp`; `mclib/llist.cpp`, `tglpp.cpp`, 7 `mlr` files, `stuff/extentpoly.cpp`, `stuff/filestreammanager_test.cpp`.
- A duplicate `GameDebugWindow` lives in `code/debugging.h`. `ARM/Microsoft.Xna.Arm.h` looks like a leftover, but `mclib/msl.cpp` and `data_tools/aseconv.cpp` use it.

**Tests.** None run. `mclib/stuff/*_test.cpp` (26 files) are compiled but never invoked. There is no `enable_testing()`.

---

## 3. The plan

Guiding rules:
- **Keep the game runnable after every step.** Small, reviewable PRs.
- **Do not break save games or data formats** unless a phase says so explicitly. Add `static_assert`s first.
- **Avoid mass reformatting/renaming**, so that cherry-picking from upstream (`alariq/mc2`) stays possible.
- **Measure before optimizing.** Add a profiler (Tracy) before touching hot paths.

### Phase 0: Build on modern toolchains, add a safety net  _(prerequisite for everything else)_

| # | Task | Notes |
|---|---|---|
| 0.1 | Fix the 3 GCC 13 build blockers (§2.1) | Fix the bugs themselves. Do not silence `-Warray-bounds`. |
| 0.2 | `CMAKE_CXX_STANDARD 17` (required), drop `-std=c++0x` | Builds as-is in C++17 per syntax pass. C++20 later if wanted. |
| 0.3 | Remove `-fpermissive` | Fix the 2 function-pointer casts in `vfx_ellipse.cpp`. |
| 0.4 | Per-compiler warning flags. Remove the non-existent `netlib` include. Use `target_compile_definitions` for debug defines. | Introduce `CMakePresets.json` (debug / release / asan / msvc). |
| 0.5 | Windows dependencies through **vcpkg manifest** (`vcpkg.json`: sdl2, sdl2-mixer, sdl2-ttf, glew, zlib) | Replaces the LFS zip and the 9-step manual process. |
| 0.6 | **GitHub Actions CI**: Ubuntu (GCC + Clang), Windows (MSVC + vcpkg), Linux **arm64** | Build-only at first. Upload artifacts. |
| 0.7 | `-DMC2_SANITIZE=address,undefined` option | Run a mission under ASan/UBSan to seed the Phase 1 bug list. |
| 0.8 | Wire up a unit-test target (doctest/Catch2 + CTest) | Start with: path conversion, `PriorityQueue`, `EString`, packet/fast-file readers, plus the existing `stuff` tests. |
| 0.9 | Add `-fsigned-char` on non-x86 | Preserves x86 semantics on ARM until the code is audited. |
| 0.10 | Delete stale `.vcproj`/`.aps`/`.rc`, unbuilt sources, duplicate `GameDebugWindow` | Pure deletion. Verify the build still succeeds. |

**Exit criteria:** green CI on 4 configurations. `mc2` builds on GCC 13+/Clang 17+/MSVC 2022 with zero errors and without `-fpermissive`.

### Phase 1: Correctness fixes from this audit  _(small, high value)_

1. Renderer: `drawQuads` indexing, `gos_UpdateBuffer` offset (use `glBufferSubData`), luminance memset, `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)`.
2. Timing: the `gos_GetHiResTime` 1e10 divisor, `timeGetTime` overflow (use 64-bit/wrapping arithmetic), Windows `ticks2ms`.
3. Audio: volume scale (254 → `MIX_MAX_VOLUME`), panning fall-through, resource leak, remove per-play `printf`, handle `Mix_OpenAudio` failure.
4. Input: unknown scancode must not map to ESC; map right Ctrl/Shift; event-driven mouse buttons (fix the inverted logic).
5. Files and shims: the `"rwb"` → `"r+b"` mode, seek return value, `FindFirstFile` size/time, `CopyFile` target, `MessageBox` stub returning `IDCANCEL`/no CD loop.
6. Engine: `EString`/`ECharString`/`EWCharString` `operator+` recursion, `move.cpp:3350` `change(openIndex,…)`,
   `ablmc2.cpp` NULL deref, `missiongui.cpp:4909` sequence point, `ablenv.cpp` `1u<<bit`, uninitialized vars.
7. Fix the 57 `-Wformat-overflow` sites first (real overflows), then the 430 `-Wformat` `%d`/`long` mismatches.
8. `move.cpp:639/645`: write `int32_t`, not `sizeof(long)`. Add `static_assert(sizeof(T) == N)` to all 48 packed structs.

**Exit criteria:** ASan/UBSan clean through the main menu, logistics, and one full mission. Warning count is down by at least 80%.

### Phase 2: Displays: HiDPI, widescreen, high refresh

1. **Window/drawable model:** handle `SDL_WINDOWEVENT_SIZE_CHANGED`, keep window size separate from drawable size, remove the manual
   DPI multiply (rely on `ALLOW_HIGHDPI`), and fix mouse coordinate mapping.
2. **Aspect ratio:** render 3D at the native drawable resolution and aspect, and lay out the 4:3 UI in a centered, integer- or
   fractionally-scaled UI viewport with pillarboxing. Make "stretch" a user option.
3. **Arbitrary resolutions:** replace the hardcoded mode tables with `SDL_GetDisplayMode` enumeration plus
   "native desktop". Keep the 4:3 UI-layout presets internally as UI scale targets.
4. **Frame pacing:** vsync becomes a runtime option (off / on / adaptive `-1`), plus an optional frame cap using a precise
   sleep (`SDL_Delay` + spin tail) so the game does not burn a full core. Support 120/144/240 Hz.
5. **Timestep:** measure the dt of the current frame, not the previous one. Then evaluate a fixed-tick simulation
   (e.g. 30 Hz) with render interpolation. This is a larger change and can be deferred if variable dt proves stable at high fps.
   Test physics, animation and ABL timers at 240 fps and at 20 fps.
6. 24-bit depth plus optional MSAA (2×/4×/8×) and anisotropic filtering in the options screen.

### Phase 3: Renderer efficiency on modern GPUs

1. **Context:** request GL **3.3 core** with GLSL `#version 330` (widest reach, including macOS 4.1 and Mesa/llvmpipe), or
   make the GLSL version follow the context version. Replace GLEW with **glad** (generated loader, works on core/ES), or at least set `glewExperimental`.
2. Use a debug context and synchronous debug output **only in Debug builds**, and compile `CHECK_GL_ERROR` out of release.
3. **Stream buffers:** a persistent per-frame ring VBO (orphaning or `GL_MAP_UNSYNCHRONIZED`/persistent mapping),
   append-only during the frame, and one upload per flush.
4. **State caching:** sampler objects instead of per-draw `glTexParameteri`, a redundant-state filter for
   blend/depth/texture binds, and VAO set up once per vertex format.
5. **Real batching:** queue `gos_Draw*` calls keyed by (material, textures, render state) and flush on state change
   or at end of frame. `txmmgr`'s per-texture node lists already group by texture, so exploit that.
6. **Kill readbacks:** keep CPU shadow copies for textures that get `gos_LockTexture`d, and upload dirty regions with `glTexSubImage2D`.
7. Generate mipmaps for world textures (`glGenerateMipmap`).
8. Make shader hot reload debug-only and fix its timer.
9. **Longer term:** finish the hardware T&L path (move lighting to shaders, as the README TODO says), so that mlr stops CPU-transforming
   every vertex. Target: draw calls per frame reduced by about 10×. Track this with an on-screen stats overlay and Tracy GPU zones.

### Phase 4: Audio and input for current hardware

- **Audio:** open at 48 kHz (or the device default), implement panning (`Mix_SetPanning`) and 3D/distance (`Mix_SetPosition`)
  so the game finally has positional sound, true streaming for music and speech (`Mix_Music` or SDL_AudioStream), and per-category volume.
- **Text:** use `SDL_StartTextInput` / `SDL_TEXTINPUT` for edit boxes (proper layouts, IME, UTF-8 → game codepage).
- **Gamepad (stretch):** an `SDL_GameController` virtual cursor plus bindable actions. Optional key rebinding via a config file.

### Phase 5: Filesystem, packaging, platforms

- Use `SDL_GetPrefPath("", "MechCommander2")` for **all** writable files (options.cfg, saves, logs, screenshots), and never write
  to the install dir (README TODO). Migrate the existing `~/.mechcommander2` on first run.
- Case-insensitive path resolution (directory cache) instead of force-lowercasing, so stock data works unchanged.
- Implement or remove the `STOP`-only `gos_*` file stubs.
- **Packaging:** AppImage and/or Flatpak (Linux), a zipped portable build with DLLs (Windows), produced by CI on tags.
- **ARM64:** CI build on Linux arm64 (Raspberry Pi 5, Snapdragon laptops, Asahi). Audit `char` signedness and then drop `-fsigned-char`.
  macOS (Apple Silicon) becomes feasible once Phase 3.1 (GL 3.3/4.1) lands. Treat it as a stretch goal, since Apple's GL is deprecated.

### Phase 6: CPU performance

- **Pathfinding:** give `PriorityQueue` an id → heap-index map, so `find`/`change` become O(1)/O(log n) (README TODO).
- `findByPartId`: use a hash map or a direct index table.
- Profile one heavy mission with Tracy before touching sensors or collision.
- **Threads, where they are safe:** async texture/asset loading and decompression, and audio decode (SDL already threads output).
  Keep the simulation single-threaded, since global state is pervasive (278 externs, ABL interpreter globals).

### Phase 7: Code health (continuous, opportunistic)

- `sprintf`/`strcpy`/`strcat` → `snprintf`/bounded helpers. Do it per-file when a file is touched anyway, prioritizing
  `-Wformat-overflow` hits and anything that handles file or packet data. Add bounds to `PacketFile::readPacket`.
- Remove the `USE_ASSEMBLER_CODE`/`_asm` dead branches.
- Add virtual destructors where `-Wdelete-non-virtual-dtor` fires. Fix `-Wclass-memaccess` memsets.
- **Heaps:** track allocations per heap so that `destroy()` frees them (fixes the leaks while keeping semantics), or migrate heap by heap to owned containers.
- **Networking decision:** either keep the stubs and harden them (no NULL-returning `getPlayerInfo`), or compile the MP
  screens out behind `MC2_ENABLE_MULTIPLAYER=OFF`. A real implementation (ENet/Steam sockets) is a separate project.
- Add clang-tidy in CI with a small, curated check set (bugprone-*, a few modernize-* checks), report-only at first.

### Phase 8 (optional): SDL3

The SDL surface is small: about 140 call sites in 6 files. Most of it is in `gos_render.cpp` (window and modes) and `gameos_sound.cpp`.
SDL3 brings better HiDPI and display-scale APIs, improved gamepad and pen support, and **SDL_GPU** (Vulkan/Metal/D3D12 backends),
which would be the natural path off OpenGL. The audio layer needs a rewrite (`SDL_AudioCVT`, `SDL_QueueAudio`, Mixer 3).
**Recommendation:** stay on SDL2 until Phases 0–4 are done. SDL2 is still maintained, and `sdl2-compat` covers distros that drop it.
Then migrate in one focused PR.

---

## 4. Suggested first 10 PRs

1. Fix the 3 GCC 13 build blockers (aseconv OOB, mpstats bool→pointer, mpprefs NULL `getPlayerInfo`).
2. CMake: C++17, drop `-fpermissive`, per-compiler warnings, presets, remove `netlib`.
3. GitHub Actions: Linux GCC/Clang + Windows MSVC/vcpkg + Linux arm64.
4. Test target plus the first tests (`PriorityQueue`, `EString`, path conversion). Fix the `operator+` recursion.
5. Renderer correctness: `drawQuads`, `gos_UpdateBuffer`, unpack alignment, luminance memset, GLSL/context version.
6. Timing and audio quick fixes: hi-res timer divisor, volume scale, fall-through, leak, log spam.
7. Input quick fixes: unknown key → ESC, right modifiers, event-driven mouse buttons, HiDPI mouse mapping.
8. Window resize, letterboxed 4:3 UI on widescreen, runtime vsync plus frame cap.
9. `SDL_GetPrefPath` for options.cfg and saves. Case-insensitive file lookup.
10. Release-only GL state: no debug context, no sync debug output, no per-draw `glGetError`, plus a stream ring buffer.

---

## 5. Risks and notes

- **Upstream policy:** upstream `alariq/mc2` states in its README that it does not accept LLM-generated contributions. Work from this audit
  belongs in this fork and should not be proposed upstream unless rewritten by a human contributor.
- **Data dependency:** the game cannot run in CI without `mc2srcdata` (Git-LFS, large). A data-driven smoke test (boot to main menu,
  load one mission headless) is valuable but needs a cached data artifact or a self-hosted runner.
- **Save compatibility:** any change to packed structs, `long` fields or packet layout can break saves. The `static_assert`s from Phase 1.8 are the guard rail.
- **Timestep changes** (Phase 2.5) affect gameplay feel and ABL mission scripts. Gate them behind a flag until missions are re-verified.
