"""Art and room data for ARROW TOWER, the climbing game.

    python3 tools/make_tower.py        # writes engine headers + tools/preview/tower_*.png

Outputs:
    include/states/tt_tower_defs.h   tile/sprite indices and constants (all tower C files)
    include/states/tt_art_tower.h    tiles, palettes, animation frames, panel titles (tt_tower_view.c)
    include/states/tt_tower_rooms.h  room templates (tt_tower_gen.c)

VRAM bank 1 holds the tower set: background tiles 0..127, sprites 128..255.
Background palettes (per zone): 0 HUD, 1 stone, 2 back wall, 3 wood, 4 metal,
5 coins, 6 decor, 7 misc (ice, springs, gems). Colour 0 of palettes 1..7 is the
back wall base so every tile blends into the wall.
"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from PIL import Image, ImageDraw  # noqa: E402
from artlib import TileSet, carr, new, planar, rgb15, tile_px  # noqa: E402
from font import glyph, glyph_width  # noqa: E402
import tower_rooms  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
INC = ROOT / 'plugins/tabletop/engine/include/states'
PREV = ROOT / 'tools/preview'
PREV.mkdir(exist_ok=True)
defines = []


def define(name, value):
    defines.append(f'#define {name} {value}')


def grid(rows, w=None, h=None):
    """ASCII art ('.' or digits 0-3) -> L image."""
    h = h or len(rows)
    w = w or max(len(r) for r in rows)
    im = new(w, h)
    for y, r in enumerate(rows):
        assert len(r) <= w, (r, len(r), w)
        for x, ch in enumerate(r):
            if ch in '123':
                im.putpixel((x, y), int(ch))
    return im


def outline(im, colors, out=3, diag=False):
    w, h = im.size
    px = im.load()
    marks = []
    for y in range(h):
        for x in range(w):
            if px[x, y] != 0:
                continue
            nb = ((1, 0), (-1, 0), (0, 1), (0, -1))
            if diag:
                nb += ((1, 1), (-1, -1), (1, -1), (-1, 1))
            for dx, dy in nb:
                xx, yy = x + dx, y + dy
                if 0 <= xx < w and 0 <= yy < h and px[xx, yy] in colors:
                    marks.append((x, y))
                    break
    for x, y in marks:
        px[x, y] = out
    return im


# =====================================================================
# zone palettes
# =====================================================================
def hx(s):
    r, g, b = bytes.fromhex(s)
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


ZONES = [
    # name, base, wall light, wall dark, wall darkest, stone face, accent, outline, wood light, wood mid, metal light, metal mid, banner
    dict(name='MOSSY GATE', base='2c3c44', wl='3a4e58', wd='223038', wk='141e24',
         face='8a979c', acc='9be06a', ol='10161c', wood1='e8b064', wood2='a86a38', m1='eef2f4', m2='8e9aa6', ban='e04848'),
    dict(name='OLD LIBRARY', base='3c2838', wl='4e3448', wd='2e1e2c', wk='1c1018',
         face='cfa874', acc='ffe6a8', ol='22121a', wood1='d0784c', wood2='8a3c2a', m1='f4ecdc', m2='a8988a', ban='5a9ce8'),
    dict(name='CLOCKWORKS', base='1e3838', wl='2a4a4a', wd='162c2c', wk='0c1a1a',
         face='b8924e', acc='ffdc80', ol='181408', wood1='d8a868', wood2='8e5e30', m1='f0f0e0', m2='a0a490', ban='e86a3a'),
    dict(name='FROST VAULT', base='223260', wl='2e4278', wd='1a2852', wk='0e1838',
         face='a0c4e4', acc='ffffff', ol='0e1630', wood1='c8a88c', wood2='7e5c4e', m1='ffffff', m2='94b4d4', ban='4ad0e0'),
    dict(name='EMBER FORGE', base='3a1c1a', wl='4c2622', wd='2c1412', wk='1a0a0a',
         face='6e5a58', acc='ff8a30', ol='140808', wood1='c88a5a', wood2='7a4428', m1='ffd8a0', m2='c8603a', ban='ffc830'),
    dict(name='STAR SPIRE', base='261e4c', wl='32286a', wd='1c163a', wk='0e0a24',
         face='cabcf0', acc='ffe066', ol='120c2a', wood1='e0b07a', wood2='9a6644', m1='ffffff', m2='a8a0d8', ban='ff6ab8'),
]
HUD = ['0d1024', '05060f', 'ffcf3a', 'fff6df']
BG_PALS = []
for z in ZONES:
    b = z['base']
    p = [
        HUD,
        [b, z['acc'], z['face'], z['ol']],              # 1 stone
        [b, z['wl'], z['wd'], z['wk']],                 # 2 back wall
        [b, z['wood1'], z['wood2'], z['ol']],           # 3 wood
        [b, z['m1'], z['m2'], z['ol']],                 # 4 metal
        [b, 'ffe46a', 'f09a22', '4a2406'],              # 5 coins
        [b, 'fff0a8', z['ban'], z['ol']],               # 6 decor (flame light, cloth, outline)
        [b, 'c8f8ff', '48b8d8', '0c2a3a'],              # 7 misc (ice, springs, gems)
    ]
    BG_PALS.append([hx(c) for pal4 in p for c in pal4])

OBJ_PAL_HEX = [
    ['000000', 'fff1d6', '3cae4c', '1a1420'],   # 0 hero
    ['000000', 'ffd6e4', 'e8508c', '2c0c1c'],   # 1 slime
    ['1b2046', 'ffffff', 'ffcf3a', 'ff8a2e'],   # 2 sparkles (same as base set)
    ['000000', 'ffffff', '9a6ad8', '1c1030'],   # 3 bat / ghost
    ['1b2046', '0d1024', 'ffcf3a', 'fff6df'],   # 4 floating text (same as base set)
    ['000000', 'ffffff', '9aa6b4', '1c2028'],   # 5 metal
    ['000000', 'f4c888', 'b07038', '3a2010'],   # 6 wood
    ['000000', 'ffe680', 'ff4a3a', '6a1010'],   # 7 fire / hearts
]
OBJ_PALS = [hx(c) for p in OBJ_PAL_HEX for c in p]

# =====================================================================
# background tiles (bank 1, 0..127)
# =====================================================================
bg = TileSet(0, 128, 'tower bg')


def bgt(im, name=None, dedupe=False):
    n = bg.add(tile_px(im), dedupe=dedupe)
    if name:
        define(f'TW_T_{name}', n)
    return n


bgt(new(8, 8), 'EMPTY')            # 0: plain back wall base


def wall_tile(variant):
    """Back wall (palette 2): big soft bricks 16x8, 4 tiles cover the 2x2 repeat."""
    im = new(8, 8, 0)
    px = im.load()
    sx = (variant & 1) * 8
    sy = (variant >> 1) * 4
    for y in range(8):
        for x in range(8):
            gx, gy = sx + x, y
            course = (gy + sy) // 4
            off = 8 if course % 2 else 0
            if (gy + sy) % 4 == 3:
                px[x, y] = 2
            elif (gx + off) % 16 == 15:
                px[x, y] = 2
            elif (gy + sy) % 4 == 0 and (gx + off) % 16 in (0, 1):
                px[x, y] = 1
    return im


for v in range(4):
    bgt(wall_tile(v), f'WALL{v}')
# cracked / mossy variants
im = wall_tile(0)
for (x, y) in ((2, 1), (3, 2), (3, 3), (4, 4), (5, 5), (5, 6)):
    im.putpixel((x, y), 3)
bgt(im, 'WALL_CRACK')
im = wall_tile(3)
for (x, y) in ((1, 5), (2, 5), (2, 6), (5, 1), (6, 1), (6, 2)):
    im.putpixel((x, y), 1)
bgt(im, 'WALL_SPECK')


def stone(top, bottom, left, right):
    """Stone block (palette 1): running-bond bricks, outline on exposed sides, accent on top."""
    rows = [
        '12222220',
        '22222220',
        '22222220',
        '00000000',
        '22201222',
        '22202222',
        '22202222',
        '00000000',
    ]
    im = grid(rows)
    px = im.load()
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            px[x, y] = int(c)
    if top:
        for x in range(8):
            px[x, 0] = 3
            px[x, 1] = 1
        for x in (1, 2, 5):
            px[x, 2] = 1
    if bottom:
        for x in range(8):
            px[x, 7] = 3
    if left:
        for y in range(8):
            px[0, y] = 3
    if right:
        for y in range(8):
            px[7, y] = 3
    if top and left:
        px[0, 0] = 0
        px[1, 1] = 3 if not top else px[1, 1]
    if top and right:
        px[7, 0] = 0
    if bottom and left:
        px[0, 7] = 0
    if bottom and right:
        px[7, 7] = 0
    return im


def wood(top, bottom, left, right):
    """Wood block (palette 3): vertical planks with nails. Arrows stick in these."""
    rows = [
        '12231223',
        '13231323',
        '12231223',
        '12231223',
        '12231223',
        '12231223',
        '13231323',
        '12231223',
    ]
    im = grid(rows)
    px = im.load()
    if top:
        for x in range(8):
            px[x, 0] = 3
            px[x, 1] = 1
    if bottom:
        for x in range(8):
            px[x, 7] = 3
    if left:
        for y in range(8):
            px[0, y] = 3
    if right:
        for y in range(8):
            px[7, y] = 3
    if top and left:
        px[0, 0] = 0
    if top and right:
        px[7, 0] = 0
    if bottom and left:
        px[0, 7] = 0
    if bottom and right:
        px[7, 7] = 0
    return im


# exposure index: top 1, bottom 2, left 4, right 8
STONE0 = None
for e in range(16):
    n = bgt(stone(e & 1, e & 2, e & 4, e & 8))
    if e == 0:
        STONE0 = n
define('TW_T_STONE', STONE0)
WOOD0 = None
for e in range(16):
    n = bgt(wood(e & 1, e & 2, e & 4, e & 8))
    if e == 0:
        WOOD0 = n
define('TW_T_WOOD', WOOD0)

# floor slab (shared floor/ceiling row): two alternating tiles
for v in range(2):
    rows = [
        '33333333',
        '11111111',
        '12122121' if v == 0 else '21211212',
        '22222220' if v == 0 else '22202222',
        '22222220' if v == 0 else '22202222',
        '00000000',
        '22202222' if v == 0 else '22222220',
        '33333333',
    ]
    bgt(grid(rows), f'SLAB{v}')

# one-way planks (palette 3): left, middle, right
for name, rows in (
    ('PLANK_L', ['33333333', '31111111', '32222222', '33333333', '.32.....', '..32....', '...3....', '........']),
    ('PLANK_M', ['33333333', '11111111', '22222322', '33333333', '........', '........', '........', '........']),
    ('PLANK_R', ['33333333', '11111113', '22222223', '33333333', '.....23.', '....23..', '....3...', '........']),
):
    bgt(grid(rows), name)

# spikes (palette 4), pointing up; flip Y in the attribute for ceiling spikes
spk = grid([
    '........',
    '.3....3.',
    '.3...13.',
    '313..323',
    '3123 3123'.replace(' ', '')[:8],
    '31233123',
    '31223122',
    '33333333',
])
spk = grid([
    '........',
    '..3...3.',
    '.313.313',
    '.312.312',
    '31123112',
    '31123112',
    '22222222',
    '33333333',
])
bgt(spk, 'SPIKE')

# crumble block (palette 1): cracked stone, two stages
for v, cracks in enumerate((((2, 2), (3, 3), (3, 4), (5, 5)), ((1, 1), (2, 2), (3, 3), (3, 4), (4, 5), (5, 5), (6, 4), (5, 2), (2, 5)))):
    im = stone(1, 1, 1, 1)
    for (x, y) in cracks:
        im.putpixel((x, y), 3)
    if v:
        im.putpixel((3, 0), 0)
        im.putpixel((4, 0), 0)
        im.putpixel((7, 4), 0)
        im.putpixel((0, 3), 0)
    bgt(im, f'CRUMBLE{v}')

# ice block (palette 7): top-exposed and plain
for top in (1, 0):
    im = new(8, 8, 2)
    d = ImageDraw.Draw(im)
    d.line((1, 6, 5, 2), fill=1)
    d.line((4, 6, 6, 4), fill=1)
    d.rectangle((0, 0, 7, 7), outline=3)
    if top:
        d.line((1, 1, 6, 1), fill=1)
    im.putpixel((0, 0), 0)
    im.putpixel((7, 0), 0)
    im.putpixel((0, 7), 0)
    im.putpixel((7, 7), 0)
    bgt(im, 'ICE_TOP' if top else 'ICE')

# spring (palette 7): idle and launched
bgt(grid([
    '........',
    '........',
    '........',
    '.333333.',
    '31111113',
    '.32..23.',
    '..3223..',
    '33333333',
]), 'SPRING')
bgt(grid([
    '.333333.',
    '31111113',
    '.322223.',
    '..3..3..',
    '.3....3.',
    '..3..3..',
    '.3....3.',
    '33333333',
]), 'SPRING_UP')

# gem (palette 7)
bgt(grid([
    '........',
    '..3333..',
    '.311223.',
    '31112223',
    '.312223.',
    '..3223..',
    '...33...',
    '........',
]), 'GEM')

# cannon (palette 4), muzzle facing right; flip X for left
bgt(grid([
    '33333...',
    '322233333',
    '32113111',
    '32213222',
    '32213222',
    '32223333',
    '32223...',
    '33333...',
][:8]).crop((0, 0, 8, 8)), 'CANNON')

# coin (palette 5): 4 frames streamed into one tile slot
COIN_FRAMES = []
for w in (6, 4, 2, 4):
    im = new(8, 8)
    d = ImageDraw.Draw(im)
    x0 = (8 - w) // 2
    if w >= 4:
        d.ellipse((x0, 1, x0 + w - 1, 6), fill=2, outline=3)
        d.line((x0 + 1, 2, x0 + 1, 4), fill=1)
        if w == 6:
            d.line((x0 + 3, 3, x0 + 3, 4), fill=3)
    else:
        d.rectangle((3, 1, 4, 6), fill=3)
        d.line((3, 2, 3, 5), fill=1)
    COIN_FRAMES.append(im)
bgt(COIN_FRAMES[0], 'COIN')

# saw rails and platform track (palette 2)
bgt(grid(['........', '........', '........', '33333333', '11111111', '33333333', '........', '........']), 'RAIL_H')
bgt(grid(['...313..', '...313..', '...313..', '...313..', '...313..', '...313..', '...313..', '...313..']), 'RAIL_V')
bgt(grid(['........', '........', '..333...', '.31113..', '.31113..', '..333...', '........', '........']), 'RAIL_END')
bgt(grid(['........', '........', '........', '........', '22..22..', '........', '........', '........']), 'TRACK')

# windows (palette 2): 2x2 arched, night sky with a star
win = grid([
    '....33333333....',
    '..331111111133..',
    '.31133333333113.',
    '.31333333333313.',
    '3133333333333313',
    '3133333331333313',
    '3133333311133313',
    '3133333331333313',
    '3133333333333313',
    '3111111111111113',
    '3133333313333313',
    '3133133313333313',
    '3133333313333313',
    '3133333313333313',
    '3111111111111113',
    '3333333333333333',
])
for i in range(4):
    bgt(Image.Image.crop(win, ((i % 2) * 8, (i // 2) * 8, (i % 2) * 8 + 8, (i // 2) * 8 + 8)), 'WINDOW' if i == 0 else None)

# chain (palette 2)
bgt(grid(['..333...', '.31.13..', '.31.13..', '..333...', '...3....', '..313...', '..313...', '...3....']), 'CHAIN')

# torch (palette 6): bracket tile + 3 flame frames streamed into one slot
bgt(grid(['...33...', '..3223..', '.322223.', '.322223.', '..3333..', '...33...', '...33...', '..3333..']), 'TORCH')
FLAME_FRAMES = [
    grid(['........', '...2....', '..212...', '..2112..', '.21112..', '.21112..', '..2112..', '...33...']),
    grid(['....2...', '...22...', '..2122..', '..21112.', '.211112.', '.21112..', '..2112..', '...33...']),
    grid(['........', '..2.....', '..22....', '.2112...', '.21112..', '.211112.', '..2112..', '...33...']),
]
bgt(FLAME_FRAMES[0], 'FLAME')

# banner (palette 6): 2x3, rod, cloth with an arrow crest, swallowtail
ban = grid([
    '3333333333333333',
    '.33333333333333.',
    '..322222222223..',
    '..322222222223..',
    '..322221122223..',
    '..322211112223..',
    '..322111111223..',
    '..322221122223..',
    '..322221122223..',
    '..322221122223..',
    '..322211112223..',
    '..322112211223..',
    '..322222222223..',
    '..322222222223..',
    '..32222332223...',
    '..3222.3.2223...',
    '..322..3..223...',
    '..32.......23...',
    '..3.........3...',
    '................',
    '................',
    '................',
    '................',
    '................',
], w=16, h=24)
for i in range(6):
    bgt(ban.crop(((i % 2) * 8, (i // 2) * 8, (i % 2) * 8 + 8, (i // 2) * 8 + 8)), 'BANNER' if i == 0 else None)

# campfire (palette 6): 2x2 logs + flame, 2 animation frames streamed
def campfire(f):
    im = new(16, 16)
    d = ImageDraw.Draw(im)
    if f == 0:
        flame = ['.......2........', '......212.......', '.....2112..2....', '....21112.212...',
                 '...211111211112.', '...21111111112..', '....211111112...', '.....2111112....']
    else:
        flame = ['........2.......', '....2..212......', '...212.2112.....', '...2112111122...',
                 '..21111111112...', '...2111111112...', '....21111112....', '.....211112.....']
    for y, r in enumerate(flame):
        for x, ch in enumerate(r):
            if ch in '12':
                im.putpixel((x, y + 2), int(ch))
    d.line((2, 12, 13, 15), fill=3, width=2)
    d.line((2, 15, 13, 12), fill=3, width=2)
    d.line((3, 12, 12, 15), fill=2)
    d.line((3, 15, 12, 12), fill=2)
    return im


CAMP_FRAMES = [campfire(0), campfire(1)]
for i in range(4):
    bgt(CAMP_FRAMES[0].crop(((i % 2) * 8, (i // 2) * 8, (i % 2) * 8 + 8, (i // 2) * 8 + 8)), 'CAMP' if i == 0 else None)

# HUD tiles (palette 0 = bar: 0 navy, 1 shadow, 2 gold, 3 cream), used in the window
bgt(grid([
    '........',
    '..2222..',
    '.233222.',
    '.232222.',
    '.232222.',
    '.222222.',
    '..2222..',
    '........',
]), 'HUD_COIN')
bgt(grid([
    '...3....',
    '..333...',
    '.33333..',
    '...3....',
    '.2222...',
    '.2..2...',
    '.2..2...',
    '.2222...',
]), 'HUD_FLOOR')
define('TW_BG_COUNT', len(bg))

# =====================================================================
# sprites (bank 1, 128..255)
# =====================================================================
spr = TileSet(128, 256, 'tower sprites')


def sp16(im, name=None):
    """16x16 -> 4 tiles in order TL, TR, BL, BR."""
    first = None
    for i in range(4):
        n = spr.add(tile_px(im, (i % 2) * 8, (i // 2) * 8), dedupe=False)
        if first is None:
            first = n
    if name:
        define(f'TW_S_{name}', first)
    return first


def sp8(im, name=None):
    n = spr.add(tile_px(im), dedupe=False)
    if name:
        define(f'TW_S_{name}', n)
    return n


def sp16x8(im, name=None):
    first = spr.add(tile_px(im, 0, 0), dedupe=False)
    spr.add(tile_px(im, 8, 0), dedupe=False)
    if name:
        define(f'TW_S_{name}', first)
    return first


# ---------------------------------------------------------------- hero (facing right)
HEAD = [
    '................',
    '......33333.....',
    '....332222233...',
    '.3332222222223..',
    '322222222222223.',
    '.33222231111113.',
    '...322231131313.',
    '...322231131313.',
    '...322231111113.',
    '....3222333333..',
]
BODY = [
    '.....3222223....',
    '.....32222213...',
    '.....3331333....',
    '.....3222223....',
]
LEGS_STAND = ['......33.33.....', '.....333.333....']


def hero(head_dy=0, body=BODY, legs=LEGS_STAND, tip_up=False, extra=None):
    rows = ['.' * 16 for _ in range(16)]
    h = list(HEAD)
    if tip_up:
        h[3] = '..3322222222223..'[:16]
        h[2] = '.3..332222233...'
    for i, r in enumerate(h):
        y = i + head_dy
        if 0 <= y < 16:
            rows[y] = r
    for i, r in enumerate(body):
        y = 10 + i + head_dy
        if 0 <= y < 14:
            rows[y] = r
    for i, r in enumerate(legs):
        rows[14 + i] = r
    im = grid(rows)
    if extra:
        for (x, y, c) in extra:
            im.putpixel((x, y), c)
    return im


RUN_LEGS = [
    ['....33...33.....', '...333....333...'],
    ['......3333......', '.....333........'],
    ['.....33..33.....', '....333...33....'],
    ['......3333......', '.......333......'],
]
RUN_BODY = [
    '.....3222223....',
    '....132222223...',
    '.....3331333....',
    '.....3222223....',
]
JUMP_BODY = [
    '...1.3222223.1..',
    '...335222223 3..'.replace('5', '2').replace(' ', ''),
    '.....3331333....',
    '.....3222223....',
]
JUMP_BODY = [
    '...13322222331..',
    '....3322222 3...'.replace(' ', '3'),
    '.....3331333....',
    '.....3222223....',
]
SHOOT_BODY = [
    '.....322222 3.3.'.replace(' ', '2'),
    '.....322221113 3'.replace(' ', '1'),
    '.....3331333.3..',
    '.....3222223.3..',
]

frames = {}
frames['IDLE0'] = hero()
frames['IDLE1'] = hero(head_dy=1, legs=LEGS_STAND)
for i in range(4):
    frames[f'RUN{i}'] = hero(head_dy=0 if i % 2 == 0 else -1, body=RUN_BODY, legs=RUN_LEGS[i])
frames['JUMP'] = hero(head_dy=-1, body=JUMP_BODY, legs=['.....33..33.....', '....33....33....'])
frames['FALL'] = hero(head_dy=0, body=JUMP_BODY, legs=['......3..3......', '......33.33.....'], tip_up=True)
# shoot: idle with a drawn bow held low in front
shoot = hero(body=SHOOT_BODY)
for (x, y) in ((13, 8), (14, 9), (15, 10), (15, 11), (15, 12), (14, 13), (13, 14)):
    shoot.putpixel((x, y), 3)
for y in range(9, 14):
    shoot.putpixel((12, y), 1)
frames['SHOOT'] = shoot
hurt = hero(head_dy=0, body=JUMP_BODY, legs=['.....3....3.....', '....33....33....'])
for (x, y) in ((10, 6), (12, 6), (10, 7), (12, 7)):
    hurt.putpixel((x, y), 1)
for (x, y) in ((9, 6), (11, 7), (11, 6), (9, 7)):
    pass
d = ImageDraw.Draw(hurt)
d.point([(10, 6), (11, 7), (10, 8)], fill=3)
d.point([(12, 6), (13, 7), (12, 8)], fill=3)
frames['HURT'] = hurt
HERO_ORDER = ['IDLE0', 'IDLE1', 'RUN0', 'RUN1', 'RUN2', 'RUN3', 'JUMP', 'FALL', 'SHOOT', 'HURT']
for k in HERO_ORDER:
    sp16(frames[k], f'HERO_{k}')

# ---------------------------------------------------------------- arrows (palette 6 wood)
sp8(grid([
    '........',
    '........',
    '1.....3.',
    '11...333',
    '22222333',
    '11....3.',
    '1.......',
    '........',
][:8]), 'ARROW')
sp8(grid([
    '........',
    '........',
    '1.......',
    '11......',
    '22222222',
    '11......',
    '1.......',
    '........',
]), 'ARROW_STUCK')

# ---------------------------------------------------------------- slime (palette 1): 2 frames
sp16(grid([
    '................',
    '................',
    '................',
    '................',
    '................',
    '......3333......',
    '....33222233....',
    '...3211222223...',
    '..321222222223..',
    '..322223223223..',
    '.32222232232223.',
    '.32222222222223.',
    '.32222111122223.',
    '.32222222222223.',
    '..322222222223..',
    '...3333333333...',
]), 'SLIME')
sp16(grid([
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................',
    '.....333333.....',
    '...3322222233...',
    '..321122222223..',
    '.3212222222222 3'.replace(' ', '2'),
    '32222232232222 3'.replace(' ', '2'),
    '322222322322222 3'.replace(' ', '')[:16],
    '3222222111122223'[:16],
    '.32222222222223.',
    '..333333333333..',
]), None)

# ---------------------------------------------------------------- bat (palette 3): 16x8, 2 frames
sp16x8(grid([
    '3..............3',
    '33.....33.....33',
    '323...3223...323',
    '3223.322223.3223',
    '.32223212123223.',
    '..3222222222223.'[:16],
    '...33.32223.33..',
    '.......333......',
]), 'BAT')
sp16x8(grid([
    '................',
    '.......33.......',
    '......3223......',
    '..33.3212123.33.',
    '.3223222222232 3'.replace(' ', '2'),
    '3222232222232223',
    '32223.32223.3223'[:16],
    '333....333....33',
]), None)

# ---------------------------------------------------------------- saw (palette 5): 2 frames


def saw(phase):
    im = new(16, 16)
    d = ImageDraw.Draw(im)
    cx, cy = 7.5, 7.5
    pts = []
    for i in range(16):
        a = (i / 16) * 2 * math.pi + phase
        r = 7.6 if i % 2 == 0 else 5.2
        pts.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    d.polygon(pts, fill=2, outline=3)
    d.ellipse((4, 4, 11, 11), fill=1, outline=3)
    d.ellipse((6, 6, 9, 9), fill=3)
    im.putpixel((5, 6), 2)
    im.putpixel((6, 5), 2)
    return im


sp16(saw(0), 'SAW')
sp16(saw(math.pi / 8), None)

# ---------------------------------------------------------------- fireball (palette 7), flying right
sp8(grid([
    '........',
    '...2222.',
    '.222112 2'.replace(' ', '')[:8],
    '22211112',
    '.2211112',
    '..22112.',
    '....22..',
    '........',
]), 'FIRE')
sp8(grid([
    '........',
    '..2.222.',
    '.2221122',
    '.2211112',
    '22211112',
    '..2211 2'.replace(' ', '2'),
    '...222..',
    '........',
]), None)

# ---------------------------------------------------------------- moving platform (palette 6), 24x8
plat = grid([
    '333333333333333333333333',
    '311111111111111111111113',
    '322322222232222223222223',
    '322222222222222222222223',
    '333333333333333333333333',
    '.3.3................3.3.',
    '..3..................3..',
    '........................',
])
for i in range(3):
    sp8(plat.crop((i * 8, 0, i * 8 + 8, 8)), 'PLAT' if i == 0 else None)

# ---------------------------------------------------------------- ghost (palette 3): 2 frames
for f in range(2):
    rows = [
        '................',
        '.....333333.....',
        '...33111111 33..'.replace(' ', '1'),
        '..3111111111113.',
        '.311111111111113',
        '.311311113111113',
        '.311331113311113',
        '.311111111111113',
        '.311111222111113',
        '.311111222111113',
        '.311111111111113',
        '.311111111111113',
        '.311111111111113',
        '.3112111121111 3'.replace(' ', '1'),
        ('.32.3112.3112.3.' if f == 0 else '.3.3112.3112.32.'),
        ('..3..33...33..3.' if f == 0 else '.3...33...33..3.'),
    ]
    rows = [r[:16].ljust(16, '.') for r in rows]
    sp16(grid(rows), 'GHOST' if f == 0 else None)

# ---------------------------------------------------------------- small things
sp8(grid([
    '...33...',
    '...33...',
    '..3113..',
    '..3123..',
    '..3123..',
    '...32...',
    '...33...',
    '....3...',
]), 'ICICLE')    # palette 5 (metal) falling icicle
sp8(grid([
    '........',
    '.33.33..',
    '3113223.',
    '3122223.',
    '.32222 3'.replace(' ', '3')[:8],
    '..3223..',
    '...33...',
    '........',
]), 'HEART')     # palette 7 pickup and HUD heart
sp8(grid([
    '........',
    '.33.33..',
    '3..3..3.',
    '3.....3.',
    '.3...3..',
    '..3.3...',
    '...3....',
    '........',
]), 'HEART_EMPTY')
sp8(grid([
    '........',
    '........',
    '..11....',
    '.1111.1.',
    '.11111..',
    '..111...',
    '........',
    '........',
]), 'DUST0')     # palette 2 (white)
sp8(grid([
    '........',
    '.1...1..',
    '........',
    '1.....1.',
    '........',
    '.1...1..',
    '........',
    '........',
]), 'DUST1')
sp8(grid([
    '........',
    '........',
    '...33...',
    '..3223..',
    '..3223..',
    '...33...',
    '........',
    '........',
]), 'CHIP')      # stone chip debris (palette 5)
sp8(grid([
    '........',
    '..3333..',
    '.311223.',
    '.312223.',
    '.312223.',
    '.322223.',
    '..3333..',
    '........',
]), 'COIN')      # palette 2 sparkles: HUD coin and collected-coin pop
define('TW_SPR_COUNT', len(spr))

# =====================================================================
# panel titles: bank 0 tiles 224..247 (12x2), two-tone cream/gold with dark outline
# =====================================================================
TITLES = {}
for key, word in (('pause', 'PAUSED'), ('over', 'GAME OVER'), ('tower', 'ARROW TOWER'), ('record', 'NEW RECORD!'),
                  ('rest', 'CAMPFIRE')):
    im = new(96, 16)
    tw = sum((glyph_width(c) + 1) if c != ' ' else 4 for c in word) - 1
    assert tw <= 94, word
    x = (96 - tw) // 2
    for ch in word:
        w = glyph_width(ch)
        rows = glyph(ch)
        for yy, r in enumerate(rows):
            for xx, c in enumerate(r):
                if c == '#':
                    im.putpixel((x + xx, 4 + yy), 3 if yy < 3 else 2)
        x += (w + 1) if ch != ' ' else 4
    outline(im, {2, 3}, 1, diag=True)
    TITLES[key] = [b for ty in range(2) for tx in range(12) for b in planar(tile_px(im, tx * 8, ty * 8))]

# =====================================================================
# rooms
# =====================================================================
ROOM_W, ROOM_H = 18, 15
CODES = tower_rooms.CODES
rooms = tower_rooms.validate(tower_rooms.ROOMS)
blob = []
offsets = []
info = []
for r in rooms:
    offsets.append(len(blob))
    flat = ''.join(r['rows'])
    i = 0
    while i < len(flat):
        ch = flat[i]
        if ch == '.':
            j = i
            while j < len(flat) and flat[j] == '.' and j - i < 127:
                j += 1
            blob.append(0x80 | (j - i))
            i = j
        else:
            blob.append(CODES[ch])
            i += 1
    info += [r['tier'], r['entry'], r['exit'], r['flags']]
define('TW_ROOM_COUNT', len(rooms))
for k, v in tower_rooms.SPECIAL.items():
    define(f'TW_ROOM_{k}', v)
for ch, v in CODES.items():
    define(f'RC_{tower_rooms.NAMES[ch]}', v)

# =====================================================================
# output
# =====================================================================
anim = []
for im in COIN_FRAMES + FLAME_FRAMES:
    anim += planar(tile_px(im))
camp = []
for im in CAMP_FRAMES:
    for i in range(4):
        camp += planar(tile_px(im, (i % 2) * 8, (i // 2) * 8))
out = ['/* Generated by tools/make_tower.py - ARROW TOWER art. */']
out.append(carr('tw_bg_tiles', bg.data()))
out.append(carr('tw_spr_tiles', spr.data()))
out.append(carr('tw_anim_tiles', anim))
out.append(carr('tw_camp_tiles', camp))
out.append(carr('tw_bg_pals', [c for z in BG_PALS for c in z], 'uint16_t'))
out.append(carr('tw_obj_pals', OBJ_PALS, 'uint16_t'))
for k, v in TITLES.items():
    out.append(carr(f'tw_title_{k}', v))
names = []
for z in ZONES:
    names.append(z['name'])
out.append('static const char tw_zone_names[6][12]={' + ','.join(f'"{n}"' for n in names) + '};')
(INC / 'tt_art_tower.h').write_text('\n'.join(out) + '\n')

out = ['/* Generated by tools/make_tower.py from tools/tower_rooms.py - room templates.',
       '   RLE: byte >= 0x80 is a run of (byte & 0x7F) empty cells; other bytes are RC_* codes.',
       '   info per room: tier, entry lane mask (bit0 L, bit1 M, bit2 R), exit lane (0 L, 1 M, 2 R), flags. */']
out.append(carr('tw_room_data', blob))
out.append(carr('tw_room_offsets', offsets, 'uint16_t'))
out.append(carr('tw_room_info', info))
(INC / 'tt_tower_rooms.h').write_text('\n'.join(out) + '\n')

hdr = ['/* Generated by tools/make_tower.py - ARROW TOWER constants. */', '#ifndef TT_TOWER_DEFS_H', '#define TT_TOWER_DEFS_H']
hdr += defines + ['#endif', '']
(INC / 'tt_tower_defs.h').write_text('\n'.join(hdr))
print(f'tower: bg {len(bg)} tiles, sprites {len(spr)}, rooms {len(rooms)} ({len(blob)} bytes)')

# =====================================================================
# previews
# =====================================================================


def rgbhex(h):
    return tuple(bytes.fromhex(h))


def zone_rgb(zi, palno):
    z = BG_PALS[zi]
    out = []
    for c in z[palno * 4:palno * 4 + 4]:
        r, g, b = c & 31, (c >> 5) & 31, (c >> 10) & 31
        out.append((r * 255 // 31, g * 255 // 31, b * 255 // 31))
    return out


def tile_img(idx, tiles):
    im = Image.new('L', (8, 8))
    im.putdata(tiles[idx])
    return im


def paint(dst, tim, x, y, cols, transparent=False, flipx=False, flipy=False):
    if flipx:
        tim = tim.transpose(Image.FLIP_LEFT_RIGHT)
    if flipy:
        tim = tim.transpose(Image.FLIP_TOP_BOTTOM)
    for yy in range(tim.height):
        for xx in range(tim.width):
            v = tim.getpixel((xx, yy))
            if transparent and v == 0:
                continue
            dst.putpixel((x + xx, y + yy), cols[v])


# sprite sheet on a mid background
sheet = Image.new('RGB', (16 * 8 + 16, ((len(spr) + 15) // 16) * 8 + 8), (60, 70, 90))
obj_rgb = [[rgbhex(c) for c in p] for p in OBJ_PAL_HEX]
spr_pal = {}
for k in HERO_ORDER:
    pass
for i, t in enumerate(spr.tiles):
    tim = Image.new('L', (8, 8))
    tim.putdata(t)
    paint(sheet, tim, 4 + (i % 16) * 8, 4 + (i // 16) * 8, obj_rgb[0], transparent=True)
sheet.resize((sheet.width * 5, sheet.height * 5), Image.NEAREST).save(PREV / 'tower_sprites.png')

# hero animation strip with real palette
strip = Image.new('RGB', (len(HERO_ORDER) * 20, 20), rgbhex(ZONES[0]['base']))
for i, k in enumerate(HERO_ORDER):
    paint(strip, frames[k], i * 20 + 2, 2, obj_rgb[0], transparent=True)
strip.resize((strip.width * 6, strip.height * 6), Image.NEAREST).save(PREV / 'tower_hero.png')


def T(name):
    for d in defines:
        parts = d.split()
        if parts[1] == name:
            return int(parts[2])
    raise KeyError(name)


def room_preview(rm, zi, mirror=False, entry=1):
    """Approximation of tt_tower_gen.c tile choice, for eyeballing the art."""
    rows = rm['rows']
    if mirror:
        rows = [r[::-1] for r in rows]
    cells = [['#'] * 20 for _ in range(16)]
    for y in range(15):
        for x in range(18):
            ch = rows[y][x]
            if ch == '1' or ch in '23':
                ch = '.' if ch == '1' else '^'
            if ch in 'ceh':
                ch = 'o' if ch != 'e' else 'E'
            cells[y][x + 1] = ch
    a, b = LANES_ROOM[entry]
    for x in range(a, b + 1):
        cells[15][x] = '='
    img = Image.new('RGB', (160, 144))
    bgp = [zone_rgb(zi, p) for p in range(8)]

    def stoneish(c):
        return c in '#%C'

    def at(x, y):
        if x < 0 or x > 19:
            return '#'
        if y < 0 or y > 15:
            return '#'
        return cells[y][x]

    sprites = []
    for y in range(16):
        for x in range(20):
            c = cells[y][x]
            flipx = flipy = False
            if y in (0, 15) and c in '#=':
                if c == '#':
                    t, pal = T('TW_T_SLAB0') + (x & 1), 1
                else:
                    l = at(x - 1, y) == '='
                    r = at(x + 1, y) == '='
                    t = T('TW_T_PLANK_M') if l and r else T('TW_T_PLANK_L') if r else T('TW_T_PLANK_R')
                    pal = 3
            elif c in '#iW' and not (c == 'i' and ZONES[zi]['name'] == 'FROST VAULT'):
                fam = 'W' if c == 'W' else '#'
                same = (lambda ch: ch == 'W') if fam == 'W' else (lambda ch: ch in '#i' or ch == '=' and False)
                e = 0
                if not (same(at(x, y - 1)) or y - 1 in (0,) and fam == '#'):
                    e |= 1
                if not (same(at(x, y + 1)) or y + 1 == 15 and fam == '#'):
                    e |= 2
                if not same(at(x - 1, y)):
                    e |= 4
                if not same(at(x + 1, y)):
                    e |= 8
                if y == 0 or y == 15:
                    e = 0
                t = (T('TW_T_WOOD') if fam == 'W' else T('TW_T_STONE')) + e
                pal = 3 if fam == 'W' else 1
            elif c == 'i':
                t, pal = (T('TW_T_ICE_TOP') if at(x, y - 1) not in '#iW%' else T('TW_T_ICE')), 7
            elif c == '=':
                l = at(x - 1, y) == '='
                r = at(x + 1, y) == '='
                t = T('TW_T_PLANK_M') if l and r else T('TW_T_PLANK_L') if r else T('TW_T_PLANK_R') if l else T('TW_T_PLANK_M')
                pal = 3
            elif c == '^':
                t, pal = T('TW_T_SPIKE'), 4
            elif c == 'v':
                t, pal, flipy = T('TW_T_SPIKE'), 4, True
            elif c == '%':
                t, pal = T('TW_T_CRUMBLE0'), 1
            elif c == 's':
                t, pal = T('TW_T_SPRING'), 7
            elif c == 'o':
                t, pal = T('TW_T_COIN'), 5
            elif c == 'g':
                t, pal = T('TW_T_GEM'), 7
            elif c == 'C':
                t, pal = T('TW_T_CANNON'), 4
                flipx = x > 9
            elif c in '-S':
                t, pal = T('TW_T_RAIL_H'), 2
            elif c in ':Z':
                t, pal = T('TW_T_RAIL_V'), 2
            elif c in 'M~':
                t, pal = T('TW_T_TRACK'), 2
            elif c == 'F':
                t, pal = T('TW_T_CAMP') + 2, 6
            elif c == 'f':
                t, pal = T('TW_T_CAMP') + 3, 6
            else:
                t, pal = T('TW_T_WALL0') + ((x & 1) | ((y & 1) << 1)), 2
                if (x * 7 + y * 13) % 23 == 0 and (x & 1) == 0 and (y & 1) == 0:
                    t = T('TW_T_WALL_CRACK')
            paint(img, tile_img(t, bg.tiles), x * 8, y * 8, bgp[pal], flipx=flipx, flipy=flipy)
            if c in 'EBGSZMI':
                sprites.append((c, x, y))
            if c == 'F':
                paint(img, tile_img(T('TW_T_CAMP'), bg.tiles), x * 8, (y - 1) * 8, bgp[6])
                paint(img, tile_img(T('TW_T_CAMP') + 1, bg.tiles), x * 8 + 8, (y - 1) * 8, bgp[6])
    # torches and a window for flavour, like the generator
    for (tx, ty) in ((3, 4), (16, 9)):
        if cells[ty][tx] == '.' and cells[ty + 1][tx] == '.':
            paint(img, FLAME_FRAMES[0], tx * 8, ty * 8, bgp[6])
            paint(img, tile_img(T('TW_T_TORCH'), bg.tiles), tx * 8, ty * 8 + 8, bgp[6])
    sp = Image.new('L', (16, 16))

    def put16(first, x, y, pal, flip=False):
        im = Image.new('L', (16, 16))
        for i in range(4):
            ti = Image.new('L', (8, 8))
            ti.putdata(spr.tiles[first - 128 + i])
            im.paste(ti, ((i % 2) * 8, (i // 2) * 8))
        paint(img, im, x, y, obj_rgb[pal], transparent=True, flipx=flip)

    for c, x, y in sprites:
        if c in 'Ee':
            put16(T('TW_S_SLIME'), x * 8 - 4, y * 8 - 8, 1)
        elif c in 'Bb':
            for i in range(2):
                ti = Image.new('L', (8, 8))
                ti.putdata(spr.tiles[T('TW_S_BAT') - 128 + i])
                paint(img, ti, x * 8 - 4 + i * 8, y * 8, obj_rgb[3], transparent=True)
        elif c in 'SZ':
            put16(T('TW_S_SAW'), x * 8 - 4, y * 8 - 4, 5)
        elif c == 'G':
            put16(T('TW_S_GHOST'), x * 8 - 4, y * 8 - 8, 3)
        elif c == 'M':
            for i in range(3):
                ti = Image.new('L', (8, 8))
                ti.putdata(spr.tiles[T('TW_S_PLAT') - 128 + i])
                paint(img, ti, x * 8 + i * 8, y * 8, obj_rgb[6], transparent=True)
        elif c == 'I':
            ti = Image.new('L', (8, 8))
            ti.putdata(spr.tiles[T('TW_S_ICICLE') - 128])
            paint(img, ti, x * 8, y * 8, obj_rgb[5], transparent=True)
    # hero on the floor in the entry lane
    put16(T('TW_S_HERO_IDLE0'), a * 8 + 4, 15 * 8 - 16, 0)
    # HUD bar
    hud = [rgbhex(c) for c in HUD]
    for yy in range(128, 144):
        for xx in range(160):
            img.putpixel((xx, yy), hud[0])
    return img


LANES_ROOM = {0: (2, 5), 1: (8, 11), 2: (14, 17)}

if __name__ == '__main__':
    picks = ['entrance', 'switchback', 'wood wall', 'bookshelves', 'saw walk', 'ferry', 'ice steps', 'cannon alley',
             'haunted stacks', 'rest', 'spike garden', 'gear tower']
    sheet = Image.new('RGB', (4 * 164, 3 * 148), (0, 0, 0))
    for i, name in enumerate(picks):
        rm = [r for r in rooms if r['name'] == name][0]
        lanes = [k for k in range(3) if rm['entry'] & (1 << k)]
        im = room_preview(rm, i % 6, mirror=False, entry=lanes[0])
        sheet.paste(im, ((i % 4) * 164, (i // 4) * 148))
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(PREV / 'tower_rooms.png')
    print('previews written')
