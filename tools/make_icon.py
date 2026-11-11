#!/usr/bin/env python3
"""Make sce_sys/icon0.png (128x128, 8-bit palette, as the Vita requires) from the game's iPhone icon.
Handles Apple's CgBI PNGs (raw deflate, BGRA, premultiplied alpha). usage: make_icon.py Icon@2x.png sce_sys/icon0.png"""
import sys, struct, zlib
from PIL import Image
def load(path):
    d = open(path, 'rb').read()
    if b'CgBI' not in d[:64]: return Image.open(path).convert('RGBA')
    p = 8; idat = b''; w = h = 0
    while p < len(d):
        n, t = struct.unpack('>I4s', d[p:p + 8]); body = d[p + 8:p + 8 + n]; p += 12 + n
        if t == b'IHDR': w, h = struct.unpack('>II', body[:8])
        elif t == b'IDAT': idat += body
        elif t == b'IEND': break
    raw = zlib.decompress(idat, -15); stride = w * 4; out = bytearray(); prev = bytearray(stride); i = 0
    for y in range(h):
        f = raw[i]; line = bytearray(raw[i + 1:i + 1 + stride]); i += 1 + stride
        for x in range(stride):
            a = line[x - 4] if x >= 4 else 0; b = prev[x]; c = prev[x - 4] if x >= 4 else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        out += line; prev = line
    for k in range(0, len(out), 4):                       # BGRA premultiplied -> RGBA
        bl, g, r, al = out[k:k + 4]
        if al: r, g, bl = min(255, r * 255 // al), min(255, g * 255 // al), min(255, bl * 255 // al)
        out[k:k + 4] = bytes((r, g, bl, al))
    return Image.frombytes('RGBA', (w, h), bytes(out))
im = load(sys.argv[1]).resize((128, 128), Image.LANCZOS)
bg = Image.new('RGBA', (128, 128), (0, 0, 0, 255)); bg.alpha_composite(im)
bg.convert('RGB').quantize(colors=256, method=Image.Quantize.MEDIANCUT).save(sys.argv[2], optimize=True)
print('wrote', sys.argv[2])
