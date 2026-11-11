#!/usr/bin/env python3
"""Annotate ARM-mode objc_msgSend selector/class loads in a range: sels.py BIN start end"""
import sys, struct, subprocess, re
B, lo, hi = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)
d = open(B, 'rb').read()
ncmds = struct.unpack_from('<I', d, 16)[0]; off = 28; segs = []; secs = {}
for _ in range(ncmds):
    cmd, sz = struct.unpack_from('<II', d, off)
    if cmd == 1:
        name = d[off+8:off+24].rstrip(b'\0').decode(); vm, vs, fo, fs = struct.unpack_from('<IIII', d, off+24)
        segs.append((vm, vs, fo)); ns = struct.unpack_from('<I', d, off+48)[0]
        for k in range(ns):
            s = off + 56 + k*68; sn = d[s:s+16].rstrip(b'\0').decode(); a, z = struct.unpack_from('<II', d, s+32); secs[sn] = (a, z)
    off += sz
def fo(vm):
    for a, s, f in segs:
        if a <= vm < a+s: return f + vm - a
def u32(vm): o = fo(vm); return struct.unpack_from('<I', d, o)[0] if o is not None else None
def cstr(vm): o = fo(vm); return d[o:d.index(b'\0', o)].decode('latin1') if o is not None else '?'
def insec(n, a): s = secs.get(n); return s and s[0] <= a < s[0]+s[1]
# class names: class ptr -> ro (+16) -> name (+16)
def clsname(p):
    if not p: return 'extern'
    ro = u32(p+16); return cstr(u32((ro & ~3)+16)) if ro else '?'
out = subprocess.run(['llvm-objdump', '--triple=armv7-apple-ios', '-d', '--no-show-raw-insn', f'--start-address={lo:#x}', f'--stop-address={hi:#x}', B], capture_output=True, text=True).stdout
lit = {}
for line in out.split('\n'):
    m = re.match(r'\s*([0-9a-f]+):\s+(\S+)\s+(.*)', line)
    if not m: continue
    a = int(m.group(1), 16); op, args = m.group(2), m.group(3); note = ''
    m1 = re.match(r'(\w+), \[pc, #(-?0x[0-9a-f]+|-?\d+)\]', args)
    m2 = re.match(r'(\w+), \[pc, (\w+)\]', args)
    m3 = re.match(r'(\w+), pc, (\w+)$', args)
    if op == 'ldr' and m1: lit[m1.group(1)] = u32(a + 8 + int(m1.group(2), 0))
    elif op in ('ldr', 'add') and (m2 or m3):
        mm = m2 or m3; r = mm.group(2)
        if r in lit and lit[r] is not None:
            t = (a + 8 + lit[r]) & 0xffffffff
            if op == 'add': note = f'&{t:#x}' + (f' "{cstr(t)}"' if insec('__cstring', t) else '')
            elif insec('__objc_selrefs', t): note = 'SEL ' + cstr(u32(t))
            elif insec('__objc_classrefs', t): note = 'CLASS ' + clsname(u32(t)) + f' ({t:#x})'
            elif insec('__cfstring', t) or insec('__cfstring', t): note = 'CFSTR'
            else: note = f'[{t:#x}]'
        lit.pop(mm.group(1), None)
    elif m1 is None: pass
    if 'bl\t' in line or op in ('bl', 'blx'): note = note or args
    if note: print(f'{a:x}: {op} {args.split("@")[0].strip():28s} ; {note}')
