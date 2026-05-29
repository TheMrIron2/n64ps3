# n64ps3 Roadmap

## Phase 0: Bones

- Buildable C99 host library.
- Runtime init/shutdown.
- Logging/config/backend identity.
- OS message queue stubs.
- PI/ROM, VI, controller, audio stubs.
- `rdpnull` graphics-task logging backend.
- Samples for each early subsystem.

## Phase 1: Useful host validation

- Blocking message queues on host.
- A real monotonic timer mapped to N64-style time units.
- Controller callback interface.
- Audio callback/ring-buffer interface.
- ROM/asset provider abstraction, not only `fopen`.
- Display-list packet validation and debug dumps.

## Phase 2: N64 runtime surface

- Libultra-compatible names where useful.
- Thread bootstrap model suitable for source ports and recompilation bridges.
- DMA request callbacks and save backends.
- VI event cadence and retrace queue support.
- Controller pak/save abstractions.

## Phase 3: Graphics translation research

- Minimal Fast3D/F3DEX command decoder.
- RSP vertex transformation path in portable C first.
- RDP combine/tile/TMEM state capture.
- PC debug visualizer for sanity checks.
- Decide which work belongs on PPE, RSX shaders, or later SPE jobs.

## Phase 4: PS3 backend

- PSL1GHT build path.
- RSX init/swap/clear.
- Texture upload and swizzling experiments.
- Basic fixed-shader pipeline for transformed triangles.
- Incremental display-list translation.
- Framebuffer effects triage.

## Non-goals for now

- CPU emulation.
- Full RDP accuracy.
- Plug-and-play ROM compatibility.
- Path tracing, model replacement, or high-framerate interpolation.
- A hard dependency on RT64/libultraship internals.
