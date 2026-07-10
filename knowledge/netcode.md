# Netcode: model, problems, remediation

## Model (as-is)
Machines multiplayer is **not** deterministic lockstep. It is peer-to-peer **event
replication**: each machine runs its own **wall-clock-driven, double-precision** simulation
and broadcasts state-change events (create actor, move, be-hit, fire, motion chunks, ~55
message types) as they happen. The host relays each received packet to all other peers.
Loose alignment is maintained by the host broadcasting its clock every 5s; clients snap to it.

Key code:
- Per-frame loop: `machgui/startup.cpp:1637` `loopCycleInGame` → `MachLogNetwork::update()`
  (poll) + `SimManager::cycle()` (advance by real wall-clock delta, `sim/manager.cpp:98,201`).
- Command protocol / dispatch: `machlog/messbrok.cpp`, `messbro1.cpp`, `messbro2.cpp`
  (`MachLogMessageBroker`). Transmit via `doSend` → `NetNetwork::sendMessage`.
- Transport (ENet/UDP, ex-DirectPlay): `network/netinet.cpp` (`NetINetwork`). Send =
  `enet_host_broadcast`; receive+relay = `pollMessages`.
- Resync: `machlog/messbro1.cpp:906-956`; hard clock set `sim/manager.cpp:338`.

## Root causes
**Lag (even on LAN):** (1) modem-era throttle (40 pkt/s, 6 KB/s) active for every net game
(`startup.cpp:821`) and applied to LAN because ENet reports UDP not IPX → orders cached and
drip-fed; (2) `enet_host_flush` never called → orders wait a frame; (3) host relay = double
hop; (4) no-op `SysWindowsAPI::sleep` → CPU-pegged busy loop starves the network stack;
(5) latency-blind 5s clock snap (600 ms threshold) → periodic rubber-band.

**Desync (cross-platform guaranteed):** not lockstep + non-portable unseeded libc `rand()`
used ~120× in `machlog` (`machphys/random.cpp`); wall-clock-seeded LCG inside pathing
(`mcmotseq.cpp`, `mcmotse2.cpp`, `optskatt.cpp`); `double` math diverges across CPU/compiler;
raw-`memcpy` serialization with `long`/`size_t` fields (32-bit Windows vs 64-bit macOS).

## Remediation plan (4 phases)
Full plan: `~/.claude/plans/there-are-insane-network-fancy-gem.md`. Scope: full incl.
lockstep; **cross-platform (macOS arm64 ↔ Windows) required.**

- **Phase 1 — lag quick wins — DONE (builds on macOS; Windows build pending Docker).**
  - 1a throttle: `netinet.cpp` `maxSentMessagesPerSecond()` — ENet transports (IPX/UDP/TCPIP)
    now treated high-capacity (1000 pkt/s, 1 MB/s defaults; raised clamp floors so stale
    registry values can't pin low).
  - 1b flush: added `NetINetwork::flush()`/`NetNetwork::flush()` (`enet_host_flush`); called
    at end of `MachLogNetwork::update()` and after the host relay loop in `pollMessages`.
  - 1c frame limiter: `SysWindowsAPI::sleep` now uses `SDL_Delay`; 60 FPS cap in
    `afx/sdlapp.cpp` `coreLoop`.
  - 1d second network pump after `SimManager::cycle()` in `loopCycleInGame` so orders leave
    same-frame.
- **Phase 2 — latency-compensated + slewed clock resync — DONE (builds on macOS).**
  - `messbro1.cpp` `processResyncTimeMessage`: add `sender()->roundTripTime/2` to the host
    timestamp (kills the constant client-behind-host offset); slew the clock by 0.5× the
    error when within `[0.1s, 3×resyncThreshold]`, hard-snap only above that.
  - `startup.cpp`: resync interval lowered 5s → 2s (smaller, smoother corrections).
- **Phase 3 — cross-platform determinism — DONE (builds on macOS; Windows build in progress).**
  - 3a: `MexBasicRandom` state now `uint32` (was `ulong` → different LCG sequence per platform).
    `MachPhysRandom` no longer uses libc `rand()`; it draws from one shared `MexBasicRandom`
    (`machphys/random.cpp` `sharedRandom()`), with `MachPhysRandom::seed(uint32)`. Host
    generates a session seed once and broadcasts it in the START_GAME message
    (`MachLogReadyMessage.randomSeed_`, added); every peer seeds before the sim resumes
    (`messbro1.cpp` send/processStartGameMessage). The 4 wall-clock `seedFromTime()` pathing
    reseeds (`mcmotseq.cpp` ×2, `mcmotse2.cpp`, `optskatt.cpp`) now use the shared stream.
  - 3b: audit found the wire structs already portable — `messages.hpp` is fully
    `#pragma pack(1)`; fields are int/unsigned/`UtlId`(=unsigned)/IEEE float/double (identical
    on both ABIs); the only `size_t`/pointer live in the local-only `NetMessageHeader`. Real
    width bug was the RNG state (fixed in 3a).
  - CAVEAT: START_GAME wire format grew 4 bytes. No version negotiation exists (`setAppUid`
    stubbed) — all peers must run the same build (already true for a source port).
  - CAVEAT: not lockstep yet, so RNG call-count still skews between peers over time; Phase 4
    closes that. 3a removes the cross-platform + replay divergence class.
- **Phase 4 — lockstep conversion (fixed tick, command scheduling, fixed-point math, retire
  event echo + add desync checksum) — TODO; re-scope after 1–3 ship.**

## Local two-instance testing (env-path isolation, native — no Docker)
Two instances on one Mac can join a localhost match. The blocker was a shared, cwd-relative
`config.xml` both processes rewrite. Fix: env-driven writable-state paths (defaults unchanged):
- `MACH_STATE_DIR` — per-instance writable dir; `config.xml` + `profiler.dat` go there
  (`system/registry.cpp` `resolveConfigPath()`, `profiler/profiler.cpp` `profilerOutputPath()`).
  `MACH_CONFIG` overrides the config path outright. Assets stay shared via `MACH_ROOT`.
- `MACH_WIN_X` / `MACH_WIN_Y` — SDL window position (`afx/sdlapp.cpp`), else centred.
- `scripts/run-two.sh` (+ `make run-two`): seeds two isolated state dirs from the base
  (windowed) config, launches host+join side by side, and reliably kills BOTH on Ctrl-C
  (trap INT/TERM/EXIT; each game `exec`'d so `$!` is the real PID). `.run/` is gitignored.
- No auto host/join yet — drive menus (host: Multiplayer→UDP→CREATE; join: UDP→IP `localhost`
  →JOIN). Verified: config/profiler land in `MACH_STATE_DIR`, shared asset config untouched,
  teardown kills both. Playability still depends on MP scenario assets being complete.

## Multiplayer bugs found via two-instance testing (both fixed)
**Start-game crash (was the "skirmish crash" too).** Only 2 of 47 planet dirs in the asset set
ship a `.env` (`1o1`, `m_desert`); MP/skirmish planets have none. `EnvIPlanetParser::parse`
then completes no sky, `EnvPlanetEnvironment::sky_` stays null, and `visibleStars()`
(`envirnmt/planet.cpp:691`) derefs `sky_->pStars()` → SIGSEGV. The file-exists and null asserts
(`plaparse.cpp:81`, `planet.cpp:121`) are compiled out in release. Fixes: `machphys/plansurf.cpp`
falls back to `models/planet/m_desert/m_desert.env` when a planet's own `.env` is missing (the
original engine had no default-env concept — each planet shipped its own); plus null-guards on the
`sky_->` derefs in `planet.cpp` (`visibleStars` :691, NVG override/reset :377/:390) as
defense-in-depth.

**Joiner's lobby roster blank (chat worked).** `netinet.cpp pollMessages` treats each peer's
first packet as its "introduction name" and drops it. The host never sends the client a name
packet, so the host's first real message — the roster sync — was swallowed on the client. Fix:
`joinAppSession` seeds the host peer's `pPeer->data` with a deletable `_NEW_ARRAY` placeholder
right after sending our name, so the client enqueues all host messages instead of eating the first.

`scripts/run-two.sh` now captures each instance's stdout/stderr to `$RUN_DIR/<inst>/output.log`
and sets `CB_ASSERT_TO` per instance (this crash was only diagnosable via the macOS `.ips` report).

## Second playtest bugs (crash fixed; 2 behavioural bugs instrumented)
**Build crashes the other client (crash fixed).** A remote-built construction is a
`W4dSubject`+`W4dObserver` parented under a planet `W4dDomain`. On scene teardown, `~W4dDomain`
frees its `pImpl_` (`world4d/domain.cpp:71`) before its base `~W4dEntity` re-parents a counted-ptr
child to `hiddenRoot`, which calls `W4dSubjectImpl::updateDomainObservers` → `observers()` on a
NULL new-domain (`subjecti.cpp:89`) / freed old-domain (`:100`) → SIGSEGV at `0x10`. Fixed by
null-guarding both domains at `subjecti.cpp:81`. Hardened the remote-build receive path too:
skip duplicate-id creates in all builds (`messbro1.cpp` `processCreateActorMessage`), null-check
`pDomainPosition` (`plandoms.cpp`) and `newLogConstruction` result (`actmaker.cpp`).

**Host can't command own units to move/locate (instrumented, not yet fixed).** Move
(`cmdmove.cpp:628`) and locate (`cmdlocto.cpp:234`) arm via `activeCommand()`, blocked when
`SimManager::isSuspended() or isNetworkStuffed()` (`ingame.cpp:834`). Name-based `localRace`
mis-tag is ruled out (distinct names, host race = PC_LOCAL); resync suspend/resume is balanced.
Leading suspect: `isNetworkStuffed()` latching on the relay host. Added always-on `std::cerr`
diagnostics (release strips `NETWORK_STREAM`): `[cmd-blocked]` logs which gate fired
(`ingame.cpp`), `[mp-ownership]` dumps localRace + per-race PC_LOCAL/REMOTE tags at game start
(`startup.cpp`), `[leave-game]` logs session-lost/terminate (`sysmess.cpp`). `run-two.sh` now
writes timestamped per-run logs under `.run/logs/<ts>/` so a crash log survives the next launch.
Next: one `make run-two` playtest reads these logs to pin the gate, then relax the
`ingame.cpp:834` check (a full send queue should pace outbound traffic, not reject local input).

## Verification notes
- macOS build: `cd buildMacOS && make -j8`. Windows: `DOCKER_API_VERSION=1.44 docker/docker_build_win64.sh`.
- Phases 1-3 verified building on BOTH macOS arm64 and Windows x86_64/mingw (machines.exe built).
- Dockerfile was fixed to build on Apple Silicon: `FROM --platform=linux/amd64` + apt
  `--force-confold`/`confdef` (the pre-edited /etc/sudoers otherwise triggers an interactive
  dpkg prompt that fails the non-interactive build). Needs `DOCKER_API_VERSION=1.44` if the
  Docker CLI is older than the daemon.
- Runtime A/B is limited in headless env (game needs art assets); rely on build-clean +
  `NET_ANALYSIS_STREAM` in/out logs (`machlog/messbrok.cpp:60,272`) when runnable.
