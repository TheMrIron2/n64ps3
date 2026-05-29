# References considered

## gcps3

Use `gcps3` as the process model: small C99 runtime, public frontend APIs, null PC backend, optional PC visualizer, and eventual RSX backend. Keep `n64ps3` secondary; borrow project discipline rather than letting it distract from `gcps3`.

## N64ModernRuntime

Useful split:

- runtime library for libultra-like services
- recompilation bridge layer
- platform callbacks for I/O
- external renderer registration

This suggests `n64ps3` should avoid owning everything. It should expose seams for controller/audio/storage/renderer callbacks early.

## RT64

Useful ideas:

- deferred frame capture
- deferred RDP operations
- RSP transform work as a backend concern
- texture/TMEM decoding and caching as a distinct subsystem
- framebuffer detection and copy tracking
- extended command concepts for source ports or patches

Do not assume PS3 can mirror RT64's modern GPU architecture. Copy the data model and debugging posture, not the API stack.

## libultraship

Useful ideas:

- source-port-friendly libultra replacement
- assets separated from executable data
- mod/patch-oriented asset packaging
- broad platform abstraction

For PS3, a lightweight `.zip` or directory-backed asset provider is a better early target than adopting a full archive stack immediately.
