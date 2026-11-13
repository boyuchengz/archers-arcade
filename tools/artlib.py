"""Shared helpers for the Archer's Tabletop art generators.

Images are PIL 'L' mode with pixel values 0..3 (Game Boy colour indices).
"""
from PIL import Image, ImageDraw

# ---------------------------------------------------------------- colours
C = dict(
    NAVY_D='0d1024', NAVY='1b2046', NAVY_M='2a3270', NAVY_L='4552a8', DUSK='8e98e8',
    CREAM='fff6df', SAND='f2d8a2', TAN='c9945a', BROWN='7a4a22',
    GOLD='ffcf3a', ORANGE='ff8a2e', CORAL='ff5a5a', RED='d8283c', PINK='ff7ab8',
    TEAL='22c0b0', MINT='7cf0c2', SKY='5cc4ff', LILAC='b48cff', WHITE='ffffff',
    INK='12152e', WOOD='e9ac5c', WOOD_L='f6c77c', LINE='8a5426', BLACK='1c1c28',
)


def rgb15(hexstr):
    r, g, b = bytes.fromhex(C.get(hexstr, hexstr))
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def pal(*names):
    assert len(names) == 4
    return [rgb15(n) for n in names]


def rgb(hexstr):
    return tuple(bytes.fromhex(C.get(hexstr, hexstr)))


# ---------------------------------------------------------------- tiles
def planar(px):
    """64 pixel values -> 16 bytes (GB 2bpp)."""
    out = []
    for y in range(8):
        lo = hi = 0
        for x in range(8):
            v = px[y * 8 + x]
            lo |= (v & 1) << (7 - x)
            hi |= ((v >> 1) & 1) << (7 - x)
        out += [lo, hi]
    return out


def tile_px(im, x=0, y=0):
    return tuple(im.getpixel((x + i % 8, y + i // 8)) for i in range(64))


class TileSet:
    """Deduplicating tile allocator starting at `base`."""

    def __init__(self, base=0, limit=256, name='tiles'):
        self.base, self.limit, self.name = base, limit, name
        self.tiles, self.index = [], {}

    def add(self, px, dedupe=True):
        px = tuple(px)
        if dedupe and px in self.index:
            return self.index[px]
        n = self.base + len(self.tiles)
        assert n < self.limit, f'{self.name} overflow ({n})'
        self.tiles.append(px)
        if dedupe:
            self.index[px] = n
        return n

    def stamp(self, im, dedupe=True):
        """Cut image into 8x8 tiles (row-major); return list of tile indices."""
        assert im.width % 8 == 0 and im.height % 8 == 0, im.size
        return [self.add(tile_px(im, x, y), dedupe)
                for y in range(0, im.height, 8) for x in range(0, im.width, 8)]

    def data(self):
        return [b for t in self.tiles for b in planar(t)]

    def __len__(self):
        return len(self.tiles)


def carr(name, data, typ='uint8_t', per=24):
    body = ',\n'.join(','.join(str(v) for v in data[i:i + per]) for i in range(0, len(data), per))
    return f'static const {typ} {name}[]={{\n{body}\n}};\n'


def new(w, h, fill=0):
    return Image.new('L', (w, h), fill)


def put_rows(im, rows, x0, y0, color=3, shadow=None, mapping=None):
    """Draw a '#'-art glyph. mapping lets other chars pick colours."""
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch == '#' or (mapping and ch in mapping):
                c = color if ch == '#' else mapping[ch]
                if shadow is not None:
                    sx, sy = x0 + x + 1, y0 + y + 1
                    if 0 <= sx < im.width and 0 <= sy < im.height and im.getpixel((sx, sy)) == 0:
                        pass
                if 0 <= x0 + x < im.width and 0 <= y0 + y < im.height:
                    im.putpixel((x0 + x, y0 + y), c)
    if shadow is not None:
        # shadow pass: one pixel down-right where empty
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch == '#':
                    sx, sy = x0 + x + 1, y0 + y + 1
                    if 0 <= sx < im.width and 0 <= sy < im.height and im.getpixel((sx, sy)) == 0:
                        im.putpixel((sx, sy), shadow)


def preview(im, palette_hex, scale=4):
    """Render a 0..3 image with a 4-colour palette to RGB for inspection."""
    out = Image.new('RGB', im.size)
    cols = [rgb(c) for c in palette_hex]
    out.putdata([cols[v] for v in im.getdata()])
    return out.resize((im.width * scale, im.height * scale), Image.NEAREST)
