#!/usr/bin/env bash
# Headless startup smoke test: runs the game without data under Xvfb (software OpenGL is fine).
# Without options.cfg the game runs its hardware check ("sniffer"), which creates the window and
# the OpenGL context and compiles all shaders, then exits. This catches context creation and
# shader compilation problems without needing the game data.
#
# usage: tests/smoke_startup.sh <path to mc2 executable> <path to the mc2res library>
set -u

if [ $# -lt 2 ]; then echo "usage: $0 <mc2 executable> <mc2res library>"; exit 2; fi
MC2=$(realpath "$1")
SRC_DIR=$(cd "$(dirname "$0")/.." && pwd)
RUN_DIR=$(mktemp -d)
trap 'rm -rf "$RUN_DIR"' EXIT

cp -r "$SRC_DIR/shaders" "$RUN_DIR/"
cp "$2" "$RUN_DIR/"
# stands in for the minprefs.cfg shipped with the data; the game copies it to the user's options.cfg
printf '[MechCommander2]\nl DigitalMasterVolume = 200\n' > "$RUN_DIR/minprefs.cfg"

cd "$RUN_DIR"
# GL debug output (and glGetError checks) on, so GL errors show up in the log. The user data
# directory goes to $XDG_CONFIG_HOME when set (some CI runners set it), so keep it in $RUN_DIR
env -u XDG_CONFIG_HOME HOME="$RUN_DIR" MC2_GL_DEBUG=1 timeout 60 xvfb-run -a -s "-screen 0 1280x1024x24" "$MC2" > startup.log 2>&1
status=$?

fail() { echo "SMOKE TEST FAILED: $1"; echo "----- log -----"; cat startup.log; exit 1; }

[ $status -eq 124 ] && fail "timed out"
grep -q "is now done checking your computer" startup.log || fail "hardware check did not finish (exit status $status)"
grep -q "SDL_GL_CONTEXT_MAJOR_VERSION: 4" startup.log || fail "no OpenGL 4.x context"
grep -q "INFO: Loading shader" startup.log || fail "no shaders were loaded"
if grep -E "ERROR:|DEBUG_TYPE_ERROR" startup.log; then fail "errors were logged"; fi
# settings are written to the user data directory, not the install (current) directory
[ -f "$RUN_DIR/.mechcommander2/options.cfg" ] || fail "options.cfg was not created in the user data directory"
[ -e "$RUN_DIR/options.cfg" ] && fail "options.cfg was written to the install directory"
# the only expected STOP is the missing test texture (testtxm.tga ships with the game data)
if grep -o "STOP[^:]*" startup.log | grep -v "Failed to create texture"; then fail "unexpected STOP"; fi

echo "smoke test passed: $(grep -c 'INFO: Loading shader' startup.log) shader stages loaded"
