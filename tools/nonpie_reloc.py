#!/usr/bin/env python3
"""Synthesize rebase info for a NON-PIE iOS armv7 executable (no LC_DYLD_INFO rebase opcodes, no local relocations).
Such a binary expects to sit at its link address (0x1000); on the Vita it is loaded elsewhere, so every absolute
pointer to the image must be found and slid. Sources, exact first:
  - __nl_symbol_ptr entries marked INDIRECT_SYMBOL_LOCAL (indirect symbol table)
  - __mod_init_func, __cfstring string pointers, absolute jump tables (ldr pc, [pc, rN, lsl #2]) in the code
Heuristic (checked):
  - words in __DATA sections whose value lands in the image AND whose target makes sense
    (code: a function boundary - prologue or right after a return/branch/literal pool; strings: a string start)
  - literal-pool words in __text loaded by `ldr rX, [pc, #imm]` and then NOT used pc-relatively (those come from
    objects compiled non-PIC), with the same target checks; code targets additionally must not be used as plain integers
__TEXT,__const is left alone (in this binary the in-range words there are integer tables).
usage: nonpie_reloc.py BINARY out_header.h"""
import struct, sys, re, subprocess, collections, hashlib

B, OUT = sys.argv[1], sys.argv[2]
d = open(B, 'rb').read()
ncmds = struct.unpack_from('<I', d, 16)[0]; off = 28; segs = []; secs = []; indoff = 0; rebase_size = None
for _ in range(ncmds):
    cmd, sz = struct.unpack_from('<II', d, off)
    if cmd == 1:
        vm, vs, fo, fs = struct.unpack_from('<IIII', d, off + 24); segs.append((vm, vs, fo, fs)); ns = struct.unpack_from('<I', d, off + 48)[0]
        for k in range(ns):
            s = off + 56 + k * 68; sn = d[s:s + 16].rstrip(b'\0').decode(); sg = d[s + 16:s + 32].rstrip(b'\0').decode()
            a, z, fo2 = struct.unpack_from('<III', d, s + 32); fl, r1 = struct.unpack_from('<II', d, s + 56)
            secs.append(dict(name=sn, seg=sg, addr=a, size=z, flags=fl, r1=r1))
    elif cmd == 0xb: indoff = struct.unpack_from('<I', d, off + 56)[0]
    elif cmd in (0x22, 0x80000022): rebase_size = struct.unpack_from('<I', d, off + 12)[0]
    off += sz
if rebase_size:
    print('binary has rebase info - nothing to do'); sys.exit(1)
VM_LO = min(s[0] for s in segs if s[1] and s[0] > 0) if False else 0x1000
VM_HI = max(s[0] + s[1] for s in segs if s[2] or s[3]) if False else None
VM_HI = max(s['addr'] + s['size'] for s in secs)
def fo(vm):
    for a, s, f, fs in segs:
        if a <= vm < a + fs: return f + vm - a
def u32(vm):
    o = fo(vm); return struct.unpack_from('<I', d, o)[0] if o is not None else None
def sec(a):
    for s in secs:
        if s['addr'] <= a < s['addr'] + s['size']: return s
def secname(a):
    s = sec(a); return (s['seg'] + ',' + s['name']) if s else None
TEXT = next(s for s in secs if s['name'] == '__text')
def w32(v): return struct.unpack_from('<I', d, fo(v))[0]
def fn_boundary(v):
    if v & 1: return in_thumb(v & ~1)            # Thumb function pointer: must land in a Thumb block
    if v & 3 or in_thumb(v): return False
    if (w32(v) & 0xffff0000) == 0xe92d0000: return True          # push {..}
    if v - 4 < TEXT['addr']: return True
    p = w32(v - 4); c = p >> 28
    if p in (0xe12fff1e, 0xe1a00000): return True                # bx lr / nop padding
    if (p & 0x0fff8000) == 0x08bd8000: return True               # pop {.., pc}
    if (p & 0x0e108000) == 0x08108000: return True               # ldm .., {.., pc}
    if (p & 0x0f000000) == 0x0a000000 and c == 0xe: return True  # b
    if (p & 0x0ffffff0) == 0x012fff10 and c == 0xe: return True  # bx rN
    if c not in (0xe, 0xf): return True                          # literal pool / data before the function
    return False
def str_start(v):
    s = sec(v)
    return v == s['addr'] or d[fo(v) - 1] == 0

# ---- ARM / Thumb ranges of __text: ARM words mostly carry condition 0xE; Thumb blocks are confirmed by Thumb push {lr}
#      prologues outnumbering ARM ones and by odd (Thumb) function pointers in the data. Small "non-ARM" runs inside ARM
#      code are data (tables / literal pools / NEON). ----
def find_ranges():
    t0, t1 = TEXT['addr'], TEXT['addr'] + TEXT['size']; W = 32
    ws = [w32(a) for a in range(t0, t1 - 3, 4)]
    ps = [0]
    for w in ws: ps.append(ps[-1] + ((w >> 28) == 0xE))
    lab = [(ps[min(len(ws), i + W)] - ps[max(0, i - W)]) / (min(len(ws), i + W) - max(0, i - W)) > 0.45 for i in range(len(ws))]
    runs = []; cur = lab[0]; st = 0
    for i, l in enumerate(lab + [not lab[-1]]):
        if i == len(lab) or l != cur: runs.append((cur, t0 + 4 * st, t0 + 4 * i)); cur = l; st = i
    oddp = set()
    for sx in secs:
        if sx['seg'] != '__DATA': continue
        for a in range(sx['addr'], sx['addr'] + sx['size'] - 3, 4):
            v = u32(a)
            if v and (v & 1) and t0 <= v < t1: oddp.add(v)
    thumb, datar = [], []
    for armish, a0, a1 in runs:
        if armish: continue
        armp = sum(1 for a in range(a0, a1, 4) if (w32(a) & 0xffff0000) == 0xe92d0000)
        thp = sum(1 for a in range(a0, a1, 2) if (struct.unpack_from('<H', d, fo(a))[0] & 0xff00) == 0xb500)
        no = sum(1 for v in oddp if a0 <= v < a1)
        if thp > armp and no and thp >= 3: thumb.append([a0, a1])
        else: datar.append((a0, a1))
    # Snap the edges (window smoothing blurs them by up to ~W words): a Thumb block starts at its first Thumb function
    # (lowest odd pointer near/in it) and ends before the first known ARM function after its last Thumb function.
    # Known ARM entries: even code pointers in the data, static initializers, ARM prologues right after a return.
    armk = set()
    for sx in secs:
        if sx['seg'] == '__DATA' and sx['name'] not in ('__la_symbol_ptr', '__common', '__bss'):
            for a in range(sx['addr'], sx['addr'] + sx['size'] - 3, 4):
                v = u32(a)
                if v and not (v & 3) and t0 <= v < t1: armk.add(v)
    for a in range(t0 + 4, t1 - 3, 4):
        if (w32(a) & 0xffff0000) == 0xe92d0000 and w32(a - 4) in (0xe12fff1e,) or ((w32(a) & 0xffff0000) == 0xe92d0000 and (w32(a - 4) & 0x0fff8000) == 0x08bd8000):
            armk.add(a)
    merged = []
    for r in sorted(thumb):
        if merged and r[0] - merged[-1][1] < 0x400: merged[-1][1] = max(merged[-1][1], r[1])
        else: merged.append(list(r))
    out = []
    for r in merged:
        odds = sorted(v & ~1 for v in oddp if r[0] - 0x400 <= v < r[1] + 0x400)
        if not odds: continue
        lo, hiodd = odds[0], odds[-1]
        nxt = [k for k in armk if k > hiodd]
        hi = min(nxt) if nxt and min(nxt) < hiodd + 0x4000 else r[1]
        # ARM entries inside [lo, hi) mean ARM code is interleaved: split the block there
        inner = sorted(k for k in armk if lo < k < hi)
        cur = lo
        for k in inner:
            seg_odds = [v for v in odds if cur <= v < k]
            if seg_odds: out.append([cur, k])
            nxto = [v for v in odds if v > k]
            if not nxto: cur = None; break
            cur = nxto[0]
        if cur is not None and cur < hi: out.append([cur, hi])
    thumb = out
    return [tuple(r) for r in thumb], datar
THUMB_R, DATA_R = find_ranges()
def in_thumb(a): return any(lo <= a < hi for lo, hi in THUMB_R)
def near_edge(a, m=0x8): return any(abs(a - lo) < m or abs(a - hi) < m for lo, hi in THUMB_R)
def target_ok(v, from_code=False):
    if not (VM_LO <= v < VM_HI): return False
    s = sec(v & ~1)
    if not s: return False
    n = s['name']; sg = s['seg']
    if n == '__text': return fn_boundary(v)
    if n in ('__symbol_stub4', '__stub_helper'): return (v & 3) == 0
    if n in ('__cstring', '__ustring'): return from_code or str_start(v)   # code use is confirmed; data must hit a string start
    if n == '__gcc_except_tab': return from_code
    return True
rel = set(); why = collections.Counter()
# ---- exact: local non-lazy pointers ----
for s in secs:
    if s['name'] in ('__nl_symbol_ptr', '__la_symbol_ptr'):
        # local entries, and lazy pointers already resolved to a function inside the binary (weak/local definitions:
        # no bind entry). Slots that ARE bound get overwritten by the bind afterwards, so relocating them is harmless.
        for i in range(s['size'] // 4):
            a = s['addr'] + 4 * i; v = u32(a)
            idx = struct.unpack_from('<I', d, indoff + 4 * (s['r1'] + i))[0]
            if v and (idx == 0x80000000 or VM_LO <= v < VM_HI): rel.add(a); why[s['name'] + ' entry'] += 1
    elif s['name'] == '__mod_init_func':
        for a in range(s['addr'], s['addr'] + s['size'], 4): rel.add(a); why['mod_init_func'] += 1
    elif s['name'] == '__cfstring':
        for a in range(s['addr'], s['addr'] + s['size'], 16):
            if target_ok(u32(a + 8)): rel.add(a + 8); why['cfstring'] += 1
# ---- exact: non-PIC import stubs (__symbol_stub4, 12 bytes: ldr ip,[pc] / ldr pc,[ip] / .long &lazy_ptr) ----
for sx in secs:
    if sx['name'] == '__symbol_stub4':
        for a in range(sx['addr'], sx['addr'] + sx['size'], 12):
            if w32(a) == 0xe59fc000 and w32(a + 4) == 0xe59cf000:
                rel.add(a + 8); why['symbol stub pointer'] += 1
            else: print('unexpected stub form at', hex(a), hex(w32(a)), file=sys.stderr)
# ---- embedded blobs (images) inside the data: never relocate words in them ----
BLOBS = []
for m in re.finditer(b'BM', d):
    o = m.start()
    if o + 54 > len(d): continue
    size, _, off = struct.unpack_from('<IIi', d, o + 2); hs = struct.unpack_from('<I', d, o + 14)[0]
    sx = sec(o + 0x1000)
    if hs in (40, 108, 124) and 54 <= off < 4096 and 100 < size < 4000000 and sx and o + 0x1000 + size <= sx['addr'] + sx['size']: BLOBS.append((o + 0x1000, o + 0x1000 + size))
for m in re.finditer(b'\x89PNG\r\n\x1a\n', d):
    o = m.start(); e = d.find(b'IEND', o); sx = sec(o + 0x1000)
    if e > 0 and sx and e + 8 + 0x1000 <= sx['addr'] + sx['size']: BLOBS.append((o + 0x1000, e + 8 + 0x1000))
def in_blob(a): return any(lo <= a < hi for lo, hi in BLOBS)
print('embedded blobs:', ', '.join(f'{lo:#x}-{hi:#x}' for lo, hi in BLOBS), file=sys.stderr)
# ---- heuristic: data sections ----
SKIP = ('__nl_symbol_ptr', '__la_symbol_ptr', '__mod_init_func', '__cfstring', '__common', '__bss')
rej = collections.Counter()
for s in secs:
    if s['seg'] != '__DATA' or s['name'] in SKIP or (s['flags'] & 0xff) in (1, 0xc): continue
    for a in range(s['addr'], s['addr'] + s['size'] - 3, 4):
        v = u32(a)
        if VM_LO <= v < VM_HI:
            if in_blob(a) or ((v & 0xffff) == 0 and not sec(v - 1)): continue   # image data / round numbers (exact 64K multiples)
            if (v & 0xffff) == 0 and sec(v) and sec(v)['addr'] != v: rej[('data round', secname(v))] += 1; continue
            if target_ok(v): rel.add(a); why['data ' + s['name']] += 1
            else: rej[('data ' + s['name'], secname(v & ~1))] += 1
# ---- code: literal pools + jump tables, each block disassembled in its own mode ----
def disasm(lo, hi, thumb):
    out = subprocess.run(['llvm-objdump', '--triple=' + ('thumbv7' if thumb else 'armv7') + '-apple-ios', '-d', '--no-show-raw-insn',
                          f'--start-address={lo:#x}', f'--stop-address={hi:#x}', B], capture_output=True, text=True).stdout.split('\n')
    res = []
    for l in out:
        m = re.match(r'\s*([0-9a-f]+):\s+(\S+)\s*([^@]*)(?:@\s*(0x[0-9a-f]+))?', l)
        if m: res.append((int(m.group(1), 16), m.group(2), m.group(3).strip(), int(m.group(4), 16) if m.group(4) else None))
    return res
blocks = []; prev = TEXT['addr']
for lo, hi in sorted(THUMB_R):
    if lo > prev: blocks.append((prev, lo, False))
    blocks.append((lo, hi, True)); prev = hi
blocks.append((prev, TEXT['addr'] + TEXT['size'], False))
REG = r'\b(r\d+|lr|ip|sb|sl|fp|pc)\b'
ARITH = re.compile(r'^(cmp|cmn|tst|teq|add|adc|sub|sbc|rsb|rsc|and|orr|eor|bic|mul|mla|mls|mvn|lsl|lsr|asr|ror|umull|smull|udiv|sdiv|movt)')
SIZE_FUNCS = {'_malloc', '_calloc', '_realloc', '__Znwm', '__Znam', '_memset', '_bzero', '_usleep', '_sleep', '_alloca', '_valloc'}
SIZE_STUBS = set()
_bd = subprocess.run(['llvm-objdump', '--macho', '--bind', '--lazy-bind', B], capture_output=True, text=True).stdout
_lazy = {}
for l in _bd.split('\n'):
    f = l.split()
    if len(f) >= 4 and f[0] == '__DATA':
        try: _lazy[int(f[2], 16)] = f[-1]
        except ValueError: pass
for sx in secs:
    if sx['name'] == '__symbol_stub4':
        for a in range(sx['addr'], sx['addr'] + sx['size'], 12):
            if _lazy.get(u32(a + 8)) in SIZE_FUNCS: SIZE_STUBS.add(a)
lit_seen = {}; lit_int = set()
DIS = [(lo, hi, thumb, disasm(lo, hi, thumb)) for lo, hi, thumb in blocks]
# known function entries: direct call targets (bl/blx), prologues, and code pointers the data sections already hold
CALLED = set()
for lo, hi, thumb, ins in DIS:
    for (a, op, args, cmt) in ins:
        mc = re.match(r'(0x[0-9a-f]+)\b', args)
        if op in ('bl', 'blx') and mc:
            t = int(mc.group(1), 16); CALLED.add(t | 1 if (thumb and op == 'bl') or (not thumb and op == 'blx') else t)
DATAPTR = set()
for sx in secs:
    if sx['seg'] == '__DATA' and sx['name'] not in ('__la_symbol_ptr', '__common', '__bss'):
        for a in range(sx['addr'], sx['addr'] + sx['size'] - 3, 4):
            v = u32(a)
            if TEXT['addr'] <= (v or 0) < TEXT['addr'] + TEXT['size']: DATAPTR.add(v)
def strict_fn(v):
    if v in CALLED or v in DATAPTR: return True
    if v & 1: return in_thumb(v & ~1) and (struct.unpack_from('<H', d, fo(v & ~1))[0] & 0xff00) == 0xb500
    if v & 3 or in_thumb(v): return False
    if (w32(v) & 0xffff0000) == 0xe92d0000: return True
    p = w32(v - 4)
    return p in (0xe12fff1e, 0xe1a00000) or (p & 0x0fff8000) == 0x08bd8000 or ((p & 0x0f000000) == 0x0a000000 and (p >> 28) == 0xe)
def path(ins, idx, i, limit=160, hops=6):
    # straight-line instructions after ins[i], following direct unconditional branches (shared tails, loop heads)
    j = i + 1; n = 0
    while n < limit and j < len(ins):
        x = ins[j]; yield x; n += 1
        mb = re.match(r'(0x[0-9a-f]+)\b', x[2])
        if x[1] in ('b', 'b.w', 'b.n') and mb and hops > 0:
            t = idx.get(int(mb.group(1), 16))
            if t is None: return
            j = t; hops -= 1; continue
        j += 1
for lo, hi, thumb, ins in DIS:
    IDX = {x[0]: k for k, x in enumerate(ins)}
    for i, (a, op, args, cmt) in enumerate(ins):
        if near_edge(a): continue
        m = re.match(r'(r\d+|lr|ip|sb|sl|fp), \[pc(?:, #(-?0x[0-9a-f]+|-?\d+))?\]$', args)   # [pc] = offset 0
        if re.match(r'ldr(\.w|\.n)?$|ldr(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le|al)$', op) and m:
            r = m.group(1)
            off = int(m.group(2) or '0', 0)
            la = cmt if cmt is not None else ((a + 4) & ~3) + off if thumb else a + 8 + off
            v = u32(la)
            if v is None: continue
            if not (VM_LO <= v < VM_HI): continue
            kind = 'unused'; op2 = ''; track = {r}; saw_cmp = False
            NODEST = ('str', 'stm', 'cmp', 'cmn', 'tst', 'teq', 'push', 'vst', 'b', 'pld')
            for (b, op2, a2, c2) in path(ins, IDX, i):   # far enough for scheduled PIC adds (0x41ac -> 0x41e4 was 14)
                ops = [x.strip() for x in re.split(r',(?![^\[{]*[\]}])', a2)] if a2 else []
                regs = re.findall(REG, a2)
                hit = track & set(regs)
                if op2.startswith(('pop', 'ldm', 'ldmia', 'ldmfd')) and 'pc' in regs:   # return: tracked regs restored, not a pc-add
                    if 'r0' in track and 'r0' not in regs: kind = 'abs'                  # ...but a value left in r0 is the return value
                    break
                if op2 == 'bx' and a2 == 'lr' and 'r0' in track: kind = 'abs'; break     # returned to the caller
                if hit:
                    if 'pc' in regs and op2.startswith(('add', 'ldr', 'sub', 'str')):
                        srcs = re.findall(REG, ','.join(ops if len(ops) == 2 else ops[1:])) if op2.startswith(('add', 'sub')) else re.findall(REG, a2[a2.find('['):]) if '[' in a2 else []
                        pc_imm = re.search(r'\[pc(, #-?(0x[0-9a-f]+|\d+))?\]', a2)
                        if (track & set(srcs)) and 'pc' in srcs and not pc_imm:
                            pcv = b + (4 if thumb else 8)
                            if VM_LO <= ((pcv + v) & 0xffffffff) < VM_HI + 0x100000: kind = 'pic'; break
                            kind = 'abs'; break                                  # "offset" would leave the image: not PIC
                        # a pc-relative literal load INTO the tracked register is an overwrite, not a use
                    dst = ops[0] if ops else ''
                    srcregs = set(re.findall(REG, ','.join(ops[1:]))) if len(ops) > 1 else set()
                    writes = not op2.startswith(NODEST + ('ldm', 'vldm', 'vld', 'pld', 'blx', 'bx')) and dst in track   # ldm/vldm: first operand is the base (read)
                    if re.match(r'mov(s)?(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)?(\.w|\.n)?$', op2) and len(ops) == 2 and ops[1] in track:
                        track = track | {dst}; continue                       # register copy: follow both
                    conditional = re.search(r'(eq|ne|cs|hs|cc|lo|mi|pl|vs|vc|hi|ls|ge|lt|gt|le)(\.w|\.n)?$', op2) and not op2.startswith(('teq', 'bl', 'cmp', 'cmn'))
                    if writes and conditional: continue                       # only one path overwrites: keep tracking
                    if writes and not (srcregs & track):
                        track = track - {dst}
                        if not track: break                                   # overwritten before any use
                        continue
                    if op2.startswith(('cmp', 'cmn', 'tst', 'teq')):
                        saw_cmp = True
                        if op2.startswith('cmp') and len(ops) == 2 and not ops[1].startswith('#') and not ops[0].startswith('#'): saw_cmp = 'reg'
                        continue                              # compared: keep looking (pointer compare vs integer)
                    if ARITH.match(op2) and not re.search(r'\[(' + '|'.join(track) + r')\b', a2): kind = 'arith'
                    else: kind = 'abs'
                    break
                if op2 in ('bl', 'blx') and track & {'r0', 'r1', 'r2', 'r3'}:
                    mt = re.match(r'(0x[0-9a-f]+)', a2); tgt = int(mt.group(1), 16) if mt else 0
                    if SIZE_STUBS and tgt in SIZE_STUBS and sec(v & ~1) and sec(v & ~1)['name'] == '__text': kind = 'arith'; break
                    kind = 'abs'; break
                if op2 == 'bx' and a2 != 'lr' and track & {'r0', 'r1', 'r2', 'r3'}: kind = 'abs'; break   # tail call with the value as an argument
                if (op2 in ('b', 'b.w', 'b.n') and not re.match(r'0x[0-9a-f]+\b', a2)) or op2 == 'bx' or (op2.startswith(('pop', 'ldm')) and 'pc' in a2): break   # end of the path
            if kind == 'unused' and saw_cmp == 'reg' and sec(v & ~1) and sec(v & ~1)['name'] != '__text': kind = 'abs'   # pointer compared with a pointer
            elif kind == 'unused' and saw_cmp: kind = 'arith'; op2 = 'cmp'
            if kind == 'pic': lit_seen[la] = 'pic'; continue
            sx = sec(v & ~1)
            if not sx: lit_int.add(la); continue
            is_code = sx['name'] == '__text'
            ok = strict_fn(v) if is_code else target_ok(v, from_code=True)
            if is_code and not ok and kind == 'abs' and op2.startswith(('str', 'stm')) and fn_boundary(v) and v % 100 and v & 0xff: ok = True   # callback stored into an object (not round in decimal or hex)
            if is_code and ok and v not in CALLED and v not in DATAPTR and v % 100 == 0: ok = False   # 67500, 120000: sizes/timeouts, not code
            if thumb: ok = False   # the Thumb objects are PIC: their literals are offsets or integers, never absolute addresses
            if is_code and kind != 'abs': ok = False
            if not is_code and kind == 'arith' and op2.startswith(('cmp', 'cmn', 'tst', 'teq')): ok = False
            if kind == 'unused':   # use not found: a PIC offset whose pc-add is further away - unless that reading leaves the image
                pcv = a + (4 if thumb else 8)
                ok = (not is_code) and not (v & 3) and not (VM_LO <= ((pcv + v) & 0xffffffff) < VM_HI + 0x100000) and target_ok(v, from_code=True)
            if ok:
                if lit_seen.get(la) != 'pic': lit_seen[la] = 'abs:' + sx['name']
            else: lit_int.add(la)
        m = re.match(r'pc, \[pc, (r\d+), lsl #2\]$', args)
        if op.startswith('ldr') and m:
            rn = m.group(1); n = None
            for (b, op2, a2, c2) in reversed(ins[max(0, i - 4):i]):
                mm = op2.startswith('cmp') and re.match(re.escape(rn) + r', #(0x[0-9a-f]+|\d+)$', a2)
                if mm: n = int(mm.group(1), 0) + 1; break
            if n is None: print('jump table without bound at', hex(a), file=sys.stderr); continue
            base = ((a + 4) & ~3) if thumb else a + 8
            for k in range(n):
                ea = base + 4 * k; v = u32(ea)
                if v and TEXT['addr'] <= v < TEXT['addr'] + TEXT['size']: rel.add(ea); why['jump table entry'] += 1
                else: print('odd jump table entry', hex(ea), hex(v or 0), file=sys.stderr)
for la, k in lit_seen.items():
    if k.startswith('abs:'): rel.add(la); why['code literal -> ' + k[4:]] += 1
# ---- pointer tables inside the code (outside Thumb blocks): >= 2 consecutive valid pointers, not used as literals ----
cand = []
for a in range(TEXT['addr'], TEXT['addr'] + TEXT['size'] - 3, 4):
    if in_thumb(a) or near_edge(a): continue
    v = u32(a)
    if VM_LO <= v < VM_HI and a not in lit_seen and a not in lit_int and target_ok(v): cand.append(a)
cs = set(cand)
for a in cand:
    if (a - 4 in cs) or (a + 4 in cs): rel.add(a); why['table in code -> ' + sec(u32(a) & ~1)['name']] += 1
print('thumb blocks:', ', '.join(f'{lo:#x}-{hi:#x}' for lo, hi in THUMB_R), file=sys.stderr)
rel = sorted(rel)
for k, n in sorted(why.items()): print(f'{n:7d}  {k}')
print(f'{len(rel):7d}  TOTAL')
print('rejected data words (by target):', dict(rej.most_common(12)))
h = hashlib.sha1(d).hexdigest()
with open(OUT, 'w') as f:
    f.write(f'/* generated by tools/nonpie_reloc.py from {B.split("/")[-1]} (sha1 {h}, {len(d)} bytes):\n')
    f.write(' * synthesized rebase list for a non-PIE binary (vm addresses of 32-bit words to slide) */\n')
    f.write(f'#define NONPIE_RELOC_FILESIZE {len(d)}u\n#define NONPIE_RELOC_COUNT {len(rel)}u\n')
    f.write('static const unsigned nonpie_reloc[NONPIE_RELOC_COUNT] = {\n')
    for i in range(0, len(rel), 12): f.write('  ' + ','.join(f'0x{x:x}' for x in rel[i:i + 12]) + ',\n')
    f.write('};\n')
