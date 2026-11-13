#ifndef TT_RIVALS_H
#define TT_RIVALS_H
#include "tt_gomoku.h"
extern uint8_t go_book_transform,rival_rematch;
void rival_style(uint8_t rival,uint8_t *out) BANKED;
uint16_t rival_rating(uint8_t rival) BANKED;
uint8_t rival_match(void) BANKED;
void rival_new_match(void) BANKED;
void rival_book_begin(uint8_t rival) BANKED;
uint8_t rival_book_step(void) BANKED;
uint8_t rival_book_move(uint8_t rival) BANKED;
void rival_info(uint8_t rival,char *name,char *role) BANKED;
extern char rival_msg[2][21];
extern uint8_t rival_msg_seq,rival_mood;
void rival_line(uint8_t event) BANKED;
void rival_react(uint8_t color) BANKED;
void rival_record_result(void) BANKED;
int16_t rival_elo_delta(uint16_t player,uint16_t opponent,uint8_t result) BANKED;
#endif
