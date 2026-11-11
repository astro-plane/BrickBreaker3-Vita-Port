# Block Breaker 3 Unlimited - PS Vita port

Runs the iPhone version of Gameloft's **Block Breaker 3 Unlimited** (v1.0.2, `com.gameloft.BBD3`) natively on the
PS Vita through a Mach-O loader: the original ARM code runs as-is, with iOS/UIKit/ObjC/OpenGL ES 1.1 calls shimmed
onto the Vita (vitaGL, newlib, sceAudio).

**No game files are included.** You need your own decrypted iPhone IPA of the game.

## Status
- Boots, menus and gameplay work; gameplay runs at 60 fps.
- Portrait game shown sideways to fill the screen (hold the Vita like a book). Touch and stick are rotated to match.
- Sound effects and music (RemoteIO AudioUnit -> sceAudio).
- Known: switching menu screens pauses 1.5-2.5 s (the game decodes its images on the CPU); online features
  (GLLive, Facebook, Twitter, store) are disabled.

## Build (macOS/Linux, VitaSDK)
Requires VitaSDK with vitaGL, vitashark, mathneon and kubridge (`kubridge.skprx` must be installed on the Vita).

```
./install_data.sh "/path/to/Block Breaker 3 Unlimited.ipa"   # once: copies the app to ux0:data/bb3/ over FTP
./go.sh                                                        # build + upload (first time: install bb3.vpk with VitaShell)
./logs.sh                                                      # fetch ux0:data/bb3/loader.txt
```
Set `VITA=<ip>` if your Vita is not at 192.168.0.19 (VitaShell FTP, port 1337).

## Controls
- Touch: the game's own touch controls.
- Left stick / D-pad: a virtual finger on the paddle row; Cross taps there.
- Select: free pointer mode (stick moves, Cross taps). Start+Select: quit.

## ux0:data/bb3/vita.cfg
| key | meaning |
|---|---|
| `rotate=1` / `2` / `0` | sideways, game top at the Vita's right edge (default) / left edge / upright letterboxed |
| `barcache=0` | do not load the .bar archives into RAM |
| `nosound=1` | no audio |
| `synclog=1` | write every log line immediately (crash hunting, slow) |
| `strictstubs=1` | abort on unimplemented imports |
| `pad=0`, `paddley=`, `padspeed=` | virtual finger options |

## How it works (short)
- `src/macho.c`, `src/vita_main.c`: maps the Mach-O at 0x98000000 with kubridge, binds imports, runs static
  initializers and `main`.
- The binary is **non-PIE** (built for iOS 3.1.3) and has no relocation info. `tools/nonpie_reloc.py` synthesizes
  the rebase list offline (`src/reloc_gen.h`, ~49.9k words). It covers data pointers, non-PIC code literal pools,
  jump tables, import stubs and vtables, with separate ARM/Thumb analysis. It is matched to the exact v1.0.2 binary
  by file size.
- `src/objc_rt.c`: minimal ObjC runtime (MainWindow.nib emulation, `.cxx_construct`, main-thread queue).
- `src/extra_shims.c`: libstdc++ bound to VitaSDK's old-ABI COW `std::string` (`tools/cxx_map.py`), SjLj hooks,
  setjmp/longjmp, AudioUnit, CoreFoundation bits.
- `src/shims.c`: every import goes through a stack-aligning trampoline (iOS keeps sp 4-byte aligned, Vita libraries
  assume 8).
- `src/gl_shims.c`: GLES 1.1 -> vitaGL, plus the rotated off-screen presentation.

Based on the loader from a Baroque Vita port by the same author. Not affiliated with Gameloft.
