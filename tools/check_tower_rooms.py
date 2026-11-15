"""Validate rooms printed by tests/test_tower_gen.c with the checker in tower_rooms.py."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tower_rooms as t  # noqa: E402

LANES = [t.LANES['L'], t.LANES['M'], t.LANES['R']]
lines = sys.stdin.read().split('\n')
i = 0
total = proc = bad = 0
shown = 0
while i < len(lines):
    if not lines[i].startswith('ROOM'):
        i += 1
        continue
    floor, entry, exit_lane, is_proc = map(int, lines[i].split()[1:])
    rows = lines[i + 1:i + 16]
    i += 16
    total += 1
    proc += is_proc
    a, b = LANES[entry]
    ok = True
    why = ''
    if floor > 1 and not t.entry_safe(rows, a, b):
        ok, why = False, 'unsafe entry'
    elif not t.reachable(rows, range(a, b + 1)):
        ok, why = False, 'exit unreachable'
    if not ok:
        bad += 1
        if shown < 6:
            shown += 1
            print(f'floor {floor} entry {"LMR"[entry]} exit {"LMR"[exit_lane]} proc {is_proc}: {why}')
            g, seen = t.search(rows, range(a, b + 1))
            for y in range(15):
                print('   ' + ''.join('*' if (x, y) in seen else rows[y][x] for x in range(18)))
print(f'{total} rooms, {proc} procedural, {bad} failed')
sys.exit(1 if bad else 0)
