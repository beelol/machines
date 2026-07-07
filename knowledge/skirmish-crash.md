# Skirmish menu crash (macOS/arm64 port)

Status: 3 cross-platform bugs fixed; user's release-build skirmish crash not yet reproduced in-agent (environment can't drive the GUI to the menu).

## How to run / catch crashes
- `make dev` — build+run debug from repo root (MACH_ROOT wired).
- `make crash` — build release, run under lldb; navigate to the crash. Dumps `scripts/crash.log` and closes on any crash.
- `make crash-skirmish` — same, but auto-opens the skirmish screen (`MACH_AUTO_SKIRMISH=1`).
- The game resolves asset paths relative to CWD, so it must run from `machines-game-data/` (the scripts cd there). `MACH_ROOT` covers `SysPathName`-resolved paths.
- Debug asserts on many debug-only preconditions during startup preload that release skips → **release is the faithful build** for reaching menus.

## Dev hooks (remove when done)
- `MACH_AUTO_SKIRMISH=1` — `startup.cpp` `loopCycle()` one-shot `switchContext(CTX_SKIRMISH)` for crash-repro without clicking.

## Bugs fixed
1. **OOB skirmish CUSTOM system** — `ctxskirm.cpp:updateMapSizeList()` loops `SMALL..CUSTOM(3)` but `skirmishSystems_` held only 3 (SMALL/MEDIUM/LARGE); `CUSTOM` was created only when `data/customscen.dat` exists (it doesn't). Fix: `database.cpp readDatabase()` always creates a (possibly empty) CUSTOM system; removed the conditional push in `parseUserCampaignFile()`; `databasi.cpp` reserve 3→4.
2. **Infinite recursion in `SysPathName` assert logging → stack overflow** — `pathname.cpp` `isAbsolute()`/`isRelative()` did `PRE_INFO(*this)`. `LOG_INFO` streams the expr immediately via `operator<<`, which calls `pathname()`→`isRelative()`→`isAbsolute()`→`PRE_INFO(*this)`→... Cut by logging raw `pathname_` string instead of `*this`. Triggers whenever a path assert fires with `MACH_ROOT` set (internal root dir active).
3. **DOS 8.3 `validFileName`** — `prepost.cpp` rejected any path component >8 chars (e.g. `machines-game-data`) / ext >3, so debug `ASSERT_FILE_EXISTS` reported valid files as "File name invalid". Relaxed to non-empty check (the ifstream open validates existence).

## Bugs fixed (cont.)
4. **Release-build `MachLogRaces` ctor trap (`brk #0x1`)** — NOT env-specific. Root cause: `MachPhys::Race` (and sibling enums) were narrowed to `: unsigned char` "to fix persistence", but the codebase iterates them with `++((int&)x)` — a 4-byte read-modify-write through an `int&` onto a 1-byte object (OOB stack write + strict-aliasing UB). At `-O2` the optimizer exploits the UB and plants a trap where the ctor's return should be; `-O0` (debug) doesn't, so it looked environmental. Fix: replaced all 64 `++((int&)x)`/`--((int&)x)` sites across 20 files with the width-safe, value-identical `x = static_cast<decltype(x)>(x ± 1)` (keeps enums 1-byte, so persistence/network layout unchanged; determinism preserved). Verified: release build now runs past `MachLogRaces::MachLogRaces()` under lldb.

## Open / next
- Debug still asserts in texture preload during startup; release now reaches the GUI. Capture any real skirmish backtrace with a desktop-session `make crash` run.
