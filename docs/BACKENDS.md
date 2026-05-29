# n64ps3 Backends

`n64ps3` keeps libultra-style frontend code backend-neutral. Backend code should stay behind internal renderer/platform interfaces and should not leak host, SDL, OpenGL, RSX, or PSL1GHT details into public APIs.

## rdpnull

`rdpnull` is the default PC graphics backend. It has no dependencies and logs graphics tasks rather than rendering.

```sh
cmake -S . -B build
cmake --build build
./build/gfx_task
```

## rdppc

`rdppc` is a future optional PC debug visualizer. It should be used to validate display-list capture, task lifetime, VI signalling, controller plumbing, and basic rasterization experiments before PS3 work.

It should not be treated as the final renderer architecture and should not become a dependency of normal builds.

## rdprsx

`rdprsx` is the planned PS3 RSX backend. Add it only after the host frontend and PC validation path are stable enough to justify PS3-specific work.

Early `rdprsx` should prefer simple, explicit milestones:

1. clear/swap a VI-sized buffer
2. draw a hard-coded triangle
3. upload a small texture
4. consume a frontend-captured display-list-shaped packet
5. add RSP/F3DEX translation incrementally
6. add framebuffer/TMEM tracking only when sample coverage demands it
