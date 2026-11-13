#pragma bank 255
/* Rival portraits: 5x5 tiles, four expressions, two palettes (face rows / shirt rows). */
#include <gb/gb.h>
#include <gb/cgb.h>
#include "states/tt_gfx.h"
#include "states/tt_art_portraits.h"

/* Load all unique tiles of one rival into VRAM bank 1 at tile_base. */
void portrait_load(uint8_t id, uint8_t tile_base) BANKED {
    id %= 10;
    VBK_REG = 1;
    set_bkg_data(tile_base, portrait_counts[id], portrait_tiles[id]);
    VBK_REG = 0;
}

/* Palettes: pal (face) and pal+1 (shirt). */
void portrait_palettes(uint8_t id, uint8_t pal) BANKED {
    uint16_t c[8];
    uint8_t i;
    id %= 10;
    for (i = 0; i < 8; i++) c[i] = portrait_pals[id][i];
    tt_pal_set(pal * 4, 8, c);
}

void portrait_draw(uint8_t id, uint8_t expr, uint8_t x, uint8_t y, uint8_t tile_base, uint8_t pal) BANKED {
    uint8_t t[25], a[25], i;
    const uint8_t *m;
    id %= 10;
    m = portrait_maps[id] + (expr & 3) * 25;
    for (i = 0; i < 25; i++) {
        t[i] = m[i] + tile_base;
        a[i] = TT_BK1 | (i < 15 ? pal : pal + 1);
    }
    tt_rows(x, y, 5, 5, t, a);
}
