#pragma bank 255
/* ARROW TOWER presentation: tile and palette loading, zone colour blending,
   animated tiles, the HUD bar and the panels that slide up from it. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_tower.h"
#include "states/tt_gfx.h"
#include "states/tt_art_tower.h"

#define P_HUD 0
#define HB (P_HUD | TT_BK1)

uint8_t tw_win_y;
static uint8_t anim_coin, anim_flame, anim_camp;

void tw_view_load(uint8_t zone) BANKED {
    uint16_t c[32];
    uint8_t i;
    VBK_REG = 1;
    set_bkg_data(0, TW_BG_COUNT, tw_bg_tiles);
    set_sprite_data(128, TW_SPR_COUNT, tw_spr_tiles);
    VBK_REG = 0;
    for (i = 0; i < 32; i++) c[i] = tw_bg_pals[zone * 32 + i];
    tt_pal_set(0, 32, c);
    for (i = 0; i < 32; i++) c[i] = tw_obj_pals[i];
    tt_pal_set(32, 32, c);
    anim_coin = anim_flame = anim_camp = 255;
}

/* Blend background palettes 1..7 from zone z0 to z1, t = 0..16. */
void tw_view_blend(uint8_t z0, uint8_t z1, uint8_t t) BANKED {
    uint16_t c[28], a, b;
    uint8_t i, ra, ga, ba, rb, gb, bb;
    for (i = 0; i < 28; i++) {
        a = tw_bg_pals[z0 * 32 + 4 + i];
        b = tw_bg_pals[z1 * 32 + 4 + i];
        ra = a & 31; ga = (a >> 5) & 31; ba = (a >> 10) & 31;
        rb = b & 31; gb = (b >> 5) & 31; bb = (b >> 10) & 31;
        ra = (uint8_t)((ra * (16 - t) + rb * t) >> 4);
        ga = (uint8_t)((ga * (16 - t) + gb * t) >> 4);
        ba = (uint8_t)((ba * (16 - t) + bb * t) >> 4);
        c[i] = ra | ((uint16_t)ga << 5) | ((uint16_t)ba << 10);
    }
    tt_pal_set(4, 28, c);
}

/* The same blend spread over three frames (a third of the colours each), so
   the camera pan into a new zone never drops a frame. Commits on part 2. */
#define blend_buf (TW->blend)

void tw_view_blend_part(uint8_t z0, uint8_t z1, uint8_t t, uint8_t part) BANKED {
    uint8_t i = part * 10, end = part == 2 ? 28 : i + 10, ra, ga, ba, rb, gb, bb;
    uint16_t a, b;
    for (; i < end; i++) {
        a = tw_bg_pals[z0 * 32 + 4 + i];
        b = tw_bg_pals[z1 * 32 + 4 + i];
        ra = a & 31; ga = (a >> 5) & 31; ba = (a >> 10) & 31;
        rb = b & 31; gb = (b >> 5) & 31; bb = (b >> 10) & 31;
        ra = (uint8_t)((ra * (16 - t) + rb * t) >> 4);
        ga = (uint8_t)((ga * (16 - t) + gb * t) >> 4);
        ba = (uint8_t)((ba * (16 - t) + bb * t) >> 4);
        blend_buf[i] = ra | ((uint16_t)ga << 5) | ((uint16_t)ba << 10);
    }
    if (part == 2) tt_pal_set(4, 28, blend_buf);
}

/* Spinning coins, flickering torches and the campfire. */
void tw_view_anim(void) BANKED {
    uint8_t f;
    f = (uint8_t)((tt_clock >> 3) & 3);
    if (f != anim_coin) {
        anim_coin = f;
        VBK_REG = 1;
        set_bkg_data(TW_T_COIN, 1, tw_anim_tiles + (uint16_t)f * 16);
        VBK_REG = 0;
    }
    f = (uint8_t)((tt_clock / 6) % 3);
    if (f != anim_flame) {
        anim_flame = f;
        VBK_REG = 1;
        set_bkg_data(TW_T_FLAME, 1, tw_anim_tiles + 64 + (uint16_t)f * 16);
        VBK_REG = 0;
    }
    f = (uint8_t)((tt_clock >> 3) & 1);
    if (f != anim_camp) {
        anim_camp = f;
        VBK_REG = 1;
        set_bkg_data(TW_T_CAMP, 4, tw_camp_tiles + (uint16_t)f * 64);
        VBK_REG = 0;
    }
}

void tw_spring_tile(uint8_t x, uint8_t ring, uint8_t up) BANKED {
    VBK_REG = 0;
    set_bkg_tile_xy(x, ring & 31, up ? TW_T_SPRING_UP : TW_T_SPRING);
}

void tw_crumble_tile(uint8_t x, uint8_t ring, uint8_t stage) BANKED {
    VBK_REG = 0;
    set_bkg_tile_xy(x, ring & 31, TW_T_CRUMBLE0 + stage);
}

/* ------------------------------------------------------------------ HUD (window rows 0-1) */
static void zone_line(void) {
    char name[14];
    uint8_t z = tw_zone_of(tw_floor), cyc = (uint8_t)((tw_floor - 1) / 60), n;
    memcpy(name, tw_zone_names[z], 12);
    name[12] = 0;
    n = (uint8_t)strlen(name);
    while (cyc-- && n < 12) name[n++] = '+';
    name[n] = 0;
    tt_win = 1;
    tt_fill(0, 1, 13, 1, 0, P_HUD);
    tt_text_far(1, 1, name, P_HUD, CURRENT_BANK);
    tt_win = 0;
}

/* the zone's name drifts up over the room as you arrive */
void tw_zone_banner(uint8_t zone) BANKED {
    char name[13];
    uint8_t n;
    memcpy(name, tw_zone_names[zone], 12);
    name[12] = 0;
    n = (uint8_t)strlen(name);
    fx_text(80 - n * 7 / 2, 30, name);
}

void tw_hud_stats(void) BANKED {
    tt_win = 1;
    tt_fill(6, 0, 5, 1, 0, P_HUD);
    tt_number(6, 0, tw_floor, 0, P_HUD);
    tt_number(15, 0, tw_coins, 4, P_HUD);
    tt_number(15, 1, tt_save.tw_best, 4, P_HUD);
    tt_win = 0;
}

void tw_hud_draw(void) BANKED {
    tt_win = 1;
    tt_fill(0, 0, 20, 2, 0, P_HUD);
    tt_put(5, 0, TW_T_HUD_FLOOR, HB);
    tt_put(14, 0, TW_T_HUD_COIN, HB);
    tt_put(14, 1, TT_T_CROWN, P_HUD);
    tt_win = 0;
    zone_line();
    tw_hud_stats();
}

void tw_hud_hint(uint8_t which) BANKED {
    tt_win = 1;
    tt_fill(0, 1, 14, 1, 0, P_HUD);
    if (which == HINT_ZONE) {
        tt_win = 0;
        zone_line();
        return;
    }
    if (which == HINT_CONTROLS) {
        tt_put(0, 1, TT_T_BTN_A, P_HUD);
        tt_text(1, 1, "Jump", P_HUD);
        tt_put(6, 1, TT_T_BTN_B, P_HUD);
        tt_text(7, 1, "Shoot", P_HUD);
    } else if (which == HINT_WOOD) {
        tt_text(0, 1, "Arrows stick", P_HUD);
    } else {
        tt_text(0, 1, "Rest by fire", P_HUD);
    }
    tt_win = 0;
}

/* ------------------------------------------------------------------ panels (window rows 2..11) */
static void title(const uint8_t *tiles) {
    uint8_t t[12], a[12], i;
    set_bkg_data(TT_POPUP_TILES, 24, tiles);
    for (i = 0; i < 12; i++) a[i] = P_HUD;
    for (i = 0; i < 12; i++) t[i] = TT_POPUP_TILES + i;
    tt_rows(4, 3, 12, 1, t, a);
    for (i = 0; i < 12; i++) t[i] = TT_POPUP_TILES + 12 + i;
    tt_rows(4, 4, 12, 1, t, a);
}


void tw_panel(uint8_t kind) BANKED {
    tt_win = 1;
    tt_fill(0, 2, 20, 11, 0, P_HUD);
    tt_fill(0, 2, 20, 1, TT_T_RULE_DOTS, P_HUD);
    if (kind == PANEL_START) {
        title(tw_title_tower);
        tt_text(2, 7, "Best floor", P_HUD);
        tt_number(13, 7, tt_save.tw_best, 0, P_HUD);
        tt_win = 0;
        tw_panel_skin(tt_save.tw_skin);
        tt_win = 1;
        tt_put(2, 9, TT_T_BTN_A, P_HUD);
        tt_text(3, 9, "Climb!", P_HUD);
        tt_put(12, 9, TT_T_BTN_B, P_HUD);
        tt_text(13, 9, "Back", P_HUD);
    } else if (kind == PANEL_PAUSE) {
        title(tw_title_pause);
        tt_text(4, 6, "Resume", P_HUD);
        tt_text(4, 7, "Restart run", P_HUD);
        tt_text(4, 8, "Main menu", P_HUD);
    } else if (kind == PANEL_OVER || kind == PANEL_RECORD) {
        uint16_t from = 1 + (uint16_t)tt_save.tw_check * 10;
        title(kind == PANEL_RECORD ? tw_title_record : tw_title_over);
        tt_text(2, 6, "Reached floor", P_HUD);
        tt_number(16, 6, tw_floor, 0, P_HUD);
        tt_text(2, 7, "Coins", P_HUD);
        tt_number(16, 7, tw_coins, 0, P_HUD);
        tt_text(2, 8, "Best floor", P_HUD);
        tt_number(16, 8, tt_save.tw_best, 0, P_HUD);
        tt_win = 0;
        tw_panel_bank();
        tt_win = 1;
        tt_put(2, 10, TT_T_BTN_A, P_HUD);
        tt_text(3, 10, "Retry from", P_HUD);
        tt_number(14, 10, from, 0, P_HUD);
        tt_put(2, 11, TT_T_BTN_B, P_HUD);
        tt_text(3, 11, "Main menu", P_HUD);
    }
    tt_win = 0;
}
