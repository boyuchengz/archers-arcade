/* Desktop check of the ARROW TOWER room builder: generates thousands of rooms
   (templates and procedural ones) and prints them for tools/check_tower_rooms.py,
   which replays the room checker from tools/tower_rooms.py on every one. */
#define TT_HOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "states/tabletop.h"
uint8_t test_sram[8192], tt_state, tt_pressed, tt_held;
uint16_t tt_clock;
uint16_t go_line_codes[225][4];
#include "tt_save.c"
#include "states/tt_tower.h"
uint8_t tw_mb, tw_exit, tw_next_exit, tw_next_flags, tw_gen_row, tw_npend, tw_hearts, tw_dl;
uint16_t tw_floor, tw_coins;
uint8_t tw_recent[4];
extern uint8_t tw_gen_proc;
int tw_debug = 0;
#include "tt_tower_gen.c"

static char kind_char(uint8_t k, uint8_t t) {
    switch (k) {
    case K_STONE: case K_SLAB: return '#';
    case K_WOOD: return 'W';
    case K_ICE: return 'i';
    case K_CRUMBLE: return '%';
    case K_CANNON: return 'C';
    case K_PLANK: return '=';
    case K_SPRING: return 's';
    case K_SPIKE: return '^';
    case K_SPIKE_D: return 'v';
    case K_COIN: return 'o';
    case K_GEM: return 'g';
    case K_FIRE: return 'F';
    default: return (t == RC_TRACK_P || t == RC_PLATFORM) ? '~' : '.';
    }
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 2000, i, y, x, k;
    memset(test_sram, 255, sizeof(test_sram));
    tt_save_init();
    tt_save.rng = argc > 2 ? (uint32_t)atol(argv[2]) : 12345;
    memset(tw_recent, 255, 4);
    tw_mb = 0;
    tw_hearts = 3;
    for (i = 0; i < n; i++) {
        uint16_t floor = 1 + (uint16_t)(i % 400);
        uint8_t entry = (uint8_t)tt_random_below(3), ring = (uint8_t)(tw_mb - 15) & 31;
        if (floor == 1) entry = 1;
        tw_gen_room(floor, entry, ring);
        printf("ROOM %u %u %u %u\n", floor, entry, tw_next_exit, floor > 1 && floor % 10 ? tw_gen_proc : 0);
        for (y = 0; y < 15; y++) {
            char line[19];
            for (x = 0; x < 18; x++) line[x] = kind_char(TW->cells[(ring + y) & 31][x + 1], TW->tmpl[y][x]);
            line[18] = 0;
            for (k = 0; k < tw_npend; k++) {
                Spawn *s = &TW->pend[k];
                if (s->type == E_ICICLE && s->y / 8 == y && s->x / 8 >= 1 && s->x / 8 <= 18) line[s->x / 8 - 1] = 'I';
            }
            printf("%s\n", line);
        }
        tw_mb = ring;
    }
    return 0;
}
