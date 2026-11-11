#!/usr/bin/env python3
"""Regenerate src/align_thunks_gen.h from src/binds_gen.h (run after tools/analyze.py)."""
import re, sys
src = sys.argv[1] if len(sys.argv) > 1 else 'src'
n = int(re.search(r'#define N_IMPORTS (\d+)', open(f'{src}/binds_gen.h').read()).group(1))
L = ['/* generated: one 2-instruction entry per import -> stack-aligning call (see shims.c, align_idx) */', '#ifdef __arm__', '__asm__(".syntax unified\\n.arm\\n.text\\n.align 2\\n"']
for i in range(n): L.append(f'"imp_thunk_{i}: movw ip, #{i}\\n b align_idx\\n"')
L.append(');')
L.append(''.join(f'extern void imp_thunk_{i}(void) __asm__("imp_thunk_{i}");\n' for i in range(n)))
L.append('static void (*const imp_thunks[N_IMPORTS])(void) = {' + ','.join(f'imp_thunk_{i}' for i in range(n)) + '};')
L.append('#endif')
open(f'{src}/align_thunks_gen.h', 'w').write('\n'.join(L) + '\n')
print(n, 'thunks')
