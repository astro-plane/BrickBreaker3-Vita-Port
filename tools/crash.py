#!/usr/bin/env python3
"""crash.py - read a PS Vita crash dump (psp2core-*.psp2dmp) of the Baroque loader and print symbols.

usage:  python3 tools/crash.py <dump.psp2dmp> <build/baroque (the unstripped ELF of THE SAME build)>

Prints every thread's pc/lr with function names, the eboot load base, and the return addresses found on the
crashed (first listed) thread's stack. Needs an addr2line: $VITASDK/bin/arm-vita-eabi-addr2line, or llvm-addr2line.
The ELF is linked at 0x81000000; the eboot is loaded at a different base each build (read from MODULE_INFO), so
symbol address = runtime address - (base - 0x81000000).
"""
import gzip, struct, sys, os, shutil, subprocess

def load(path):
    raw = open(path, 'rb').read()
    try: return gzip.decompress(raw)
    except OSError: return raw

def notes(d):
    e, = struct.unpack_from('<I', d, 0x1c); n, = struct.unpack_from('<H', d, 0x2c)
    segs = [struct.unpack_from('<IIIIIIII', d, e + i * 32) for i in range(n)]
    out = {}
    for p in segs:
        if p[0] != 4: continue
        nd = d[p[1]:p[1] + p[4]]; off = 0
        while off + 12 <= len(nd):
            nsz, dsz, typ = struct.unpack_from('<III', nd, off); off += 12
            name = nd[off:off + nsz].rstrip(b'\0'); off += (nsz + 3) & ~3
            out.setdefault(name, []).append(nd[off:off + dsz]); off += (dsz + 3) & ~3
    return segs, out

def main():
    if len(sys.argv) < 3: print(__doc__); return
    d = load(sys.argv[1]); elf = sys.argv[2]
    segs, nt = notes(d)
    mi = nt[b'MODULE_INFO'][0]; i = max(mi.find(b'baroque'), mi.find(b'bb3'))
    base, = struct.unpack_from('<I', mi, i + 0x34)
    delta = base - 0x81000000
    tool = None
    for t in (os.path.join(os.environ.get('VITASDK', ''), 'bin', 'arm-vita-eabi-addr2line'), 'llvm-addr2line', 'arm-none-eabi-addr2line', 'addr2line'):
        if shutil.which(t) or os.path.exists(t): tool = t; break
    def sym(a):
        if not (base <= a < base + 0x200000) or not tool: return ''
        r = subprocess.run([tool, '-f', '-e', elf, hex((a & ~1) - delta)], capture_output=True, text=True).stdout.split('\n')
        return r[0] + ' ' + (r[1] if len(r) > 1 else '')
    print('eboot base %08x (delta %x)' % (base, delta))
    regs = nt[b'THREAD_REG_INFO'][0]
    first_sp = None
    for b in range(8, len(regs) - 0x178 + 1, 0x178):
        blk = regs[b:b + 0x178]
        uid, = struct.unpack_from('<I', blk, 4)
        r = struct.unpack_from('<13I', blk, 0x18)
        sp, lr, pc, cpsr = struct.unpack_from('<4I', blk, 0x3c)
        if first_sp is None: first_sp = sp
        print('thread %08x pc=%08x %s' % (uid, pc, sym(pc)))
        print('                lr=%08x %s  sp=%08x' % (lr, sym(lr), sp))
    def rd(a, ln):
        for p in segs:
            if p[0] == 1 and p[2] <= a < p[2] + p[4]: return d[p[1] + a - p[2]:p[1] + a - p[2] + ln]
        return b''
    print('\nreturn addresses on the first thread\'s stack:')
    m = rd(first_sp, 0x800)
    for k in range(0, len(m) - 3, 4):
        v, = struct.unpack_from('<I', m, k)
        if base <= v < base + 0x120000 and (v & 1): print('  %08x %08x %s' % (first_sp + k, v, sym(v)))

main()
