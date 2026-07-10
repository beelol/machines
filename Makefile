# Convenience wrapper around the CMake build for the native macOS port.
# This does NOT replace CMake — it just builds the existing build dirs and
# runs the game with MACH_ROOT wired up so assets resolve.

# Homebrew cmake is not always on the default PATH.
export PATH := /opt/homebrew/bin:$(PATH)

CMAKE       ?= cmake
JOBS        ?= 8

# Asset root. The engine opens most bitmaps via paths relative to the CURRENT
# DIRECTORY, so the game must be RUN FROM here (not the repo root). MACH_ROOT
# additionally prefixes SysPathName-resolved paths.
ASSET_DIR   := $(CURDIR)/machines-game-data

# Build directories (already configured; see CMakeCache.txt in each).
DEBUG_DIR   := buildMacOSDebug
RELEASE_DIR := buildMacOS
DEBUG_BIN   := $(CURDIR)/$(DEBUG_DIR)/machines
RELEASE_BIN := $(CURDIR)/$(RELEASE_DIR)/machines

# Windowed geometry passed to the binary.
WIN         ?= -w 1280 720

.DEFAULT_GOAL := help

## dev: build + run the release binary from the asset dir (the playable path)
.PHONY: dev
dev: run

## run: build release, then run it from the asset dir (reaches the menus)
.PHONY: run
run: build-release
	cd "$(ASSET_DIR)" && MACH_ROOT="$(ASSET_DIR)" "$(RELEASE_BIN)" $(WIN)

## run-two: build release, then launch two isolated instances (side-by-side windows,
##          separate MACH_STATE_DIR each) for local multiplayer testing. Ctrl-C closes both.
.PHONY: run-two
run-two: build-release
	scripts/run-two.sh

## debug: build + run the DEBUG binary from the asset dir. Note: debug aborts on
##        debug-only asset preconditions during startup preload that release skips,
##        so it is for diagnosis/backtraces, not for reaching the menus.
.PHONY: debug
debug: build
	cd "$(ASSET_DIR)" && MACH_ROOT="$(ASSET_DIR)" "$(DEBUG_BIN)" $(WIN)

## build: compile the debug build
.PHONY: build
build: | $(DEBUG_DIR)/CMakeCache.txt
	$(CMAKE) --build $(DEBUG_DIR) -j$(JOBS)

## crash: build release, run under lldb; navigate to the crash. Dumps a backtrace to scripts/crash.log and closes.
.PHONY: crash
crash: build-release
	BUILD=release scripts/crashrun.sh

## crash-skirmish: like crash, but auto-jumps into the skirmish screen on boot
.PHONY: crash-skirmish
crash-skirmish: build-release
	BUILD=release MACH_AUTO_SKIRMISH=1 MACH_RUN_TIME=30 scripts/crashrun.sh

## release: compile and run the optimized build from the asset dir (same as run)
.PHONY: release
release: run

## build-release: compile the optimized build only
.PHONY: build-release
build-release: | $(RELEASE_DIR)/CMakeCache.txt
	$(CMAKE) --build $(RELEASE_DIR) -j$(JOBS)

## configure: (re)generate both build dirs from scratch
.PHONY: configure
configure:
	$(CMAKE) -S . -B $(DEBUG_DIR)   -DCMAKE_BUILD_TYPE=Debug
	$(CMAKE) -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release

# Auto-configure a build dir if its cache is missing (e.g. after a clean clone).
$(DEBUG_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(DEBUG_DIR) -DCMAKE_BUILD_TYPE=Debug

$(RELEASE_DIR)/CMakeCache.txt:
	$(CMAKE) -S . -B $(RELEASE_DIR) -DCMAKE_BUILD_TYPE=Release

## help: list targets
.PHONY: help
help:
	@echo "Targets:"
	@grep -E '^## ' $(MAKEFILE_LIST) | sed 's/^## /  /'
