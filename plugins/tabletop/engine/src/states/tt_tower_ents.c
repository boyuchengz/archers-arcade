#pragma bank 255
/* ARROW TOWER actors: enemies, hazards, pickups, arrows, crumbling blocks and
   all sprite drawing. The hero's physics and the run flow live in tt_tower.c. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "states/tt_tower.h"
#include "states/tt_gfx.h"

#define PB (S_BANK)
#define JUMP_V 64

uint8_t tw_stk_next;
static uint8_t oam_i;

static const int8_t sine16[16] = {0, 2, 4, 5, 6, 5, 4, 2, 0, -2, -4, -5, -6, -5, -4, -2};
static const uint8_t ent_slots[17] = {11, 12, 13, 14, 15, 16, 17, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39};

/* ------------------------------------------------------------------ cells */
static uint8_t cell(int16_t x, int16_t y) {
    return TW->cells[(uint8_t)(tw_mb - 16 + (uint8_t)((uint16_t)y >> 3)) & 31][(uint8_t)x >> 3];
}

static uint8_t ring_of(int16_t y) {
    return (uint8_t)(tw_mb - 16 + (uint8_t)((uint16_t)y >> 3)) & 31;
}

static void set_cell(int16_t x, int16_t y, uint8_t k) {
    uint8_t r = ring_of(y);
    TW->cells[r][(uint8_t)x >> 3] = k;
    tw_redraw((uint8_t)x >> 3, r);
}

static uint8_t land_kind(uint8_t k) {
    return K_SOLID(k) || k == K_PLANK;
}

static void put(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop) {
    uint8_t id;
    if (oam_i >= sizeof(ent_slots)) return;
    if (sx <= -8 || sx >= 160 || sy <= -8 || sy >= 128) return;
    if (sy + 8 > (int16_t)tw_win_y) return;
    id = ent_slots[oam_i++];
    set_sprite_tile(id, tile);
    set_sprite_prop(id, prop);
    move_sprite(id, (uint8_t)(sx + 8), (uint8_t)(sy + 16));
}

static void put16(uint8_t tile, int16_t sx, int16_t sy, uint8_t prop) {
    uint8_t f = prop & S_FLIPX ? 1 : 0;
    put(tile + f, sx, sy, prop);
    put(tile + 1 - f, sx + 8, sy, prop);
    put(tile + 2 + f, sx, sy + 8, prop);
    put(tile + 3 - f, sx + 8, sy + 8, prop);
}

void tw_dust(int16_t x, int16_t y, int8_t vx) BANKED {
    int16_t sy = y - (TW_Y0 - tw_pan);
    if (sy < 0 || sy > 127) return;
    fx_spawn(FX_STILL, (uint8_t)x, (uint8_t)sy, vx, -4, TW_S_DUST0, OP_SPARK | PB, 10);
}

static Ent *ent_new(uint8_t type) {
    uint8_t i;
    Ent *e = TW->ent;
    for (i = 0; i < MAX_ENTS; i++, e++) {
        if (e->type == E_NONE) {
            memset(e, 0, sizeof(Ent));
            e->type = type;
            return e;
        }
    }
    return 0;
}

void tw_spawn_pending(int16_t y0) BANKED {
    uint8_t i, cyc = (uint8_t)((tw_floor) / 60);
    Spawn *s = TW->pend;
    Ent *e;
    for (i = 0; i < tw_npend; i++, s++) {
        e = ent_new(s->type);
        if (!e) break;
        e->x = (int16_t)s->x << 4;
        e->y = (y0 + s->y) << 4;
        e->a = s->a;
        e->b = s->b;
        switch (s->type) {
        case E_SLIME: e->vx = (tt_random_below(2) ? 8 : -8) * (cyc ? 2 : 1); if (tw_dl >= 8) e->vx += e->vx > 0 ? 3 : -3; break;
        case E_BAT: e->vx = tt_random_below(2) ? 12 : -12; e->base = s->y; e->anim = (uint8_t)tt_random_below(16); break;
        case E_SAW: e->vx = 16 + cyc * 4; break;
        case E_SAWV: e->vy = 16 + cyc * 4; break;
        case E_PLAT: e->vx = 8; break;
        case E_CANNON: e->timer = 60 + s->b; break;
        default: break;
        }
    }
    tw_npend = 0;
}

static uint8_t overlap(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah, int16_t bx, int16_t by, uint8_t bw, uint8_t bh) {
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static void kill_ent(Ent *e) {
    int16_t x = e->x >> 4, y = (e->y >> 4) - (TW_Y0 - tw_pan);
    if (y > 0 && y < 128) {
        fx_burst((uint8_t)x + 8, (uint8_t)y + (e->type == E_SLIME ? -6 : 4), 6);
        fx_text((uint8_t)x + 4, (uint8_t)y - 8, "+1");
    }
    e->type = E_NONE;
    tw_coins++;
    tt_sound(SFX_STOMP);
}

static uint8_t ent_box(Ent *e, int16_t *x, int16_t *y, uint8_t *w, uint8_t *h) {
    int16_t ex = e->x >> 4, ey = e->y >> 4;
    switch (e->type) {
    case E_SLIME: *x = ex + 2; *y = ey - 10; *w = 12; *h = 10; return 1;
    case E_BAT: *x = ex + 2; *y = ey + 1; *w = 12; *h = 6; return 1;
    case E_GHOST: *x = ex + 2; *y = ey + 2; *w = 12; *h = 12; return 1;
    case E_SAW: case E_SAWV: *x = ex - 6; *y = ey - 6; *w = 12; *h = 12; return 2;
    case E_FIRE: *x = ex + 1; *y = ey + 1; *w = 6; *h = 6; return 2;
    case E_ICICLE: *x = ex + 2; *y = ey; *w = 4; *h = 8; return e->state == 2 ? 2 : 0;
    default: return 0;
    }
}

void tw_collect(int16_t x, int16_t y) BANKED {
    uint8_t k = cell(x, y);
    int16_t sx = (x & ~7), sy = (y & ~7) - (TW_Y0 - tw_pan);
    if (k != K_COIN && k != K_GEM) return;
    set_cell(x, y, K_EMPTY);
    if (k == K_COIN) {
        tw_coins++;
        tt_sound(SFX_COIN);
        fx_spawn(FX_FLOAT, (uint8_t)sx, (uint8_t)sy, 0, -14, TW_S_COIN, OP_SPARK | PB, 16);
    } else {
        tw_coins += 5;
        tt_sound(SFX_GEM);
        fx_burst((uint8_t)sx + 4, (uint8_t)sy + 4, 8);
        fx_text((uint8_t)sx - 4, (uint8_t)sy - 6, "+5");
    }
    tw_hud_stats();
}

void tw_crumble_start(int16_t x, int16_t y) BANKED {
    uint8_t i, r = ring_of(y), cx = (uint8_t)x >> 3, fr = 255;
    for (i = 0; i < 6; i++) {
        if (tw_crum[i].t && tw_crum[i].x == cx && tw_crum[i].ring == r) return;
        if (!tw_crum[i].t && fr == 255) fr = i;
    }
    if (fr == 255) return;
    tw_crum[fr].x = cx;
    tw_crum[fr].ring = r;
    tw_crum[fr].t = 1;
}

void tw_crumble_tick(void) BANKED {
    uint8_t i;
    Crumble *c = tw_crum;
    for (i = 0; i < 6; i++, c++) {
        if (!c->t) continue;
        c->t++;
        if (c->t == 10) tw_crumble_tile(c->x, c->ring, 1);
        else if (c->t == 22) {
            int16_t sy = (int16_t)(((uint8_t)(c->ring - tw_mb) & 31)) * 8;
            TW->cells[c->ring][c->x] = K_EMPTY;
            tw_redraw(c->x, c->ring);
            tt_sound(SFX_CRUMBLE);
            fx_spawn(FX_CONFETTI, c->x * 8, (uint8_t)sy, -6, -8, TW_S_CHIP, 5 | PB, 30);
            fx_spawn(FX_CONFETTI, c->x * 8 + 4, (uint8_t)sy, 6, -10, TW_S_CHIP, 5 | PB, 30);
        } else if (c->t >= 200) {
            int16_t top = (int16_t)(TW_Y0 + (((uint8_t)(c->ring - tw_mb) & 31) << 3));
            int16_t X = tw_px >> 4, Y = tw_py >> 4;
            if (overlap(X, Y - 13, 8, 13, c->x * 8, top, 8, 8)) { c->t = 199; continue; }
            TW->cells[c->ring][c->x] = K_CRUMBLE;
            tw_redraw(c->x, c->ring);
            c->t = 0;
        }
    }
}

static void stick(int16_t x, int16_t y, uint8_t flip) {
    Stuck *s = &tw_stk[tw_stk_next];
    if (s->on) tw_dust(s->x + 4, s->y, 0);
    s->on = 1;
    s->x = x;
    s->y = y;
    s->flip = flip;
    s->wob = 8;
    tw_stk_next ^= 1;
}

static void arrow_hit_ents(Arrow *a) {
    uint8_t i, kind, w, h;
    int16_t bx, by;
    Ent *e = TW->ent;
    for (i = 0; i < MAX_ENTS; i++, e++) {
        if (e->type == E_NONE || e->old) continue;
        kind = ent_box(e, &bx, &by, &w, &h);
        if (!kind && e->type != E_SLIME) continue;
        if (!overlap(a->x, a->y + 2, 8, 4, bx, by, w, h)) continue;
        if (e->type == E_SLIME || e->type == E_BAT || e->type == E_GHOST) {
            kill_ent(e);
        } else if (e->type == E_FIRE) {
            e->type = E_NONE;
            fx_burst((uint8_t)a->x + 4, (uint8_t)(a->y - (TW_Y0 - tw_pan)) + 4, 4);
            tt_sound(SFX_TINK);
        } else {
            tt_sound(SFX_TINK);
            fx_spawn(FX_CONFETTI, (uint8_t)a->x, (uint8_t)(a->y - (TW_Y0 - tw_pan)), -a->dir * 8, -12, TW_S_ARROW, 6 | PB, 24);
        }
        a->on = 0;
        return;
    }
}

void tw_arrows_tick(void) BANKED {
    uint8_t i;
    Arrow *a = tw_fly;
    int16_t tip;
    uint8_t k;
    for (i = 0; i < 2; i++, a++) {
        if (!a->on) continue;
        a->x += a->dir * 4;
        tip = a->dir > 0 ? a->x + 7 : a->x;
        if (tip < 0 || tip > 159) { a->on = 0; continue; }
        k = cell(tip, a->y + 4);
        if (k == K_COIN || k == K_GEM) tw_collect(tip, a->y + 4);
        else if (K_SOLID(k)) {
            if (k == K_WOOD) {
                if (a->dir > 0) a->x = (int16_t)((tip & ~7) - 6);
                else a->x = (int16_t)((tip & ~7) + 6);
                stick(a->x, a->y + 4, a->dir < 0);
                tt_sound(SFX_THUNK);
            } else {
                tt_sound(SFX_TINK);
                fx_spawn(FX_CONFETTI, (uint8_t)a->x, (uint8_t)(a->y - (TW_Y0 - tw_pan)), -a->dir * 10, -14, TW_S_ARROW,
                         (a->dir > 0 ? S_FLIPX : 0) | 6 | PB, 20);
            }
            a->on = 0;
            continue;
        }
        arrow_hit_ents(a);
    }
}

void tw_shoot(void) BANKED {
    uint8_t i;
    int16_t X = tw_px >> 4, Y = tw_py >> 4;
    for (i = 0; i < 2; i++) {
        if (!tw_fly[i].on) {
            tw_fly[i].on = 1;
            tw_fly[i].dir = tw_face ? -1 : 1;
            tw_fly[i].x = tw_face ? X - 6 : X + 6;
            tw_fly[i].y = Y - 6;
            tw_cool = 14;
            tw_shoot_t = 10;
            tt_sound(SFX_SHOOT);
            return;
        }
    }
}

void tw_ents_tick(void) BANKED {
    uint8_t i, w, h, kind;
    int16_t X = tw_px >> 4, Y = tw_py >> 4, ex, ey, bx, by;
    Ent *e = TW->ent;
    for (i = 0; i < MAX_ENTS; i++, e++) {
        if (e->type == E_NONE || e->old) continue;
        e->anim++;
        ex = e->x >> 4;
        ey = e->y >> 4;
        switch (e->type) {
        case E_SLIME: {
            int16_t ahead = e->vx > 0 ? ex + 14 : ex + 1;
            e->x += e->vx;
            if (!land_kind(cell(ahead, ey)) || K_SOLID(cell(ahead, ey - 4))) e->vx = -e->vx;
            break;
        }
        case E_BAT: {
            int16_t front = e->vx > 0 ? ex + 15 : ex;
            e->x += e->vx;
            if (K_SOLID(cell(front, ey + 4)) || front < 8 || front > 151) e->vx = -e->vx;
            e->y = ((int16_t)(TW_Y0 + e->base) + sine16[(e->anim >> 2) & 15]) << 4;
            break;
        }
        case E_SAW:
            e->x += e->vx;
            if ((e->x >> 4) >= e->b) { e->x = (int16_t)e->b << 4; e->vx = -e->vx; }
            if ((e->x >> 4) <= e->a) { e->x = (int16_t)e->a << 4; e->vx = -e->vx; }
            break;
        case E_SAWV:
            e->y += e->vy;
            if ((e->y >> 4) >= TW_Y0 + e->b) { e->y = (int16_t)(TW_Y0 + e->b) << 4; e->vy = -e->vy; }
            if ((e->y >> 4) <= TW_Y0 + e->a) { e->y = (int16_t)(TW_Y0 + e->a) << 4; e->vy = -e->vy; }
            break;
        case E_PLAT:
            e->x += e->vx;
            if ((e->x >> 4) >= e->b) { e->x = (int16_t)e->b << 4; e->vx = -e->vx; }
            if ((e->x >> 4) <= e->a) { e->x = (int16_t)e->a << 4; e->vx = -e->vx; }
            break;
        case E_CANNON:
            if (e->timer) e->timer--;
            if (e->timer == 24) {
                fx_spawn(FX_SPARK, (uint8_t)(e->a ? ex - 6 : ex + 6), (uint8_t)(ey - TW_Y0), 0, 0, TT_S_SPARK0, OP_SPARK, 20);
            }
            if (!e->timer) {
                Ent *f = ent_new(E_FIRE);
                uint8_t period = tw_dl > 11 ? 70 : 160 - tw_dl * 8;
                e->timer = period;
                if (f) {
                    f->x = (int16_t)(e->a ? ex - 8 : ex + 8) << 4;
                    f->y = e->y;
                    f->vx = e->a ? -20 : 20;
                    tt_sound(SFX_CANNON);
                }
            }
            break;
        case E_FIRE:
            e->x += e->vx;
            if (K_SOLID(cell((e->x >> 4) + (e->vx > 0 ? 7 : 0), ey + 4)) || (e->x >> 4) < 4 || (e->x >> 4) > 150) {
                fx_burst((uint8_t)((e->x >> 4) + 4), (uint8_t)(ey - TW_Y0 + 4), 4);
                e->type = E_NONE;
                continue;
            }
            break;
        case E_GHOST: {
            int16_t tx = X - 4, ty = Y - 16;
            if ((e->anim & 1) == 0) {
                if (ex < tx) e->x += 6; else if (ex > tx) e->x -= 6;
                if (ey < ty) e->y += 5; else if (ey > ty) e->y -= 5;
            }
            break;
        }
        case E_ICICLE:
            if (e->state == 0) {
                if (X + 4 > ex - 10 && X + 4 < ex + 18 && Y > ey) { e->state = 1; e->timer = 24; }
            } else if (e->state == 1) {
                if (!--e->timer) e->state = 2;
            } else {
                if (e->vy < 60) e->vy += 4;
                e->y += e->vy;
                if (K_SOLID(cell(ex + 4, (e->y >> 4) + 8))) {
                    fx_spawn(FX_CONFETTI, (uint8_t)ex, (uint8_t)(ey - TW_Y0 + 4), -8, -10, TW_S_CHIP, 5 | PB, 20);
                    fx_spawn(FX_CONFETTI, (uint8_t)ex + 4, (uint8_t)(ey - TW_Y0 + 4), 8, -12, TW_S_CHIP, 5 | PB, 20);
                    tt_sound(SFX_TINK);
                    e->type = E_NONE;
                    continue;
                }
            }
            break;
        case E_HEART:
            if (overlap(X, Y - 13, 8, 13, ex, ey + 1, 8, 7)) {
                if (tw_hearts < 3) tw_hearts++;
                tt_sound(SFX_HEART);
                fx_burst((uint8_t)ex + 4, (uint8_t)(ey - TW_Y0 + 4), 6);
                e->type = E_NONE;
                continue;
            }
            break;
        }
        /* contact with the hero */
        kind = ent_box(e, &bx, &by, &w, &h);
        if (!kind || tw_inv || tw_st != S_PLAY) continue;
        if (!overlap(X, Y - 13, 8, 13, bx, by, w, h)) continue;
        if (kind == 1 && e->type != E_GHOST && tw_pvy > 0 && Y <= by + 6) {
            kill_ent(e);
            tw_pvy = (tt_held & TT_A) ? -JUMP_V : -46;
            tw_jhold = (tt_held & TT_A) ? 1 : 0;
            continue;
        }
        tw_hurt(X + 4 < bx + (w >> 1) ? -1 : 1, 0);
    }
}

static void draw_player(void) {
    uint8_t base, prop, f;
    int16_t sx = (tw_px >> 4) - 4, sy = (tw_py >> 4) - 16 - (TW_Y0 - tw_pan);
    if (tw_st == S_DEAD) base = TW_S_HERO_HURT;
    else if (tw_stun && tw_inv) base = TW_S_HERO_HURT;
    else if (!tw_ground) base = tw_pvy < 0 ? TW_S_HERO_JUMP : TW_S_HERO_FALL;
    else if (tw_shoot_t) base = TW_S_HERO_SHOOT;
    else if (tw_pvx > 4 || tw_pvx < -4) base = TW_S_HERO_RUN0 + (((uint8_t)(tt_clock >> 2)) & 3) * 4;
    else base = ((tt_clock >> 5) & 1) ? TW_S_HERO_IDLE1 : TW_S_HERO_IDLE0;
    if ((tw_inv & 4) || sy <= -16 || sy >= 128 || sy + 16 > (int16_t)tw_win_y) {
        tt_spr_hide(0, 4);
        return;
    }
    prop = 0 | PB | (tw_face ? S_FLIPX : 0);
    f = tw_face;
    tt_spr(0, (uint8_t)sx, (uint8_t)sy, base + f, prop);
    tt_spr(1, (uint8_t)(sx + 8), (uint8_t)sy, base + 1 - f, prop);
    tt_spr(2, (uint8_t)sx, (uint8_t)(sy + 8), base + 2 + f, prop);
    tt_spr(3, (uint8_t)(sx + 8), (uint8_t)(sy + 8), base + 3 - f, prop);
}

void tw_draw_all(void) BANKED {
    uint8_t i, t, prop;
    int16_t vt = TW_Y0 - tw_pan, sx, sy;
    Ent *e;
    draw_player();
    /* arrows in flight */
    for (i = 0; i < 2; i++) {
        sy = tw_fly[i].y - vt;
        if (tw_fly[i].on && tw_st != S_PAUSE && sy > -8 && sy + 8 <= (int16_t)tw_win_y)
            tt_spr(4 + i, (uint8_t)tw_fly[i].x, (uint8_t)sy, TW_S_ARROW, 6 | PB | (tw_fly[i].dir < 0 ? S_FLIPX : 0));
        else hide_sprite(4 + i);
        sy = tw_stk[i].y - 4 - vt;
        if (tw_stk[i].on && sy > -8 && sy + 8 <= (int16_t)tw_win_y) {
            sx = tw_stk[i].x;
            if (tw_stk[i].wob) { tw_stk[i].wob--; sx += (tw_stk[i].wob & 2) ? 1 : 0; }
            tt_spr(6 + i, (uint8_t)sx, (uint8_t)sy, TW_S_ARROW_STUCK, 6 | PB | (tw_stk[i].flip ? S_FLIPX : 0));
        } else hide_sprite(6 + i);
    }
    /* hearts on the HUD */
    for (i = 0; i < 3; i++) tt_spr(8 + i, 4 + i * 9, tw_win_y, i < tw_hearts ? TW_S_HEART : TW_S_HEART_EMPTY, 7 | PB);
    /* entities */
    oam_i = 0;
    e = TW->ent;
    for (i = 0; i < MAX_ENTS; i++, e++) {
        if (e->type == E_NONE) continue;
        sx = e->x >> 4;
        sy = (e->y >> 4) - vt;
        switch (e->type) {
        case E_SLIME:
            put16(TW_S_SLIME + ((e->anim >> 4) & 1) * 4, sx, sy - 16, 1 | PB | (e->vx < 0 ? S_FLIPX : 0));
            break;
        case E_BAT:
            t = TW_S_BAT + ((e->anim >> 3) & 1) * 2;
            put(t, sx, sy, 3 | PB);
            put(t + 1, sx + 8, sy, 3 | PB);
            break;
        case E_SAW:
        case E_SAWV:
            put16(TW_S_SAW + ((e->anim >> 2) & 1) * 4, sx - 8, sy - 8, 5 | PB);
            break;
        case E_PLAT:
            put(TW_S_PLAT, sx, sy, 6 | PB);
            put(TW_S_PLAT + 1, sx + 8, sy, 6 | PB);
            put(TW_S_PLAT + 2, sx + 16, sy, 6 | PB);
            break;
        case E_FIRE:
            put(TW_S_FIRE + ((e->anim >> 2) & 1), sx, sy, 7 | PB | (e->vx < 0 ? S_FLIPX : 0));
            break;
        case E_GHOST:
            prop = 3 | PB | ((tw_px >> 4) < sx ? S_FLIPX : 0);
            put16(TW_S_GHOST + ((e->anim >> 4) & 1) * 4, sx, sy + sine16[(e->anim >> 2) & 15] / 2, prop);
            break;
        case E_ICICLE:
            put(TW_S_ICICLE, sx + (e->state == 1 ? ((e->anim & 2) ? 1 : -1) : 0), sy, 5 | PB);
            break;
        case E_HEART:
            put(TW_S_HEART, sx, sy + (sine16[(e->anim >> 2) & 15] >> 1), 7 | PB);
            break;
        }
    }
    for (i = oam_i; i < sizeof(ent_slots); i++) hide_sprite(ent_slots[i]);
}
