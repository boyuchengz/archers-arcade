#pragma bank 255
/* Title screen, card carousels (main menu, gomoku modes, rival select),
   settings and the ranked-match roulette. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_rivals.h"
#include "states/tt_gfx.h"
#include "states/tt_art_menu.h"

extern uint16_t go_line_codes[225][4];
extern uint8_t tt_to_buffer;
#define BUF_TILES ((uint8_t *)go_line_codes)
#define BUF_ATTRS (((uint8_t *)go_line_codes) + 260)

#define P_BG 0
#define P_CARD 1
#define P_BAR 2
#define P_SUB 3
#define SLOT_TILES(s) ((s) ? 80 : 16)
#define SLOT_PAL(s) ((s) ? 6 : 4)

#define SCR_MAIN 0
#define SCR_MODES 1
#define SCR_RIVALS 2

static const uint16_t menu_pals[] = {
    /* 0 bg   */ RGB8(27, 32, 70), RGB8(13, 16, 36), RGB8(42, 50, 112), RGB8(255, 246, 223),
    /* 1 card */ RGB8(255, 246, 223), RGB8(242, 216, 162), RGB8(27, 32, 70), RGB8(18, 21, 46),
    /* 2 bar  */ RGB8(13, 16, 36), RGB8(5, 6, 15), RGB8(255, 207, 58), RGB8(255, 246, 223),
    /* 3 sub  */ RGB8(255, 246, 223), RGB8(242, 216, 162), RGB8(27, 32, 70), RGB8(84, 98, 196),
};
static const uint16_t title_pals[] = {
    /* 0 bg     */ RGB8(27, 32, 70), RGB8(13, 16, 36), RGB8(42, 50, 112), RGB8(255, 246, 223),
    /* 1 logo   */ RGB8(13, 16, 36), RGB8(255, 207, 58), RGB8(255, 138, 46), RGB8(18, 21, 46),
    /* 2 ribbon */ RGB8(13, 16, 36), RGB8(255, 90, 90), RGB8(196, 40, 60), RGB8(255, 246, 223),
    /* 3 band   */ RGB8(13, 16, 36), RGB8(5, 6, 15), RGB8(255, 207, 58), RGB8(255, 246, 223),
    /* 4 gold   */ RGB8(27, 32, 70), RGB8(13, 16, 36), RGB8(42, 50, 112), RGB8(255, 207, 58),
};
static const uint16_t settings_pals[] = {
    /* 4 card off  */ RGB8(42, 50, 112), RGB8(13, 16, 36), RGB8(27, 32, 70), RGB8(255, 246, 223),
    /* 5 (unused)  */ RGB8(42, 50, 112), RGB8(13, 16, 36), RGB8(27, 32, 70), RGB8(255, 246, 223),
    /* 6 toggles on cream */ RGB8(255, 246, 223), RGB8(190, 182, 166), RGB8(34, 192, 176), RGB8(255, 255, 255),
    /* 7 toggles on dark  */ RGB8(42, 50, 112), RGB8(69, 82, 168), RGB8(34, 192, 176), RGB8(255, 255, 255),
};

static uint8_t screen, page, count, slot, scroll, anim, from_scroll, wrote_rest, last_main, last_mode;
static int8_t dir;
static uint8_t settings_row, confirm, match_rival, match_step, match_wait, match_shown;
static uint16_t title_t;

/* ------------------------------------------------------------------ pattern */
static uint8_t pattern_frame = 255;

void tt_pattern_tick(void) BANKED {
    uint8_t f = (uint8_t)((tt_clock >> 2) & 15);
    if (f == pattern_frame) return;
    pattern_frame = f;
    VBK_REG = 1;
    set_bkg_data(0, 4, tt_pattern_frames + (uint16_t)f * 64);
    VBK_REG = 0;
}

void tt_pattern_load(void) BANKED {
    pattern_frame = 255;
    tt_pattern_tick();
    tt_pattern_on = 1;
}

static void pattern_rows(uint8_t y0, uint8_t y1) {
    uint8_t t[20], a[20], x, y;
    memset(a, P_BG | TT_BK1, 20);
    for (y = y0; y <= y1; y++) {
        for (x = 0; x < 20; x++) t[x] = TT_M_PATTERN + (x & 1) + ((y & 1) << 1);
        tt_rows(0, y, 20, 1, t, a);
    }
}

void tt_menu_bg(void) BANKED {
    pattern_rows(0, 17);
}

/* ------------------------------------------------------------------ helpers */
static const char *rank_name(uint16_t r) {
    if (r < 700) return "Rookie";
    if (r < 900) return "Bronze";
    if (r < 1100) return "Silver";
    if (r < 1300) return "Gold";
    if (r < 1500) return "Platinum";
    if (r < 1700) return "Diamond";
    return "Master";
}

static void bars(const char *title) {
    tt_fill(0, 0, 20, 2, 0, P_BAR);
    tt_fill(0, 2, 20, 1, TT_T_RULE_BOTTOM, P_BAR);
    tt_ctext(1, title, P_BAR);
    tt_win = 1;
    tt_fill(0, 0, 20, 2, 0, P_BAR);
    tt_win = 0;
    move_win(7, 128);
    SHOW_WIN;
}

static void footer_hint(const char *a_label, uint8_t back) {
    tt_win = 1;
    tt_fill(0, 1, 20, 1, 0, P_BAR);
    tt_put(1, 1, TT_T_LEFT, P_BAR);
    tt_put(2, 1, TT_T_RIGHT, P_BAR);
    tt_text(3, 1, "Pick", P_BAR);
    tt_put(8, 1, TT_T_BTN_A, P_BAR);
    tt_text(9, 1, a_label, P_BAR);
    if (back) {
        tt_put(14, 1, TT_T_BTN_B, P_BAR);
        tt_text(15, 1, "Back", P_BAR);
    }
    tt_win = 0;
}

static void footer_stat(void) {
    tt_win = 1;
    tt_fill(0, 0, 20, 1, 0, P_BAR);
    if ((screen == SCR_MAIN && page == 0) || (screen == SCR_MODES && page == 0)) {
        tt_put(1, 0, TT_T_STAR, P_BAR);
        tt_text(3, 0, "Elo", P_BAR);
        tt_number(7, 0, tt_save.rating, 0, P_BAR);
        tt_text(12, 0, rank_name(tt_save.rating), P_BAR);
    } else if (screen == SCR_MAIN && page == 1) {
        tt_put(1, 0, TT_T_CROWN, P_BAR);
        tt_text(3, 0, "Best", P_BAR);
        tt_number(8, 0, tt_save.puzzle.best, 0, P_BAR);
    } else if (screen == SCR_MAIN && page == 2) {
        tt_put(1, 0, TT_T_SOUND, P_BAR);
        tt_text(3, 0, tt_save_available ? "Autosave is on" : "No save battery", P_BAR);
    } else if (screen == SCR_MODES && page == 1) {
        tt_put(1, 0, TT_T_HEART, P_BAR);
        tt_text(3, 0, "You play black", P_BAR);
    } else if (screen == SCR_MODES && page == 2) {
        tt_put(1, 0, TT_T_STONE_B, P_BAR);
        tt_text(3, 0, "Black moves first", P_BAR);
    } else if (screen == SCR_RIVALS) {
        uint8_t stars = 1 + (uint8_t)((rival_rating(page) - 600) / 300), i;
        tt_text(1, 0, "Skill", P_BAR);
        for (i = 0; i < 5; i++) tt_put(7 + i, 0, i < stars ? TT_T_STAR : TT_T_DOT, P_BAR);
        tt_number(14, 0, rival_rating(page), 4, P_BAR);
    }
    tt_win = 0;
}

static void load_illustration(uint8_t n, uint8_t s) {
    uint16_t c[8];
    uint8_t i;
    VBK_REG = 1;
    set_bkg_data(SLOT_TILES(s), ill_counts[n], ill_tiles[n]);
    VBK_REG = 0;
    for (i = 0; i < 8; i++) c[i] = ill_pals[n][i];
    tt_pal_set(SLOT_PAL(s) * 4, 8, c);
}

static void draw_illustration(uint8_t n, uint8_t s, uint8_t x, uint8_t y) {
    uint8_t t[36], a[36], i, base = SLOT_TILES(s), pal = SLOT_PAL(s);
    for (i = 0; i < 36; i++) {
        t[i] = ill_maps[n][i] + base;
        a[i] = (ill_attrs[n][i] ? pal + 1 : pal) | TT_BK1;
    }
    tt_rows(x, y, 6, 6, t, a);
}

/* ------------------------------------------------------------------ page content */
static const uint8_t main_ill[3] = {TT_ILL_GOMOKU, TT_ILL_2048, TT_ILL_SETTINGS};
static const uint8_t mode_ill[3] = {TT_ILL_RANKED, TT_ILL_PRACTICE, TT_ILL_VERSUS};

static void page_draw(uint8_t p, uint8_t s) {
    pattern_rows(3, 15);
    tt_box(1, 4, 18, 11, TT_T_CARD, P_CARD);
    if (screen == SCR_RIVALS) {
        char name[6], role[20];
        RivalRecord *r = &tt_save.rivals[p];
        portrait_load(p, SLOT_TILES(s));
        portrait_palettes(p, SLOT_PAL(s));
        portrait_draw(p, r->result == 2 ? 1 : 0, 7, 5, SLOT_TILES(s), SLOT_PAL(s));
        rival_info(p, name, role);
        tt_text_far((20 - (uint8_t)strlen(name)) >> 1, 11, name, P_CARD, CURRENT_BANK);
        tt_text_far((20 - (uint8_t)strlen(role)) >> 1, 12, role, P_SUB, CURRENT_BANK);
        tt_text(4, 13, "Won", P_SUB);
        tt_number(8, 13, r->wins, 0, P_CARD);
        tt_text(11, 13, "Lost", P_SUB);
        tt_number(16, 13, r->losses, 0, P_CARD);
        return;
    }
    {
        uint8_t n = screen == SCR_MAIN ? main_ill[p] : mode_ill[p];
        load_illustration(n, s);
        draw_illustration(n, s, 7, 5);
    }
    if (screen == SCR_MAIN) {
        if (p == 0) { tt_ctext(12, "GOMOKU", P_CARD); tt_ctext(13, "Five in a row", P_SUB); }
        else if (p == 1) { tt_ctext(12, "2048", P_CARD); tt_ctext(13, "Slide & merge", P_SUB); }
        else { tt_ctext(12, "SETTINGS", P_CARD); tt_ctext(13, "Sound & save", P_SUB); }
    } else {
        if (p == 0) { tt_ctext(12, "RANKED", P_CARD); tt_ctext(13, "Climb the ladder", P_SUB); }
        else if (p == 1) { tt_ctext(12, "PRACTICE", P_CARD); tt_ctext(13, "Pick your rival", P_SUB); }
        else { tt_ctext(12, "2 PLAYERS", P_CARD); tt_ctext(13, "Pass and play", P_SUB); }
    }
}

static void page_to_buffer(uint8_t p, uint8_t s) {
    tt_to_buffer = 1;
    page_draw(p, s);
    tt_to_buffer = 0;
}

static void blit_column(uint8_t j, uint8_t mapcol) {
    uint8_t t[13], a[13], r;
    for (r = 0; r < 13; r++) {
        t[r] = BUF_TILES[r * 20 + j];
        a[r] = BUF_ATTRS[r * 20 + j];
    }
    VBK_REG = 1;
    set_bkg_tiles(mapcol & 31, 3, 1, 13, a);
    VBK_REG = 0;
    set_bkg_tiles(mapcol & 31, 3, 1, 13, t);
}

static void dots(void) {
    uint8_t i, x0 = 80 - count * 4;
    for (i = 0; i < 10; i++) {
        if (i < count) tt_spr(SPR_PAGE + i, x0 + i * 8, 121, i == page ? TT_S_PAGE_ON : TT_S_PAGE_OFF, OP_CURSOR);
        else hide_sprite(SPR_PAGE + i);
    }
}

static void arrows(void) {
    uint8_t b = (uint8_t)((tt_clock >> 3) & 3);
    if (b == 3) b = 1;
    tt_spr(SPR_ARROW_L, 0 + (anim ? 0 : b), 72, TT_S_ARROW_L, OP_CURSOR);
    tt_spr(SPR_ARROW_R, 152 - (anim ? 0 : b), 72, TT_S_ARROW_R, OP_CURSOR);
}

static void screen_setup(void) {
    uint16_t c[16];
    uint8_t i;
    for (i = 0; i < 16; i++) c[i] = menu_pals[i];
    tt_pal_set(0, 16, c);
    tt_pattern_load();
    tt_menu_bg();
}

static void carousel_open(uint8_t scr, uint8_t first_page, uint8_t n) {
    uint8_t j;
    tt_fade_out();
    tt_screen_reset();
    screen = scr;
    page = first_page;
    count = n;
    slot = 0;
    scroll = 0;
    anim = 0;
    screen_setup();
    page_to_buffer(page, slot);
    for (j = 0; j < 20; j++) blit_column(j, j);
    tt_split_scx = 0;
    tt_split_on = 1;
}

static void carousel_finish_open(void) {
    footer_stat();
    dots();
    arrows();
    tt_music(SONG_TITLE);
    tt_fade_in();
}

static void slide(int8_t d) {
    uint8_t j, base;
    dir = d;
    page = (uint8_t)((page + count + d) % count);
    slot ^= 1;
    page_to_buffer(page, slot);
    tt_pal_commit();
    from_scroll = scroll;
    base = scroll >> 3;
    if (d > 0) for (j = 0; j < 12; j++) blit_column(j, base + 20 + j);
    else for (j = 8; j < 20; j++) blit_column(j, base + j + 12);
    wrote_rest = 0;
    anim = 1;
    tt_sound(SFX_WHOOSH);
    dots();
}

static const uint8_t ease[15] = {0, 3, 9, 19, 32, 48, 66, 84, 102, 118, 131, 142, 150, 156, 160};

static void slide_tick(void) {
    uint8_t off, j, base = from_scroll >> 3;
    off = ease[anim];
    if (!wrote_rest && (off >= 64 || anim == 14)) {
        if (dir > 0) for (j = 12; j < 20; j++) blit_column(j, base + 20 + j);
        else for (j = 0; j < 8; j++) blit_column(j, base + j + 12);
        wrote_rest = 1;
    }
    scroll = dir > 0 ? (uint8_t)(from_scroll + off) : (uint8_t)(from_scroll - off);
    tt_split_scx = scroll;
    if (++anim > 14) {
        anim = 0;
        footer_stat();
    }
}

/* ------------------------------------------------------------------ title */
static void title_draw(void) {
    uint8_t t[120], a[120], i;
    uint16_t c[20];
    for (i = 0; i < 20; i++) c[i] = title_pals[i];
    tt_pal_set(0, 20, c);
    VBK_REG = 1;
    set_bkg_data(0, TT_TITLE_TILE_COUNT, tt_title_tiles);
    VBK_REG = 0;
    tt_pattern_load();
    tt_menu_bg();
    tt_fill(0, 1, 20, 8, 0, 3);
    tt_fill(0, 1, 20, 1, TT_T_RULE_TOP, 3);
    tt_fill(0, 8, 20, 1, TT_T_RULE_BOTTOM, 3);
    for (i = 0; i < 120; i++) {
        t[i] = tt_logo_map[i];
        a[i] = (tt_logo_pals[i] ? 2 : 1) | TT_BK1;
    }
    tt_rows(0, 2, 20, 6, t, a);
    tt_ctext(15, "GOMOKU  &  2048", 0);
    tt_ctext(17, "2026 ARCHER", 0);
}

void tt_title(void) BANKED {
    tt_fade_out();
    tt_screen_reset();
    tt_state = TT_TITLE;
    title_t = 0;
    title_draw();
    tt_music(SONG_TITLE);
    tt_fade_in();
}

static void title_tick(void) {
    uint8_t bob, i;
    title_t++;
    /* blinking prompt */
    if ((title_t & 31) == 0) tt_ctext(12, "PRESS START", 4);
    else if ((title_t & 31) == 22) pattern_rows(12, 12);
    /* bobbing stones either side of the prompt */
    bob = (uint8_t)((title_t >> 3) & 7);
    bob = bob > 3 ? 7 - bob : bob;
    for (i = 0; i < 4; i++) {
        tt_spr(SPR_FX - 8 + i, 12 + (i & 1) * 8, 92 + bob + (i >> 1) * 8, TT_S_BIG_B + i, OP_STONE);
        tt_spr(SPR_FX - 4 + i, 132 + (i & 1) * 8, 95 - bob + (i >> 1) * 8, TT_S_BIG_W + i, OP_STONE);
    }
    if ((title_t & 15) == 0) {
        fx_spawn(FX_SPARK, 8 + (uint8_t)tt_random_below(144), 16 + (uint8_t)tt_random_below(40), 0, 0, TT_S_SPARK0, OP_SPARK, 20);
    }
    if (tt_pressed & (TT_START | TT_A)) {
        tt_sound(SFX_OK);
        fx_burst(80, 96, 8);
        tt_wait(6);
        tt_menu();
    }
}

/* ------------------------------------------------------------------ carousels */
static uint8_t last_pick;

void tt_menu(void) BANKED {
    carousel_open(SCR_MAIN, last_main, 3);
    bars("ARCHER'S TABLETOP");
    footer_hint("Open", 1);
    tt_state = TT_MENU;
    carousel_finish_open();
}

void gomoku_menu(void) BANKED {
    carousel_open(SCR_MODES, last_mode, 3);
    bars("GOMOKU");
    footer_hint("Play", 1);
    tt_state = TT_GMODE;
    carousel_finish_open();
}

static void rivals_open(void) {
    carousel_open(SCR_RIVALS, last_pick, 10);
    bars("CHOOSE A RIVAL");
    footer_hint("Play", 1);
    tt_state = TT_PICK;
    carousel_finish_open();
}

static void settings_open(void);
static void match_open(void);

static void carousel_select(void) {
    if (screen == SCR_MAIN) {
        last_main = page;
        if (page == 0) gomoku_menu();
        else if (page == 1) puzzle_enter();
        else settings_open();
    } else if (screen == SCR_MODES) {
        last_mode = page;
        if (page == 0) match_open();
        else if (page == 1) rivals_open();
        else gomoku_enter(GO_TWO, 0);
    } else {
        last_pick = page;
        gomoku_enter(GO_PRACTICE, page);
    }
}

static void carousel_tick(void) {
    arrows();
    if (anim) {
        slide_tick();
        return;
    }
    if (tt_pressed & TT_LEFT) slide(-1);
    else if (tt_pressed & TT_RIGHT) slide(1);
    else if (tt_pressed & (TT_A | TT_START)) {
        tt_sound(SFX_OK);
        carousel_select();
    } else if (tt_pressed & TT_B) {
        tt_sound(SFX_BACK);
        if (screen == SCR_MAIN) tt_title();
        else if (screen == SCR_MODES) tt_menu();
        else gomoku_menu();
    }
}

/* ------------------------------------------------------------------ settings */
static void settings_item(uint8_t i) {
    uint8_t sel = settings_row == i, y = 4 + i * 3, cp = sel ? P_CARD : 4, tp = sel ? 6 : 7, on;
    static const uint8_t icons[4] = {TT_T_SOUND, TT_T_NOTE, TT_T_TRASH, TT_T_LEFT};
    tt_box(1, y, 18, 3, TT_T_CARD, cp);
    tt_put(3, y + 1, icons[i], tp);
    if (i == 0) tt_text(5, y + 1, "Sound", cp);
    else if (i == 1) tt_text(5, y + 1, "Music", cp);
    else if (i == 2) tt_text(5, y + 1, "Erase save", cp);
    else tt_text(5, y + 1, "Back", cp);
    if (i < 2) {
        on = i == 0 ? tt_save.sound : tt_music_on();
        tt_put(14, y + 1, on ? TT_T_TOGGLE_ON : TT_T_TOGGLE_OFF, tp);
        tt_put(15, y + 1, (on ? TT_T_TOGGLE_ON : TT_T_TOGGLE_OFF) + 1, tp);
        tt_text(16, y + 1, on ? "ON" : "  ", cp);
    }
}

static void settings_draw(void) {
    uint8_t i;
    pattern_rows(3, 15);
    for (i = 0; i < 4; i++) settings_item(i);
}

static void settings_footer(void) {
    tt_win = 1;
    tt_fill(0, 0, 20, 2, 0, P_BAR);
    tt_put(1, 0, TT_T_CHECK, P_BAR);
    tt_text(3, 0, tt_save_available ? "Autosave is on" : "No save battery", P_BAR);
    tt_put(1, 1, TT_T_BTN_A, P_BAR);
    tt_text(3, 1, "Change", P_BAR);
    tt_put(11, 1, TT_T_BTN_B, P_BAR);
    tt_text(13, 1, "Back", P_BAR);
    tt_win = 0;
}

static void settings_open(void) {
    uint16_t c[16];
    uint8_t i;
    tt_fade_out();
    tt_screen_reset();
    screen_setup();
    for (i = 0; i < 16; i++) c[i] = settings_pals[i];
    c[3] = RGB8(142, 152, 232);
    tt_pal_set(16, 16, c);
    bars("SETTINGS");
    settings_footer();
    settings_row = 0;
    confirm = 0;
    settings_draw();
    tt_state = TT_SETTINGS;
    tt_fade_in();
}

static void settings_tick(void) {
    if (confirm) {
        if (tt_pressed & TT_A) {
            tt_save_reset();
            tt_audio_apply();
            tt_sound(SFX_CLACK);
            tt_shake = 12;
            last_main = 0;
            confirm = 0;
            settings_draw();
        } else if (tt_pressed & TT_B) {
            tt_sound(SFX_BACK);
            confirm = 0;
            settings_draw();
        }
        return;
    }
    if (tt_pressed & (TT_UP | TT_DOWN)) {
        uint8_t old = settings_row;
        settings_row = (settings_row + ((tt_pressed & TT_DOWN) ? 1 : 3)) & 3;
        settings_item(old);
        settings_item(settings_row);
        tt_sound(SFX_MOVE);
    } else if (tt_pressed & TT_B) {
        tt_sound(SFX_BACK);
        tt_menu();
    } else if (tt_pressed & TT_A) {
        if (settings_row == 0) {
            tt_save.sound = !tt_save.sound;
            tt_audio_apply();
            tt_sound(SFX_OK);
        } else if (settings_row == 1) {
            tt_save.music = 0x80 | (tt_music_on() ? 0 : 1);
            tt_audio_apply();
            tt_sound(SFX_OK);
        } else if (settings_row == 2) {
            tt_sound(SFX_ERROR);
            confirm = 1;
            tt_box(2, 6, 16, 7, TT_T_CARD, P_CARD);
            tt_ctext(7, "Erase ratings,", P_CARD);
            tt_ctext(8, "scores & rivals?", P_CARD);
            tt_put(4, 10, TT_T_BTN_A, P_CARD);
            tt_text(6, 10, "Erase", P_CARD);
            tt_put(12, 10, TT_T_BTN_B, P_CARD);
            tt_text(14, 10, "Keep", P_CARD);
            return;
        } else {
            tt_sound(SFX_BACK);
            tt_menu();
            return;
        }
        tt_save_request();
        settings_item(settings_row);
    }
}

/* ------------------------------------------------------------------ ranked roulette */
static void match_card(uint8_t id, uint8_t expr) {
    char name[6], role[20];
    slot ^= 1;
    portrait_load(id, SLOT_TILES(slot));
    portrait_palettes(id, SLOT_PAL(slot));
    tt_pal_commit();
    portrait_draw(id, expr, 7, 5, SLOT_TILES(slot), SLOT_PAL(slot));
    rival_info(id, name, role);
    tt_fill(2, 11, 16, 3, 0, P_CARD);
    tt_text_far((20 - (uint8_t)strlen(name)) >> 1, 11, name, P_CARD, CURRENT_BANK);
    tt_text_far((20 - (uint8_t)strlen(role)) >> 1, 12, role, P_SUB, CURRENT_BANK);
    tt_put(7, 13, TT_T_STAR, P_CARD);
    tt_number(9, 13, rival_rating(id), 0, P_CARD);
}

static void match_open(void) {
    tt_fade_out();
    tt_screen_reset();
    screen_setup();
    bars("RANKED MATCH");
    tt_win = 1;
    tt_fill(0, 0, 20, 2, 0, P_BAR);
    tt_ctext(0, "Finding a rival...", P_BAR);
    tt_put(1, 1, TT_T_BTN_B, P_BAR);
    tt_text(3, 1, "Cancel", P_BAR);
    tt_win = 0;
    tt_box(1, 4, 18, 11, TT_T_CARD, P_CARD);
    match_rival = rival_match();
    tt_save.last_rival = match_rival;
    match_shown = (uint8_t)tt_random_below(10);
    match_step = 0;
    match_wait = 1;
    slot = 0;
    match_card(match_shown, 0);
    tt_state = TT_MATCH;
    tt_fade_in();
}

static void match_tick(void) {
    if (match_step < 26) {
        if (tt_pressed & TT_B) {
            tt_sound(SFX_BACK);
            gomoku_menu();
            return;
        }
        if (--match_wait) return;
        match_step++;
        if (match_step == 26) {
            match_card(match_rival, 1);
            tt_music(JINGLE_FOUND);
            fx_burst(76, 52, 8);
            fx_burst(76, 52, 6);
            tt_win = 1;
            tt_fill(0, 0, 20, 2, 0, P_BAR);
            tt_text(1, 0, "You", P_BAR);
            tt_number(5, 0, tt_save.rating, 0, P_BAR);
            tt_text(10, 0, "vs", P_BAR);
            tt_number(13, 0, rival_rating(match_rival), 0, P_BAR);
            tt_put(1, 1, TT_T_BTN_A, P_BAR);
            tt_text(3, 1, "Let's play!", P_BAR);
            tt_win = 0;
            match_wait = 150;
            return;
        }
        do {
            match_shown = (uint8_t)((match_shown + 1 + tt_random_below(9)) % 10);
        } while (match_step >= 22 && match_shown == match_rival);
        if (match_step == 25) match_shown = match_rival;
        match_card(match_shown, 0);
        tt_sound(SFX_TICK);
        match_wait = 2 + (uint8_t)((match_step * match_step) >> 5);
        return;
    }
    if (!--match_wait || (tt_pressed & (TT_A | TT_START))) {
        tt_sound(SFX_OK);
        gomoku_enter(GO_RANKED, match_rival);
    }
}

/* ------------------------------------------------------------------ dispatcher */
void tt_menu_tick(void) BANKED {
    switch (tt_state) {
    case TT_TITLE: title_tick(); break;
    case TT_MENU:
    case TT_GMODE:
    case TT_PICK: carousel_tick(); break;
    case TT_SETTINGS: settings_tick(); break;
    case TT_MATCH: match_tick(); break;
    }
}
