#pragma bank 255
/* ARROW TOWER: an endless tower climb. One room per floor; jump up through the
   planks in the ceiling and the camera pans to the floor above. B shoots an
   arrow; arrows stick in wood and make footholds, pop slimes, bats and ghosts,
   and grab coins on the way. Every tenth floor is a campfire checkpoint.
   This file holds the hero's physics and the run flow; actors are in
   tt_tower_ents.c, rooms in tt_tower_gen.c and drawing in tt_tower_view.c. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_tower.h"
#include "states/tt_gfx.h"

uint8_t tw_mb, tw_exit, tw_next_exit, tw_next_flags, tw_gen_row, tw_npend, tw_hearts, tw_dl;
uint16_t tw_floor, tw_coins;
uint8_t tw_recent[4];
int16_t tw_px, tw_py, tw_pvx, tw_pvy;
uint8_t tw_st, tw_pan, tw_inv, tw_stun, tw_face, tw_ground, tw_shoot_t, tw_cool, tw_on_plat, tw_jhold;
extern uint8_t fx_n;

/* movement, in 1/16 pixel per frame */
#define GRAV_HOLD 4
#define GRAV_CUT 12
#define GRAV_FALL 5
#define MAX_FALL 64
#define JUMP_V 64
#define SPRING_V 100
#define RUN_MAX 22

static uint8_t st_t, coyote, jbuf, spring_hold, was_ground;
static uint8_t room_flags, camp_used, hint_t, wood_hinted, best_shown, pause_row, win_target;
static uint16_t start_floor, best_at_start;
static uint8_t pan_step, zone_from, zone_to, resp_x;
static uint8_t spring_x, spring_ring, spring_t, pend_exit, pend_flags, just_landed, banked;
static uint16_t heart_goal;

static const uint8_t lane_px[3] = {28, 76, 124};
static const uint8_t pan_ease[31] = {0, 1, 2, 4, 7, 10, 14, 19, 24, 30, 36, 42, 48, 54, 60, 66, 72, 78, 84, 89, 94,
                                     99, 103, 107, 110, 113, 115, 117, 118, 119, 120};

/* ------------------------------------------------------------------ cells */
static uint8_t cell(int16_t x, int16_t y) {
    return TW->cells[(uint8_t)(tw_mb - 16 + (uint8_t)((uint16_t)y >> 3)) & 31][(uint8_t)x >> 3];
}

static uint8_t ring_of(int16_t y) {
    return (uint8_t)(tw_mb - 16 + (uint8_t)((uint16_t)y >> 3)) & 31;
}

static uint8_t solid_span(int16_t x, int16_t top, int16_t bot) {
    if (K_SOLID(cell(x, top))) return 1;
    if (top + 8 < bot && K_SOLID(cell(x, top + 8))) return 1;
    return K_SOLID(cell(x, bot));
}

static uint8_t land_kind(uint8_t k) {
    return K_SOLID(k) || k == K_PLANK;
}

static void tw_die(void);

void tw_hurt(int8_t from_dir, uint8_t respawn) BANKED {
    if (tw_inv || tw_st != S_PLAY) return;
    tt_shake = 10;
    tt_sound(SFX_HURT);
    if (tw_hearts) tw_hearts--;
    if (!tw_hearts) {
        tw_die();
        return;
    }
    tw_inv = 100;
    if (respawn) {
        fx_burst((uint8_t)((tw_px >> 4) + 4), (uint8_t)((tw_py >> 4) - (TW_Y0 - tw_pan) - 8), 8);
        tw_px = (int16_t)(resp_x - 4) << 4;
        tw_py = (int16_t)(TW_Y0 + 120) << 4;
        tw_pvx = tw_pvy = 0;
        tw_stun = 10;
    } else {
        tw_pvx = from_dir > 0 ? 28 : -28;
        tw_pvy = -40;
        tw_stun = 18;
        tw_jhold = 0;
    }
}

static void move_x(int16_t dv) {
    int16_t nx = tw_px + dv, X = nx >> 4, Y = tw_py >> 4, e;
    if (dv > 0) {
        e = X + 7;
        if (solid_span(e, Y - 13, Y - 1)) {
            tw_px = ((e & ~7) - 8) << 4;
            tw_pvx = 0;
            return;
        }
    } else if (dv < 0) {
        if (solid_span(X, Y - 13, Y - 1)) {
            tw_px = ((X & ~7) + 8) << 4;
            tw_pvx = 0;
            return;
        }
    }
    tw_px = nx;
}

static void move_y(void) {
    int16_t ny = tw_py + tw_pvy, X = tw_px >> 4, Y = tw_py >> 4, NY = ny >> 4, T, S;
    uint8_t i, k1, k2;
    tw_ground = 0;
    tw_on_plat = 255;
    if (tw_pvy >= 0) {
        T = NY & ~7;
        if (Y <= T && NY >= T) {
            k1 = cell(X, T);
            k2 = cell(X + 7, T);
            if (land_kind(k1) || land_kind(k2)) {
                tw_py = T << 4;
                tw_pvy = 0;
                tw_ground = 1;
                if (k1 == K_CRUMBLE) tw_crumble_start(X, T);
                if (k2 == K_CRUMBLE) tw_crumble_start(X + 7, T);
                return;
            }
        }
        /* stuck arrows */
        for (i = 0; i < 2; i++) {
            if (!tw_stk[i].on) continue;
            S = tw_stk[i].y;
            if (Y <= S && NY >= S && X + 7 >= tw_stk[i].x - 1 && X <= tw_stk[i].x + 8) {
                tw_py = S << 4;
                tw_pvy = 0;
                tw_ground = 1;
                return;
            }
        }
        /* moving platforms */
        {
            Ent *e = TW->ent;
            for (i = 0; i < MAX_ENTS; i++, e++) {
                if (e->type != E_PLAT || e->old) continue;
                S = e->y >> 4;
                if (Y <= S && NY >= S && X + 7 >= (e->x >> 4) && X <= (e->x >> 4) + 23) {
                    tw_py = S << 4;
                    tw_pvy = 0;
                    tw_ground = 1;
                    tw_on_plat = i;
                    return;
                }
            }
        }
    } else {
        T = NY - 13;
        if (K_SOLID(cell(X, T)) || K_SOLID(cell(X + 7, T))) {
            tw_py = ((T & ~7) + 8 + 13) << 4;
            tw_pvy = 0;
            spring_hold = 0;
            return;
        }
    }
    tw_py = ny;
}

static void player_tick(void) {
    int16_t X, Y;
    uint8_t ice, k, dir = 0, i;
    if (tw_inv) tw_inv--;
    if (tw_cool) tw_cool--;
    if (tw_shoot_t) tw_shoot_t--;
    X = tw_px >> 4;
    Y = tw_py >> 4;
    ice = tw_ground && (cell(X + 1, Y) == K_ICE || cell(X + 6, Y) == K_ICE);
    if (tw_stun) {
        tw_stun--;
    } else {
        if (tt_held & TT_LEFT) { dir = 1; tw_face = 1; }
        else if (tt_held & TT_RIGHT) { dir = 2; tw_face = 0; }
    }
    /* run */
    if (dir == 2) {
        if (tw_pvx < RUN_MAX) tw_pvx += ice ? 1 : tw_ground ? (tw_pvx < 0 ? 5 : 3) : 2;
        if (tw_pvx > RUN_MAX) tw_pvx = RUN_MAX;
    } else if (dir == 1) {
        if (tw_pvx > -RUN_MAX) tw_pvx -= ice ? 1 : tw_ground ? (tw_pvx > 0 ? 5 : 3) : 2;
        if (tw_pvx < -RUN_MAX) tw_pvx = -RUN_MAX;
    } else if (!tw_stun || tw_ground) {
        uint8_t d = ice ? ((tt_clock & 3) == 0) : tw_ground ? 4 : 1;
        if (tw_pvx > 0) tw_pvx = tw_pvx > d ? tw_pvx - d : 0;
        else if (tw_pvx < 0) tw_pvx = -tw_pvx > d ? tw_pvx + d : 0;
    }
    /* jump: coyote time and buffering */
    if (tt_pressed & TT_A) jbuf = 7;
    else if (jbuf) jbuf--;
    if (tw_ground) coyote = 6;
    else if (coyote) coyote--;
    if (jbuf && coyote && !tw_stun) {
        tw_pvy = -JUMP_V;
        tw_jhold = 1;
        jbuf = coyote = 0;
        tw_ground = 0;
        tt_sound(SFX_JUMP);
        tw_dust(X + 4, Y - 2, -6);
        tw_dust(X + 4, Y - 2, 6);
    }
    if (!(tt_held & TT_A)) tw_jhold = 0;
    /* shoot */
    if ((tt_pressed & TT_B) && !tw_cool && !tw_stun) tw_shoot();
    /* gravity */
    if (tw_pvy < 0) tw_pvy += (tw_jhold || spring_hold) ? GRAV_HOLD : GRAV_CUT;
    else {
        spring_hold = 0;
        tw_pvy += GRAV_FALL;
    }
    if (tw_pvy > MAX_FALL) tw_pvy = MAX_FALL;
    was_ground = tw_ground;
    move_x(tw_pvx);
    move_y();
    X = tw_px >> 4;
    Y = tw_py >> 4;
    if (tw_ground && !was_ground) {
        tt_sound(SFX_LAND);
        tw_dust(X, Y - 3, -4);
        tw_dust(X + 8, Y - 3, 4);
    }
    /* ride a moving platform */
    if (tw_on_plat != 255) move_x(TW->ent[tw_on_plat].vx);
    /* springs: feet inside a spring cell */
    if (tw_pvy >= 0) {
        for (i = 0; i < 2; i++) {
            int16_t sx = X + 1 + i * 5;
            if (cell(sx, Y - 1) == K_SPRING || (tw_ground && cell(sx, Y) == K_SPRING)) {
                int16_t sy = cell(sx, Y - 1) == K_SPRING ? Y - 1 : Y;
                tw_pvy = -SPRING_V;
                spring_hold = 1;
                tw_ground = 0;
                tw_py = ((sy & ~7) + 3) << 4;
                spring_x = (uint8_t)sx >> 3;
                spring_ring = ring_of(sy);
                spring_t = 10;
                tw_spring_tile(spring_x, spring_ring, 1);
                tt_sound(SFX_SPRING);
                break;
            }
        }
    }
    X = tw_px >> 4;
    Y = tw_py >> 4;
    /* pickups */
    tw_collect(X + 1, Y - 4);
    tw_collect(X + 6, Y - 4);
    tw_collect(X + 1, Y - 11);
    tw_collect(X + 6, Y - 11);
    /* campfire */
    k = cell(X + 4, Y - 4);
    if (k == K_FIRE && !camp_used) {
        camp_used = 1;
        tw_hearts = 3;
        if (tt_save.tw_check < tw_floor / 10) {
            tt_save.tw_check = (uint8_t)(tw_floor / 10);
            tt_save_request();
        }
        fx_burst((uint8_t)X + 4, (uint8_t)(Y - (TW_Y0 - tw_pan) - 16), 8);
        fx_text((uint8_t)X - 16, (uint8_t)(Y - (TW_Y0 - tw_pan) - 30), "SAVED!");
        tt_music(JINGLE_CHECK);
    }
    /* spikes: generous hitbox */
    if ((cell(X + 2, Y - 2) == K_SPIKE && ((Y - 2) & 7) >= 3) || (cell(X + 5, Y - 2) == K_SPIKE && ((Y - 2) & 7) >= 3) ||
        (cell(X + 2, Y - 12) == K_SPIKE_D && ((Y - 12) & 7) <= 4) || (cell(X + 5, Y - 12) == K_SPIKE_D && ((Y - 12) & 7) <= 4)) {
        tw_hurt(0, 1);
    }
}

static void scroll_to(void) {
    tt_scy = (uint8_t)((tw_mb << 3) - tw_pan);
}

static void enter_room_effects(void) {
    if (tw_floor > tt_save.tw_best) {
        tt_save.tw_best = tw_floor;
        tt_save_request();
        if (!best_shown && best_at_start) {
            best_shown = 1;
            fx_text(52, 20, "NEW BEST");
            tt_music(JINGLE_BEST);
        }
    }
    hint_t = 0;
    if (tw_floor == 1 || tw_floor == start_floor) { tw_hud_hint(HINT_CONTROLS); hint_t = 200; }
    else if ((room_flags & 1) && !wood_hinted) { wood_hinted = 1; tw_hud_hint(HINT_WOOD); hint_t = 180; }
    else if (room_flags & 2) { tw_hud_hint(HINT_CAMP); hint_t = 180; }
    else tw_hud_hint(HINT_ZONE);
}

static void reset_room_state(void) {
    memset(tw_fly, 0, sizeof(tw_fly));
    memset(tw_stk, 0, sizeof(tw_stk));
    memset(tw_crum, 0, sizeof(tw_crum));
    tw_stk_next = 0;
    camp_used = 0;
    spring_t = 0;
}

static void setup_run(uint16_t floor) {
    uint8_t i;
    memset(TW, 0, sizeof(TowerRAM));
    memset(tw_recent, 255, 4);
    tw_floor = floor;
    tw_hearts = 3;
    tw_coins = 0;
    tw_dl = tw_difficulty(floor);
    tw_mb = 0;
    tw_pan = 0;
    reset_room_state();
    wood_hinted = 0;
    best_shown = 0;
    best_at_start = tt_save.tw_best;
    heart_goal = 50;
    banked = 0;
    tw_new_skin = 255;
    fx_clear();
    for (i = 0; i < 40; i++) hide_sprite(i);
    tw_view_blend(tw_zone_of(floor), tw_zone_of(floor), 0);
    tw_draw_slab((uint8_t)(tw_mb + 15), 1);
    tw_gen_room(floor, 1, tw_mb);
    tw_gen_finish();
    tw_spawn_pending(TW_Y0);
    tw_exit = tw_next_exit;
    room_flags = tw_next_flags;
    tw_gen_room(floor + 1, tw_exit, (uint8_t)(tw_mb - 15) & 31);
    tw_gen_finish();
    resp_x = lane_px[1];
    tw_px = (int16_t)(resp_x - 4) << 4;
    tw_py = (int16_t)(TW_Y0 + 120) << 4;
    tw_pvx = tw_pvy = 0;
    tw_face = 0;
    tw_inv = tw_stun = jbuf = coyote = 0;
    tw_ground = 1;
    scroll_to();
    tw_hud_draw();
}

static void start_pan(void) {
    uint8_t i;
    tw_gen_finish();
    for (i = 0; i < MAX_ENTS; i++) {
        TW->ent[i].old = 1;
        if (TW->ent[i].type == E_FIRE) TW->ent[i].type = E_NONE;
    }
    tw_spawn_pending(TW_Y0 - TW_PAN);
    for (i = 0; i < 2; i++) tw_fly[i].on = 0;
    zone_from = tw_zone_of(tw_floor);
    zone_to = tw_zone_of(tw_floor + 1);
    /* start building the floor after next while the camera moves */
    pend_exit = tw_next_exit;
    pend_flags = tw_next_flags;
    tw_gen_begin(tw_floor + 2, tw_next_exit, (uint8_t)(tw_mb - 30) & 31);
    pan_step = 0;
    tw_st = S_PAN;
    tt_sound(SFX_WHOOSH);
}

static void finish_pan(void) {
    uint8_t i;
    Ent *e = TW->ent;
    for (i = 0; i < MAX_ENTS; i++, e++) {
        if (e->old) e->type = E_NONE;
        else e->y += TW_PAN << 4;
    }
    tw_py += TW_PAN << 4;
    reset_room_state();
    resp_x = lane_px[tw_exit];
    tw_mb = (uint8_t)(tw_mb - 15) & 31;
    tw_pan = 0;
    tw_floor++;
    tw_dl = tw_difficulty(tw_floor);
    tw_exit = pend_exit;
    room_flags = pend_flags;
    scroll_to();
    tw_hud_stats();
    if (zone_from != zone_to) {
        tt_music(JINGLE_ZONE);
        tw_zone_banner(zone_to);
    } else {
        char buf[8];
        uint16_t f = tw_floor;
        uint8_t n = 0, d[5], k = 0;
        do { d[n++] = (uint8_t)(f % 10); f /= 10; } while (f && n < 5);
        while (n) buf[k++] = '0' + d[--n];
        buf[k++] = 'F';
        buf[k] = 0;
        fx_text(80 - k * 4, 12, buf);
    }
    enter_room_effects();
    just_landed = 1;
    tw_st = S_PLAY;
}

static void tw_die(void) {
    tw_st = S_DEAD;
    st_t = 0;
    tw_pvy = -56;
    tw_pvx = 0;
    tw_inv = 0;
    tt_music(JINGLE_LOSE);
}

/* Add this run's coins to the lifetime bank (once per run); may unlock a hood. */
static void bank_coins(void) {
    uint8_t before;
    uint16_t t;
    if (banked) return;
    banked = 1;
    before = tw_skins_unlocked(tt_save.tw_coins_total);
    t = tt_save.tw_coins_total + tw_coins;
    tt_save.tw_coins_total = t < tt_save.tw_coins_total ? 65535 : t;
    t = tw_skins_unlocked(tt_save.tw_coins_total);
    tw_new_skin = t > before ? (uint8_t)(t - 1) : 255;
    tt_save_request();
}

/* Every 50 coins in a run: a heart back, or a coin bonus at full health. */
static void coin_reward(void) {
    int16_t sx = (tw_px >> 4), sy = (tw_py >> 4) - (TW_Y0 - tw_pan) - 26;
    if (sy < 8) sy = 8;
    if (tw_hearts < 3) {
        tw_hearts++;
        tt_sound(SFX_HEART);
        fx_spawn(FX_FLOAT, (uint8_t)sx, (uint8_t)sy, 0, -10, TW_S_HEART, 7 | S_BANK, 40);
        fx_burst(8 + (tw_hearts - 1) * 9, 132, 6);
    } else {
        tw_coins += 10;
        tt_sound(SFX_GEM);
        fx_text((uint8_t)(sx - 6), (uint8_t)sy, "+10");
    }
    tw_hud_stats();
}

static void game_over(void) {
    uint8_t record = tw_floor > best_at_start;
    bank_coins();
    tw_panel(record ? PANEL_RECORD : PANEL_OVER);
    win_target = 48;
    tw_st = S_OVER;
    if (record || tw_new_skin != 255) fx_confetti(10);
    if (tw_new_skin != 255) tt_music(JINGLE_BEST);
}

void tower_enter(void) BANKED {
    tt_fade_out();
    tt_screen_reset();
    fx_n = 12;
    tw_view_load(tw_zone_of(1));
    start_floor = 1 + (uint16_t)tt_save.tw_check * 10;
    tw_win_y = 64;
    win_target = 64;
    move_win(7, tw_win_y);
    SHOW_WIN;
    setup_run(start_floor);
    tw_view_load(tw_zone_of(start_floor));
    if (tt_save.tw_skin >= tw_skins_unlocked(tt_save.tw_coins_total)) tt_save.tw_skin = 0;
    tw_skin_apply(tt_save.tw_skin);
    tw_panel(PANEL_START);
    tw_panel_start_floor(start_floor);
    tw_st = S_START;
    tt_state = TT_TOWER;
    tt_music(SONG_TOWER);
    tw_draw_all();
    tt_fade_in();
}

static void restart(uint16_t floor) {
    tt_fade_out();
    start_floor = floor;
    setup_run(floor);
    win_target = 128;
    tw_st = S_PLAY;
    enter_room_effects();
    tw_draw_all();
    tt_music(SONG_TOWER);
    tt_fade_in();
}

static void slide_window(void) {
    if (tw_win_y != win_target) {
        uint8_t d = tw_win_y > win_target ? tw_win_y - win_target : win_target - tw_win_y;
        d = (d >> 2) + 1;
        tw_win_y = tw_win_y > win_target ? tw_win_y - d : tw_win_y + d;
        move_win(7, tw_win_y);
    }
}

void tower_tick(void) BANKED {
    tw_view_anim();
    slide_window();
    /* keep the frames that start and end a pan light */
    if (!(tw_st == S_PAN && (pan_step == 0 || pan_step >= 29 || zone_from != zone_to)) && !just_landed)
        tw_gen_work();
    if (tw_st != S_PAN && !just_landed) tw_gen_step();
    just_landed = 0;
    switch (tw_st) {
    case S_START:
        if (tt_save.tw_check && (tt_pressed & (TT_LEFT | TT_RIGHT))) {
            uint16_t top = 1 + (uint16_t)tt_save.tw_check * 10;
            if (tt_pressed & TT_RIGHT) start_floor = start_floor >= top ? 1 : start_floor + 10;
            else start_floor = start_floor <= 1 ? top : start_floor - 10;
            tt_sound(SFX_MOVE);
            setup_run(start_floor);
            tw_panel(PANEL_START);
            tw_panel_start_floor(start_floor);
        } else if (tt_pressed & (TT_UP | TT_DOWN)) {
            uint8_t n = tw_skins_unlocked(tt_save.tw_coins_total);
            if (n > 1) {
                tt_save.tw_skin = (uint8_t)((tt_save.tw_skin + ((tt_pressed & TT_DOWN) ? 1 : n - 1)) % n);
                tw_skin_apply(tt_save.tw_skin);
                tw_panel_skin(tt_save.tw_skin);
                tt_save_request();
                tt_sound(SFX_MOVE);
            }
        } else if (tt_pressed & (TT_A | TT_START)) {
            tt_sound(SFX_OK);
            win_target = 128;
            tw_st = S_PLAY;
            enter_room_effects();
        } else if (tt_pressed & TT_B) {
            tt_sound(SFX_BACK);
            tt_menu();
            return;
        }
        break;
    case S_PLAY:
        if (tt_pressed & TT_START) {
            tt_sound(SFX_OK);
            tw_panel(PANEL_PAUSE);
            pause_row = 0;
            win_target = 64;
            tw_st = S_PAUSE;
            break;
        }
        player_tick();
        if (tw_st != S_PLAY) break;
        tw_ents_tick();
        tw_arrows_tick();
        tw_crumble_tick();
        if (spring_t && !--spring_t) tw_spring_tile(spring_x, spring_ring, 0);
        if (hint_t && !--hint_t) tw_hud_hint(HINT_ZONE);
        if (tw_coins >= heart_goal) {
            heart_goal += 50;
            coin_reward();
        }
        if ((tw_py >> 4) <= TW_Y0) start_pan();
        break;
    case S_PAN:
        pan_step++;
        tw_pan = pan_ease[pan_step];
        scroll_to();
        if (zone_from != zone_to && pan_step <= 27) {
            uint8_t part = (pan_step - 1) % 3, t = (uint8_t)(((pan_step - part + 2) * 16) / 27);
            tw_view_blend_part(zone_from, zone_to, t > 16 ? 16 : t, part);
        }
        if (pan_step >= 30) finish_pan();
        break;
    case S_DEAD:
        st_t++;
        if (st_t > 20) {
            tw_pvy += 4;
            tw_py += tw_pvy;
        }
        if (st_t == 90) game_over();
        break;
    case S_OVER:
        if (tw_win_y > win_target + 8) break;
        if (tt_pressed & (TT_A | TT_START)) {
            tt_sound(SFX_OK);
            restart(1 + (uint16_t)tt_save.tw_check * 10);
        } else if (tt_pressed & TT_B) {
            tt_sound(SFX_BACK);
            tt_menu();
            return;
        }
        break;
    case S_PAUSE:
        if (tt_pressed & (TT_UP | TT_DOWN)) {
            pause_row = (pause_row + ((tt_pressed & TT_DOWN) ? 1 : 2)) % 3;
            tt_sound(SFX_MOVE);
        } else if (tt_pressed & (TT_B | TT_START)) {
            tt_sound(SFX_BACK);
            win_target = 128;
            tw_st = S_PLAY;
            tw_hud_draw();
        } else if (tt_pressed & TT_A) {
            tt_sound(SFX_OK);
            if (pause_row == 0) { win_target = 128; tw_st = S_PLAY; tw_hud_draw(); }
            else if (pause_row == 1) { bank_coins(); restart(start_floor); hide_sprite(SPR_FX - 1); return; }
            else { bank_coins(); tt_menu(); return; }
        }
        break;
    }
    tw_draw_all();
    if (tt_save.tw_skin == 7 && (tt_clock & 7) == 0) tw_skin_apply(7);
    if (tw_st == S_START && tw_win_y == win_target) {
        /* hood preview beside the picker */
        uint8_t f = ((tt_clock >> 5) & 1) ? TW_S_HERO_IDLE1 : TW_S_HERO_IDLE0;
        tt_spr(4, 128, tw_win_y + 56, f, S_BANK);
        tt_spr(5, 136, tw_win_y + 56, f + 1, S_BANK);
        tt_spr(6, 128, tw_win_y + 64, f + 2, S_BANK);
        tt_spr(7, 136, tw_win_y + 64, f + 3, S_BANK);
    }
    if (tw_st == S_PAUSE && tw_win_y == win_target) tt_spr(SPR_FX - 1, 22, tw_win_y + 48 + pause_row * 8, TT_S_ARROW_R, OP_CURSOR);
    else hide_sprite(SPR_FX - 1);
}
