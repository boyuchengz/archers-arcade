"""Ten original rival portraits, 40x40 (5x5 tiles), four expressions each.

Tile rows 0-2 use palette A [backdrop, skin, hair/accent, ink];
tile rows 3-4 use palette B [backdrop, skin, shirt, ink].
Writes include/states/tt_art_portraits.h and tools/preview/portraits.png.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from PIL import Image, ImageDraw  # noqa: E402
from artlib import TileSet, carr, rgb, rgb15  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
INC = ROOT / 'plugins/tabletop/engine/include/states'
PREV = ROOT / 'tools/preview'

BD, SKIN, HAIR, INK, SHIRT = 0, 1, 2, 3, 4
INKHEX = '12152e'

RIVALS = [
    # name, backdrop, skin, hair/accent, shirt
    ('PIP', 'ffd23a', 'ffd0a8', 'ff7a1a', '3aa0ff'),
    ('RUSH', 'ff7448', 'e8a070', 'c0102a', '2f3a7a'),
    ('MARA', '2ec4b6', 'f2c27e', '7a4322', 'ff6f91'),
    ('JUNE', '8ee88a', 'b9773f', 'e8c070', 'ff9a2e'),
    ('VEX', '6a46d8', 'ffe2d2', 'c8a6ff', 'c8a6ff'),
    ('ORIN', '4cb050', 'd8956a', 'f4f4f4', 'f4f4f4'),
    ('NOVA', '5cc4ff', 'ffd4bc', 'ff4fa8', 'ffffff'),
    ('ROOK', '8290b8', 'f0c49c', '9ee0ff', '263a66'),
    ('IRIS', 'c6a8ff', 'ffdcc8', 'e8ecf8', '22a898'),
    ('SOREN', '3d4fc8', 'e0aa80', 'c8c8d8', 'b8283c'),
]
EXPRS = ['neutral', 'happy', 'shocked', 'sad']


def base_body(d, shirt=SHIRT, neck=True):
    # shoulders / torso
    d.rounded_rectangle((6, 31, 33, 45), radius=7, fill=INK)
    d.rounded_rectangle((7, 32, 32, 45), radius=6, fill=shirt)
    if neck:
        d.rectangle((16, 27, 23, 32), fill=INK)
        d.rectangle((17, 27, 22, 32), fill=SKIN)
        d.line((17, 32, 22, 32), fill=INK)


def face(d, x0=11, y0=9, x1=28, y1=29):
    d.ellipse((x0, y0, x1, y1), fill=INK)
    d.ellipse((x0 + 1, y0 + 1, x1 - 1, y1 - 1), fill=SKIN)
    # ears
    d.rectangle((x0 - 1, 18, x0, 21), fill=INK)
    d.rectangle((x1, 18, x1 + 1, 21), fill=INK)
    d.point([(x0, 19), (x0, 20), (x1, 19), (x1, 20)], fill=SKIN)


def eyes(d, expr, y=18, lx=15, rx=23, closed=False, brow=INK, brow_y=None):
    by = (y - 3) if brow_y is None else brow_y
    if expr == 'neutral':
        if closed:
            d.line((lx - 1, y + 1, lx + 1, y + 1), fill=INK)
            d.line((rx, y + 1, rx + 2, y + 1), fill=INK)
        else:
            d.rectangle((lx, y, lx + 1, y + 2), fill=INK)
            d.rectangle((rx, y, rx + 1, y + 2), fill=INK)
        d.line((lx - 1, by, lx + 2, by), fill=brow)
        d.line((rx - 1, by, rx + 2, by), fill=brow)
    elif expr == 'happy':
        for cx in (lx, rx):
            d.point([(cx - 1, y + 1), (cx, y), (cx + 1, y), (cx + 2, y + 1)], fill=INK)
        d.line((lx - 1, by - 1, lx + 2, by - 1), fill=brow)
        d.line((rx - 1, by - 1, rx + 2, by - 1), fill=brow)
    elif expr == 'shocked':
        for cx in (lx, rx):
            d.rectangle((cx - 1, y - 1, cx + 2, y + 2), fill=INK)
            d.rectangle((cx, y, cx + 1, y + 1), fill=SKIN)
            d.point((cx + 1, y + 1), fill=INK)
        d.line((lx - 1, by - 2, lx + 2, by - 2), fill=brow)
        d.line((rx - 1, by - 2, rx + 2, by - 2), fill=brow)
    elif expr == 'sad':
        d.rectangle((lx, y + 1, lx + 1, y + 2), fill=INK)
        d.rectangle((rx, y + 1, rx + 1, y + 2), fill=INK)
        d.line((lx - 1, by + 1, lx + 2, by - 1), fill=brow)
        d.line((rx - 1, by - 1, rx + 2, by + 1), fill=brow)


def mouth(d, expr, y=25, cx=19.5, wide=False):
    c = int(cx)
    if expr == 'neutral':
        d.line((c - 2, y, c + 1, y), fill=INK)
    elif expr == 'happy':
        w = 3 if wide else 2
        d.point([(c - w - 1, y - 1), (c + w, y - 1)], fill=INK)
        d.line((c - w, y, c + w - 1, y), fill=INK)
        d.line((c - w + 1, y + 1, c + w - 2, y + 1), fill=INK)
    elif expr == 'shocked':
        d.point([(c - 1, y), (c - 2, y + 1), (c, y + 1), (c - 1, y + 2)], fill=INK)
    elif expr == 'sad':
        d.line((c - 2, y, c + 1, y), fill=INK)
        d.point([(c - 3, y + 1), (c + 2, y + 1)], fill=INK)


def sweat(d, x=29, y=12):
    d.polygon([(x, y), (x - 2, y + 4), (x + 2, y + 4)], fill=INK)
    d.ellipse((x - 2, y + 3, x + 2, y + 6), fill=INK)
    d.point([(x, y + 4)], fill=HAIR if y < 20 else SKIN)


def draw(k, expr):
    im = Image.new('L', (40, 40), BD)
    d = ImageDraw.Draw(im)
    name = RIVALS[k][0]
    if name == 'PIP':
        base_body(d)
        d.polygon([(19, 33), (20, 35), (22, 35), (20, 37), (21, 39), (19, 38), (17, 39), (18, 37), (16, 35), (18, 35)], fill=BD)
        face(d)
        # spiky orange hair
        d.polygon([(9, 17), (8, 9), (12, 11), (13, 4), (17, 8), (20, 2), (23, 8), (27, 4), (27, 11), (32, 9), (30, 17), (27, 12), (13, 12)], fill=INK)
        d.polygon([(10, 15), (10, 11), (13, 12), (14, 6), (17, 9), (20, 4), (23, 9), (26, 6), (26, 12), (30, 11), (29, 15), (26, 11), (14, 11)], fill=HAIR)
        eyes(d, expr)
        d.point([(13, 22), (15, 23), (24, 23), (26, 22)], fill=HAIR)
        mouth(d, 'happy' if expr == 'neutral' else expr, wide=True)
    elif name == 'RUSH':
        base_body(d)
        d.line((16, 33, 19, 37), fill=INK)
        d.line((23, 33, 20, 37), fill=INK)
        face(d)
        d.polygon([(10, 15), (6, 3), (13, 9), (14, 0), (19, 7), (24, 0), (26, 9), (33, 3), (29, 15)], fill=INK)
        d.polygon([(11, 13), (8, 5), (13, 10), (15, 3), (19, 9), (23, 3), (25, 10), (31, 5), (28, 13)], fill=HAIR)
        d.rectangle((10, 12, 29, 14), fill=INK)
        d.line((30, 13, 35, 17), fill=INK)
        d.line((30, 14, 34, 19), fill=INK)
        if expr == 'neutral':
            d.line((13, 15, 17, 17), fill=INK)
            d.line((26, 15, 22, 17), fill=INK)
            d.rectangle((15, 18, 16, 20), fill=INK)
            d.rectangle((23, 18, 24, 20), fill=INK)
            d.line((17, 25, 22, 24), fill=INK)
        else:
            eyes(d, expr, brow_y=16)
            mouth(d, expr, wide=True)
    elif name == 'MARA':
        base_body(d)
        d.polygon([(12, 33), (19, 39), (27, 33)], fill=BD)
        d.line((12, 33, 19, 39), fill=INK)
        d.line((27, 33, 19, 39), fill=INK)
        face(d)
        d.ellipse((14, 0, 25, 8), fill=INK)
        d.ellipse((15, 1, 24, 7), fill=HAIR)
        d.chord((10, 6, 29, 24), 180, 360, fill=INK)
        d.chord((11, 7, 28, 23), 180, 360, fill=HAIR)
        d.rectangle((10, 14, 12, 23), fill=INK)
        d.rectangle((27, 14, 29, 23), fill=INK)
        d.rectangle((11, 14, 11, 22), fill=HAIR)
        d.rectangle((28, 14, 28, 22), fill=HAIR)
        eyes(d, expr, brow_y=14)
        d.rectangle((12, 16, 18, 21), outline=INK)
        d.rectangle((21, 16, 27, 21), outline=INK)
        d.line((18, 17, 21, 17), fill=INK)
        mouth(d, expr)
    elif name == 'JUNE':
        base_body(d)
        d.line((10, 33, 14, 39), fill=INK)
        d.line((29, 33, 25, 39), fill=INK)
        face(d)
        # braid down the side (ink)
        d.rectangle((27, 16, 31, 30), fill=INK)
        # explorer hat
        d.ellipse((5, 9, 34, 16), fill=INK)
        d.ellipse((6, 10, 33, 15), fill=HAIR)
        d.chord((10, 1, 29, 22), 180, 360, fill=INK)
        d.chord((11, 2, 28, 21), 180, 360, fill=HAIR)
        d.rectangle((11, 9, 28, 10), fill=INK)
        d.line((10, 15, 29, 15), fill=INK)
        eyes(d, expr)
        mouth(d, expr)
    elif name == 'VEX':
        # hood
        d.ellipse((5, 2, 34, 34), fill=INK)
        d.ellipse((6, 3, 33, 33), fill=HAIR)
        base_body(d, neck=False)
        d.ellipse((5, 2, 34, 30), fill=INK)
        d.ellipse((6, 3, 33, 29), fill=HAIR)
        d.ellipse((11, 9, 28, 30), fill=INK)
        d.ellipse((12, 10, 27, 29), fill=SKIN)
        # bangs covering the left eye
        d.polygon([(11, 15), (14, 8), (25, 8), (21, 12), (19, 21), (16, 17), (12, 21)], fill=INK)
        if expr == 'neutral':
            d.rectangle((23, 18, 24, 20), fill=INK)
            d.line((22, 16, 25, 15), fill=INK)
            d.line((17, 25, 21, 25), fill=INK)
            d.point((22, 24), fill=INK)
        else:
            eyes(d, expr, lx=23, rx=23)
            mouth(d, expr)
    elif name == 'ORIN':
        base_body(d, shirt=INK)
        face(d, 11, 7, 28, 28)
        # big white beard flowing into the robe area
        d.polygon([(11, 20), (13, 30), (16, 36), (20, 38), (24, 36), (27, 30), (28, 20), (24, 23), (15, 23)], fill=INK)
        d.polygon([(12, 21), (14, 30), (17, 35), (20, 37), (23, 35), (26, 30), (27, 21), (24, 24), (15, 24)], fill=SHIRT)
        # bald head shine and side hair
        d.point([(15, 10), (16, 9), (17, 9)], fill=BD)
        d.rectangle((10, 13, 11, 18), fill=HAIR)
        d.rectangle((28, 13, 29, 18), fill=HAIR)
        eyes(d, expr, y=16, brow=HAIR, closed=True)
        d.line((13, 13, 17, 13), fill=HAIR)
        d.line((22, 13, 26, 13), fill=HAIR)
        if expr == 'shocked':
            d.ellipse((18, 22, 21, 26), fill=INK)
        elif expr == 'happy':
            d.line((17, 23, 22, 23), fill=INK)
        elif expr == 'sad':
            d.line((17, 23, 22, 23), fill=INK)
            d.point([(16, 24), (23, 24)], fill=INK)
    elif name == 'NOVA':
        base_body(d)
        d.polygon([(19, 33), (20, 35), (22, 35), (20, 37), (21, 39), (19, 38), (17, 39), (18, 37), (16, 35), (18, 35)], fill=HAIR if False else INK)
        # side ponytail
        d.ellipse((26, 4, 37, 22), fill=INK)
        d.ellipse((27, 5, 36, 21), fill=HAIR)
        face(d)
        d.chord((9, 4, 30, 26), 180, 360, fill=INK)
        d.chord((10, 5, 29, 25), 180, 360, fill=HAIR)
        d.polygon([(10, 15), (16, 9), (17, 15), (22, 9), (29, 15), (29, 13), (10, 13)], fill=HAIR)
        d.line((11, 15, 15, 12), fill=INK)
        # star clip
        d.polygon([(27, 6), (28, 8), (30, 8), (28.5, 9.5), (29, 12), (27, 10.5), (25, 12), (25.5, 9.5), (24, 8), (26, 8)], fill=INK)
        d.point((27, 9), fill=BD)
        if expr == 'neutral':
            d.rectangle((15, 18, 16, 20), fill=INK)
            d.point([(22, 19), (23, 18), (24, 18), (25, 19)], fill=INK)  # wink
            mouth(d, 'happy')
        else:
            eyes(d, expr)
            mouth(d, expr, wide=True)
    elif name == 'ROOK':
        base_body(d)
        d.polygon([(15, 32), (19, 36), (24, 32)], fill=BD)
        d.polygon([(18, 34), (19, 39), (20, 39), (21, 34)], fill=INK)
        face(d)
        d.chord((10, 5, 29, 22), 180, 360, fill=INK)
        d.polygon([(10, 14), (11, 9), (28, 9), (29, 14), (24, 11), (14, 12)], fill=INK)
        if expr in ('neutral', 'happy'):
            d.rectangle((12, 16, 18, 20), fill=INK)
            d.rectangle((21, 16, 27, 20), fill=INK)
            d.rectangle((13, 17, 17, 19), fill=HAIR)
            d.rectangle((22, 17, 26, 19), fill=HAIR)
            d.line((18, 17, 21, 17), fill=INK)
            d.point([(14, 17), (23, 17)], fill=SKIN)
            mouth(d, 'neutral' if expr == 'neutral' else 'happy')
            if expr == 'neutral':
                d.point((22, 24), fill=INK)
        else:
            eyes(d, expr)
            mouth(d, expr)
    elif name == 'IRIS':
        base_body(d)
        d.polygon([(14, 32), (19, 36), (25, 32)], fill=SKIN)
        d.line((14, 32, 19, 36), fill=INK)
        d.line((25, 32, 19, 36), fill=INK)
        face(d)
        d.chord((8, 5, 31, 26), 180, 360, fill=INK)
        d.chord((9, 6, 30, 25), 180, 360, fill=HAIR)
        d.rectangle((8, 15, 12, 23), fill=INK)
        d.rectangle((27, 15, 31, 23), fill=INK)
        d.rectangle((9, 15, 11, 22), fill=HAIR)
        d.rectangle((28, 15, 30, 22), fill=HAIR)
        d.polygon([(12, 14), (22, 9), (29, 14), (29, 12), (10, 12)], fill=HAIR)
        d.line((13, 14, 21, 10), fill=INK)
        # flower
        d.ellipse((7, 7, 12, 12), fill=INK)
        d.ellipse((8, 8, 11, 11), fill=BD)
        eyes(d, expr, closed=True)
        mouth(d, 'happy' if expr == 'happy' else expr)
    elif name == 'SOREN':
        base_body(d)
        d.polygon([(13, 32), (19, 38), (26, 32)], fill=INK)
        d.polygon([(15, 32), (19, 36), (24, 32)], fill=BD)
        face(d)
        d.polygon([(10, 19), (10, 11), (14, 8), (25, 8), (29, 11), (29, 19), (27, 13), (12, 13)], fill=INK)
        d.polygon([(11, 18), (11, 12), (14, 9), (25, 9), (28, 12), (28, 18), (26, 12), (13, 12)], fill=HAIR)
        d.point([(16, 10), (17, 10)], fill=SKIN)
        eyes(d, expr, brow=HAIR, brow_y=15)
        d.line((13, 15, 17, 15), fill=HAIR)
        d.line((22, 15, 26, 15), fill=HAIR)
        # monocle
        d.ellipse((21, 16, 27, 22), outline=INK)
        d.line((27, 21, 28, 26), fill=INK)
        # moustache in hair colour (row 2 only)
        d.polygon([(15, 23), (19, 21), (20, 21), (24, 23), (20, 23), (19, 23)], fill=INK)
        d.line((16, 22, 23, 22), fill=HAIR)
        mouth(d, expr, y=26)
    # frame
    d.rectangle((0, 0, 39, 39), outline=INK)
    d.rectangle((1, 1, 38, 38), outline=INK)
    if expr == 'shocked':
        for x0, y0, x1, y1 in ((3, 6, 5, 9), (8, 3, 9, 6), (36, 6, 34, 9), (31, 3, 30, 6)):
            d.line((x0, y0, x1, y1), fill=INK)
    return im


def to_indices(im, k):
    """Map semantic colours to per-row palette indices; enforce constraints."""
    name, bd, skin, hair, shirt = RIVALS[k]
    out = Image.new('L', im.size)
    for y in range(40):
        for x in range(40):
            v = im.getpixel((x, y))
            if v == SHIRT:
                if y < 24:
                    assert hair == shirt, (name, x, y, 'shirt colour in top rows')
                v = HAIR
            elif v == HAIR and y >= 24:
                assert hair == shirt, (name, x, y, 'hair colour in bottom rows')
            out.putpixel((x, y), v)
    return out


tiles_all, maps_all, pals_all = [], [], []
sheet = Image.new('RGB', (10 * 44 * 3, 4 * 44 * 3), (40, 40, 40))
counts = []
for k in range(10):
    name, bd, skin, hair, shirt = RIVALS[k]
    ts = TileSet(0, 64, name)
    maps = []
    for e, expr in enumerate(EXPRS):
        im = to_indices(draw(k, expr), k)
        maps += ts.stamp(im)
        rgbim = Image.new('RGB', (40, 40))
        for y in range(40):
            for x in range(40):
                v = im.getpixel((x, y))
                cols = [bd, skin, hair if y < 24 else shirt, INKHEX]
                rgbim.putpixel((x, y), rgb(cols[v]))
        sheet.paste(rgbim.resize((120, 120), Image.NEAREST), (k * 132, e * 132))
    counts.append(len(ts))
    tiles_all.append(ts.data())
    maps_all.append(maps)
    pals_all.append([rgb15(c) for c in (bd, skin, hair, INKHEX)] + [rgb15(c) for c in (bd, skin, shirt, INKHEX)])
sheet.save(PREV / 'portraits.png')
out = ['/* Generated by tools/make_portraits.py - ten rivals x four expressions, 5x5 tiles. */']
for k in range(10):
    out.append(carr(f'portrait_tiles_{k}', tiles_all[k]))
out.append('static const uint8_t * const portrait_tiles[10]={' + ','.join(f'portrait_tiles_{k}' for k in range(10)) + '};')
out.append(carr('portrait_counts', counts))
out.append('static const uint8_t portrait_maps[10][100]={' + ',\n'.join('{' + ','.join(map(str, m)) + '}' for m in maps_all) + '};')
out.append('static const uint16_t portrait_pals[10][8]={' + ',\n'.join('{' + ','.join(map(str, p)) + '}' for p in pals_all) + '};')
(INC / 'tt_art_portraits.h').write_text('\n'.join(out) + '\n')
print('portrait unique tiles', counts)
