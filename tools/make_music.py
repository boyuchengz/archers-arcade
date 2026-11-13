"""Original music for Archer's Tabletop -> include/states/tt_music_data.h

Step format per channel byte: 0 = continue, 1 = cut, 2.. = note (2 = C2).
Drums (ch4): 0 none, 1 kick, 2 snare, 3 hat, 4 woodblock.
Tokens: NOTE:len  (e.g. E5:4, Bb4:2, F#5:2), r:len rest (cut), h:len hold.
A trailing '.' on a note (E5:2.) cuts on the last step for a staccato feel.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'plugins/tabletop/engine/include/states/tt_music_data.h'
NAMES = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}


def note_index(tok):
    name = tok[0]
    i = 1
    acc = 0
    while i < len(tok) and tok[i] in '#b':
        acc += 1 if tok[i] == '#' else -1
        i += 1
    octave = int(tok[i:])
    return (octave - 2) * 12 + NAMES[name] + acc


def seq(text, bass=False, steps=None):
    out = []
    for tok in text.split():
        name, length = tok.split(':')
        stac = length.endswith('.')
        n = int(length.rstrip('.'))
        if name == 'r':
            out += [1] + [0] * (n - 1)
        elif name == 'h':
            out += [0] * n
        else:
            v = note_index(name) + (12 if bass else 0)
            assert 0 <= v < 72, tok
            cell = [v + 2] + [0] * (n - 1)
            if stac and n > 1:
                cell[-1] = 1
            out += cell
    if steps is not None:
        assert len(out) == steps, (len(out), steps, text[:40])
    return out


def drums(pattern_bars):
    out = []
    for bar in pattern_bars:
        assert len(bar) == 16, bar
        out += [{'.': 0, 'k': 1, 's': 2, 'h': 3, 'w': 4}[c] for c in bar]
    return out


SONGS = []


def song(name, tempo, loop, ch1, ch2, ch3, ch4, inst):
    n = len(ch1)
    assert len(ch2) == n and len(ch3) == n and len(ch4) == n, (name, len(ch1), len(ch2), len(ch3), len(ch4))
    SONGS.append(dict(name=name, tempo=tempo, loop=loop, n=n, ch=[ch1, ch2, ch3, ch4], inst=inst))


# ------------------------------------------------------------------ TITLE: "Game Night" (C major)
lead = seq("""
E5:4 G5:2 C6:2 B5:2 G5:2 E5:4
A5:4 C6:2 E6:2 D6:4 C6:4
F5:2 A5:2 C6:4 A5:2 F5:2 G5:4
D6:6 B5:2 G5:4 r:4
E5:2 G5:2 C6:2 E6:2 D6:2 C6:2 G5:4
A5:2 B5:2 C6:4 E6:4 A5:4
F5:2 A5:2 C6:4 B5:2 D6:2 G6:4
C6:6 G5:2 E5:2 G5:2 C6:2 r:2
""", steps=128)
harm = seq("""
r:2 E4:2 r:2 G4:2 r:2 E4:2 r:2 G4:2
r:2 E4:2 r:2 A4:2 r:2 E4:2 r:2 A4:2
r:2 F4:2 r:2 A4:2 r:2 F4:2 r:2 A4:2
r:2 D4:2 r:2 G4:2 r:2 B4:2 r:2 G4:2
r:2 E4:2 r:2 G4:2 r:2 E4:2 r:2 G4:2
r:2 E4:2 r:2 A4:2 r:2 E4:2 r:2 A4:2
r:2 F4:2 r:2 A4:2 r:2 D4:2 r:2 G4:2
r:2 E4:2 r:2 G4:2 r:2 C5:2 r:2 G4:2
""", steps=128)


def walk(root, fifth, up):
    return f"{root}:1. h:1 {root}:1. h:1 {up}:1. h:1 {root}:1. h:1 {root}:1. h:1 {fifth}:1. h:1 {up}:1. h:1 {fifth}:1. h:1"


bass = seq(' '.join([
    walk('C3', 'G2', 'C4'), walk('A2', 'E3', 'A3'), walk('F2', 'C3', 'F3'), walk('G2', 'D3', 'G3'),
    walk('C3', 'G2', 'C4'), walk('A2', 'E3', 'A3'),
    'F2:1. h:1 F2:1. h:1 F3:1. h:1 F2:1. h:1 G2:1. h:1 G2:1. h:1 G3:1. h:1 D3:1. h:1',
    walk('C3', 'G2', 'C4')]).replace(':1.', ':1'), bass=True, steps=128)
bass = [b if b != 0 else 0 for b in bass]
# make the walking bass staccato: cut on every odd step
bass = [1 if (i % 2 == 1) else v for i, v in enumerate(bass)]
dr = drums(['k.h.s.h.k.h.s.h.'] * 7 + ['k.h.s.h.k.h.sss.'])
song('title', 6, 1, lead, harm, bass, dr, dict(d1=0x80, e1=0xA4, d2=0x40, e2=0x62, v3=0x20))

# ------------------------------------------------------------------ PUZZLE: "Sliding Tiles" (F major, relaxed)
lead = seq("""
A5:4 C6:4 r:2 A5:2 G5:4
F5:4 A5:2 F5:2 E5:4 D5:4
D5:2 F5:2 Bb5:4 A5:2 G5:2 F5:4
E5:4 G5:4 C6:6 r:2
A5:2 C6:2 F6:4 E6:2 C6:2 A5:4
F5:2 A5:2 D6:4 C6:2 A5:2 F5:4
D5:2 F5:2 Bb5:4 C6:2 E6:2 G6:4
F6:8 r:8
""", steps=128)


def arp(a, b, c):
    return f"{a}:2 {b}:2 {c}:2 {b}:2 {a}:2 {b}:2 {c}:2 {b}:2"


harm = seq(' '.join([arp('F4', 'A4', 'C5'), arp('D4', 'F4', 'A4'), arp('D4', 'F4', 'Bb4'), arp('E4', 'G4', 'C5'),
                     arp('F4', 'A4', 'C5'), arp('D4', 'F4', 'A4'),
                     'D4:2 F4:2 Bb4:2 F4:2 E4:2 G4:2 C5:2 G4:2', arp('F4', 'A4', 'C5')]), steps=128)


def groove(root, fifth, up):
    return f"{root}:6 {fifth}:2 {root}:4 {up}:3 r:1"


bass = seq(' '.join([groove('F2', 'C3', 'F3'), groove('D3', 'A2', 'D3'), groove('Bb2', 'F2', 'Bb2'), groove('C3', 'G2', 'C3'),
                     groove('F2', 'C3', 'F3'), groove('D3', 'A2', 'D3'),
                     'Bb2:6 F2:2 C3:4 G2:3 r:1', groove('F2', 'C3', 'F3')]), bass=True, steps=128)
dr = drums(['k...h.w.k.k.h.w.'] * 7 + ['k...h.w.k...h.hh'])
song('puzzle', 7, 1, lead, harm, bass, dr, dict(d1=0x40, e1=0x93, d2=0x00, e2=0x41, v3=0x40))

# ------------------------------------------------------------------ GOMOKU: "Stone Garden" (A minor pentatonic)
lead = seq("""
E5:2 A5:2 r:2 C6:2 D6:4 C6:2 A5:2
G5:4 E5:2 D5:2 E5:6 r:2
A5:2 C6:2 D6:2 E6:2 D6:4 C6:2 A5:2
C6:4 A5:2 G5:2 A5:6 r:2
E6:2 D6:2 C6:2 A5:2 G5:4 A5:2 C6:2
D6:4 C6:2 A5:2 G5:2 E5:2 D5:4
E5:2 G5:2 A5:2 C6:2 D6:2 E6:2 G6:4
A6:8 r:8
""", steps=128)
harm = seq("""
A4:8 E4:8
G4:8 D4:8
A4:8 E4:8
C5:8 G4:8
A4:8 E4:8
D5:8 A4:8
E4:8 G4:8
A4:8 r:8
""", steps=128)
bass = seq("""
A2:15 r:1
G2:15 r:1
A2:15 r:1
C3:15 r:1
A2:15 r:1
D3:15 r:1
E2:15 r:1
A2:15 r:1
""", bass=True, steps=128)
dr = drums(['w.......w.......', 'w.......w...w...'] * 4)
song('gomoku', 9, 1, lead, harm, bass, dr, dict(d1=0x00, e1=0xA2, d2=0x80, e2=0x34, v3=0x40))

# ------------------------------------------------------------------ jingles
song('win', 4, 0,
     seq('C5:2 E5:2 G5:2 C6:2 E6:2 G6:2 C7:12 r:2'),
     seq('E4:2 G4:2 C5:2 E5:2 G5:2 C6:2 E6:12 r:2'),
     seq('C3:4 G3:4 C3:4 C4:12 r:2', bass=True),
     drums(['k.k.k.k.s.......']) + [0] * 10,
     dict(d1=0x80, e1=0xC5, d2=0x40, e2=0x95, v3=0x20))
song('lose', 8, 0,
     seq('G4:4 F#4:4 F4:4 E4:10 r:2'),
     seq('E4:4 Eb4:4 D4:4 C4:10 r:2'),
     seq('C3:12 C2:10 r:2', bass=True),
     [0] * 24,
     dict(d1=0x40, e1=0xA7, d2=0x00, e2=0x77, v3=0x40))
song('2048', 3, 0,
     seq('C5:2 E5:2 G5:2 C6:2 E6:2 G6:2 C7:2 r:2 C7:2 E7:2 G7:14 r:2'),
     seq('G4:2 C5:2 E5:2 G5:2 C6:2 E6:2 G6:2 r:2 G6:2 C7:2 E7:14 r:2'),
     seq('C3:4 E3:4 G3:4 C3:4 C4:18 r:2', bass=True),
     drums(['h.h.h.h.h.h.h.k.', 'k.s.s.s.s.......']) + [0] * 4,
     dict(d1=0x80, e1=0xC4, d2=0x40, e2=0x94, v3=0x20))
song('found', 5, 0,
     seq('G5:2 C6:10 r:2'),
     seq('E5:2 G5:10 r:2'),
     seq('C3:12 r:2', bass=True),
     [1, 0, 2] + [0] * 11,
     dict(d1=0x80, e1=0xB4, d2=0x40, e2=0x84, v3=0x20))
song('best', 3, 0,
     seq('E6:2 G6:2 C7:10 r:2'),
     seq('C6:2 E6:2 G6:10 r:2'),
     [0] * 16,
     [0] * 16,
     dict(d1=0x80, e1=0xB3, d2=0x40, e2=0x83, v3=0x20))

# ------------------------------------------------------------------ write
freq = []
for i in range(72):
    midi = 36 + i
    f = 440.0 * 2 ** ((midi - 69) / 12)
    freq.append(max(0, min(2047, round(2048 - 131072 / f))))
lines = ['/* Generated by tools/make_music.py - original compositions. */']
lines.append('static const uint16_t note_period[72]={' + ','.join(map(str, freq)) + '};')
for s in SONGS:
    for c in range(4):
        lines.append(f"static const uint8_t song_{s['name']}_{c}[]={{" + ','.join(map(str, s['ch'][c])) + '};')
lines.append('typedef struct {const uint8_t *ch[4];uint8_t len,tempo,loop,d1,e1,d2,e2,v3;} Song;')
lines.append('static const Song songs[]={')
for s in SONGS:
    i = s['inst']
    lines.append('{{' + ','.join(f"song_{s['name']}_{c}" for c in range(4)) +
                 f"}},{s['n']},{s['tempo']},{s['loop']},{i['d1']},{i['e1']},{i['d2']},{i['e2']},{i['v3']}}},")
lines.append('};')
OUT.write_text('\n'.join(lines) + '\n')
print('songs', [(s['name'], s['n']) for s in SONGS])
