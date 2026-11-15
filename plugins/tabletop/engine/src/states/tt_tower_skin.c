#pragma bank 255
/* ARROW TOWER hoods: hero colours unlocked by lifetime coins and picked with
   Up/Down on the start panel. Prism cycles through the rainbow. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_tower.h"
#include "states/tt_gfx.h"

#define P_HUD 0
uint8_t tw_new_skin = 255;

/* Hood colours: unlocked by lifetime coins, picked on the start panel. */
static const uint16_t skin_cost[TW_SKINS] = {0, 50, 150, 300, 500, 800, 1200, 2000};
static const char skin_names[TW_SKINS][7] = {"Forest", "Ruby", "Ocean", "Sunny", "Violet", "Shadow", "Frost", "Prism"};
static const uint16_t skin_face[TW_SKINS] = {
    RGB8(255, 241, 214), RGB8(255, 241, 214), RGB8(255, 241, 214), RGB8(255, 241, 214),
    RGB8(255, 241, 214), RGB8(200, 200, 216), RGB8(255, 241, 214), RGB8(255, 241, 214)};
static const uint16_t skin_hood[TW_SKINS] = {
    RGB8(60, 174, 76), RGB8(224, 72, 72), RGB8(58, 140, 232), RGB8(240, 176, 32),
    RGB8(154, 90, 216), RGB8(58, 58, 80), RGB8(200, 232, 255), RGB8(224, 72, 72)};
static const uint16_t prism[8] = {
    RGB8(224, 72, 72), RGB8(240, 138, 48), RGB8(240, 208, 64), RGB8(80, 192, 80),
    RGB8(64, 184, 224), RGB8(80, 112, 232), RGB8(160, 96, 224), RGB8(224, 96, 176)};

uint8_t tw_skins_unlocked(uint16_t total) BANKED {
    uint8_t n = 0;
    while (n < TW_SKINS && skin_cost[n] <= total) n++;
    return n;
}

void tw_skin_apply(uint8_t skin) BANKED {
    uint16_t c[2];
    c[0] = skin_face[skin];
    c[1] = skin == 7 ? prism[(tt_clock >> 3) & 7] : skin_hood[skin];
    tt_pal_set(33, 2, c);
}

void tw_skin_name(uint8_t skin, char *out) BANKED {
    memcpy(out, skin_names[skin], 7);
    out[7] = 0;
}

void tw_panel_skin(uint8_t skin) BANKED {
    char name[8];
    uint8_t n = tw_skins_unlocked(tt_save.tw_coins_total);
    tt_win = 1;
    tt_fill(1, 8, 15, 1, 0, P_HUD);
    tt_text(2, 8, "Hood", P_HUD);
    if (n > 1) {
        tt_put(7, 8, TT_T_UP, P_HUD);
        tt_put(8, 8, TT_T_DOWN, P_HUD);
    }
    memcpy(name, skin_names[skin], 7);
    name[7] = 0;
    tt_text_far(10, 8, name, P_HUD, CURRENT_BANK);
    tt_win = 0;
}


/* game-over panel line: a newly unlocked hood, or the lifetime coin bank */
void tw_panel_bank(void) BANKED {
    tt_win = 1;
    if (tw_new_skin != 255) {
        char name[8];
        tw_skin_name(tw_new_skin, name);
        tt_text(2, 9, "New hood!", P_HUD);
        tt_text_far(13, 9, name, P_HUD, CURRENT_BANK);
    } else {
        tt_text(2, 9, "Coin bank", P_HUD);
        tt_number(16, 9, tt_save.tw_coins_total, 0, P_HUD);
    }
    tt_win = 0;
}

void tw_panel_start_floor(uint16_t start) BANKED {
    tt_win = 1;
    tt_fill(1, 6, 18, 1, 0, P_HUD);
    tt_text(2, 6, "Start at floor", P_HUD);
    if (tt_save.tw_check) {
        tt_put(16, 6, TT_T_LEFT, P_HUD);
        tt_put(19, 6, TT_T_RIGHT, P_HUD);
    }
    tt_number(17, 6, start, 0, P_HUD);
    tt_win = 0;
}
