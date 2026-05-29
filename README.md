# n64ps3

`n64ps3` is an experimental Nintendo 64-to-PlayStation 3 portability runtime.

It is **not** an emulator - not an R4300i CPU emulator, and not a ParaLLEl/RT64 port. The goal is to provide a libultra-style runtime surface that source ports, recomp projects and small samples can target while keeping host and PS3 backends cleanly separated.

`n64ps3` imitates the groundwork of `gcps3`: small C99 runtime, backend-neutral frontend code, dependency-free host validation first, and PS3/RSX work only after the API seams are stable.

## Current scaffold

The repository currently contains a host-buildable C99 skeleton with:

- core runtime init/shutdown
- logging and configuration
- backend identity hooks
- minimal OS message-queue and timer stubs
- controller, audio, PI/ROM, and VI stubs
- graphics-task frontend with a null PC renderer backend named `rdpnull`
- sample programs for runtime init, OS queues, and graphics-task submission

There is no RSP, RDP, RSX renderer, libultra compatibility layer, audio mixer, save backend, controller implementation, recompilation bridge, or commercial game support yet.

## Objectives

- Provide libultra-style APIs for portable N64 source-level ports and future recompilation bridges.
- Keep frontend runtime code backend-neutral.
- Build a PS3 RSX renderer backend named `rdprsx` after the graphics-task frontend has useful validation coverage.
- Use PC backends for fast validation before PS3 hardware work.
- Treat RSP/RDP as task/display-list translation boundaries rather than as a CPU-emulation project.
- Feed practical PS3 findings back into PSL1GHT and shared PS3 homebrew tooling.

## Host build

```sh
cmake -S . -B build
cmake --build build
./build/hello_runtime
./build/os_queue
./build/gfx_task
```

## References

- `gcps3`: frontend/backend split, null host backend, incremental samples, later RSX backend.
- N64ModernRuntime: libultra-like runtime services plus platform callbacks and renderer registration.
- RT64: deferred frame/task capture, texture/TMEM awareness, framebuffer tracking, high-level display-list architecture.
- libultraship: source-port-friendly runtime layer, explicit asset/data packaging, mod-friendly boundaries.
