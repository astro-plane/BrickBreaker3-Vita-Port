# Block Breaker 3 Unlimited (Gameloft, iPhone v1.0.2) -> PS Vita

Same Mach-O loader as the Baroque port (ObjC mini-runtime, GLES1 -> vitaGL, softfp shims), made generic:
everything game-specific is in `src/game.h`. Game files live in `ux0:data/bb3/` (the whole `Payload/BBD3.app`).

## What is different about this binary
* **Non-PIE** (built for iOS 3.1.3): no rebase info, no relocations. It expects to sit at 0x1000; we load it at
  0x98000000, so `tools/nonpie_reloc.py` synthesizes the rebase list offline (`src/reloc_gen.h`, ~47.9k words):
  - exact: local non-lazy pointers (indirect symbol table), `__mod_init_func`, `__cfstring`, jump tables (`ldr pc,[pc,rN,lsl#2]`)
  - checked heuristics: in-image words in `__DATA` (code targets must be function boundaries, string targets string starts),
    literal-pool words in `__text` loaded by `ldr rX,[pc,#]` and NOT used pc-relatively (some objects were compiled
    non-PIC), code targets from literals only if they are known function entries.
  - ARM and Thumb blocks are detected and disassembled in their own mode (Thumb: ObjC glue 0x147d4c.., GLLive/IGP/FB 0x3bcaec..).
  - `__TEXT,__const` is left alone (integer tables).
  The loader refuses a different binary (file size check). Regenerate with `python3 tools/nonpie_reloc.py BBD3 src/reloc_gen.h`.
* **ARM main** at 0x2208 (crt `start` falls through to it).
* **libstdc++ (old COW std::string)**: bound to the VitaSDK libstdc++'s old-ABI symbols (`tools/cxx_map.py` ->
  `src/cxx_map_gen.h`; on 32-bit ARM only size_t mangling differs, m -> j). C++ exceptions: a throw logs and crashes
  on purpose (SjLj unwinding not implemented).
* **Nib**: `BBD3AppDelegate` + `UIWindow` + `EAGLView` (no view controller). `initWithCoder:` schedules a one-shot
  timer -> `startGameThread` -> pthread game loop; the game thread posts work with `performSelectorOnMainThread:`
  (queue drained by the run loop).
* **Audio**: RemoteIO AudioUnit render callback, pulled by a thread into a BGM port at the game's rate (`extra_shims.c`).
* **Screen**: portrait 320x480 points @2x, letterboxed into 960x544 (rotation option to come).
* Online (GLLive, IGP, Facebook, Twitter, StoreKit, sockets): offline stubs.

## Controls
Front touch = the game's touch. Left stick / D-pad = a finger held on the paddle row (`paddley=` in vita.cfg),
cross = tap there, SELECT = free pointer mode (stick moves, cross taps). START+SELECT quits.

## vita.cfg (ux0:data/bb3/vita.cfg)
`synclog=0` buffered log (default 1 for first boots), `strictstubs=1` abort on unimplemented imports,
`hangdump=1` crash dump after 10 s without a frame, `nosound=1`, `pad=0`, `paddley=`, `padspeed=`, `verbose=1`,
plus the Baroque GL switches (`width= height= scale= fps=` ...).

## Scripts
`./install_data.sh <iphone.ipa>` (once) - `./go.sh` build + upload - `./logs.sh` fetch loader.txt.
