#!/bin/zsh
# Run the game under lldb; on any crash (SIGSEGV / SIGABRT / trap) dump a full
# backtrace to scripts/crash.log and exit. Run this in a real desktop Terminal
# (it needs the window server). Play to the crash (e.g. click Skirmish), or set
# MACH_AUTO_SKIRMISH=1 to jump straight into the skirmish screen.
#
# Env knobs:
#   BUILD=release|debug        which build to run          (default: release)
#   MACH_AUTO_SKIRMISH=1       auto-open skirmish on boot  (default: off - you navigate)
#   MACH_RUN_TIME=<secs>       auto-quit if it does NOT crash (default: unset - runs until you quit)
set -e
export PATH="/opt/homebrew/bin:$PATH"
ROOT="${0:A:h:h}"                       # repo root (dir above scripts/)

BUILD="${BUILD:-release}"
case "$BUILD" in
  release) BINDIR="$ROOT/buildMacOS" ;;
  debug)   BINDIR="$ROOT/buildMacOSDebug" ;;
  *) echo "BUILD must be release or debug" >&2; exit 2 ;;
esac
BIN="$BINDIR/machines"
LOG="$ROOT/scripts/crash.log"

if [[ ! -x "$BIN" ]]; then echo "Missing $BIN - build it first (make build / make build-release)" >&2; exit 1; fi
if ! command -v lldb >/dev/null 2>&1; then
  echo "lldb not found. Install: xcode-select --install" >&2; exit 127
fi

# The game resolves asset paths relative to the current directory, so run from
# the asset root. MACH_ROOT is also honoured for SysPathName-resolved paths.
cd "$ROOT/machines-game-data"
export MACH_ROOT="$PWD"

echo "build=$BUILD  cwd=$PWD  MACH_AUTO_SKIRMISH=${MACH_AUTO_SKIRMISH:-0}" | tee "$LOG"

# --batch: non-interactive. -o run: start. -k: run on crash, then quit.
lldb --batch \
  -o "process handle SIGSEGV --stop true --pass false --notify true" \
  -o "run" \
  -k "bt all" \
  -k "quit 1" \
  -- "$BIN" -w 1280 720 2>&1 | tee -a "$LOG"

echo "---- backtrace (if any) saved to scripts/crash.log ----"
