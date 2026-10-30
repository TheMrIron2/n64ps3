# n64ps3 Backends

`n64ps3` keeps libultra-style frontend code backend-neutral. Backend code stays
behind internal renderer/platform interfaces and should not leak host, SDL,
OpenGL, RSX, or PSL1GHT details into public APIs.

## rdpnull

`rdpnull` is the default PC graphics backend. It has no dependencies and logs
graphics tasks rather than rendering.

```sh
cmake -S . -B build
cmake --build build
./build/gfx_task
```

## rdppc

`rdppc` is a future optional PC debug visualizer. It should validate display
list capture, task lifetime, VI signalling, controller plumbing, and basic
rasterization before advanced PS3 rendering work. It should not become a
dependency of normal builds.

## rdprsx

`rdprsx` is an early PSL1GHT backend prototype selected by the PS3 Makefile
fragment. It initializes RSX, allocates two XRGB framebuffers and a depth
buffer, clears the active surface, and flips at vsync. Submitted tasks pass
through the portable F3DEX1 decoder; decoded triangles are collected into
RSX-resident buffers and drawn with interpolated vertex RGBA. The decoder
applies a small big-endian F3DEX1 matrix subset and supports geometry mode,
directional/ambient lighting, and lit vertex colors. The backend batches by
geometry mode to apply RSP z-buffer enable and front/back culling. RDP
render-mode depth rules, viewport conversion, textures, and other RDP state
are still absent. The prior SELF boot was visibly
validated in RPCS3; the updated vertex-color SELF now boots and runs there, but
its current capture remains covered by the PPU compilation hint. Host builds
continue to use `rdpnull`. The newest SELF adds per-triangle depth and culling
state; it is loaded at about 60 FPS, but its frame remains covered by the
compilation hint.

Next milestones:

1. visually confirm the updated vertex-color SELF in RPCS3, then on PS3 hardware
2. add host-validated viewport conversion
3. upload RGBA16 tiles and translate texture combine state for the Arwing
4. translate RDP render-mode depth rules and additional RSP/F3DEX state
5. add framebuffer/TMEM tracking when title-screen coverage requires it
