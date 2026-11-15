"""Generate every tile, map and palette for Archer's Arcade.

    python3 tools/make_art.py            # writes engine headers + tools/preview/*.png

Outputs (all static const data, included by the C file that owns each bank):
    include/states/tt_tiles.h        tile index / palette constants shared by C code
    include/states/tt_art_base.h     font, UI kit, sprites, big digits  (tt_gfx.c)
    include/states/tt_art_menu.h     pattern, title logo, illustrations  (tt_menu.c)
    include/states/tt_art_puzzle.h   2048 slots, cards, labels           (tt_puzzle_view.c)
    include/states/tt_art_board.h    gomoku board and stones             (tt_gomoku.c)
"""
import math
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from PIL import Image, ImageDraw  # noqa: E402
from artlib import (C, TileSet, carr, new, pal, planar, preview, put_rows, rgb, rgb15, tile_px)  # noqa: E402
from font import glyph, glyph_width, tiny  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
INC = ROOT / 'plugins/tabletop/engine/include/states'
PREV = ROOT / 'tools/preview'
PREV.mkdir(exist_ok=True)
defines = []


def define(name, value):
    defines.append(f'#define {name} {value}')


def draw_text(im, s, x, y, color=3, shadow=1):
    for ch in s:
        w = glyph_width(ch)
        put_rows(im, glyph(ch), x, y, color=color, shadow=shadow)
        x += w + 1 if ch != ' ' else 4
    return x


def text_width(s):
    return sum((glyph_width(c) + 1) if c != ' ' else 4 for c in s) - 1


def draw_tiny(im, s, x, y, color=3, shadow=None):
    for ch in s:
        rows = tiny(ch)
        put_rows(im, rows, x, y, color=color, shadow=shadow)
        x += len(rows[0]) + 1
    return x


def tiny_width(s):
    return sum(len(tiny(c)[0]) + 1 for c in s) - 1


NARROW_SPECIAL = {
    'M': ['#...#', '##.##', '#.#.#', '#.#.#', '#...#', '#...#', '#...#'],
    'W': ['#...#', '#...#', '#...#', '#.#.#', '#.#.#', '##.##', '#...#'],
    'I': ['###', '.#.', '.#.', '.#.', '.#.', '.#.', '###'],
    ' ': ['..'] * 7, '!': ['#'] * 5 + ['.', '#'], '1': ['.#.', '##.', '.#.', '.#.', '.#.', '.#.', '###'],
}


def narrow(ch):
    if ch in NARROW_SPECIAL:
        return NARROW_SPECIAL[ch]
    rows = glyph(ch)
    out = []
    for r in rows:
        r = (r + '......')[:6]
        out.append(r[0] + r[2] + r[3] + r[5])
    return out


def draw_narrow(im, s, x, y, color=3, shadow=1):
    for ch in s:
        rows = narrow(ch)
        put_rows(im, rows, x, y, color=color, shadow=shadow)
        x += len(rows[0]) + 1
    return x


def narrow_width(s):
    return sum(len(narrow(c)[0]) + 1 for c in s) - 1


def scaled_glyph(ch, sx, sy):
    rows = glyph(ch)
    out = []
    for r in rows:
        line = ''.join(c * sx for c in r)
        out += [line] * sy
    return out


def outline(im, src_color_set, out_color, diag=True):
    """Add a 1px outline in out_color around pixels whose value is in src_color_set."""
    w, h = im.size
    px = im.load()
    marks = []
    for y in range(h):
        for x in range(w):
            if px[x, y] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)) + (((1, 1), (-1, -1), (1, -1), (-1, 1)) if diag else ()):
                xx, yy = x + dx, y + dy
                if 0 <= xx < w and 0 <= yy < h and px[xx, yy] in src_color_set:
                    marks.append((x, y))
                    break
    for x, y in marks:
        px[x, y] = out_color


# =====================================================================
# BANK 0: UI kit (0..31, 128..159), font (32..127), sprites (192..255)
# =====================================================================
base = [None] * 160


def setb(n, im):
    base[n] = tile_px(im)


blank = new(8, 8)
setb(0, blank)
for c in (1, 2, 3):
    setb(c, new(8, 8, c))
define('TT_T_BLANK', 0)
define('TT_T_SOLID1', 1)
define('TT_T_SOLID2', 2)
define('TT_T_SOLID3', 3)


def nine_slice(kind):
    """24x24 image of a rounded card; colours: 0 face, 1 lip, 2 outside, 3 outline."""
    im = new(24, 24, 0)
    d = ImageDraw.Draw(im)
    if kind == 'card':
        # outline rounded rect with a 2px lip at the bottom
        d.rectangle((0, 0, 23, 23), fill=2)
        d.rounded_rectangle((0, 0, 23, 23), radius=3, fill=3)
        d.rounded_rectangle((1, 1, 22, 22), radius=2, fill=1)
        d.rounded_rectangle((1, 1, 22, 20), radius=2, fill=0)
    else:  # 'plate': flat rounded, 1px outline in colour 3, no lip
        d.rectangle((0, 0, 23, 23), fill=2)
        d.rounded_rectangle((0, 0, 23, 23), radius=3, fill=3)
        d.rounded_rectangle((1, 1, 22, 22), radius=2, fill=0)
    return im


for start, kind in ((4, 'card'), (13, 'plate')):
    im = nine_slice(kind)
    for i in range(9):
        setb(start + i, im.crop(((i % 3) * 8, (i // 3) * 8, (i % 3) * 8 + 8, (i // 3) * 8 + 8)))
define('TT_T_CARD', 4)
define('TT_T_PLATE', 13)


def badge(letter_rows):
    im = new(8, 8)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((0, 0, 7, 7), radius=2, fill=2)
    put_rows(im, letter_rows, 2, 1, color=0)
    return im


setb(22, badge(['.##.', '#..#', '####', '#..#', '#..#']))  # A
setb(23, badge(['###.', '#..#', '###.', '#..#', '###.']))  # B
im = new(8, 8)
put_rows(im, ['..##..', '..##..', '######', '######', '..##..', '..##..'], 1, 1, color=2)
setb(24, im)  # d-pad
for start, word in ((25, 'START'), (28, 'SELECT')):
    im = new(24, 8)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((0, 0, 23, 7), radius=3, fill=2)
    draw_tiny(im, word, (24 - tiny_width(word)) // 2, 1, color=0)
    for i in range(3):
        setb(start + i, im.crop((i * 8, 0, i * 8 + 8, 8)))
define('TT_T_BTN_A', 22)
define('TT_T_BTN_B', 23)
define('TT_T_DPAD', 24)
define('TT_T_START', 25)
define('TT_T_SELECT', 28)


def stone_icon(white):
    im = new(8, 8)
    d = ImageDraw.Draw(im)
    if white:
        d.ellipse((0, 0, 6, 6), fill=3)
        d.arc((0, 0, 6, 6), 20, 160, fill=2)
        im.putpixel((2, 1), 3)
    else:
        d.ellipse((0, 0, 6, 6), fill=2)
        d.point([(1, 2), (2, 1)], fill=3)
    return im


# stone icons for dark panels: palette [bg, rim, black, white]
im = new(8, 8)
d = ImageDraw.Draw(im)
d.ellipse((0, 0, 6, 6), fill=1)
d.ellipse((1, 1, 5, 5), fill=2)
d.point([(2, 2)], fill=1)
setb(31, im)
im = new(8, 8)
d = ImageDraw.Draw(im)
d.ellipse((0, 0, 6, 6), fill=3)
d.arc((0, 0, 6, 6), 30, 150, fill=1)
setb(128, im)
define('TT_T_STONE_B', 31)
define('TT_T_STONE_W', 128)

ICONS = {
    129: ('STAR', ['...#...', '..###..', '#######', '.#####.', '..###..', '.##.##.', '.#...#.'], 2),
    130: ('UP', ['..#..', '.###.', '#####'], 2),
    131: ('DOWN', ['#####', '.###.', '..#..'], 2),
    132: ('LEFT', ['..#', '.##', '###', '.##', '..#'], 2),
    133: ('RIGHT', ['#..', '##.', '###', '##.', '#..'], 2),
    134: ('DOT', ['##', '##'], 2),
    137: ('HEART', ['.##.##.', '#######', '#######', '.#####.', '..###..', '...#...'], 2),
    138: ('CROWN', ['#..#..#', '##.#.##', '#######', '#######', '.#####.'], 2),
    139: ('UNDO', ['..#...', '.##...', '######', '.##..#', '..#..#', '.....#', '.####.'], 2),
    140: ('CHECK', ['.....##', '....##.', '##.##..', '.###...', '..#....'], 2),
    141: ('CROSS', ['##...##', '.##.##.', '..###..', '.##.##.', '##...##'], 2),
    146: ('SOUND', ['...#...', '..##.#.', '####..#', '####.#.', '####..#', '..##.#.', '...#...'], 2),
    147: ('NOTE', ['..####', '..#..#', '..#..#', '..#..#', '###.##.', '###.##.', '###....'], 2),
    148: ('TRASH', ['.####.', '######', '.#..#.', '.#..#.', '.#..#.', '.####.'], 2),
    149: ('PAUSE', ['##.##', '##.##', '##.##', '##.##', '##.##'], 2),
}
for n, (name, rows, color) in ICONS.items():
    im = new(8, 8)
    w, h = len(rows[0]), len(rows)
    put_rows(im, rows, (8 - w) // 2, (8 - h) // 2, color=color)
    setb(n, im)
    define(f'TT_T_{name}', n)
# horizontal rules
im = new(8, 8)
ImageDraw.Draw(im).line((0, 7, 7, 7), fill=2)
setb(135, im)
define('TT_T_RULE_BOTTOM', 135)
im = new(8, 8)
ImageDraw.Draw(im).line((0, 0, 7, 0), fill=2)
setb(136, im)
define('TT_T_RULE_TOP', 136)
im = new(8, 8)
for x in range(0, 8, 2):
    im.putpixel((x, 3), 2)
setb(150, im)
define('TT_T_RULE_DOTS', 150)
# toggle pills (2 tiles each): ON knob right in colour 2, OFF knob left in colour 1
for start, on in ((142, True), (144, False)):
    im = new(16, 8)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((0, 0, 15, 7), radius=3, fill=2 if on else 1)
    if on:
        d.ellipse((9, 1, 14, 6), fill=3)
    else:
        d.ellipse((1, 1, 6, 6), fill=3)
    setb(start, im.crop((0, 0, 8, 8)))
    setb(start + 1, im.crop((8, 0, 16, 8)))
define('TT_T_TOGGLE_ON', 142)
define('TT_T_TOGGLE_OFF', 144)
# medal (2x1 icons would be wide; single tile medal)
im = new(8, 8)
d = ImageDraw.Draw(im)
d.polygon([(1, 0), (3, 3), (4, 3), (6, 0)], fill=1)
d.ellipse((1, 2, 6, 7), fill=2)
d.point([(3, 4), (4, 5)], fill=3)
setb(151, im)
define('TT_T_MEDAL', 151)

# font 32..127
for code in range(32, 128):
    im = new(8, 8)
    ch = chr(code)
    if code == 127:
        ch = ' '
    w = glyph_width(ch)
    put_rows(im, glyph(ch), (7 - w) // 2, 0, color=3, shadow=1)
    setb(code, im)
for i in range(160):
    if base[i] is None:
        base[i] = tuple([0] * 64)

# sprites (stored separately, loaded at tile 192)
spr = TileSet(192, 256, 'sprites')


def sp(im, name):
    n = spr.add(tile_px(im), dedupe=False)
    define(f'TT_S_{name}', n)
    return n


# cursor corner brackets (colour 2 fill, 3 outline)
for name, flipx, flipy in (('CUR_TL', 0, 0), ('CUR_TR', 1, 0), ('CUR_BL', 0, 1), ('CUR_BR', 1, 1)):
    im = new(8, 8)
    rows = ['#####...', '#222#...', '#2###...', '#2#.....', '###.....']
    put_rows(im, rows, 0, 0, color=3, mapping={'2': 2})
    if flipx:
        im = im.transpose(Image.FLIP_LEFT_RIGHT)
    if flipy:
        im = im.transpose(Image.FLIP_TOP_BOTTOM)
    sp(im, name)
arrow = ['##......', '#2##....', '#122##..', '#12222#.', '#12222#.', '#122##..', '#2##....', '##......']
im = new(8, 8)
put_rows(im, arrow, 0, 0, color=3, mapping={'2': 2, '1': 1})
sp(im, 'ARROW_R')
sp(im.transpose(Image.FLIP_LEFT_RIGHT), 'ARROW_L')
sp(im.transpose(Image.ROTATE_270), 'ARROW_D')
sparkles = [
    ['........', '........', '........', '...#....', '........', '........', '........', '........'],
    ['........', '...#....', '...#....', '.##2##..', '...#....', '...#....', '........', '........'],
    ['...#....', '...#....', '..#2#...', '###2###.', '..#2#...', '...#....', '...#....', '........'],
    ['........', '.#...#..', '..#.#...', '...2....', '..#.#...', '.#...#..', '........', '........'],
]
for i, rows in enumerate(sparkles):
    im = new(8, 8)
    put_rows(im, rows, 0, 0, color=2, mapping={'2': 1})
    sp(im, f'SPARK{i}')
confetti = [
    ['##', '##', '33'], ['.#', '##', '3.'], ['###', '333'], ['#', '#', '3'],
]
for i, rows in enumerate(confetti):
    im = new(8, 8)
    put_rows(im, rows, 3, 3, color=1, mapping={'3': 2})
    sp(im, f'CONFETTI{i}')


def board_stone(white, bg=0):
    """Stone in board palette [wood, line, black, white] on an 8x8 cell centred at (3,3)."""
    im = new(8, 8, bg)
    d = ImageDraw.Draw(im)
    # contact shadow
    d.ellipse((1, 1, 7, 7), fill=1)
    if white:
        d.ellipse((0, 0, 6, 6), fill=3)
        d.arc((0, 0, 6, 6), 0, 110, fill=1)
        d.point([(5, 4), (4, 5)], fill=1)
    else:
        d.ellipse((0, 0, 6, 6), fill=2)
        d.point([(2, 1), (1, 2), (2, 2)], fill=3)
    return im


sp(board_stone(False), 'STONE_B')
sp(board_stone(True), 'STONE_W')
im = new(8, 8)
d = ImageDraw.Draw(im)
d.rectangle((2, 2, 4, 4), fill=3)
d.rectangle((3, 2, 3, 4), fill=2)
d.rectangle((2, 3, 4, 3), fill=2)
d.point((3, 3), fill=1)
sp(im, 'MARK')
im = new(8, 8)
ImageDraw.Draw(im).rectangle((3, 3, 4, 4), fill=3)
sp(im, 'DOT')
im = new(8, 8)
put_rows(im, ICONS[129][1], 0, 0, color=2)
outline(im, {2}, 3, diag=False)
sp(im, 'STAR')
im = new(8, 8)
put_rows(im, ICONS[137][1], 0, 1, color=2)
sp(im, 'HEART')
im = new(8, 8)
ImageDraw.Draw(im).ellipse((1, 1, 5, 5), fill=2)
im.putpixel((2, 2), 1)
sp(im, 'BALL')
# page-indicator dots
im = new(8, 8)
d = ImageDraw.Draw(im)
d.ellipse((0, 0, 6, 6), fill=3)
d.ellipse((1, 1, 5, 5), fill=2)
im.putpixel((2, 2), 1)
sp(im, 'PAGE_ON')
im = new(8, 8)
ImageDraw.Draw(im).rectangle((2, 2, 4, 4), fill=1)
im.putpixel((2, 2), 0); im.putpixel((4, 2), 0); im.putpixel((2, 4), 0); im.putpixel((4, 4), 0)
sp(im, 'PAGE_OFF')
# big stones for title decoration (2x2 = 4 tiles each), 16px
for white in (False, True):
    im = new(16, 16)
    d = ImageDraw.Draw(im)
    if white:
        d.ellipse((0, 0, 14, 14), fill=3)
        d.ellipse((0, 0, 14, 14), outline=1)
        d.arc((1, 1, 13, 13), 10, 120, fill=1)
        d.point([(4, 3), (3, 4)], fill=2)
    else:
        d.ellipse((0, 0, 14, 14), fill=2)
        d.ellipse((0, 0, 14, 14), outline=1)
        d.ellipse((3, 3, 6, 6), fill=3)
        d.point([(4, 4)], fill=2)
    name = 'BIG_W' if white else 'BIG_B'
    for i in range(4):
        n = spr.add(tile_px(im, (i % 2) * 8, (i // 2) * 8), dedupe=False)
        if i == 0:
            define(f'TT_S_{name}', n)
define('TT_SPRITE_TILE_COUNT', len(spr))

# big digits 4x7 for software-rendered numbers
BIGD = {
    '0': ['.##.', '#..#', '#.##', '####', '##.#', '#..#', '.##.'],
}
DIG = []
for ch in '0123456789':
    rows = glyph(ch)
    # compress the 6-wide bold glyph into a 4-wide one by dropping columns 1 and 4 (keeps 2px stems readable)
    out = []
    for r in rows:
        r = (r + '......')[:6]
        out.append(r[0] + r[2] + r[3] + r[5])
    bits = []
    for r in out:
        b = 0
        for i, c in enumerate(r):
            if c == '#':
                b |= 8 >> i
        bits.append(b)
    DIG += bits
# fix '1' (looks odd when compressed)
DIG[7:14] = [0b0110, 0b1110, 0b0110, 0b0110, 0b0110, 0b0110, 0b1111]
DIG[0:7] = [0b0110, 0b1001, 0b1011, 0b1111, 0b1101, 0b1001, 0b0110]
DIG[28:35] = [0b0011, 0b0111, 0b1011, 0b1011, 0b1111, 0b0011, 0b0011]  # 4

# ---------------------------------------------------------------- palettes
OBJ_PALS = [
    pal('NAVY', 'CREAM', 'GOLD', 'INK'),      # 0 player cursor / arrows
    pal('NAVY', 'WHITE', 'CORAL', 'INK'),     # 1 rival cursor
    pal('NAVY', 'WHITE', 'GOLD', 'ORANGE'),   # 2 sparkles
    pal('NAVY', 'CORAL', 'TEAL', 'GOLD'),     # 3 confetti A
    pal('NAVY', 'NAVY_D', 'GOLD', 'CREAM'),   # 4 text popups (font: 3 ink, 1 shadow)
    pal('NAVY', 'LINE', 'BLACK', 'WHITE'),    # 5 stones (same indices as the board)
    pal('NAVY', 'WHITE', 'CORAL', 'GOLD'),    # 6 last-move marker
    pal('NAVY', 'SKY', 'PINK', 'MINT'),       # 7 confetti B / misc
]

base_tiles = [b for t in base for b in planar(t)]
out = ['/* Generated by tools/make_art.py - UI kit, bold font and sprites. */']
out.append(carr('tt_base_tiles', base_tiles))
out.append(carr('tt_sprite_tiles', spr.data()))
out.append(carr('tt_digit_rows', DIG))
out.append(carr('tt_obj_palettes', [c for p in OBJ_PALS for c in p], 'uint16_t'))
(INC / 'tt_art_base.h').write_text('\n'.join(out))

# preview of base tiles
sheet = new(16 * 8, 10 * 8)
for i, t in enumerate(base):
    tim = Image.new('L', (8, 8))
    tim.putdata(t)
    sheet.paste(tim, ((i % 16) * 8, (i // 16) * 8))
preview(sheet, ['CREAM', 'SAND', 'CORAL', 'INK'], 4).save(PREV / 'base.png')
sheet = new(16 * 8, 4 * 8)
for i, t in enumerate(spr.tiles):
    tim = Image.new('L', (8, 8))
    tim.putdata(t)
    sheet.paste(tim, ((i % 16) * 8, (i // 16) * 8))
preview(sheet, ['NAVY_M', 'WHITE', 'GOLD', 'INK'], 4).save(PREV / 'sprites.png')

# =====================================================================
# MENU theme (VRAM bank 1): pattern 0..3, title logo / illustrations
# =====================================================================
menu = TileSet(0, 256, 'menu')
# pattern: 16x16 lattice of small diamonds, colour 1 on colour 0
pat = new(16, 16)
for (cx, cy) in ((3, 3), (11, 11)):
    for (dx, dy) in ((0, -1), (-1, 0), (0, 0), (1, 0), (0, 1)):
        pat.putpixel((cx + dx, cy + dy), 2)
pat.putpixel((11, 3), 1)
pat.putpixel((3, 11), 1)
pattern_frames = []
for f in range(16):
    shifted = new(16, 16)
    for y in range(16):
        for x in range(16):
            shifted.putpixel(((x + f) % 16, (y + f) % 16), pat.getpixel((x, y)))
    for i in range(4):
        pattern_frames += planar(tile_px(shifted, (i % 2) * 8, (i // 2) * 8))
for i in range(4):  # reserve tiles 0..3 for the animated pattern
    menu.add(tile_px(pat, (i % 2) * 8, (i // 2) * 8), dedupe=False)
define('TT_M_PATTERN', 0)

# ---- title logo -----------------------------------------------------
# "ARCADE" in 3x-wide/3x-tall bold letters, gold over orange, ink outline + extrude
LOGO_W, LOGO_H = 160, 48
logo = new(LOGO_W, LOGO_H)
word = 'ARCADE'
sx, sy = 3, 3
widths = [glyph_width(c) * sx for c in word]
gap = 2
total = sum(widths) + gap * (len(word) - 1)
x = (LOGO_W - total) // 2
top = 18
for ch, w in zip(word, widths):
    rows = scaled_glyph(ch, sx, sy)
    for yy, r in enumerate(rows):
        for xx, c in enumerate(r):
            if c == '#':
                logo.putpixel((x + xx, top + yy), 1 if yy < len(rows) * 0.55 else 2)
    x += w + gap
outline(logo, {1, 2}, 3)
# extrude down-right 2px in ink
px = logo.load()
for _ in range(2):
    marks = []
    for y in range(LOGO_H - 1, 0, -1):
        for x in range(LOGO_W - 1, 0, -1):
            if px[x, y] == 0 and px[x - 1, y - 1] in (1, 2, 3):
                marks.append((x, y))
    for x, y in marks:
        px[x, y] = 3
# small "ARCHER'S" on a ribbon above (cream text on coral ribbon -> uses palette B)
rib = new(LOGO_W, 16)
rw = text_width("ARCHER'S") + 14
rx = (LOGO_W - rw) // 2
d = ImageDraw.Draw(rib)
d.polygon([(rx - 6, 2), (rx + 2, 2), (rx + 2, 13), (rx - 6, 13), (rx - 2, 7)], fill=2)
d.polygon([(rx + rw + 6, 2), (rx + rw - 2, 2), (rx + rw - 2, 13), (rx + rw + 6, 13), (rx + rw + 2, 7)], fill=2)
d.rectangle((rx, 0, rx + rw, 11), fill=1)
d.line((rx, 12, rx + rw, 12), fill=2)
draw_text(rib, "ARCHER'S", rx + 7, 2, color=3, shadow=2)
logo.paste(rib, (0, 0))
# the logo occupies rows of 8px; ribbon rows (0..1) use palette B, letters use palette A
logo_map = menu.stamp(logo)
define('TT_M_LOGO_W', LOGO_W // 8)
define('TT_M_LOGO_H', LOGO_H // 8)
logo_pals = []
for ty in range(LOGO_H // 8):
    for tx in range(LOGO_W // 8):
        logo_pals.append(1 if ty < 2 else 0)
preview(logo, ['NAVY_D', 'GOLD', 'ORANGE', 'INK'], 3).save(PREV / 'logo.png')
title_count = len(menu)

# ---- illustrations: 6x6 tiles each, loaded into a slot at runtime ------
ILL = {}


def illus(name, im, palsel, pa, pb):
    """palsel: 36-char string of 'a'/'b' choosing palette per tile."""
    ts = TileSet(0, 64, name)
    m = ts.stamp(im)
    ILL[name] = dict(tiles=ts.data(), count=len(ts), map=m, pals=[0 if c == 'a' else 1 for c in palsel],
                     pa=pa, pb=pb, image=im)


def ill_gomoku():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    # board with drop shadow; palette A [cream, wood, line, black]
    d.rounded_rectangle((5, 7, 46, 46), radius=3, fill=2)
    d.rounded_rectangle((2, 3, 43, 43), radius=3, fill=1)
    for v in range(7, 42, 7):
        d.line((5, v, 40, v), fill=2)
        d.line((v, 5, v, 40), fill=2)
    stones = [(14, 14, 'b'), (21, 21, 'b'), (28, 28, 'b'), (35, 35, 'b'), (21, 14, 'w'), (28, 21, 'w'), (14, 28, 'w'), (35, 21, 'w')]
    for x, y, c in stones:
        if c == 'b':
            d.ellipse((x - 3, y - 3, x + 3, y + 3), fill=3)
            d.point((x - 1, y - 1), fill=0)
        else:
            d.ellipse((x - 3, y - 3, x + 3, y + 3), fill=0, outline=2)
    return im


illus('gomoku', ill_gomoku(), 'a' * 36, pal('CREAM', 'WOOD', 'LINE', 'BLACK'), pal('CREAM', 'WOOD', 'LINE', 'BLACK'))


def card2048(d, im, x, y, s, face, label, ink=3):
    d.rounded_rectangle((x, y, x + s, y + s), radius=3, fill=ink)
    d.rounded_rectangle((x + 1, y + 1, x + s - 1, y + s - 3), radius=2, fill=face)
    tw = text_width(label)
    draw_text(im, label, x + (s - tw) // 2 + 1, y + (s - 7) // 2 - 1, color=ink, shadow=None)


def ill_2048():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    card2048(d, im, 2, 1, 22, 2, '2')     # palette b: [cream, sky, coral, ink]
    card2048(d, im, 24, 10, 22, 1, '8')
    card2048(d, im, 6, 22, 38, 1, '2048')  # palette a: [cream, gold, orange, ink]
    return im


illus('2048', ill_2048(), 'bbbbbb' 'bbbbbb' 'bbbbbb' 'aaaaaa' 'aaaaaa' 'aaaaaa',
      pal('CREAM', 'GOLD', 'ORANGE', 'INK'), pal('CREAM', 'CORAL', 'SKY', 'INK'))


def ill_settings():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    cx, cy = 22, 24
    for i in range(8):
        a = i * math.pi / 4
        x, y = cx + math.cos(a) * 15, cy + math.sin(a) * 15
        d.rectangle((x - 4, y - 4, x + 4, y + 4), fill=3)
    d.ellipse((cx - 16, cy - 16, cx + 16, cy + 16), fill=3)
    for i in range(8):
        a = i * math.pi / 4
        x, y = cx + math.cos(a) * 15, cy + math.sin(a) * 15
        d.rectangle((x - 3, y - 3, x + 3, y + 3), fill=1)
    d.ellipse((cx - 14, cy - 14, cx + 14, cy + 14), fill=1)
    d.ellipse((cx - 6, cy - 6, cx + 6, cy + 6), fill=3)
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), fill=0)
    d.arc((cx - 12, cy - 12, cx + 12, cy + 12), 200, 290, fill=2)
    # music note badge
    d.ellipse((30, 30, 46, 46), fill=3)
    d.ellipse((31, 31, 45, 45), fill=2)
    put_rows(im, ICONS[147][1], 35, 35, color=0)
    return im


illus('settings', ill_settings(), 'a' * 36, pal('CREAM', 'TEAL', 'CORAL', 'INK'), pal('CREAM', 'TEAL', 'CORAL', 'INK'))


def ill_trophy():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((12, 4, 36, 26), radius=6, fill=3)
    d.ellipse((4, 7, 16, 21), outline=3, width=3)
    d.ellipse((32, 7, 44, 21), outline=3, width=3)
    d.rounded_rectangle((13, 5, 35, 25), radius=5, fill=1)
    d.rectangle((13, 5, 35, 10), fill=1)
    d.line((16, 7, 16, 20), fill=0)
    d.rectangle((21, 26, 27, 33), fill=3)
    d.rectangle((22, 26, 26, 32), fill=2)
    d.rounded_rectangle((12, 33, 36, 44), radius=2, fill=3)
    d.rectangle((14, 35, 34, 42), fill=2)
    put_rows(im, ICONS[129][1], 21, 11, color=2)
    return im


illus('ranked', ill_trophy(), 'a' * 36, pal('CREAM', 'GOLD', 'ORANGE', 'INK'), pal('CREAM', 'GOLD', 'ORANGE', 'INK'))


def ill_target():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    cx, cy = 22, 26
    for r, c in ((20, 3), (19, 1), (14, 0), (9, 1), (4, 0)):
        d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=c)
    d.ellipse((cx - 2, cy - 2, cx + 2, cy + 2), fill=3)
    # arrow stuck in the bullseye
    d.line((cx, cy, 44, 4), fill=3, width=3)
    d.line((cx + 1, cy - 1, 43, 5), fill=2)
    d.polygon([(40, 2), (46, 2), (46, 8), (43, 5)], fill=2)
    d.line((40, 2, 46, 8), fill=3)
    return im


illus('practice', ill_target(), 'a' * 36, pal('CREAM', 'CORAL', 'SKY', 'INK'), pal('CREAM', 'CORAL', 'SKY', 'INK'))


def ill_versus():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    d.ellipse((1, 14, 23, 36), fill=3)
    d.ellipse((2, 15, 22, 35), fill=2)
    d.ellipse((6, 19, 10, 23), fill=1)
    d.ellipse((25, 14, 47, 36), fill=3)
    d.ellipse((26, 15, 46, 35), fill=0)
    d.arc((26, 15, 46, 35), 0, 110, fill=1)
    # spark between them (palette b gold)
    put_rows(im, ['...#...', '..###..', '#######', '..###..', '...#...'], 21, 4, color=1)
    put_rows(im, ['...#...', '..###..', '#######', '..###..', '...#...'], 21, 40, color=1)
    return im


illus('versus', ill_versus(), 'a' * 36, pal('CREAM', 'NAVY_L', 'BLACK', 'INK'), pal('CREAM', 'GOLD', 'BLACK', 'INK'))
# tweak: sparks use palette b
ILL['versus']['pals'] = [1 if (i in (2, 3, 32, 33)) else 0 for i in range(36)]



def ill_tower():
    im = new(48, 48)
    d = ImageDraw.Draw(im)
    # tower body with battlements (palette a: cream, stone, green, ink)
    d.rectangle((8, 9, 33, 47), fill=1, outline=3)
    for x in (8, 15, 22, 29):
        d.rectangle((x, 4, x + 4, 9), fill=1, outline=3)
    for y in (20, 32, 44):
        d.line((9, y, 32, y), fill=3)
    for y, off in ((14, 0), (26, 4), (38, 0)):
        for x in range(10 + off, 32, 8):
            d.point((x, y), fill=3)
    # arched windows
    for (x, y) in ((13, 23), (24, 35)):
        d.rectangle((x, y + 1, x + 3, y + 6), fill=3)
        d.line((x + 1, y, x + 2, y), fill=3)
    # a wooden ledge sticking out with two arrows stuck in the wall
    d.rectangle((33, 30, 44, 32), fill=0, outline=3)
    d.line((34, 21, 41, 21), fill=3)
    d.line((36, 20, 36, 22), fill=3)
    # the hero on the ledge: green hood, cream face
    hero = ['..333...', '.32223..', '3222233.', '3223003.', '3223003.', '.322223.', '.32223..', '..3.3...']
    for yy, r in enumerate(hero):
        for xx, ch in enumerate(r):
            if ch in '023':
                im.putpixel((36 + xx, 22 + yy), int(ch))
    # pennant on top (palette b)
    d.line((19, 0, 19, 4), fill=3)
    d.polygon([(20, 0), (26, 1), (20, 3)], fill=2)
    # coins floating up the side (palette b)
    for (x, y) in ((40, 9), (42, 14)):
        d.ellipse((x - 2, y - 2, x + 2, y + 2), fill=1, outline=3)
    return im


illus('tower', ill_tower(), 'abbbab' 'aaaaab' 'aaaaaa' 'aaaaaa' 'aaaaaa' 'aaaaaa',
      pal('CREAM', '9aaec8', '3cae4c', 'INK'), pal('CREAM', 'GOLD', 'CORAL', 'INK'))

ILL_ORDER = ['gomoku', '2048', 'settings', 'ranked', 'practice', 'versus', 'tower']
for i, n in enumerate(ILL_ORDER):
    define(f'TT_ILL_{n.upper()}', i)
    preview(ILL[n]['image'], ['CREAM', 'GOLD', 'ORANGE', 'INK'], 3).save(PREV / f'ill_{n}.png')

out = ['/* Generated by tools/make_art.py - menu theme. */']
out.append(carr('tt_pattern_frames', pattern_frames))
out.append(carr('tt_title_tiles', menu.data()))
out.append(f'#define TT_TITLE_TILE_COUNT {title_count}')
out.append(carr('tt_logo_map', logo_map))
out.append(carr('tt_logo_pals', logo_pals))
for n in ILL_ORDER:
    v = ILL[n]
    out.append(carr(f'ill_{n}_tiles', v['tiles']))
    out.append(carr(f'ill_{n}_map', v['map']))
    out.append(carr(f'ill_{n}_attr', v['pals']))
    out.append(carr(f'ill_{n}_pal', v['pa'] + v['pb'], 'uint16_t'))
out.append('static const uint8_t * const ill_tiles[]={' + ','.join(f'ill_{n}_tiles' for n in ILL_ORDER) + '};')
out.append('static const uint8_t ill_counts[]={' + ','.join(str(ILL[n]['count']) for n in ILL_ORDER) + '};')
out.append('static const uint8_t * const ill_maps[]={' + ','.join(f'ill_{n}_map' for n in ILL_ORDER) + '};')
out.append('static const uint8_t * const ill_attrs[]={' + ','.join(f'ill_{n}_attr' for n in ILL_ORDER) + '};')
out.append('static const uint16_t * const ill_pals[]={' + ','.join(f'ill_{n}_pal' for n in ILL_ORDER) + '};')
(INC / 'tt_art_menu.h').write_text('\n'.join(out) + '\n')
print('menu: title tiles', title_count, 'illustrations', [ILL[n]['count'] for n in ILL_ORDER])

# =====================================================================
# PUZZLE theme (bank 1)
# =====================================================================
pz = TileSet(0, 256, 'puzzle')
pz.add([0] * 64)  # 0 = plain board background
# empty slot, 32x32: colour 2 rounded rect on colour 0
slot = new(32, 32)
ImageDraw.Draw(slot).rounded_rectangle((2, 2, 29, 29), radius=4, fill=2)
slot_map = pz.stamp(slot)


def numeral_image(value):
    """Return a 0/3 image of the number sized to fit a 26x16 area."""
    s = str(value)
    n = len(s)
    if n <= 2:
        sx, sy, glyphs = 2, 2, [scaled_glyph(c, 2, 2) for c in s]
        gap = 2
    elif n <= 4:
        sx, sy, glyphs = 1, 2, [scaled_glyph(c, 1, 2) for c in s]
        gap = 1
    else:
        glyphs = []
        for c in s:
            rows = glyph(c)
            narrow = []
            for r in rows:
                r = (r + '......')[:6]
                narrow.append(r[0] + r[2] + r[3] + r[5])
            glyphs.append([row for row in narrow for _ in range(2)])
        gap = 1
    w = sum(len(gl[0]) for gl in glyphs) + gap * (n - 1)
    h = len(glyphs[0])
    im = new(w, h)
    x = 0
    for gl in glyphs:
        for yy, r in enumerate(gl):
            for xx, c in enumerate(r):
                if c == '#':
                    im.putpixel((x + xx, yy), 3)
        x += len(gl[0]) + gap
    return im


def card_image(exp, small=False):
    face = 1 if exp % 2 == 1 else 2
    im = new(32, 32)
    d = ImageDraw.Draw(im)
    if small:
        d.rounded_rectangle((8, 8, 23, 23), radius=3, fill=3)
        d.rounded_rectangle((8, 8, 23, 21), radius=3, fill=face)
        return im
    d.rounded_rectangle((2, 2, 29, 29), radius=4, fill=3)
    d.rounded_rectangle((2, 2, 29, 26), radius=4, fill=face)
    num = numeral_image(2 ** exp)
    nx = 2 + (28 - num.width) // 2
    ny = 2 + (25 - num.height) // 2
    for y in range(num.height):
        for x in range(num.width):
            if num.getpixel((x, y)) == 3:
                im.putpixel((nx + x, ny + y), 3)
    return im


card_maps = []
for e in range(1, 15):
    card_maps.append(pz.stamp(card_image(e)))
small_maps = [pz.stamp(card_image(1, True)), pz.stamp(card_image(2, True))]
# sidebar labels in the narrow 4x7 font (colour 3 text, colour 1 shadow)
labels = {}
for word in ('SCORE', 'BEST', 'MOVES'):
    im = new(32, 8)
    draw_narrow(im, word, 0, 0, color=3, shadow=1)
    labels[word] = pz.stamp(im, dedupe=False)
# hint chips: badge in colour 2, label colour 3
hints = {}
for key, word in (('B', 'UNDO'), ('ST', 'MENU')):
    im = new(32, 8)
    d = ImageDraw.Draw(im)
    if key == 'B':
        d.rounded_rectangle((0, 0, 7, 7), radius=2, fill=3)
        put_rows(im, ['###.', '#..#', '###.', '#..#', '###.'], 2, 1, color=0)
        draw_narrow(im, word, 10, 0, color=3, shadow=1)
    else:
        d.rounded_rectangle((0, 0, 10, 7), radius=3, fill=3)
        draw_tiny(im, 'ST', 2, 1, color=0)
        draw_narrow(im, word, 12, 0, color=3, shadow=1)
    hints[key] = pz.stamp(im, dedupe=False)
# popup titles: bold font, colour 1 fill with colour 3 outline (2 tile rows, 12 tiles wide)
titles = {}
for key, word in (('win', 'YOU MADE 2048!'), ('over', 'GAME OVER'), ('pause', 'PAUSED'), ('best', 'NEW BEST!')):
    im = new(96, 16)
    tw = text_width(word)
    x = (96 - tw) // 2
    for ch in word:
        w = glyph_width(ch)
        put_rows(im, glyph(ch), x, 4, color=1)
        x += (w + 1) if ch != ' ' else 4
    outline(im, {1}, 3)
    titles[key] = [b for ty in range(2) for tx in range(12) for b in planar(tile_px(im, tx * 8, ty * 8))]
# mini "2048" logo for the sidebar: 32x16 two-tone
mini = new(32, 16)
d = ImageDraw.Draw(mini)
for i, ch in enumerate('2048'):
    d.rounded_rectangle((i * 8, 1, i * 8 + 7, 14), radius=2, fill=3)
    d.rounded_rectangle((i * 8, 1, i * 8 + 7, 12), radius=2, fill=1 if i % 2 == 0 else 2)
    put_rows(mini, glyph(ch), i * 8 + 1, 3, color=3)
mini_map = pz.stamp(mini)
define('TT_PUZZLE_TILE_COUNT', len(pz))
print('puzzle tiles', len(pz))

PZ_FACES = ['F4F1E8', 'FFE39B', 'FFB866', 'FF8C5A', 'FF7088', 'FF4F5E', 'C49BFF', '9D88FF',
            '6DB7FF', '4FE0CC', 'FFD23A', '9EEA6C', 'FF8AD8', 'FFFFFF']
PZ_PALS = [pal('NAVY', 'NAVY_D', 'NAVY_M', 'CREAM')]
for i in range(7):
    PZ_PALS.append(pal('NAVY', PZ_FACES[2 * i], PZ_FACES[2 * i + 1], 'INK'))

out = ['/* Generated by tools/make_art.py - 2048 theme. */']
out.append(carr('tt_puzzle_tiles', pz.data()))
out.append(carr('tt_slot_map', slot_map))
out.append(carr('tt_card_maps', [v for m in card_maps for v in m]))
out.append(carr('tt_small_maps', [v for m in small_maps for v in m]))
out.append(carr('tt_label_score', labels['SCORE']))
out.append(carr('tt_label_best', labels['BEST']))
out.append(carr('tt_label_moves', labels['MOVES']))
out.append(carr('tt_hint_undo', hints['B']))
out.append(carr('tt_hint_menu', hints['ST']))
out.append(carr('tt_mini_logo', mini_map))
for k in ('win', 'over', 'pause', 'best'):
    out.append(carr(f'tt_title_{k}', titles[k]))
define('TT_POPUP_TILES', 224)
out.append(carr('tt_puzzle_palettes', [c for p in PZ_PALS for c in p], 'uint16_t'))
(INC / 'tt_art_puzzle.h').write_text('\n'.join(out) + '\n')

sheet = Image.new('RGB', (14 * 34 * 3, 34 * 3), rgb('NAVY'))
for e in range(1, 15):
    p = PZ_PALS[1 + (e - 1) // 2]
    hexes = ['NAVY', PZ_FACES[(e - 1) // 2 * 2], PZ_FACES[(e - 1) // 2 * 2 + 1], 'INK']
    sheet.paste(preview(card_image(e), hexes, 3), ((e - 1) * 34 * 3, 3))
sheet.save(PREV / 'cards.png')

# =====================================================================
# BOARD theme (gomoku, bank 1): grid variants + stones, portraits go at 128+
# =====================================================================
bd = TileSet(0, 128, 'board')


def grid_cell(gx, gy, star=False):
    """gx/gy: 0 = first line, 1 = middle, 2 = last line."""
    im = new(8, 8, 0)
    d = ImageDraw.Draw(im)
    x0 = 3 if gx == 0 else 0
    x1 = 3 if gx == 2 else 7
    y0 = 3 if gy == 0 else 0
    y1 = 3 if gy == 2 else 7
    d.line((x0, 3, x1, 3), fill=1)
    d.line((3, y0, 3, y1), fill=1)
    if star:
        d.rectangle((2, 2, 4, 4), fill=1)
    return im


board_tiles = {}
for gy in range(3):
    for gx in range(3):
        board_tiles[('g', gx, gy)] = bd.add(tile_px(grid_cell(gx, gy)))
board_tiles[('star',)] = bd.add(tile_px(grid_cell(1, 1, True)))


def stone_on(gx, gy, white):
    im = grid_cell(gx, gy)
    st = board_stone(white)
    for y in range(8):
        for x in range(8):
            v = st.getpixel((x, y))
            if v:
                im.putpixel((x, y), v)
    return im


for white in (0, 1):
    for gy in range(3):
        for gx in range(3):
            board_tiles[('s', white, gx, gy)] = bd.add(tile_px(stone_on(gx, gy, white)))
# order table for C: [kind][gy*3+gx]
grid_list = [board_tiles[('g', i % 3, i // 3)] for i in range(9)]
black_list = [board_tiles[('s', 0, i % 3, i // 3)] for i in range(9)]
white_list = [board_tiles[('s', 1, i % 3, i // 3)] for i in range(9)]
# big turn stones (2x2) for the sidebar: palette [navy, rim, black, white]
big = {}
for white in (0, 1):
    im = new(16, 16)
    d = ImageDraw.Draw(im)
    if white:
        d.ellipse((1, 1, 14, 14), fill=3)
        d.arc((1, 1, 14, 14), 0, 120, fill=1)
        d.point([(5, 4), (4, 5)], fill=1)
    else:
        d.ellipse((1, 1, 14, 14), fill=1)
        d.ellipse((2, 2, 13, 13), fill=2)
        d.ellipse((4, 4, 7, 7), fill=1)
    big[white] = bd.stamp(im, dedupe=False)
define('TT_BOARD_TILE_COUNT', len(bd))
print('board tiles', len(bd))
out = ['/* Generated by tools/make_art.py - gomoku board. */']
out.append(carr('tt_board_tiles', bd.data()))
out.append(carr('tt_grid_tiles', grid_list))
out.append(carr('tt_black_tiles', black_list))
out.append(carr('tt_white_tiles', white_list))
out.append(f'#define TT_BOARD_STAR {board_tiles[("star",)]}')
out.append(carr('tt_big_stone', big[0] + big[1]))
gtitles = {}
for key, word in (('win', 'YOU WIN!'), ('lose', 'YOU LOSE'), ('draw', 'DRAW GAME'), ('black', 'BLACK WINS!'),
                  ('white', 'WHITE WINS!'), ('menu', 'GAME MENU')):
    im = new(96, 16)
    tw = text_width(word)
    x = (96 - tw) // 2
    for ch in word:
        w = glyph_width(ch)
        put_rows(im, glyph(ch), x, 4, color=2)
        x += (w + 1) if ch != ' ' else 4
    outline(im, {2}, 3)
    out.append(carr(f'tt_gtitle_{key}', [b for ty in range(2) for tx in range(12) for b in planar(tile_px(im, tx * 8, ty * 8))]))
(INC / 'tt_art_board.h').write_text('\n'.join(out) + '\n')

bprev = new(15 * 8, 15 * 8)
for p in range(225):
    x, y = p % 15, p // 15
    gx = 0 if x == 0 else 2 if x == 14 else 1
    gy = 0 if y == 0 else 2 if y == 14 else 1
    if (x, y) in ((6, 6), (7, 7), (8, 8)):
        t = stone_on(gx, gy, 0)
    elif (x, y) in ((7, 6), (8, 7)):
        t = stone_on(gx, gy, 1)
    elif x in (3, 7, 11) and y in (3, 7, 11):
        t = grid_cell(1, 1, True)
    else:
        t = grid_cell(gx, gy)
    bprev.paste(t, (x * 8, y * 8))
preview(bprev, ['WOOD', 'LINE', 'BLACK', 'WHITE'], 4).save(PREV / 'board.png')

# =====================================================================
hdr = ['/* Generated by tools/make_art.py - tile and sprite indices. */', '#ifndef TT_TILES_H', '#define TT_TILES_H']
hdr += defines
hdr += ['#endif', '']
(INC / 'tt_tiles.h').write_text('\n'.join(hdr))
print('ok')
