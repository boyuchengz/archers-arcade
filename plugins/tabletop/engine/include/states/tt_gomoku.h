#ifndef TT_GOMOKU_H
#define TT_GOMOKU_H
#include "tabletop.h"
#define GO_RANKED 0
#define GO_PRACTICE 1
#define GO_TWO 2
extern uint8_t go_cells[225],go_live_cells[225],go_history[225],go_count,go_turn,go_winner;
extern uint8_t go_mode,go_rival,go_player_color,go_cursor,go_opening;
extern uint16_t go_patterns[2][225];
extern uint8_t tt_game_cache[512];
#define go_strength(color) (tt_game_cache + ((color) ? 225 : 0))
void go_reset(void) BANKED;
void go_set(uint8_t p,uint8_t color) BANKED;
uint8_t go_place(uint8_t p) BANKED;
uint8_t go_wins_at(uint8_t p,uint8_t color) BANKED;
void go_undo_pair(void) BANKED;
void go_undo(uint8_t n) BANKED;
void go_refresh_cell(uint8_t p) BANKED;
uint8_t patterns_lookup(uint16_t code) BANKED;
void gomoku_render(void) BANKED;
void ai_begin(uint8_t color,uint8_t profile) BANKED;
void ai_tick(void) BANKED;
extern uint8_t ai_done,ai_move,ai_resign;
extern uint8_t ai_hint,ai_cancelled;
extern uint16_t ai_started,ai_visible_at,ai_max_visible;
void ai_cancel(void) BANKED;
void ai_delay(uint16_t frames) BANKED;
#endif
