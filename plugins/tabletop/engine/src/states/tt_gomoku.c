#pragma bank 255
/* Gomoku table: board, sidebar, rival dialogue, cursor, stone drops, results. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_rivals.h"
#include "states/tt_gfx.h"
#include "states/tt_art_board.h"

uint8_t go_mode, go_rival, go_player_color, go_cursor, go_opening;

#define PORTRAIT_TILES 32
#define P_BOARD (0 | TT_BK1)
#define P_TEXT 1
#define P_GOLD 2
#define P_MINT 3
#define P_FACE 4
#define P_STONE 6
#define P_TITLE 7

static const uint16_t game_pals[] = {
    /* 0 board */ RGB8(233, 172, 92), RGB8(138, 84, 38), RGB8(28, 28, 40), RGB8(255, 255, 255),
    /* 1 text  */ RGB8(27, 32, 70), RGB8(13, 16, 36), RGB8(69, 82, 168), RGB8(255, 246, 223),
    /* 2 gold  */ RGB8(27, 32, 70), RGB8(13, 16, 36), RGB8(69, 82, 168), RGB8(255, 207, 58),
    /* 3 mint  */ RGB8(27, 32, 70), RGB8(13, 16, 36), RGB8(69, 82, 168), RGB8(124, 240, 194),
    /* 4,5 portrait (set per rival) */ 0, 0, 0, 0, 0, 0, 0, 0,
    /* 6 stones */ RGB8(27, 32, 70), RGB8(110, 122, 200), RGB8(28, 28, 40), RGB8(255, 255, 255),
    /* 7 title */ RGB8(27, 32, 70), RGB8(255, 90, 90), RGB8(255, 207, 58), RGB8(18, 21, 46),
};
static const uint8_t voices[10] = {52, 47, 43, 48, 45, 36, 50, 40, 46, 38};

static uint8_t menu_row, game_menu, repeat_dir, ai_active, popup_hidden;
static uint16_t repeat_at;
static uint8_t drop_cell, drop_t, drop_color, after_drop_ai;
static uint8_t seen_seq, type_pos, type_wait, face_expr, face_timer, status_shown;
static uint8_t win_cells[5], win_count, win_timer;
static int16_t elo_delta;
static uint16_t elo_before;

/* ------------------------------------------------------------------ board */
static uint8_t grid_index(uint8_t p) {
    uint8_t x = p % 15, y = p / 15;
    return (y == 0 ? 0 : y == 14 ? 6 : 3) + (x == 0 ? 0 : x == 14 ? 2 : 1);
}

static uint8_t cell_tile(uint8_t p) {
    uint8_t c = go_live_cells[p], x, y;
    if (p == drop_cell && drop_t) c = 0;
    if (c == 1) return tt_black_tiles[grid_index(p)];
    if (c == 2) return tt_white_tiles[grid_index(p)];
    x = p % 15;
    y = p / 15;
    if ((x == 3 || x == 7 || x == 11) && (y == 3 || y == 7 || y == 11) && (x == y || x + y == 14)) return TT_BOARD_STAR;
    return tt_grid_tiles[grid_index(p)];
}

static void cell_draw(uint8_t p) {
    tt_put(p % 15, p / 15, cell_tile(p), P_BOARD);
}

static void board_draw(void) {
    uint8_t t[15], a[15], x, y, p = 0;
    memset(a, P_BOARD, 15);
    for (y = 0; y < 15; y++) {
        for (x = 0; x < 15; x++) t[x] = cell_tile(p++);
        tt_rows(0, y, 15, 1, t, a);
    }
}

/* ------------------------------------------------------------------ sidebar */
static void big_stone(uint8_t x, uint8_t y, uint8_t color) {
    uint8_t a[4], t[4];
    memset(a, P_STONE | TT_BK1, 4);
    memcpy(t, tt_big_stone + (color == 2 ? 4 : 0), 4);
    tt_rows(x, y, 2, 2, t, a);
}

static void turn_draw(void) {
    char name[6], role[20];
    tt_fill(15, 12, 5, 1, 0, P_TEXT);
    big_stone(16, 10, go_turn);
    if (go_mode == GO_TWO) {
        tt_text(15, 12, go_turn == 1 ? "BLACK" : "WHITE", P_TEXT);
    } else if (go_turn == go_player_color) {
        tt_text(16, 12, "YOU", P_MINT);
    } else {
        rival_info(go_rival, name, role);
        tt_text_far(15 + ((5 - (uint8_t)strlen(name)) >> 1), 12, name, P_GOLD, CURRENT_BANK);
    }
}

static void sidebar_draw(void) {
    char name[6], role[20];
    tt_fill(15, 0, 5, 15, 0, P_TEXT);
    if (go_mode == GO_TWO) {
        big_stone(15, 1, 1);
        big_stone(18, 1, 2);
        tt_text(17, 1, "v", P_GOLD);
        tt_text(17, 2, "s", P_GOLD);
        tt_text(15, 4, "LOCAL", P_TEXT);
        tt_text(16, 5, "2P", P_GOLD);
    } else {
        portrait_draw(go_rival, face_expr, 15, 0, PORTRAIT_TILES, P_FACE);
        rival_info(go_rival, name, role);
        tt_text_far(15 + ((5 - (uint8_t)strlen(name)) >> 1), 5, name, P_GOLD, CURRENT_BANK);
        tt_put(15, 6, TT_T_STAR, P_TITLE);
        tt_number(16, 6, rival_rating(go_rival), 4, P_TEXT);
        tt_fill(15, 7, 5, 1, TT_T_RULE_DOTS, P_TEXT);
        tt_put(15, 8, go_player_color == 1 ? TT_T_STONE_B : TT_T_STONE_W, P_STONE);
        tt_text(16, 8, "YOU", P_TEXT);
    }
    tt_fill(15, 9, 5, 1, TT_T_RULE_DOTS, P_TEXT);
    turn_draw();
    tt_put(15, 14, TT_T_BTN_B, P_TITLE);
    tt_text(16, 14, "MENU", P_TEXT);
}

/* ------------------------------------------------------------------ dialogue band */
static void band_clear(void) {
    tt_fill(0, 15, 20, 3, 0, P_TEXT);
}

static void status_far(uint8_t pal, uint8_t which) {
    /* right side of row 15 */
    tt_fill(10, 15, 10, 1, 0, P_TEXT);
    if (which == 1) tt_text(11, 15, "Your move", pal);
    else if (which == 2) tt_text(11, 15, "Thinking", pal);
    else if (which == 3) tt_text(10, 15, "Black move", pal);
    else if (which == 4) tt_text(10, 15, "White move", pal);
    else if (which == 5) tt_text(11, 15, "Game over", pal);
    status_shown = which;
}

static void speaker(void) {
    char name[6], role[20];
    tt_fill(0, 15, 10, 1, 0, P_TEXT);
    if (go_mode == GO_TWO) return;
    rival_info(go_rival, name, role);
    tt_text_far(0, 15, name, P_GOLD, CURRENT_BANK);
    tt_put((uint8_t)strlen(name), 15, ':', P_GOLD);
}

static void set_face(uint8_t expr) {
    if (go_mode == GO_TWO) return;
    face_expr = expr;
    face_timer = expr ? 150 : 0;
    portrait_draw(go_rival, expr, 15, 0, PORTRAIT_TILES, P_FACE);
}

static void dialogue_tick(void) {
    uint8_t line, col;
    char c;
    if (go_mode == GO_TWO) return;
    if (seen_seq != rival_msg_seq) {
        seen_seq = rival_msg_seq;
        type_pos = 0;
        type_wait = 0;
        tt_fill(0, 16, 20, 2, 0, P_TEXT);
        set_face(rival_mood);
    }
    if (face_timer && !--face_timer && face_expr) set_face(0);
    if (type_pos >= 40) return;
    if (type_wait) { type_wait--; return; }
    line = type_pos / 20;
    col = type_pos % 20;
    c = rival_msg[line][col];
    if (!c) {
        type_pos = line ? 40 : 20;
        return;
    }
    tt_put(col, 16 + line, (uint8_t)c, P_TEXT);
    if (c != ' ' && !(type_pos & 1)) tt_sound_pitch(SFX_BLIP, voices[go_rival % 10] + (type_pos & 2 ? 2 : 0));
    type_pos++;
    type_wait = (c == '.' || c == '!' || c == '?' || c == ',') ? 6 : 1;
}

static void two_player_band(void) {
    tt_fill(0, 16, 20, 2, 0, P_TEXT);
    tt_text(1, 16, go_turn == 1 ? "Black to move." : "White to move.", P_TEXT);
    tt_text(1, 17, "Five in a row wins!", P_TEXT);
    status_far(P_MINT, go_turn == 1 ? 3 : 4);
}

/* ------------------------------------------------------------------ sprites */
static void cursor_at(uint8_t p, uint8_t prop) {
    uint8_t x = (p % 15) * 8, y = (p / 15) * 8;
    uint8_t o = (uint8_t)((tt_clock >> 4) & 1);
    tt_spr(SPR_CURSOR + 0, x - 2 - o, y - 2 - o, TT_S_CUR_TL, prop);
    tt_spr(SPR_CURSOR + 1, x + 2 + o, y - 2 - o, TT_S_CUR_TR, prop);
    tt_spr(SPR_CURSOR + 2, x - 2 - o, y + 2 + o, TT_S_CUR_BL, prop);
    tt_spr(SPR_CURSOR + 3, x + 2 + o, y + 2 + o, TT_S_CUR_BR, prop);
}

static void cursor_hide(void) {
    tt_spr_hide(SPR_CURSOR, 4);
}

static void mark_tick(void) {
    uint8_t p;
    if (!go_count || drop_t || game_menu || win_timer || tt_state == TT_RESULT) {
        hide_sprite(SPR_MARK);
        return;
    }
    p = go_history[go_count - 1];
    if ((tt_clock & 63) < 48) tt_spr(SPR_MARK, (p % 15) * 8, (p / 15) * 8, TT_S_MARK, OP_MARK);
    else hide_sprite(SPR_MARK);
}

/* ------------------------------------------------------------------ popups */
static void popup_title(const uint8_t *tiles) {
    uint8_t t[24], a[24], i;
    set_bkg_data(TT_POPUP_TILES, 24, tiles);
    for (i = 0; i < 24; i++) { t[i] = TT_POPUP_TILES + i; a[i] = P_TITLE; }
    tt_rows(1, 4, 12, 2, t, a);
}

static void menu_draw(void) {
    cursor_hide();
    tt_box(0, 3, 15, 10, TT_T_CARD, P_TEXT);
    popup_title(tt_gtitle_menu);
    tt_text(3, 7, "Resume", P_TEXT);
    tt_text(3, 9, go_mode == GO_PRACTICE ? "Undo move" : "No undo", P_TEXT);
    tt_text(3, 11, "Resign", P_TEXT);
    tt_spr(SPR_ARROW_R, 8, 56 + menu_row * 16, TT_S_ARROW_R, OP_CURSOR);
}

static void result_draw(void) {
    const uint8_t *title;
    cursor_hide();
    hide_sprite(SPR_ARROW_R);
    if (go_mode == GO_TWO) title = go_winner == 3 ? tt_gtitle_draw : go_winner == 1 ? tt_gtitle_black : tt_gtitle_white;
    else title = go_winner == 3 ? tt_gtitle_draw : go_winner == go_player_color ? tt_gtitle_win : tt_gtitle_lose;
    tt_box(0, 3, 15, 10, TT_T_CARD, P_TEXT);
    popup_title(title);
    if (go_mode == GO_RANKED) {
        tt_text(1, 7, "Elo", P_TEXT);
        tt_number(5, 7, tt_save.rating, 0, P_GOLD);
        tt_put(10, 7, elo_delta >= 0 ? '+' : '-', elo_delta >= 0 ? P_MINT : P_TEXT);
        tt_number(11, 7, (uint32_t)(elo_delta >= 0 ? elo_delta : -elo_delta), 0, elo_delta >= 0 ? P_MINT : P_TEXT);
    } else if (go_mode == GO_PRACTICE) {
        tt_text(1, 7, "Practice game", P_TEXT);
    } else {
        tt_text(1, 7, "Well played!", P_TEXT);
    }
    tt_text(1, 8, "in", P_TEXT);
    tt_number(4, 8, go_count, 0, P_TEXT);
    tt_text(go_count >= 100 ? 8 : go_count >= 10 ? 7 : 6, 8, "moves", P_TEXT);
    tt_put(2, 10, TT_T_BTN_A, P_TITLE);
    tt_text(4, 10, rival_rematch ? "Rematch!" : "Play again", P_TEXT);
    tt_put(2, 11, TT_T_BTN_B, P_TITLE);
    tt_text(4, 11, "Menu", P_TEXT);
}

static void restore_board_area(void) {
    uint8_t t[15], a[15], x, y, p;
    memset(a, P_BOARD, 15);
    for (y = 3; y < 13; y++) {
        p = y * 15;
        for (x = 0; x < 15; x++) t[x] = cell_tile(p + x);
        tt_rows(0, y, 15, 1, t, a);
    }
}

/* ------------------------------------------------------------------ win line */
static void find_win_line(uint8_t p) {
    static const int8_t dx[4] = {1, 0, 1, 1}, dy[4] = {0, 1, 1, -1};
    uint8_t d, color = go_cells[p], n;
    int8_t x0 = p % 15, y0 = p / 15, x, y, s;
    win_count = 0;
    if (!color) return;
    for (d = 0; d < 4; d++) {
        n = 0;
        win_cells[n++] = p;
        for (s = -1; s <= 1; s += 2) {
            x = x0 + dx[d] * s;
            y = y0 + dy[d] * s;
            while (x >= 0 && x < 15 && y >= 0 && y < 15 && go_cells[y * 15 + x] == color) {
                if (n < 5) win_cells[n++] = y * 15 + x;
                else break;
                x += dx[d] * s;
                y += dy[d] * s;
            }
        }
        if (n >= 5) { win_count = 5; return; }
    }
}

/* ------------------------------------------------------------------ flow */
static void result(void) {
    tt_state = TT_RESULT;
    ai_active = 0;
    popup_hidden = 0;
    elo_before = tt_save.rating;
    if (go_mode != GO_TWO) {
        rival_record_result();
        elo_delta = (int16_t)tt_save.rating - (int16_t)elo_before;
        rival_line(rival_rematch ? 7 : go_winner == go_player_color ? 5 : 6);
    } else {
        elo_delta = 0;
        tt_fill(0, 16, 20, 2, 0, P_TEXT);
        tt_text(1, 16, go_winner == 3 ? "A drawn game!" : go_winner == 1 ? "Black wins!" : "White wins!", P_GOLD);
        tt_text(1, 17, "Great game.", P_TEXT);
        tt_fill(10, 15, 10, 1, 0, P_TEXT);
    }
    status_far(P_GOLD, 5);
    if (go_count) find_win_line(go_history[go_count - 1]);
    win_timer = win_count ? 80 : 1;
    cursor_hide();
    if (go_mode == GO_TWO || go_winner == go_player_color) {
        tt_music(JINGLE_WIN);
    } else if (go_winner == 3) {
        tt_music(JINGLE_FOUND);
    } else {
        tt_music(JINGLE_LOSE);
    }
    tt_save_request();
}

static void begin_drop(uint8_t p, uint8_t color) {
    drop_cell = p;
    drop_color = color;
    drop_t = 7;
    cell_draw(p);
}

static void commit(uint8_t p) {
    uint8_t color = go_turn;
    if (!go_place(p)) {
        tt_sound(SFX_ERROR);
        return;
    }
#ifndef TT_HOST
    if (go_mode != GO_TWO && go_turn == go_player_color) {
        uint16_t elapsed;
        ai_visible_at = sys_time;
        elapsed = ai_visible_at - ai_started;
        if (elapsed > ai_max_visible) ai_max_visible = elapsed;
    }
#endif
    cursor_hide();
    begin_drop(p, color);
}

static void drop_tick(void) {
    static const uint8_t lift[8] = {0, 0, 1, 2, 4, 6, 9, 12};
    uint8_t x = (drop_cell % 15) * 8, y = (drop_cell / 15) * 8;
    drop_t--;
    if (drop_t) {
        tt_spr(SPR_DROP, x, y - lift[drop_t], drop_color == 1 ? TT_S_STONE_B : TT_S_STONE_W, OP_STONE);
        return;
    }
    hide_sprite(SPR_DROP);
    cell_draw(drop_cell);
    tt_sound(SFX_CLACK);
    fx_spawn(FX_SPARK, x - 3, y + 1, -10, 2, TT_S_SPARK0, OP_SPARK, 8);
    fx_spawn(FX_SPARK, x + 3, y + 1, 10, 2, TT_S_SPARK0, OP_SPARK, 8);
    turn_draw();
    if (go_winner) {
        result();
        return;
    }
    if (go_mode == GO_TWO) {
        two_player_band();
    } else if (go_turn != go_player_color) {
        rival_react(go_player_color);
    } else {
        status_far(P_MINT, 1);
    }
}

void gomoku_render(void) BANKED {
    board_draw();
    sidebar_draw();
    band_clear();
    speaker();
    if (go_mode == GO_TWO) two_player_band();
    else {
        uint8_t i;
        for (i = 0; i < 2; i++) tt_text_far(0, 16 + i, rival_msg[i], P_TEXT, CURRENT_BANK);
        type_pos = 40;
        status_far(P_MINT, go_turn == go_player_color ? 1 : 2);
    }
}

void gomoku_enter(uint8_t mode, uint8_t rival) BANKED {
    uint16_t c[32];
    uint8_t i;
    tt_fade_out();
    tt_screen_reset();
    go_mode = mode;
    go_rival = rival;
    go_player_color = mode == GO_PRACTICE ? 1 : 1 + tt_random_below(2);
    go_cursor = 112;
    go_opening = 255;
    go_reset();
    rival_new_match();
    game_menu = ai_active = repeat_dir = popup_hidden = 0;
    drop_t = win_timer = win_count = 0;
    drop_cell = 255;
    face_expr = face_timer = 0;
    tt_state = TT_GOMOKU;
    VBK_REG = 1;
    set_bkg_data(0, TT_BOARD_TILE_COUNT, tt_board_tiles);
    VBK_REG = 0;
    for (i = 0; i < 32; i++) c[i] = game_pals[i];
    tt_pal_set(0, 32, c);
    if (go_mode != GO_TWO) {
        portrait_load(go_rival, PORTRAIT_TILES);
        portrait_palettes(go_rival, P_FACE);
        rival_msg[0][0] = rival_msg[1][0] = 0;
        seen_seq = rival_msg_seq;
    }
    gomoku_render();
    tt_music(SONG_GOMOKU);
    tt_fade_in();
    if (go_mode != GO_TWO) rival_line(0);
    tt_save_request();
}

void gomoku_pause_tick(void) BANKED {}

void gomoku_resume(void) BANKED {
    tt_state = TT_GOMOKU;
    gomoku_render();
}

static void close_menu(void) {
    game_menu = 0;
    hide_sprite(SPR_ARROW_R);
    restore_board_area();
}

void gomoku_tick(void) BANKED {
    uint8_t d, x, y;
    dialogue_tick();
    mark_tick();
    if (drop_t) {
        drop_tick();
        return;
    }
    if (tt_state == TT_RESULT) {
        if (win_timer) {
            win_timer--;
            if (win_count && (win_timer & 7) == 7) {
                uint8_t k = (79 - win_timer) >> 3;
                if (k < 5) {
                    uint8_t p = win_cells[k];
                    fx_burst((p % 15) * 8, (p / 15) * 8, 4);
                    tt_sound_pitch(SFX_MERGE, 4 + k);
                }
            }
            if (win_count && win_timer == 40 && (go_mode == GO_TWO || go_winner == go_player_color)) fx_confetti(16);
            if (!win_timer) result_draw();
            return;
        }
        if (tt_pressed & TT_SELECT) {
            popup_hidden = !popup_hidden;
            if (popup_hidden) { restore_board_area(); }
            else result_draw();
            tt_sound(SFX_MOVE);
        } else if (tt_pressed & TT_A) {
            tt_sound(SFX_OK);
            gomoku_enter(go_mode, go_rival);
        } else if (tt_pressed & TT_B) {
            tt_sound(SFX_BACK);
            gomoku_menu();
        }
        return;
    }
    if (game_menu) {
        if (tt_pressed & (TT_UP | TT_DOWN)) {
            menu_row = (menu_row + ((tt_pressed & TT_DOWN) ? 1 : 2)) % 3;
            tt_spr(SPR_ARROW_R, 8, 56 + menu_row * 16, TT_S_ARROW_R, OP_CURSOR);
            tt_sound(SFX_MOVE);
        } else if (tt_pressed & (TT_B | TT_START)) {
            tt_sound(SFX_BACK);
            close_menu();
        } else if (tt_pressed & TT_A) {
            if (menu_row == 0) {
                tt_sound(SFX_BACK);
                close_menu();
            } else if (menu_row == 1) {
                if (go_mode == GO_PRACTICE && go_count) {
                    go_undo(go_turn == go_player_color ? 2 : 1);
                    tt_sound(SFX_UNDO);
                    close_menu();
                    board_draw();
                    turn_draw();
                } else {
                    tt_sound(SFX_ERROR);
                }
            } else {
                close_menu();
                go_winner = 3 - (go_mode == GO_TWO ? go_turn : go_player_color);
                result();
            }
        }
        return;
    }
    if (tt_pressed & (TT_B | TT_START)) {
        if (ai_active) { ai_cancel(); ai_active = 0; }
        game_menu = 1;
        menu_row = 0;
        tt_sound(SFX_OK);
        menu_draw();
        return;
    }
    if (go_mode != GO_TWO && go_turn != go_player_color) {
        if (!ai_active) {
            ai_begin(go_turn, go_rival);
            ai_active = 1;
            status_far(P_GOLD, 2);
        }
        ai_tick();
        if (ai_hint < 225) cursor_at(ai_hint, OP_RIVAL);
        if (ai_done) {
            cursor_hide();
            ai_active = 0;
            if (ai_cancelled) return;
            if (ai_resign) {
                go_winner = go_player_color;
                result();
            } else {
                commit(ai_move);
            }
        }
        return;
    }
    if (status_shown != 1 && go_mode != GO_TWO) status_far(P_MINT, 1);
    d = tt_pressed & TT_DPAD;
    if (d) {
        repeat_dir = d;
        repeat_at = tt_clock + 14;
    } else if ((tt_held & repeat_dir) && (int16_t)(tt_clock - repeat_at) >= 0) {
        d = repeat_dir;
        repeat_at = tt_clock + 4;
    } else if (!(tt_held & repeat_dir)) {
        repeat_dir = 0;
    }
    x = go_cursor % 15;
    y = go_cursor / 15;
    if ((d & TT_LEFT) && x) x--;
    else if ((d & TT_RIGHT) && x < 14) x++;
    else if ((d & TT_UP) && y) y--;
    else if ((d & TT_DOWN) && y < 14) y++;
    if (d && (uint8_t)(y * 15 + x) != go_cursor) tt_sound(SFX_MOVE);
    go_cursor = y * 15 + x;
    cursor_at(go_cursor, OP_CURSOR);
    if (tt_pressed & TT_A) {
        if (go_cells[go_cursor]) tt_sound(SFX_ERROR);
        else commit(go_cursor);
    }
}
