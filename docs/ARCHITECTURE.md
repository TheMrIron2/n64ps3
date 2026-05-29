# n64ps3 Architecture

`n64ps3` should be a portability runtime, not an emulator. The project boundary is source/recompilation-level code calling libultra-like services, with platform work hidden behind host and PS3 backends.

## Layers

1. **Public runtime API**: small C headers under `include/n64ps3`.
2. **Runtime services**: OS queues, timers, controllers, audio, PI/ROM, VI, save I/O.
3. **Graphics task frontend**: accepts display-list/RSP-task-like packets and records enough metadata for validation.
4. **Renderer backend**: starts with `rdpnull`, later `rdppc` for debug visualization, then `rdprsx` for PS3 RSX.
5. **Port/recomp bridge**: future wrappers for N64Recomp-style generated code or hand-written source ports.

## First compatibility target

Start with the runtime pieces that source ports want immediately:

- message queues
- timers
- controller polling
- ROM/asset reads
- audio sample submission
- VI mode/swap signalling
- graphics task submission as an opaque validation path

Do not start with full RDP accuracy. The first useful milestone is getting a small N64-style sample to submit OS messages, poll input, and hand a display-list-shaped packet to a backend.

## Graphics philosophy

The frontend should capture task/display-list boundaries without committing the public API to RSX details. RT64 is the strongest architectural reference here: it defers frame contents, RDP work, RSP transforms, texture decoding, framebuffer detection, and higher-level enhancements. `n64ps3` cannot simply copy a modern Vulkan/D3D/Metal architecture onto PS3, but it can copy the separation of concerns:

- capture a frame before rendering it
- keep RSP/RDP data visible to the backend
- track framebuffer ownership early
- treat TMEM/tiles as first-class concepts
- add PS3-specific acceleration only behind backend seams
