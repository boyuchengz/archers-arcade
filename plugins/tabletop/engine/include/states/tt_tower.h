#ifndef TT_TOWER_H
#define TT_TOWER_H
/* ARROW TOWER: shared state between the game (tt_tower.c), the room builder
   (tt_tower_gen.c) and the presentation layer (tt_tower_view.c). */
#include "tabletop.h"
#include "tt_tower_defs.h"

/* ------------------------------------------------------------------ geometry
   The background map is a 32-row ring. The current room occupies ring rows
   tw_mb .. tw_mb+15 (row 15 is the floor, shared with the room below); the
   next room is built in the 15 rows above it. Positions use pixels with room
   row 0 at y = TW_Y0, so the room above sits at y = 8 .. 127. */
#define TW_Y0 128
#define TW_PAN 120

/* ------------------------------------------------------------------ cell kinds */
#define K_EMPTY 0
#define K_STONE 1
#define K_WOOD 2
#define K_ICE 3
#define K_CRUMBLE 4
#define K_CANNON 5
#define K_SLAB 6
#define K_PLANK 7
#define K_SPRING 8
#define K_SPIKE 9
#define K_SPIKE_D 10
#define K_COIN 11
#define K_GEM 12
#define K_FIRE 13
#define K_SOLID(k) ((uint8_t)((k) - 1) < 6)

/* decoration codes written into the template buffer (drawn, never collide) */
#define D_TORCH 0x40
#define D_FLAME 0x41
#define D_WINDOW 0x42   /* +0..3 */
#define D_BANNER 0x46   /* +0..5 */
#define D_CHAIN 0x4C
#define D_RAIL_END 0x4D
#define D_CAMP 0x4E     /* +0..1 top half of the campfire */

/* ------------------------------------------------------------------ entities */
#define E_NONE 0
#define E_SLIME 1
#define E_BAT 2
#define E_SAW 3
#define E_PLAT 4
#define E_FIRE 5
#define E_CANNON 6
#define E_GHOST 7
#define E_ICICLE 8
#define E_HEART 9
#define E_SAWV 10

typedef struct { uint8_t type, x, y, a, b; } Spawn;
typedef struct {
    uint8_t type, state, timer, anim;
    int16_t x, y;       /* 1/16 px; y uses the TW_Y0 origin */
    int8_t vx, vy;
    uint8_t a, b;       /* track bounds in pixels (or misc) */
    uint8_t base;       /* bat hover line / ghost phase */
    uint8_t old;        /* belongs to the room scrolling away */
} Ent;

#define MAX_ENTS 12
#define MAX_PEND 10

typedef struct { uint8_t on; int16_t x, y; int8_t dir; } Arrow;
typedef struct { uint8_t on; int16_t x, y; uint8_t flip, wob; } Stuck;
typedef struct { uint8_t x, ring, t; } Crumble;

typedef struct {
    uint8_t cells[32][20];
    uint8_t tmpl[15][18];
    Spawn pend[MAX_PEND];
    Ent ent[MAX_ENTS];
    Arrow fly[2];
    Stuck stk[2];
    Crumble crum[6];
    uint8_t row_t[20], row_a[20];
    uint16_t blend[28];
} TowerRAM;
/* The tower borrows the gomoku line-code table (1800 bytes), which is free
   whenever the tower is on screen. */
extern uint16_t go_line_codes[225][4];
#define TW ((TowerRAM *)go_line_codes)

/* ------------------------------------------------------------------ shared state */
extern uint8_t tw_mb;            /* ring row of the current room's row 0 */
extern uint16_t tw_floor;        /* floor number of the current room (1-based) */
extern uint8_t tw_exit;          /* exit lane of the current room (0 L, 1 M, 2 R) */
extern uint8_t tw_next_exit;     /* exit lane of the room above */
extern uint8_t tw_next_flags;
extern uint8_t tw_gen_row;       /* next template row to draw; 15 = done */
extern uint8_t tw_npend;
extern uint8_t tw_hearts, tw_dl;
extern uint16_t tw_coins;
extern uint8_t tw_recent[4];

uint8_t tw_zone_of(uint16_t floor) BANKED;
uint8_t tw_difficulty(uint16_t floor) BANKED;

/* tt_tower_gen.c */
void tw_gen_room(uint16_t floor, uint8_t entry, uint8_t ring0) BANKED;
void tw_gen_begin(uint16_t floor, uint8_t entry, uint8_t ring0) BANKED;
void tw_gen_work(void) BANKED;
void tw_gen_step(void) BANKED;
void tw_gen_finish(void) BANKED;
void tw_redraw(uint8_t x, uint8_t ring) BANKED;
void tw_draw_slab(uint8_t ring, uint8_t lane) BANKED;

/* tt_tower_view.c */
void tw_view_load(uint8_t zone) BANKED;
void tw_view_blend(uint8_t z0, uint8_t z1, uint8_t t) BANKED;
void tw_view_blend_part(uint8_t z0, uint8_t z1, uint8_t t, uint8_t part) BANKED;
void tw_view_anim(void) BANKED;
void tw_hud_draw(void) BANKED;
void tw_hud_stats(void) BANKED;
void tw_hud_hint(uint8_t which) BANKED;
void tw_zone_banner(uint8_t zone) BANKED;
/* hood colours bought with lifetime coins */
#define TW_SKINS 8
extern uint8_t tw_new_skin;
uint8_t tw_skins_unlocked(uint16_t total) BANKED;
void tw_skin_apply(uint8_t skin) BANKED;
void tw_panel_skin(uint8_t skin) BANKED;
void tw_skin_name(uint8_t skin, char *out) BANKED;
void tw_panel_bank(void) BANKED;
void tw_panel(uint8_t kind) BANKED;
void tw_panel_start_floor(uint16_t start) BANKED;
void tw_spring_tile(uint8_t x, uint8_t ring, uint8_t up) BANKED;
void tw_crumble_tile(uint8_t x, uint8_t ring, uint8_t stage) BANKED;
extern uint8_t tw_win_y;

#define PANEL_NONE 0
#define PANEL_START 1
#define PANEL_PAUSE 2
#define PANEL_OVER 3
#define PANEL_RECORD 4

#define HINT_ZONE 0
#define HINT_CONTROLS 1
#define HINT_WOOD 2
#define HINT_CAMP 3

/* ------------------------------------------------------------------ run state (tt_tower.c) */
#define S_START 0
#define S_PLAY 1
#define S_PAN 2
#define S_DEAD 3
#define S_OVER 4
#define S_PAUSE 5
extern int16_t tw_px, tw_py, tw_pvx, tw_pvy;   /* 1/16 px: hitbox left edge, feet */
extern uint8_t tw_st, tw_pan, tw_inv, tw_stun, tw_face, tw_ground, tw_shoot_t, tw_cool, tw_on_plat, tw_jhold;
void tw_hurt(int8_t from_dir, uint8_t respawn) BANKED;

/* ------------------------------------------------------------------ actors (tt_tower_ents.c) */
#define tw_fly (TW->fly)
#define tw_stk (TW->stk)
#define tw_crum (TW->crum)
extern uint8_t tw_stk_next;
void tw_dust(int16_t x, int16_t y, int8_t vx) BANKED;
void tw_spawn_pending(int16_t y0) BANKED;
void tw_collect(int16_t x, int16_t y) BANKED;
void tw_crumble_start(int16_t x, int16_t y) BANKED;
void tw_crumble_tick(void) BANKED;
void tw_arrows_tick(void) BANKED;
void tw_shoot(void) BANKED;
void tw_ents_tick(void) BANKED;
void tw_draw_all(void) BANKED;

void tower_enter(void) BANKED;
void tower_tick(void) BANKED;
#endif
