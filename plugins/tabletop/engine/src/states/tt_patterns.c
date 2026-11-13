#pragma bank 255
#include "states/tt_gomoku.h"
extern uint8_t pattern_page_0(uint16_t code) BANKED;
extern uint8_t pattern_page_1(uint16_t code) BANKED;
extern uint8_t pattern_page_2(uint16_t code) BANKED;
extern uint8_t pattern_page_3(uint16_t code) BANKED;
extern uint8_t pattern_page_4(uint16_t code) BANKED;
extern uint8_t pattern_page_5(uint16_t code) BANKED;
extern uint8_t pattern_page_6(uint16_t code) BANKED;
extern uint8_t pattern_page_7(uint16_t code) BANKED;
uint8_t patterns_lookup(uint16_t code) BANKED {switch(code>>13){
case 0:return pattern_page_0(code&8191);
case 1:return pattern_page_1(code&8191);
case 2:return pattern_page_2(code&8191);
case 3:return pattern_page_3(code&8191);
case 4:return pattern_page_4(code&8191);
case 5:return pattern_page_5(code&8191);
case 6:return pattern_page_6(code&8191);
default:return pattern_page_7(code&8191);
}}
