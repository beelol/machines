#!/usr/bin/env bash
#
# run-two.sh - launch two isolated local instances of Machines for multiplayer testing.
#
# Each instance gets its own writable state directory (config.xml, profiler.dat) via
# MACH_STATE_DIR so the two processes don't read/rewrite the same config and clobber it,
# while sharing the read-only assets via MACH_ROOT. Windows are placed side by side.
#
# Ctrl-C in this terminal (or the script exiting for any reason) tears down BOTH instances
# reliably: each game is exec'd as its subshell so $! is the real game PID, and the trap
# kills those PIDs on INT/TERM/EXIT.

set -u

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO="$( dirname "$SCRIPT_DIR" )"

ASSET_DIR="$REPO/machines-game-data"
BIN="${MACH_BIN:-$REPO/buildMacOS/machines}"
RUN_DIR="${MACH_RUN_DIR:-$REPO/.run}"
BASE_CONFIG="$ASSET_DIR/config.xml"

[ -x "$BIN" ]       || { echo "Binary not found/executable: $BIN (run 'make build-release')" >&2; exit 1; }
[ -d "$ASSET_DIR" ] || { echo "Asset dir not found: $ASSET_DIR" >&2; exit 1; }

# Seed each state dir with the windowed base config so neither instance starts fullscreen
# (a missing config defaults to fullscreen via the "Screen Resolution\Windowed" check).
for inst in host join; do
	mkdir -p "$RUN_DIR/$inst"
	if [ -f "$BASE_CONFIG" ]; then
		cp -f "$BASE_CONFIG" "$RUN_DIR/$inst/config.xml"
	fi
done

# Per-run log dir so a crash log survives the next launch (state/config dirs stay stable so
# settings persist between runs).
RUN_TS="$( date +%Y%m%d-%H%M%S )"
LOG_DIR="$RUN_DIR/logs/$RUN_TS"
mkdir -p "$LOG_DIR"
echo "Logs for this run: $LOG_DIR"

PIDS=()

cleanup() {
	trap - INT TERM EXIT           # disarm so cleanup runs exactly once
	echo
	echo "Shutting down both instances..."
	kill "${PIDS[@]}" 2>/dev/null
	wait 2>/dev/null
}
trap cleanup INT TERM EXIT

echo "Launching HOST instance  (state: $RUN_DIR/host, log: $LOG_DIR/host.log)"
( cd "$ASSET_DIR" && exec env MACH_ROOT="$ASSET_DIR" MACH_STATE_DIR="$RUN_DIR/host" \
	CB_ASSERT_TO="$LOG_DIR/host-assert.log" \
	MACH_WIN_X=40 MACH_WIN_Y=60 "$BIN" -w 1280 720 ) > "$LOG_DIR/host.log" 2>&1 &
PIDS+=($!)

echo "Launching JOIN instance  (state: $RUN_DIR/join, log: $LOG_DIR/join.log)"
( cd "$ASSET_DIR" && exec env MACH_ROOT="$ASSET_DIR" MACH_STATE_DIR="$RUN_DIR/join" \
	CB_ASSERT_TO="$LOG_DIR/join-assert.log" \
	MACH_WIN_X=1360 MACH_WIN_Y=60 "$BIN" -w 1280 720 ) > "$LOG_DIR/join.log" 2>&1 &
PIDS+=($!)

cat <<'EOF'

Two windows launched (HOST left, JOIN right). There is no auto host/join yet, so drive the menus:
  HOST window:  Multiplayer -> name -> "UDP connection for IP v4" -> CREATE a game
  JOIN window:  Multiplayer -> name -> "UDP connection for IP v4" -> IP is "localhost" -> select game -> JOIN

Press Ctrl-C in this terminal to close BOTH instances.
EOF

# Block until both exit (or Ctrl-C fires the trap, which kills both).
wait
