#pragma bank 255
/* 2048 presentation: board, sidebar, rolling score, popups and effects. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_gfx.h"
#include "states/tt_art_puzzle.h"

extern uint8_t tt_scratch[512];
extern uint8_t tt_game_cache[512];
extern uint8_t pz_paused, pz_pause_row;
#define MAP (tt_scratch)
#define ATT (tt_scratch + 256)
#define CACHE_T (tt_game_cache)
#define CACHE_A (tt_game_cache + 256)

#define BOARD_Y 1
#define SIDE_X 16
#define D_SCORE (TT_DYN)
#define D_BEST (TT_DYN + 4)
#define D_MOVES (TT_DYN + 8)

static uint8_t cache_ok, popup, shown_moves, best_shown;
static uint32_t shown_score, shown_best;
static const uint8_t tween[7][4] = {{0, 0, 0, 0}, {0, 0, 1, 2}, {0, 1, 2, 4}, {0, 2, 4, 6}, {0, 2, 5, 8}, {0, 3, 6, 10}, {0, 4, 8, 12}};

static uint8_t card_pal(uint8_t e) {
    if (e > 14) e = 14;
    return (uint8_t)(1 + ((e - 1) >> 1)) | TT_BK1;
}

static void stamp(uint8_t tx, uint8_t ty, const uint8_t *m, uint8_t attr) {
    uint8_t x, y, i;
    for (y = 0; y < 4; y++) {
        if (ty + y >= 16) break;
        for (x = 0; x < 4; x++) {
            if (tx + x >= 16) continue;
            i = (uint8_t)((ty + y) * 16 + tx + x);
            MAP[i] = m[y * 4 + x];
            ATT[i] = attr;
        }
    }
}

static void card(uint8_t tx, uint8_t ty, uint8_t e, uint8_t small) {
    if (!e) return;
    if (e > 14) e = 14;
    if (small) stamp(tx, ty, tt_small_maps + ((e & 1) ? 0 : 16), card_pal(e));
    else stamp(tx, ty, tt_card_maps + (uint16_t)(e - 1) * 16, card_pal(e));
}

static void upload(void) {
    uint8_t y, x, start;
    uint16_t i;
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16;) {
            i = (uint16_t)y * 16 + x;
            if (cache_ok && CACHE_T[i] == MAP[i] && CACHE_A[i] == ATT[i]) { x++; continue; }
            start = x;
            do {
                CACHE_T[i] = MAP[i];
                CACHE_A[i] = ATT[i];
                x++;
                i++;
            } while (x < 16 && (!cache_ok || CACHE_T[i] != MAP[i] || CACHE_A[i] != ATT[i]));
            i = (uint16_t)y * 16 + start;
            tt_rows(start, BOARD_Y + y, x - start, 1, MAP + i, ATT + i);
        }
    }
    cache_ok = 1;
}

void puzzle_view_board(void) BANKED {
    uint8_t i, j, fx, fy, tx, ty, step;
    int16_t px, py;
    uint8_t *cells = tt_save.puzzle.cells;
    for (i = 0; i < 16; i++) stamp((i & 3) * 4, (i >> 2) * 4, tt_slot_map, 0 | TT_BK1);
    if (!pz_anim) {
        for (i = 0; i < 16; i++) card((i & 3) * 4, (i >> 2) * 4, cells[i], pz_spawn_age > 3 && i == pz_spawn_cell);
    } else {
        step = 6 - pz_anim;
        for (j = 0; j < pz_motion_count; j++) {
            i = pz_motion_from[j];
            fx = (i & 3) * 4; fy = (i >> 2) * 4;
            i = pz_motion_to[j];
            tx = (i & 3) * 4; ty = (i >> 2) * 4;
            px = tx >= fx ? fx + tween[step][(tx - fx) >> 2] : fx - tween[step][(fx - tx) >> 2];
            py = ty >= fy ? fy + tween[step][(ty - fy) >> 2] : fy - tween[step][(fy - ty) >> 2];
            card((uint8_t)px, (uint8_t)py, pz_motion_exp[j], 0);
        }
    }
    upload();
}

static void digits_now(void) {
    tt_digits(D_SCORE, 4, shown_score, 1, 3);
    tt_digits(D_BEST, 4, shown_best, 3, 1);
    tt_digits(D_MOVES, 4, tt_save.puzzle.moves, 3, 1);
    shown_moves = (uint8_t)tt_save.puzzle.moves;
}

static void sidebar(void) {
    uint8_t t[8], a[8], i;
    for (i = 0; i < 8; i++) { t[i] = tt_mini_logo[i]; a[i] = 2 | TT_BK1; }
    tt_rows(SIDE_X, 1, 4, 2, t, a);
    memset(a, 0 | TT_BK1, 4);
    memcpy(t, tt_label_score, 4); tt_rows(SIDE_X, 4, 4, 1, t, a);
    memcpy(t, tt_label_best, 4); tt_rows(SIDE_X, 7, 4, 1, t, a);
    memcpy(t, tt_label_moves, 4); tt_rows(SIDE_X, 10, 4, 1, t, a);
    memcpy(t, tt_hint_undo, 4); tt_rows(SIDE_X, 14, 4, 1, t, a);
    memcpy(t, tt_hint_menu, 4); tt_rows(SIDE_X, 16, 4, 1, t, a);
    for (i = 0; i < 4; i++) { t[i] = D_SCORE + i; a[i] = 6; }
    tt_rows(SIDE_X, 5, 4, 1, t, a);
    for (i = 0; i < 4; i++) { t[i] = D_BEST + i; a[i] = 0; }
    tt_rows(SIDE_X, 8, 4, 1, t, a);
    for (i = 0; i < 4; i++) t[i] = D_MOVES + i;
    tt_rows(SIDE_X, 11, 4, 1, t, a);
}

static void load_title(const uint8_t *tiles) {
    set_bkg_data(TT_POPUP_TILES, 24, tiles);
}

static void popup_box(const uint8_t *title) {
    uint8_t t[24], a[24], i;
    load_title(title);
    tt_box(1, 4, 14, 10, TT_T_CARD, 0);
    for (i = 0; i < 24; i++) { t[i] = TT_POPUP_TILES + i; a[i] = 6; }
    tt_rows(2, 5, 12, 2, t, a);
}

static void draw_popup(void) {
    char buf[12];
    tt_spr_hide(SPR_ARROW_L, 2);
    if (pz_paused) {
        popup_box(tt_title_pause);
        tt_text(5, 8, "Resume", 0);
        tt_text(5, 10, "New game", 0);
        tt_text(5, 12, "Main menu", 0);
        tt_spr(SPR_ARROW_R, 26, 64 + pz_pause_row * 16, TT_S_ARROW_R, OP_CURSOR);
        popup = 3;
        return;
    }
    if (pz_notice == 1) {
        popup_box(tt_title_win);
        tt_text(2, 8, "Incredible!", 0);
        tt_text(2, 9, "Aim for 4096", 0);
        tt_put(2, 11, TT_T_BTN_A, 6);
        tt_text(4, 11, "Keep going", 0);
        popup = 1;
    } else if (pz_notice == 2) {
        popup_box(tt_title_over);
        tt_text(2, 8, "Score", 0);
        tt_number(8, 8, tt_save.puzzle.score, 0, 0);
        if (tt_save.puzzle.score >= tt_save.puzzle.best && tt_save.puzzle.score) tt_text(2, 9, "New record!", 0);
        tt_put(2, 11, TT_T_BTN_A, 6);
        tt_text(4, 11, "New game", 0);
        if (tt_save.puzzle.undo) {
            tt_put(2, 12, TT_T_BTN_B, 6);
            tt_text(4, 12, "Undo move", 0);
        }
        popup = 2;
    }
    (void)buf;
}

static void redraw_all(void) {
    cache_ok = 0;
    puzzle_view_board();
}

void puzzle_view_enter(void) BANKED {
    uint16_t c[32];
    uint8_t i;
    tt_fade_out();
    tt_screen_reset();
    VBK_REG = 1;
    set_bkg_data(0, TT_PUZZLE_TILE_COUNT, tt_puzzle_tiles);
    VBK_REG = 0;
    for (i = 0; i < 32; i++) c[i] = tt_puzzle_palettes[i];
    tt_pal_set(0, 32, c);
    shown_score = tt_save.puzzle.score;
    shown_best = tt_save.puzzle.best;
    best_shown = 0;
    popup = 0;
    digits_now();
    sidebar();
    redraw_all();
    if (pz_notice) draw_popup();
    tt_music(SONG_PUZZLE);
    tt_fade_in();
}

void puzzle_view_stats(void) BANKED {
    shown_best = tt_save.puzzle.best;
    if (shown_score > tt_save.puzzle.score) shown_score = tt_save.puzzle.score;
    digits_now();
    if (pz_paused) {
        tt_sound(SFX_OK);
        draw_popup();
    }
}

void puzzle_view_moved(void) BANKED {
    tt_sound(SFX_SLIDE);
    puzzle_view_board();
}

static void close_popup(void) {
    popup = 0;
    tt_spr_hide(SPR_ARROW_R, 1);
    redraw_all();
}

void puzzle_view_settled(void) BANKED {
    uint8_t i, top = 0, e, cell;
    char buf[12];
    redraw_all();
    if (pz_merges) {
        for (i = 0; i < pz_merges; i++) {
            cell = pz_merge_cells[i];
            e = tt_save.puzzle.cells[cell];
            if (e > top) top = e;
            if (i < 3) fx_burst((cell & 3) * 32 + 12, BOARD_Y * 8 + (cell >> 2) * 32 + 10, e >= 7 ? 6 : 4);
        }
        tt_sound_pitch(SFX_MERGE, top);
        if (top >= 7) tt_shake = top >= 10 ? 16 : 8;
        /* "+gain" floats up beside the score */
        buf[0] = '+';
        {
            uint32_t g = pz_gain;
            uint8_t n = 0, d[10], k = 1;
            do { d[n++] = (uint8_t)(g % 10); g /= 10; } while (g && n < 7);
            while (n) buf[k++] = '0' + d[--n];
            buf[k] = 0;
            fx_text(SIDE_X * 8 - (k > 4 ? (k - 4) * 7 : 0), 52, buf);
        }
    } else {
        tt_sound(SFX_POP);
    }
    if (!best_shown && pz_best_before && tt_save.puzzle.score == tt_save.puzzle.best && tt_save.puzzle.score) {
        best_shown = 1;
        fx_burst(SIDE_X * 8 + 14, 64, 8);
        tt_music(JINGLE_BEST);
    }
    shown_best = tt_save.puzzle.best;
    tt_digits(D_BEST, 4, shown_best, 3, 1);
    if (pz_notice == 1) {
        fx_confetti(16);
        tt_music(JINGLE_2048);
        draw_popup();
    } else if (pz_notice == 2) {
        tt_shake = 10;
        tt_music(JINGLE_LOSE);
        draw_popup();
    }
}

void puzzle_view_tick(void) BANKED {
    uint32_t target = tt_save.puzzle.score;
    /* rolling score counter */
    if (shown_score != target) {
        if (shown_score > target) shown_score = target;
        else shown_score += ((target - shown_score) >> 2) + 1;
        tt_digits(D_SCORE, 4, shown_score, 1, 3);
    }
    if (shown_moves != (uint8_t)tt_save.puzzle.moves) {
        shown_moves = (uint8_t)tt_save.puzzle.moves;
        tt_digits(D_MOVES, 4, tt_save.puzzle.moves, 3, 1);
    }
    if (pz_spawn_age) {
        pz_spawn_age--;
        if (!popup) puzzle_view_board();
    }
    if (tt_save.puzzle.won && !popup && (tt_clock & 31) == 0) {
        /* a little twinkle over any 2048+ tile */
        uint8_t i;
        for (i = 0; i < 16; i++) {
            if (tt_save.puzzle.cells[i] >= 11) {
                fx_spawn(FX_SPARK, (i & 3) * 32 + 4 + (uint8_t)tt_random_below(20), BOARD_Y * 8 + (i >> 2) * 32 + 2 + (uint8_t)tt_random_below(18), 0, 0, TT_S_SPARK0, OP_SPARK, 16);
                break;
            }
        }
    }
    /* popup input */
    if (popup == 3) {
        if (tt_pressed & (TT_UP | TT_DOWN)) {
            pz_pause_row = (pz_pause_row + ((tt_pressed & TT_DOWN) ? 1 : 2)) % 3;
            tt_spr(SPR_ARROW_R, 26, 64 + pz_pause_row * 16, TT_S_ARROW_R, OP_CURSOR);
            tt_sound(SFX_MOVE);
        } else if (tt_pressed & (TT_START | TT_B)) {
            pz_paused = 0;
            tt_sound(SFX_BACK);
            close_popup();
        } else if (tt_pressed & TT_A) {
            pz_paused = 0;
            tt_sound(SFX_OK);
            if (pz_pause_row == 0) close_popup();
            else if (pz_pause_row == 1) {
                puzzle_new();
                shown_score = 0;
                best_shown = 0;
                digits_now();
                close_popup();
            } else {
                popup = 0;
                tt_menu();
            }
        }
        return;
    }
    if (popup == 1) {
        if (tt_pressed & (TT_A | TT_B | TT_START)) {
            pz_notice = 0;
            tt_sound(SFX_OK);
            close_popup();
        }
        return;
    }
    if (popup == 2) {
        if (tt_pressed & TT_A) {
            pz_notice = 0;
            tt_sound(SFX_OK);
            puzzle_new();
            shown_score = 0;
            best_shown = 0;
            digits_now();
            close_popup();
        } else if ((tt_pressed & TT_B) && tt_save.puzzle.undo) {
            pz_notice = 0;
            puzzle_undo();
            tt_sound(SFX_UNDO);
            digits_now();
            close_popup();
        } else if (tt_pressed & TT_SELECT) {
            popup = 0;
            tt_menu();
        }
    }
}
