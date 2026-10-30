# n64ps3 Roadmap

## Phase 0: Bones

- Buildable C99 host library.
- Runtime init/shutdown.
- Logging/config/backend identity.
- OS message queue stubs.
- PI/ROM, VI, controller, and audio stubs.
- `rdpnull` graphics-task logging backend.
- Samples for early subsystems.

## Phase 1: Useful Host Validation

- Blocking message queues on host.
- A real monotonic timer mapped to N64-style time units.
- Controller callback interface.
- Audio callback/ring-buffer interface.
- ROM/asset provider abstraction, not only `fopen`.
- Display-list packet validation and debug dumps.

## Phase 2: N64 Runtime Surface

- Libultra-compatible names where useful.
- Thread bootstrap model suitable for source ports and recompilation bridges.
- DMA request callbacks and save backends.
- Backend-driven VI event cadence and retrace queue support.
- Controller pak/save abstractions.

## Phase 3: Graphics Translation Research

- [x] Bounded F3DEX1 decoding for vertices, triangles, nested display-list
  flow, segment bases, and the initial matrix load/multiply/push/pop subset.
- [x] Portable fixed-point N64 matrix decoding and vertex transformation to
  clip coordinates, with host sample coverage.
- [x] F3DEX1 geometry mode, directional/ambient light commands, and host-checked
  lit vertex color generation; preserve the active geometry mode on triangle
  callbacks.
- Viewport conversion and broader RSP state.
- RDP combine/tile/TMEM state capture.
- PC debug visualizer for sanity checks.
- Decide which work belongs on PPE, RSX shaders, or later SPE jobs.

## Phase 4: PS3 Backend

- [x] PSL1GHT Makefile path selects a prototype RSX backend.
- [x] RSX context, current-mode framebuffers, clear, and synchronized flip.
- [x] Decoded task triangles submitted with per-vertex RGBA by the RSX backend.
- [x] Apply F3DEX `G_ZBUFFER` depth compare/write and front/back culling per
  geometry-mode batch.
- [x] Boot the normal PSL1GHT SELF and run the PPU main loop in RPCS3.
- [x] Visually validate the prior task-fed triangle output in RPCS3.
- [x] Boot the updated vertex-color SELF and confirm PPU compilation and
  repeated F3DEX1 task execution in RPCS3.
- [ ] Visually confirm the updated vertex-color output and repeat on PS3 hardware.
- Texture upload and swizzling experiments.
- Incremental display-list state and additional RSP matrix/viewport semantics.
- Framebuffer effects triage.

## Non-goals for Now

- CPU emulation.
- Full RDP accuracy.
- Plug-and-play ROM compatibility.
- Path tracing, model replacement, or high-framerate interpolation.
- A hard dependency on RT64/libultraship internals.
