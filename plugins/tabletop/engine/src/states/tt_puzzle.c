#pragma bank 255
/* 2048 rules and input flow. Drawing lives in tt_puzzle_view.c. */
#include "states/tabletop.h"
#include <string.h>

uint8_t pz_anim, pz_spawn_cell, pz_spawn_age, pz_notice, pz_over, pz_merges, pz_best_before;
uint8_t pz_motion_from[16], pz_motion_to[16], pz_motion_exp[16], pz_motion_count;
uint8_t pz_merge_cells[16];
uint32_t pz_gain;
uint8_t pz_paused, pz_pause_row;
static uint16_t anim_start;
static uint8_t from_cells[16];

static void spawn(void) {
    uint8_t free[16], n = 0, i;
    for (i = 0; i < 16; i++) if (!tt_save.puzzle.cells[i]) free[n++] = i;
    if (n) {
        pz_spawn_cell = free[tt_random_below(n)];
        tt_save.puzzle.cells[pz_spawn_cell] = tt_random_below(10) ? 1 : 2;
        pz_spawn_age = 6;
    }
}

static uint8_t cell_at(uint8_t dir, uint8_t line, uint8_t p) {
    if (dir == TT_LEFT) return line * 4 + p;
    if (dir == TT_RIGHT) return line * 4 + 3 - p;
    if (dir == TT_UP) return p * 4 + line;
    return (3 - p) * 4 + line;
}

uint8_t puzzle_can_move(void) BANKED {
    uint8_t i, *c = tt_save.puzzle.cells;
    for (i = 0; i < 16; i++) if (!c[i]) return 1;
    for (i = 0; i < 16; i++)
        if ((i % 4 < 3 && c[i] < 31 && c[i] == c[i + 1]) || (i < 12 && c[i] < 31 && c[i] == c[i + 4])) return 1;
    return 0;
}

void puzzle_new(void) BANKED {
    uint32_t best = tt_save.puzzle.best;
    memset(&tt_save.puzzle, 0, sizeof(Puzzle));
    tt_save.puzzle.best = best;
    tt_save.puzzle.active = 1;
    spawn();
    spawn();
    pz_anim = pz_notice = pz_over = pz_paused = 0;
    tt_save_request();
}

uint8_t puzzle_move(uint8_t dir) BANKED {
    uint8_t line, p, n, src[4], vals[4], out[4], dst, k, changed = 0;
    uint32_t gain = 0;
    Puzzle *g = &tt_save.puzzle;
    if (dir != TT_LEFT && dir != TT_RIGHT && dir != TT_UP && dir != TT_DOWN) return 0;
    memcpy(from_cells, g->cells, 16);
    pz_motion_count = 0;
    pz_merges = 0;
    for (line = 0; line < 4; line++) {
        n = 0;
        memset(out, 0, 4);
        for (p = 0; p < 4; p++) {
            k = cell_at(dir, line, p);
            if (g->cells[k]) { src[n] = k; vals[n++] = g->cells[k]; }
        }
        p = dst = 0;
        while (p < n) {
            out[dst] = vals[p];
            pz_motion_from[pz_motion_count] = src[p];
            pz_motion_to[pz_motion_count] = cell_at(dir, line, dst);
            pz_motion_exp[pz_motion_count++] = vals[p];
            if (p + 1 < n && vals[p] == vals[p + 1] && vals[p] < 31) {
                uint32_t add;
                pz_motion_from[pz_motion_count] = src[p + 1];
                pz_motion_to[pz_motion_count] = cell_at(dir, line, dst);
                pz_motion_exp[pz_motion_count++] = vals[p + 1];
                out[dst]++;
                pz_merge_cells[pz_merges++] = cell_at(dir, line, dst);
                add = (uint32_t)1 << out[dst];
                if (UINT32_MAX - gain < add) gain = UINT32_MAX; else gain += add;
                p += 2;
            } else p++;
            dst++;
        }
        for (p = 0; p < 4; p++) {
            k = cell_at(dir, line, p);
            if (g->cells[k] != out[p]) changed = 1;
            g->cells[k] = out[p];
        }
    }
    if (!changed) {
        memcpy(g->cells, from_cells, 16);
        return 0;
    }
    memcpy(g->previous, from_cells, 16);
    g->previous_score = g->score;
    g->previous_moves = g->moves;
    g->undo = 1;
    pz_best_before = g->score <= g->best && g->best;
    if (UINT32_MAX - g->score < gain) g->score = UINT32_MAX; else g->score += gain;
    if (g->score > g->best) g->best = g->score;
    if (g->moves < 65535) g->moves++;
    pz_gain = gain;
    pz_anim = 6;
    anim_start = tt_clock;
    pz_spawn_cell = 255;
    pz_spawn_age = 0;
    return 1;
}

void puzzle_undo(void) BANKED {
    uint8_t i, has_win = 0;
    Puzzle *g = &tt_save.puzzle;
    if (!g->undo) return;
    memcpy(g->cells, g->previous, 16);
    g->score = g->previous_score;
    g->moves = g->previous_moves;
    g->undo = 0;
    for (i = 0; i < 16; i++) if (g->cells[i] >= 11) has_win = 1;
    if (!has_win) g->won = 0;
    pz_anim = pz_notice = pz_over = pz_spawn_age = 0;
    tt_save_request();
}

/* Called once the slide animation finishes. */
void puzzle_settle(void) BANKED {
    uint8_t i;
    spawn();
    for (i = 0; i < 16; i++) {
        if (tt_save.puzzle.cells[i] >= 11 && !tt_save.puzzle.won) {
            tt_save.puzzle.won = 1;
            pz_notice = 1;
        }
    }
    if (!puzzle_can_move()) {
        pz_over = 1;
        pz_notice = 2;
    }
    tt_save_request();
}

void puzzle_enter(void) BANKED {
    if (!tt_save.puzzle.active) puzzle_new();
    pz_anim = pz_spawn_age = pz_notice = pz_over = pz_paused = 0;
    if (!puzzle_can_move()) { pz_over = 1; pz_notice = 2; }
    tt_state = TT_PUZZLE;
    puzzle_view_enter();
}

void puzzle_tick(void) BANKED {
    uint8_t d = tt_pressed & TT_DPAD;
    if (pz_anim) {
        uint16_t elapsed = tt_clock - anim_start;
        pz_anim = elapsed >= 6 ? 0 : (uint8_t)(6 - elapsed);
        if (!pz_anim) {
            puzzle_settle();
            puzzle_view_settled();
        } else {
            puzzle_view_board();
        }
        return;
    }
    puzzle_view_tick();
    if (pz_notice || pz_paused) return; /* the view handles popup input */
    if (tt_pressed & TT_SELECT) { tt_menu(); return; }
    if (tt_pressed & TT_START) { pz_paused = 1; pz_pause_row = 0; puzzle_view_stats(); return; }
    if (tt_pressed & TT_B) {
        if (tt_save.puzzle.undo) {
            puzzle_undo();
            tt_sound(SFX_UNDO);
            puzzle_view_board();
            puzzle_view_stats();
        } else {
            tt_sound(SFX_ERROR);
        }
        return;
    }
    if (d) {
        if (d & TT_LEFT) d = TT_LEFT;
        else if (d & TT_RIGHT) d = TT_RIGHT;
        else if (d & TT_UP) d = TT_UP;
        else d = TT_DOWN;
        if (puzzle_move(d)) puzzle_view_moved();
        else tt_sound(SFX_ERROR);
    }
}
