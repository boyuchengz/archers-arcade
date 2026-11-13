#pragma bank 255
/* Graphics core: palette mirror + fades, tile/text drawing, software digits, sprites. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "bankdata.h"
#include "states/tt_gfx.h"
#include "states/tt_art_base.h"

uint16_t tt_pal[64];
uint8_t tt_bright, tt_win, tt_shake, tt_scx, tt_split_scx, tt_split_on, tt_pattern_on;
static uint8_t pal_dirty;

/* Off-screen page buffer for the carousel: 20x13 tiles (+attrs). Lives in the
   gomoku line-code area, which is unused whenever a menu is on screen. */
extern uint16_t go_line_codes[225][4];
#define BUF_TILES ((uint8_t *)go_line_codes)
#define BUF_ATTRS (((uint8_t *)go_line_codes) + 260)
uint8_t tt_to_buffer;

static uint8_t row_t[20], row_a[20];

/* ------------------------------------------------------------------ palettes */
static const uint8_t level_lut[9 * 32] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,
0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,
0,0,0,1,1,1,2,2,3,3,3,4,4,4,5,5,6,6,6,7,7,7,8,8,9,9,9,10,10,10,11,11,
0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,
0,0,1,1,2,3,3,4,5,5,6,6,7,8,8,9,10,10,11,11,12,13,13,14,15,15,16,16,17,18,18,19,
0,0,1,2,3,3,4,5,6,6,7,8,9,9,10,11,12,12,13,14,15,15,16,17,18,18,19,20,21,21,22,23,
0,0,1,2,3,4,5,6,7,7,8,9,10,11,12,13,14,14,15,16,17,18,19,20,21,21,22,23,24,25,26,27,
0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
};
static uint8_t pal_out[128], pal_ready;

/* Scale the palette mirror by tt_bright into pal_out (slow part, done outside vblank). */
static void pal_prepare(void) {
    uint8_t i, r, g, b;
    uint16_t c;
    const uint8_t *lut = level_lut + (uint16_t)tt_bright * 32;
    uint8_t *o = pal_out;
    for (i = 0; i < 64; i++) {
        c = tt_pal[i];
        if (tt_bright >= 8) {
            *o++ = (uint8_t)c;
            *o++ = (uint8_t)(c >> 8);
            continue;
        }
        r = lut[c & 31];
        g = lut[(c >> 5) & 31];
        b = lut[(c >> 10) & 31];
        c = r | ((uint16_t)g << 5) | ((uint16_t)b << 10);
        *o++ = (uint8_t)c;
        *o++ = (uint8_t)(c >> 8);
    }
    pal_ready = 1;
}

/* Fast copy into palette RAM (call during vblank). */
static void pal_write(void) {
    uint8_t i;
    const uint8_t *o = pal_out;
    BCPS_REG = 0x80;
    for (i = 0; i < 64; i++) {
        while (STAT_REG & STATF_BUSY);
        BCPD_REG = *o++;
    }
    OCPS_REG = 0x80;
    for (i = 0; i < 64; i++) {
        while (STAT_REG & STATF_BUSY);
        OCPD_REG = *o++;
    }
    pal_ready = 0;
    pal_dirty = 0;
}

void tt_pal_commit(void) BANKED {
    pal_prepare();
    pal_write();
}

void tt_pal_set(uint8_t first, uint8_t count, const uint16_t *colors) BANKED {
    memcpy(tt_pal + first, colors, (uint16_t)count << 1);
    pal_dirty = 1;
}

void tt_pal_obj_reset(void) BANKED {
    memcpy(tt_pal + 32, tt_obj_palettes, 64);
    pal_dirty = 1;
}

/* ------------------------------------------------------------------ frame loop */
void tt_frame(void) BANKED {
    if (pal_dirty) pal_prepare();
    wait_vbl_done();
    if (pal_ready) pal_write();
    if (tt_pattern_on) tt_pattern_tick();
    tt_clock = sys_time;
    if (tt_shake) tt_shake--;
    fx_tick();
    tt_music_tick();
    tt_save_tick();
}

void tt_wait(uint8_t frames) BANKED {
    while (frames--) tt_frame();
}

void tt_fade_out(void) BANKED {
    while (tt_bright) {
        tt_bright--;
        pal_dirty = 1;
        tt_frame();
    }
}

void tt_fade_in(void) BANKED {
    while (tt_bright < 8) {
        tt_bright++;
        pal_dirty = 1;
        tt_frame();
    }
}

/* ------------------------------------------------------------------ tiles */
static void write_row(uint8_t x, uint8_t y, uint8_t w) {
    if (tt_to_buffer) {
        uint16_t o;
        if (y < 3 || y > 15) return;
        o = (uint16_t)(y - 3) * 20 + x;
        memcpy(BUF_TILES + o, row_t, w);
        memcpy(BUF_ATTRS + o, row_a, w);
        return;
    }
    VBK_REG = 1;
    if (tt_win) set_win_tiles(x, y, w, 1, row_a); else set_bkg_tiles(x, y, w, 1, row_a);
    VBK_REG = 0;
    if (tt_win) set_win_tiles(x, y, w, 1, row_t); else set_bkg_tiles(x, y, w, 1, row_t);
}

void tt_put(uint8_t x, uint8_t y, uint8_t tile, uint8_t attr) BANKED {
    row_t[0] = tile;
    row_a[0] = attr;
    write_row(x, y, 1);
}

void tt_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t tile, uint8_t attr) BANKED {
    if (!w) return;
    memset(row_t, tile, w);
    memset(row_a, attr, w);
    while (h--) write_row(x, y++, w);
}

void tt_attr(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t attr) BANKED {
    memset(row_a, attr, w);
    while (h--) {
        if (tt_to_buffer) {
            if (y >= 3 && y <= 15) memcpy(BUF_ATTRS + (uint16_t)(y - 3) * 20 + x, row_a, w);
        } else {
            VBK_REG = 1;
            if (tt_win) set_win_tiles(x, y, w, 1, row_a); else set_bkg_tiles(x, y, w, 1, row_a);
            VBK_REG = 0;
        }
        y++;
    }
}

void tt_rows(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *tiles, const uint8_t *attrs) BANKED {
    while (h--) {
        memcpy(row_t, tiles, w);
        memcpy(row_a, attrs, w);
        write_row(x, y++, w);
        tiles += w;
        attrs += w;
    }
}

void tt_box(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t style, uint8_t attr) BANKED {
    uint8_t i;
    memset(row_a, attr, w);
    row_t[0] = style;
    memset(row_t + 1, style + 1, w - 2);
    row_t[w - 1] = style + 2;
    write_row(x, y, w);
    row_t[0] = style + 3;
    memset(row_t + 1, 0, w - 2);
    row_t[w - 1] = style + 5;
    for (i = 1; i < h - 1; i++) write_row(x, y + i, w);
    row_t[0] = style + 6;
    memset(row_t + 1, style + 7, w - 2);
    row_t[w - 1] = style + 8;
    write_row(x, y + h - 1, w);
}

void tt_clear(uint8_t attr) BANKED {
    uint8_t y;
    memset(row_t, 0, 20);
    memset(row_a, attr, 20);
    for (y = 0; y < 18; y++) write_row(0, y, 20);
}

/* ------------------------------------------------------------------ text */
uint8_t tt_strlen_far(const char *s, uint8_t bank) BANKED {
    uint8_t n = 0;
    while (n < 20 && ReadBankedUBYTE(s + n, bank)) n++;
    return n;
}

void tt_text_far(uint8_t x, uint8_t y, const char *s, uint8_t attr, uint8_t bank) BANKED {
    uint8_t n = 0, c;
    while (x + n < 20) {
        c = ReadBankedUBYTE(s++, bank);
        if (!c) break;
        row_t[n] = (c < 32 || c > 126) ? '?' : c;
        row_a[n] = attr;
        n++;
    }
    if (n) write_row(x, y, n);
}

void tt_number(uint8_t x, uint8_t y, uint32_t n, uint8_t digits, uint8_t attr) BANKED {
    char s[11];
    uint8_t i = 10, len;
    s[10] = 0;
    do {
        s[--i] = '0' + (uint8_t)(n % 10);
        n /= 10;
    } while (n && i);
    len = 10 - i;
    if (digits > len) {
        memset(row_t, ' ', digits - len);
        memcpy(row_t + digits - len, s + i, len);
        len = digits;
    } else {
        memcpy(row_t, s + i, len);
    }
    if (x + len > 20) len = 20 - x;
    memset(row_a, attr, len);
    write_row(x, y, len);
}

/* ------------------------------------------------------------------ software digits */
static void px(uint8_t *buf, uint8_t x, uint8_t y, uint8_t c) {
    uint8_t *p = buf + ((x >> 3) << 4) + (y << 1);
    uint8_t m = 0x80 >> (x & 7);
    if (c & 1) p[0] |= m; else p[0] &= ~m;
    if (c & 2) p[1] |= m; else p[1] &= ~m;
}

/* Right-aligned 4x7 digits with a drop shadow, 5px advance, up to 6 digits in 4 tiles. */
void tt_digits(uint8_t tile, uint8_t ntiles, uint32_t n, uint8_t ink, uint8_t shadow) BANKED {
    uint8_t buf[64], dig[10], count = 0, i, r, b, x0, width, pass;
    memset(buf, 0, (uint16_t)ntiles << 4);
    do {
        dig[count++] = (uint8_t)(n % 10);
        n /= 10;
    } while (n && count < 10);
    while ((uint8_t)(count * 5) > (uint8_t)(ntiles * 8)) count--;
    width = count * 5 - 1;
    x0 = ntiles * 8 - 1 - width;
    for (pass = 0; pass < 2; pass++) {
        for (i = 0; i < count; i++) {
            const uint8_t *g = tt_digit_rows + dig[count - 1 - i] * 7;
            uint8_t gx = x0 + i * 5;
            for (r = 0; r < 7; r++) {
                for (b = 0; b < 4; b++) {
                    if (g[r] & (8 >> b)) {
                        if (pass == 0) px(buf, gx + b + 1, r + 1, shadow);
                        else px(buf, gx + b, r, ink);
                    }
                }
            }
        }
    }
    set_bkg_data(tile, ntiles, buf);
}

/* ------------------------------------------------------------------ sprites */
void tt_spr(uint8_t id, uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) BANKED {
    set_sprite_tile(id, tile);
    set_sprite_prop(id, prop);
    move_sprite(id, x + 8, y + 16);
}

void tt_spr_hide(uint8_t first, uint8_t count) BANKED {
    while (count--) hide_sprite(first++);
}

/* ------------------------------------------------------------------ setup */
void tt_gfx_init(void) BANKED {
    VBK_REG = 0;
    set_bkg_data(0, 160, tt_base_tiles);
    set_sprite_data(192, TT_SPRITE_TILE_COUNT, tt_sprite_tiles);
    memset(tt_pal, 0, sizeof(tt_pal));
    memcpy(tt_pal + 32, tt_obj_palettes, 64);
    tt_bright = 0;
    tt_pal_commit();
}

void tt_screen_reset(void) BANKED {
    tt_spr_hide(0, 40);
    fx_clear();
    tt_split_on = 0;
    tt_split_scx = 0;
    tt_scx = 0;
    tt_shake = 0;
    tt_pattern_on = 0;
    tt_to_buffer = 0;
    tt_win = 0;
    HIDE_WIN;
    tt_clear(0);
    tt_pal_obj_reset();
}
